#include "lua-video.h"
#include "lua-thread.h"
#include "lua-frame.h"
#include "int.h"
#include "str.h"
#include "fmt.h"

#include <lauxlib.h>
#include <assert.h>

#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <libavutil/error.h>

static video_generator *video = NULL;

static const AVFilter *vflip_filter = NULL;
static const AVFilter *format_filter = NULL;
static const AVFilter *fps_filter = NULL;
static const AVFilter *buffer_filter = NULL;
static const AVFilter *buffersink_filter = NULL;


enum LUAVIDEO_STATUS {
    LUAVIDEO_ERROR,
    LUAVIDEO_LOADING,
    LUAVIDEO_OK,
    LUAVIDEO_DONE,
};

typedef struct luavideo_status_rep {
    const char *str;
    int boolean;
} luavideo_status_rep;

static const luavideo_status_rep luavideo_status_reps[] = {
    { "error", 0 },
    { "loading", 1 },
    { "ok", 1 },
    { "done", 0 },
};

enum LUAVIDEO_COMMAND {
    LUAVIDEO_STOP,
    LUAVIDEO_FRAME,
    LUAVIDEO_SEEK,
};

typedef struct luavideo_cmd_t {
    enum LUAVIDEO_COMMAND cmd;
    double dblparam;
} luavideo_cmd_t;

typedef struct luavideo_frame_t {
    unsigned int width;
    unsigned int height;
    unsigned int channels;
    unsigned int linesize;
    double timestamp;
    jpr_uint8 *data;
} luavideo_frame_t;

typedef struct luavideo_ctrl_s {
    volatile int c;
    volatile int status;
    int loops;
    lmv_thread_t *thread;
    jpr_int64 pts_last;
    jpr_int64 pts_offset;
    void **cmd_storage;
    luavideo_cmd_t cmds[3];
    thread_queue_t cmd;
    thread_signal_t cmd_signal;
} luavideo_ctrl_t;

typedef struct luavideo_queue_s {
    char *url;

    AVFilterGraph *graph;
    AVFilterContext *buffer_ctx;
    AVFilterContext *buffersink_ctx;
    AVFilterContext **filter_ctxs;
    unsigned int filters;
    luavideo_ctrl_t *ctrl;
    unsigned int channels;
} luavideo_queue_t;

#if (!defined LUA_VERSION_NUM) || LUA_VERSION_NUM==501
#define lua_len(L,i) lua_pushinteger(L,lua_objlen(L,i))
#define lua_geti(L,i,n) lua_rawgeti(L,i,n)
#define lua_setuservalue(L,i) lua_setfenv((L),(i))
#define lua_getuservalue(L,i) lua_getfenv((L),(i))
#endif

static void luavideo_queue_cleanup(lua_State *L, void *userdata) {
    unsigned int fidx;
    luavideo_queue_t *q = (luavideo_queue_t *)userdata;

    if(userdata == NULL) return;

    fidx = 0;
    while(fidx < q->filters) {
        if(q->filter_ctxs[fidx] != NULL) avfilter_free(q->filter_ctxs[fidx]);
        fidx++;
    }
    free(q->filter_ctxs);
    if(q->buffer_ctx != NULL) avfilter_free(q->buffer_ctx);
    if(q->buffersink_ctx != NULL) avfilter_free(q->buffersink_ctx);
    avfilter_graph_free(&q->graph);
    free(q->url);
    free(q);

    (void)L;
}

static void luavideo_outqueue_cleanup(lua_State *L, void *userdata) {
    (void)L;
    (void)userdata;
}

static int luavideo_process(void *userdata, lmv_produce produce, void *queue) {
    AVFormatContext *ic = NULL;
    AVCodec *decoder = NULL;
    AVCodecContext *avctx = NULL;
    AVCodecParameters *codecParameters = NULL;
    AVStream *stream = NULL;
    AVPacket *packet = NULL;
    AVFrame *iframe = NULL;
    AVFrame *oframe = NULL;
    AVBufferSrcParameters *bufferSrcParams = NULL;
    char args[512];
    luavideo_frame_t f;
    unsigned int fidx;
    int video_idx;
    unsigned int i;
    int err;
    int seek_status;
    luavideo_queue_t *q = (luavideo_queue_t *)userdata;
    luavideo_cmd_t *cmd = NULL;

    if(avformat_open_input(&ic,q->url, NULL, NULL) < 0) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_cleanup;
    }
    if(avformat_find_stream_info(ic, NULL) < 0) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_cleanup;
    }

    for(i=0;i<ic->nb_streams;i++) {
        ic->streams[i]->discard = AVDISCARD_ALL;
    }

    video_idx = av_find_best_stream(ic,AVMEDIA_TYPE_VIDEO,-1,-1,&decoder, 0);
    if(video_idx < 0) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_cleanup;
    }
    if(decoder == NULL) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_cleanup;
    }

    stream = ic->streams[video_idx];
    stream->discard = AVDISCARD_DEFAULT;

    codecParameters = stream->codecpar;

    avctx = avcodec_alloc_context3(decoder);
    if(avctx == NULL) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_cleanup;
    }
    avcodec_parameters_to_context(avctx,codecParameters);

    if(avcodec_open2(avctx, decoder, NULL) < 0) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_cleanup;
    }

    snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
      avctx->width, avctx->height, avctx->pix_fmt, stream->time_base.num,
      stream->time_base.den, avctx->sample_aspect_ratio.num,
      avctx->sample_aspect_ratio.den);
    if(avfilter_graph_create_filter(&q->buffer_ctx, buffer_filter, "in", args, NULL, q->graph) < 0) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_cleanup;
    }
    if(avfilter_graph_create_filter(&q->buffersink_ctx, buffersink_filter, "out", NULL, NULL, q->graph) < 0) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_cleanup;
    }

    if(avfilter_link(q->buffer_ctx, 0, q->filter_ctxs[0], 0) < 0) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_cleanup;
    }

    fidx = 0;
    while(fidx < q->filters - 1) {
        if(avfilter_link(q->filter_ctxs[fidx], 0, q->filter_ctxs[fidx+1], 0) < 0) {
            q->ctrl->status = LUAVIDEO_ERROR;
            goto luavideo_cleanup;
        }
        fidx++;
    }

    if(avfilter_link(q->filter_ctxs[fidx], 0, q->buffersink_ctx, 0) < 0) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_cleanup;
    }
    if(avfilter_graph_config(q->graph, NULL) < 0) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_cleanup;
    }

    q->ctrl->pts_last = 0;
    q->ctrl->pts_offset = 0 - stream->start_time;

    f.width    = av_buffersink_get_w(q->buffersink_ctx);
    f.height   = av_buffersink_get_h(q->buffersink_ctx);
    f.channels = q->channels;
    f.timestamp = 0.0f;

    packet = av_packet_alloc();
    iframe = av_frame_alloc();
    oframe = av_frame_alloc();

    while(1) {
        while(thread_queue_count(&q->ctrl->cmd) > 0) {
            av_frame_unref(oframe);
            cmd = (luavideo_cmd_t *)thread_queue_consume(&q->ctrl->cmd,THREAD_QUEUE_WAIT_INFINITE);
            if(cmd == NULL) abort();

            switch(cmd->cmd) {
                case LUAVIDEO_STOP: {
                    q->ctrl->status = LUAVIDEO_DONE;
                    goto luavideo_cleanup;
                }

                case LUAVIDEO_FRAME: {
                    q->ctrl->status = LUAVIDEO_OK;
                    while(1) {
                        buffersink_get:
                        if( (err = av_buffersink_get_frame(q->buffersink_ctx,oframe)) >= 0) {
                            f.data = oframe->data[0];
                            f.linesize = oframe->linesize[0];
                            produce(queue,&f);
                            break;
                        }

                        if(err != AVERROR(EAGAIN)) {
                            q->ctrl->status = LUAVIDEO_ERROR;
                            goto luavideo_cleanup;
                        }

                        codec_get:
                        if( (err = avcodec_receive_frame(avctx,iframe)) >= 0) {
                            if(av_buffersrc_add_frame(q->buffer_ctx,iframe) < 0) {
                                q->ctrl->status = LUAVIDEO_ERROR;
                                goto luavideo_cleanup;
                            }
                            goto buffersink_get;
                        }

                        if(err != AVERROR(EAGAIN)) {
                            q->ctrl->status = LUAVIDEO_ERROR;
                            goto luavideo_cleanup;
                        }

                        format_get:
                        if( (err = av_read_frame(ic,packet)) >= 0) {
                            if(packet->stream_index == video_idx) {
                                f.timestamp = (double)packet->pts * (double)stream->time_base.num / (double)stream->time_base.den;
                                /* mess with the packet pts */
                                q->ctrl->pts_last = packet->pts;
                                packet->pts += q->ctrl->pts_offset;

                                if(avcodec_send_packet(avctx,packet) < 0) {
                                    q->ctrl->status = LUAVIDEO_ERROR;
                                    av_packet_unref(packet);
                                    goto luavideo_cleanup;
                                }
                                av_packet_unref(packet);
                                goto codec_get;
                            }
                            av_packet_unref(packet);
                            goto format_get;
                        }

                        if(err != AVERROR_EOF) {
                            q->ctrl->status = LUAVIDEO_ERROR;
                            goto luavideo_cleanup;
                        }

                        q->ctrl->loops--;
                        if(q->ctrl->loops == 0 || av_seek_frame(ic,video_idx,0,AVSEEK_FLAG_BACKWARD) < 0) {
                            q->ctrl->status = LUAVIDEO_DONE;
                            goto luavideo_cleanup;
                        }
                        q->ctrl->pts_offset += q->ctrl->pts_last;
                        goto format_get;
                    }
                    break;
                }

                case LUAVIDEO_SEEK: {
                    int64_t pts_prev = 0;
                    q->ctrl->status = LUAVIDEO_OK;

                    /* first flush the codec and drain everything */
                    avcodec_flush_buffers(avctx);

                    /* then decode any remaining frames into the buffersrc */
                    while( (err = avcodec_receive_frame(avctx,iframe)) >= 0) {
                        if(av_buffersrc_add_frame(q->buffer_ctx,iframe) < 0) {
                            q->ctrl->status = LUAVIDEO_ERROR;
                            goto luavideo_cleanup;
                        }
                    }

                    if(err != AVERROR(EAGAIN)) {
                        q->ctrl->status = LUAVIDEO_ERROR;
                        goto luavideo_cleanup;
                    }

                    while( (err = av_buffersink_get_frame(q->buffersink_ctx,oframe)) >= 0) {
                        av_frame_unref(oframe);
                    }

                    /* find what the next PTS should have been */
                    while( (err = av_read_frame(ic,packet)) > 0) {
                        if(packet->stream_index == video_idx) {
                            pts_prev = q->ctrl->pts_last;
                            packet->pts += q->ctrl->pts_offset;
                            q->ctrl->pts_last = packet->pts;
                            break;
                        }
                        av_packet_unref(packet);
                    }
                    if(err < 0) {
                        break;
                    }

                    /* figure out a new PTS offset after seeking */
                    double ts = cmd->dblparam * (double)stream->time_base.den / (double)stream->time_base.num;
                    seek_status = avformat_seek_file(ic,video_idx,0,(jpr_int64)ts,(jpr_int64)ts,0);
                    /* if we were successful, figure out a new pts offset */
                    if(seek_status >= 0) {
                        while( (err = av_read_frame(ic,packet)) >= 0) {
                            if(packet->stream_index == video_idx) {
                                f.timestamp = (double)packet->pts * (double)stream->time_base.num / (double)stream->time_base.den;

                                q->ctrl->pts_offset -= (packet->pts - pts_prev) - (q->ctrl->pts_last - pts_prev);
                                q->ctrl->pts_last = packet->pts;
                                packet->pts += q->ctrl->pts_offset;

                                if(avcodec_send_packet(avctx,packet) < 0) {
                                    q->ctrl->status = LUAVIDEO_ERROR;
                                    goto luavideo_cleanup;
                                }
                                av_packet_unref(packet);
                                break;
                            }
                            av_packet_unref(packet);
                        }
                    }
                    break;
                }
                default: {
                    abort();
                }
            }
        }
        thread_signal_wait(&q->ctrl->cmd_signal,THREAD_SIGNAL_WAIT_INFINITE);
    }

    luavideo_cleanup:
    if(ic != NULL)  avformat_close_input(&ic);
    if(avctx != NULL) avcodec_free_context(&avctx);
    if(bufferSrcParams != NULL) av_free(&bufferSrcParams);
    if(packet != NULL) av_packet_free(&packet);
    if(iframe != NULL) av_frame_free(&iframe);
    if(oframe != NULL) av_frame_free(&oframe);
    luavideo_queue_cleanup(NULL,q);

    return 1;
}

static int luavideo__gc(lua_State *L) {
    luavideo_ctrl_t *ctrl = (luavideo_ctrl_t *)lua_touserdata(L,1);
    thread_queue_produce(&ctrl->cmd,&(ctrl->cmds[LUAVIDEO_STOP]),THREAD_QUEUE_WAIT_INFINITE);
    thread_signal_raise(&ctrl->cmd_signal);
    lmv_thread_free(L,ctrl->thread);
    thread_queue_term(&ctrl->cmd);
    thread_signal_term(&ctrl->cmd_signal);
    free(ctrl->cmd_storage);
    return 0;
}

static int luavideo_stop(lua_State *L) {
    luavideo_ctrl_t *ctrl = NULL;
    ctrl = (luavideo_ctrl_t *)lua_touserdata(L,1);
    if(thread_queue_count(&ctrl->cmd) == 3) {
        return luaL_error(L,"unable to queue command, something is wrong");
    }
    thread_queue_produce(&ctrl->cmd,&(ctrl->cmds[LUAVIDEO_STOP]),THREAD_QUEUE_WAIT_INFINITE);
    thread_signal_raise(&ctrl->cmd_signal);
    return 0;
}

static int luavideo_start(lua_State *L) {
    luavideo_ctrl_t *ctrl = NULL;
    ctrl = (luavideo_ctrl_t *)lua_touserdata(L,1);
    if(thread_queue_count(&ctrl->cmd) == 3) {
        return luaL_error(L,"unable to queue command, something is wrong");
    }
    thread_queue_produce(&ctrl->cmd,&(ctrl->cmds[LUAVIDEO_FRAME]),THREAD_QUEUE_WAIT_INFINITE);
    thread_signal_raise(&ctrl->cmd_signal);
    return 0;
}

static int luavideo_seek(lua_State *L) {
    luavideo_ctrl_t *ctrl = NULL;
    ctrl = (luavideo_ctrl_t *)lua_touserdata(L,1);
    if(thread_queue_count(&ctrl->cmd) == 3) {
        return luaL_error(L,"unable to queue command, something is wrong");
    }
    ctrl->cmds[LUAVIDEO_SEEK].dblparam = lua_tonumber(L,2);
    thread_queue_produce(&ctrl->cmd,&(ctrl->cmds[LUAVIDEO_SEEK]),THREAD_QUEUE_WAIT_INFINITE);
    thread_signal_raise(&ctrl->cmd_signal);
    return 0;
}

static int luavideo_status(lua_State *L) {
    luavideo_ctrl_t *ctrl = NULL;
    ctrl = (luavideo_ctrl_t *)lua_touserdata(L,1);

    lua_pushstring(L,luavideo_status_reps[ctrl->status].str);
    return 1;
}

static int luavideo_ok(lua_State *L) {
    luavideo_ctrl_t *ctrl = NULL;
    ctrl = (luavideo_ctrl_t *)lua_touserdata(L,1);

    lua_pushboolean(L,luavideo_status_reps[ctrl->status].boolean);
    return 1;
}

static int luavideo_frame(lua_State *L) {
    luavideo_ctrl_t *ctrl = NULL;
    luavideo_frame_t *frame = NULL;
    int res = 0;

    ctrl = (luavideo_ctrl_t *)lua_touserdata(L,1);

    frame = lmv_thread_result(ctrl->thread,&res);

    if(frame != NULL) {
        luaframe_new(L, frame->width, frame->height, frame->channels, frame->data, frame->linesize);
        lua_pushnumber(L,frame->timestamp);
        thread_queue_produce(&ctrl->cmd,&(ctrl->cmds[LUAVIDEO_FRAME]),THREAD_QUEUE_WAIT_INFINITE);
        thread_signal_raise(&ctrl->cmd_signal);
        return 2;
    }
    return 0;
}

/* video.new(params)
 * params is a table with the following keys:
 *   url (string)
 *   colordepth (integer)
 *   filters (table)
 *
 * url is a path to a file, a url, etc - anything that ffmpeg can understand
 * colordepth is an optional integer, it can be 3 (for BGR) or 4 (for BGRA)
 * filters should be an array-like table of filters to apply, there's a
 * few filters that are always applied, though:
 *   vflip (since interally images are stored bottom-up)
 *   format (to choose BGR or BGRA)
 *   fps (always set to match video fps)
 * these are always done at the end of the filter chain, user filters
 * are applied first */

static int
luavideo_new(lua_State *L) {
    luavideo_queue_t *v = NULL;
    luavideo_ctrl_t *ctrl = NULL;
    const char *url = NULL;
    const char *filtername;
    const char *filterparams;
    unsigned int channels = 3;
    int loops = 1;

    AVFilterGraph *graph = NULL;
    AVFilterContext **filter_ctxs = NULL;
    const AVFilter *filter = NULL;
    unsigned int filters = 3; /* we always need vflip, format, fps */
    unsigned int filteridx = 0;
    unsigned int filterslen = 0;
    int errflag = 0;
    char *buf;
    char args[512];
    luaL_Buffer errbuf;
    luaL_buffinit(L,&errbuf);

    if(!lua_istable(L,1)) {
        luaL_addstring(&errbuf,"missing parameters table");
        luaL_pushresult(&errbuf);
        lua_pushnil(L);
        lua_insert(L,-2);
        return 2;
    }

    lua_getfield(L,1,"url");
    if(!lua_isstring(L,-1)) {
        luaL_addstring(&errbuf,"missing parameter: url");
        luaL_pushresult(&errbuf);
        lua_pushnil(L);
        lua_insert(L,-2);
        return 2;
    }

    url = lua_tostring(L,-1);
    lua_pop(L,1);

    lua_getfield(L,1,"loops");
    loops = luaL_optinteger(L,-1,1);
    lua_pop(L,1);

    lua_getfield(L,1,"colordepth");
    if(lua_isnumber(L,-1)) {
        channels = (unsigned int)lua_tointeger(L,-1);
        switch(channels) {
            case 3: break;
            case 4: break;
            case 24: channels = 3; break;
            case 32: channels = 4; break;
            default: {
                luaL_addstring(&errbuf,"invalid colordepth");
                luaL_pushresult(&errbuf);
                lua_pushnil(L);
                lua_insert(L,-2);
                return 2;
            }
        }
    }
    lua_pop(L,1);

    v = (luavideo_queue_t *)malloc(sizeof(luavideo_queue_t));
    if(v == NULL) {
        return luaL_error(L,"out of memory");
    }

    graph = avfilter_graph_alloc();
    if(graph == NULL) {
        errflag = -1;
        luaL_addstring(&errbuf,"error allocating graph");
        goto cleanup;
    }

    lua_getfield(L,1,"filters");
    if(lua_istable(L,-1)) {
        lua_len(L,-1);
        filterslen = lua_tointeger(L,-1);
        lua_pop(L,1);
        filters += filterslen;
    }

    filter_ctxs = (AVFilterContext **)malloc(sizeof(AVFilterContext *) * filters);
    if(filter_ctxs == NULL) {
        errflag = -1;
        luaL_addstring(&errbuf,"error allocating filters");
        goto cleanup;
    }

    for(filteridx=0;filteridx<filters;filteridx++) {
        filter_ctxs[filteridx] = NULL;
    }

    filteridx = 0;
    if(filterslen > 0) {
        while(filteridx < filterslen) {
            lua_geti(L,-1,filteridx+1); /* push the next table */
            if(!lua_istable(L,-1)) {
                luaL_addstring(&errbuf,"filters field has invalid index");
                errflag = -1;
                break;
            }

            /* need to iterate through keys */
            lua_pushnil(L);
            if(lua_next(L,-2) == 0) {
                /* this pushed nothing */
                luaL_addstring(&errbuf,"filter: no key found");
                errflag = -1;
                break;
            }

            filtername = lua_tostring(L,-2);
            if(filtername == NULL) {
                luaL_addstring(&errbuf,"filter: non-string key");
                errflag = -1;
                break;
            }
            filterparams = lua_tostring(L,-1);

            filter = avfilter_get_by_name(filtername);

            if(filter == NULL) {
                luaL_addstring(&errbuf,"filter ");
                luaL_addstring(&errbuf,filtername);
                luaL_addstring(&errbuf,": not found");
                errflag = -1;
                break;
            }

            errflag = avfilter_graph_create_filter(&filter_ctxs[filteridx++], filter,
              NULL, filterparams, NULL, graph);
            if(errflag < 0) {
                luaL_addstring(&errbuf,"filter ");
                luaL_addstring(&errbuf,filtername);
                luaL_addstring(&errbuf,": ");
                buf = luaL_prepbuffer(&errbuf);
                luaL_addsize(&errbuf,av_strerror(errflag,buf,LUAL_BUFFERSIZE));
                break;
            }

            lua_pop(L,3); /* pop value, key, table */
        }
    }

    lua_pop(L,1); /* remove "filters" field */

    if(errflag != 0) {
        /* there was some kind of error, so clean up */
        goto cleanup;
    }

    /* finally add the needed filters */
    /* vflip */
    errflag = avfilter_graph_create_filter(&filter_ctxs[filteridx++], vflip_filter,
      NULL, NULL, NULL, graph);
    if(errflag < 0) {
        luaL_addstring(&errbuf,"filter vflip:");
        buf = luaL_prepbuffer(&errbuf);
        luaL_addsize(&errbuf,av_strerror(errflag,buf,LUAL_BUFFERSIZE));
        goto cleanup;
    }

    /* format */
    if(channels == 3) {
        errflag = avfilter_graph_create_filter(&filter_ctxs[filteridx++], format_filter,
          NULL, "pix_fmts=bgr24", NULL, graph);
    } else {
        errflag = avfilter_graph_create_filter(&filter_ctxs[filteridx++], format_filter,
          NULL, "pix_fmts=bgr32", NULL, graph);
    }
    if(errflag < 0) {
        luaL_addstring(&errbuf,"filter format:");
        buf = luaL_prepbuffer(&errbuf);
        luaL_addsize(&errbuf,av_strerror(errflag,buf,LUAL_BUFFERSIZE));
        goto cleanup;
    }

    snprintf(args,sizeof(args),"fps=%d",video->fps);
    errflag = avfilter_graph_create_filter(&filter_ctxs[filteridx++], fps_filter,
      NULL, args, NULL, graph);
    if(errflag < 0) {
        luaL_addstring(&errbuf,"filter fps:");
        buf = luaL_prepbuffer(&errbuf);
        luaL_addsize(&errbuf,av_strerror(errflag,buf,LUAL_BUFFERSIZE));
        goto cleanup;
    }

    goto success;

    success:

    ctrl = (luavideo_ctrl_t *)lua_newuserdata(L,sizeof(luavideo_ctrl_t));
    if(ctrl == NULL) {
        luaL_addstring(&errbuf,"out of memory");
        goto cleanup;
    }

    ctrl->cmd_storage = (void **)malloc(sizeof(void *) * 3);
    if(ctrl->cmd_storage == NULL) {
        luaL_addstring(&errbuf,"out of memory");
        goto cleanup;
    }

    ctrl->status = LUAVIDEO_LOADING;
    ctrl->loops = loops;

    thread_queue_init(&ctrl->cmd,3,ctrl->cmd_storage,0);
    thread_signal_init(&ctrl->cmd_signal);

    ctrl->cmds[LUAVIDEO_STOP].cmd = LUAVIDEO_STOP;
    ctrl->cmds[LUAVIDEO_STOP].dblparam = 0.0f;

    ctrl->cmds[LUAVIDEO_FRAME].cmd = LUAVIDEO_FRAME;
    ctrl->cmds[LUAVIDEO_FRAME].dblparam = 0.0f;

    ctrl->cmds[LUAVIDEO_SEEK].cmd = LUAVIDEO_SEEK;
    ctrl->cmds[LUAVIDEO_SEEK].dblparam = 0.0f;

    snprintf(args,sizeof(args),"video: %s",url);

    if(lmv_thread_newmalloc(L, luavideo_process, luavideo_queue_cleanup, luavideo_outqueue_cleanup, 1, args) != 1) {
        luaL_addstring(&errbuf,"error allocating thread");
        thread_queue_term(&ctrl->cmd);
        thread_signal_term(&ctrl->cmd_signal);
        goto cleanup;
    }
    ctrl->thread = (lmv_thread_t *)lua_touserdata(L,-1);
    lua_pop(L,1);

    luaL_getmetatable(L,"lmv.video");
    lua_setmetatable(L,-2);

    v->url = str_dup(url);
    if(v->url == NULL) {
        luaL_addstring(&errbuf,"error copying URL");
        thread_queue_term(&ctrl->cmd);
        goto cleanup;
    }
    v->graph = graph;
    v->buffer_ctx = NULL;
    v->buffersink_ctx = NULL;
    v->filter_ctxs = filter_ctxs;
    v->filters = filters;
    v->ctrl = ctrl;
    v->channels = channels;

    lmv_thread_inject(ctrl->thread,v);
    return 1;

    cleanup:
    {
        free(v);
        unsigned int fidx = 0;
        while(fidx < filters) {
            if(filter_ctxs[fidx] != NULL) avfilter_free(filter_ctxs[fidx]);
            fidx++;
        }
        free(filter_ctxs);
        avfilter_graph_free(&graph);
    }
    luaL_pushresult(&errbuf);
    lua_pushnil(L);
    lua_insert(L,-2);
    return 2;
}

void luavideo_init(lua_State *L, video_generator *v) {
    video = v;
#if ( LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100) )
    av_register_all();
#endif
    lmv_thread_init(L);
    vflip_filter = avfilter_get_by_name("vflip");
    if(vflip_filter == NULL) luaL_error(L,"libavfilter init: required filter vflip is missing");
    format_filter = avfilter_get_by_name("format");
    if(format_filter == NULL) luaL_error(L,"libavfilter init: required filter format is missing");
    fps_filter = avfilter_get_by_name("fps");
    if(fps_filter == NULL) luaL_error(L,"libavfilter init: required filter fps is missing");
    buffer_filter = avfilter_get_by_name("buffer");
    if(buffer_filter == NULL) luaL_error(L,"libavfilter init: required filter buffer is missing");
    buffersink_filter = avfilter_get_by_name("buffersink");
    if(buffersink_filter == NULL) luaL_error(L,"libavfilter init: required filter buffersink is missing");

    if(luaL_newmetatable(L,"lmv.video")) {
        lua_pushcclosure(L,luavideo__gc,0);
        lua_setfield(L,-2,"__gc");

        lua_newtable(L);

        lua_pushcclosure(L,luavideo_frame,0);
        lua_setfield(L,-2,"frame");
        lua_pushcclosure(L,luavideo_status,0);
        lua_setfield(L,-2,"status");
        lua_pushcclosure(L,luavideo_stop,0);
        lua_setfield(L,-2,"stop");
        lua_pushcclosure(L,luavideo_start,0);
        lua_setfield(L,-2,"start");
        lua_pushcclosure(L,luavideo_seek,0);
        lua_setfield(L,-2,"seek");
        lua_pushcclosure(L,luavideo_ok,0);
        lua_setfield(L,-2,"ok");

        lua_setfield(L,-2,"__index");
    }
    lua_pop(L,1);
}

int
luaopen_video(lua_State *L) {

    lua_newtable(L);
    lua_pushcclosure(L,luavideo_new,0);
    lua_setfield(L,-2,"new");

    return 1;
}
