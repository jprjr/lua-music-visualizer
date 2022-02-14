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
    int linesize;
    double timestamp;
    jpr_uint8 *data;
} luavideo_frame_t;

typedef struct luavideo_ctrl_s {
    volatile int c;
    volatile int status;
    int loops;
    lmv_thread_t *thread;
    void **cmd_storage;
    luavideo_cmd_t cmds[3];
    thread_queue_t cmd;
    thread_signal_t cmd_signal;
} luavideo_ctrl_t;

typedef struct luavideo_queue_s {
    char *url;
    AVFilterGraph *graph;
    AVFilterContext **filter_ctxs;
    unsigned int filters;
    luavideo_ctrl_t *ctrl;
    unsigned int channels;
    unsigned int buffer;
} luavideo_queue_t;

/* parameters to pass to the packet-reading thread */
typedef struct luavideo_packet_thread_s {
    AVFormatContext *avFormatContext;
    thread_queue_t *queue;
    thread_signal_t *signal;
    thread_signal_t *outsignal;
    int queueSize;
    volatile int running;
} luavideo_packet_thread_s;

typedef struct luavideo_packet_receiver_s {
    thread_signal_t *signal;
    thread_queue_t *queue;
    thread_signal_t *readysignal;
    luavideo_packet_thread_s *thread;
} luavideo_packet_receiver_s;

typedef struct luavideo_codec_receiver_s {
    AVCodecContext *avCodecContext;
    AVPacket *packet;
    luavideo_packet_receiver_s *packet_receiver;
    int videoStreamIndex;
} luavideo_codec_receiver_s;

typedef struct luavideo_frame_receiver_s {
    AVFilterContext *bufferSrcContext;
    AVFilterContext *bufferSinkContext;
    jpr_int64 pts_real;
    jpr_int64 pts_diff;
    jpr_int64 pts;
    AVFrame *rawFrame;
    luavideo_codec_receiver_s *codec_receiver;
} luavideo_frame_receiver_s;

typedef struct luavideo_avformat_s {
    AVFormatContext *avFormatContext;
    AVCodec *avCodec;
    AVCodecContext *avCodecContext;
    AVCodecParameters *avCodecParameters;
    AVStream *avStream;
    int videoStreamIndex;
    AVFilterContext *bufferSrcContext;
    AVFilterContext *bufferSinkContext;
} luavideo_avformat_s;

typedef struct luavideo_seeker_s {
    AVFormatContext *avFormatContext;
    int videoStreamIndex;
    thread_queue_t *queue;
    luavideo_frame_receiver_s *fr;
    double time_base_den;
    double time_base_num;
    jpr_int64 pts_prev;
} luavideo_seeker_s;



#if (!defined LUA_VERSION_NUM) || LUA_VERSION_NUM==501
#define lua_len(L,i) lua_pushinteger(L,lua_objlen(L,i))
#define lua_geti(L,i,n) lua_rawgeti(L,i,n)
#define lua_setuservalue(L,i) lua_setfenv((L),(i))
#define lua_getuservalue(L,i) lua_getfenv((L),(i))
#endif

static void luavideo_frame_receiver_close(luavideo_frame_receiver_s *fr) {
    if(fr->rawFrame != NULL) av_frame_free(&fr->rawFrame);
}

static void luavideo_packet_receiver_init(luavideo_packet_receiver_s *pr, thread_signal_t *signal, thread_queue_t *queue,thread_signal_t *readysignal, luavideo_packet_thread_s *thread) {
    pr->signal = signal;
    pr->queue = queue;
    pr->readysignal = readysignal;
    pr->thread = thread;
}

static void luavideo_avformat_close(luavideo_avformat_s *av) {
    if(av->avFormatContext != NULL) avformat_close_input(&av->avFormatContext);
    if(av->avCodecContext != NULL) avcodec_free_context(&av->avCodecContext);
    if(av->bufferSrcContext != NULL) avfilter_free(av->bufferSrcContext);
    if(av->bufferSinkContext != NULL) avfilter_free(av->bufferSinkContext);
    av->avFormatContext = NULL;
    av->avCodec = NULL;
    av->avCodecContext = NULL;
    av->avCodecParameters = NULL;
    av->avStream = NULL;
    av->bufferSrcContext = NULL;
    av->bufferSinkContext = NULL;
}

static void luavideo_codec_receiver_init(luavideo_codec_receiver_s *cr, luavideo_avformat_s *av, luavideo_packet_receiver_s *pr, int videoStreamIndex) {
    cr->avCodecContext  = av->avCodecContext;
    cr->packet_receiver = pr;
    cr->videoStreamIndex = videoStreamIndex;
    cr->packet = NULL;
}

static void luavideo_frame_receiver_init(luavideo_frame_receiver_s *fr, luavideo_avformat_s *av, luavideo_codec_receiver_s *cr) {
    fr->rawFrame = NULL;
    fr->rawFrame = av_frame_alloc();
    if(fr->rawFrame == NULL) abort();

    fr->bufferSrcContext = av->bufferSrcContext;
    fr->bufferSinkContext = av->bufferSinkContext;;
    fr->codec_receiver = cr;
    fr->pts_diff = (jpr_int64)av->avStream->r_frame_rate.den * (jpr_int64)av->avStream->time_base.den / (jpr_int64)av->avStream->r_frame_rate.num;
    fr->pts = 0 - fr->pts_diff;
}

static int luavideo_avformat_open(luavideo_avformat_s *av, luavideo_queue_t *q) {
    unsigned int i;
    int videoStreamIndex;
    av->avFormatContext = NULL;
    av->avCodec = NULL;
    av->avCodecContext = NULL;
    av->avCodecParameters = NULL;
    av->avStream = NULL;
    av->bufferSrcContext = NULL;
    av->bufferSinkContext = NULL;
    char args[512];

    if(avformat_open_input(&av->avFormatContext,q->url, NULL, NULL) < 0) {
        return -1;
    }

    if(avformat_find_stream_info(av->avFormatContext, NULL) < 0) {
        return -1;
    }

    for(i=0;i<av->avFormatContext->nb_streams;i++) {
        av->avFormatContext->streams[i]->discard = AVDISCARD_ALL;
    }

    videoStreamIndex = av_find_best_stream(av->avFormatContext,AVMEDIA_TYPE_VIDEO,-1,-1,&av->avCodec, 0);
    if(videoStreamIndex < 0) {
        return -1;
    }
    av->videoStreamIndex = videoStreamIndex;

    if(av->avCodec == NULL) {
        return -1;
    }

    av->avStream = av->avFormatContext->streams[videoStreamIndex];
    av->avStream->discard = AVDISCARD_DEFAULT;

    av->avCodecParameters = av->avStream->codecpar;

    av->avCodecContext = avcodec_alloc_context3(av->avCodec);
    if(av->avCodecContext == NULL) {
        return -1;
    }
    avcodec_parameters_to_context(av->avCodecContext,av->avCodecParameters);

    if(avcodec_open2(av->avCodecContext, av->avCodec, NULL) < 0) {
        return 1;
    }

    snprintf(args, sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
      av->avCodecContext->width, av->avCodecContext->height, av->avCodecContext->pix_fmt, av->avStream->time_base.num,
      av->avStream->time_base.den, av->avCodecContext->sample_aspect_ratio.num,
      av->avCodecContext->sample_aspect_ratio.den);
    if(avfilter_graph_create_filter(&av->bufferSrcContext, buffer_filter, "in", args, NULL, q->graph) < 0) {
        return 1;
    }
    if(avfilter_graph_create_filter(&av->bufferSinkContext, buffersink_filter, "out", NULL, NULL, q->graph) < 0) {
        return 1;
    }

    if(avfilter_link(av->bufferSrcContext, 0, q->filter_ctxs[0], 0) < 0) {
        return 1;
    }

    i = 0;
    while(i < q->filters - 1) {
        if(avfilter_link(q->filter_ctxs[i], 0, q->filter_ctxs[i+1], 0) < 0) {
            return 1;
        }
        i++;
    }

    if(avfilter_link(q->filter_ctxs[i], 0, av->bufferSinkContext, 0) < 0) {
        return 1;
    }
    if(avfilter_graph_config(q->graph, NULL) < 0) {
        return 1;
    }

    return 0;
}

static int luavideo_packet_feeder(void *userdata) {
    /* feeds packets into a queue, which acts as a buffer */
    AVPacket *packet;
    AVPacket *clone;
    int err;
    luavideo_packet_thread_s *packet_thread_data = (luavideo_packet_thread_s *)userdata;

    packet = av_packet_alloc();

    while(packet_thread_data->running) {
        err = av_read_frame(packet_thread_data->avFormatContext,packet);
        if(err < 0) break;
        clone = av_packet_clone(packet);
        if(clone == NULL) {
            abort();
        }
        av_packet_unref(packet);

        while(thread_queue_count(packet_thread_data->queue) == packet_thread_data->queueSize && packet_thread_data->running) {
            thread_signal_raise(packet_thread_data->outsignal);
            thread_signal_wait(packet_thread_data->signal,THREAD_SIGNAL_WAIT_INFINITE);
        }
        if(!packet_thread_data->running) {
            av_packet_free(&clone);
            break;
        }
        thread_queue_produce(packet_thread_data->queue,clone,THREAD_QUEUE_WAIT_INFINITE);
        thread_signal_raise(packet_thread_data->outsignal);
    }
    packet_thread_data->running = 0;
    av_packet_free(&packet);

    if(thread_queue_count(packet_thread_data->queue) < packet_thread_data->queueSize) {
        thread_queue_produce(packet_thread_data->queue,NULL,THREAD_QUEUE_WAIT_INFINITE);
    }
    thread_signal_raise(packet_thread_data->outsignal);

    return 0;
}


/* returns an AVPacket, which must be freed */
static AVPacket *luavideo_packet_receive(luavideo_packet_receiver_s *packet_receiver) {
    AVPacket *packet;

    while(thread_queue_count(packet_receiver->queue) == 0) {
        if(!packet_receiver->thread->running) return NULL;
        thread_signal_wait(packet_receiver->readysignal, THREAD_QUEUE_WAIT_INFINITE);
    }
    packet = thread_queue_consume(packet_receiver->queue,THREAD_QUEUE_WAIT_INFINITE);
    thread_signal_raise(packet_receiver->signal);
    return packet;
}

/* tries to pull a frame of data from the codec */
static int luavideo_codec_receive(luavideo_codec_receiver_s *codec_receiver, AVFrame *frame) {
    int err;
    while(1) {
          if( (err = avcodec_receive_frame(codec_receiver->avCodecContext,frame)) >= 0) {
              return err;
          }
          if(err != AVERROR(EAGAIN)) {
              return err;
          }
          while(codec_receiver->packet == NULL) {
              codec_receiver->packet = luavideo_packet_receive(codec_receiver->packet_receiver);
              if(codec_receiver->packet == NULL) {
                  return AVERROR_EOF;
              }
              if(codec_receiver->packet->stream_index != codec_receiver->videoStreamIndex) {
                  av_packet_free(&codec_receiver->packet);
                  if(codec_receiver->packet != NULL) abort();
                  continue;
              }
          }
          err = avcodec_send_packet(codec_receiver->avCodecContext,codec_receiver->packet);
          if(err >= 0) {
              av_packet_free(&codec_receiver->packet);
              continue;
          }
          if(err == AVERROR(EAGAIN)) continue;
    }
}

/* tries to pull a frame of data from the graph buffer */
static int luavideo_frame_receive(luavideo_frame_receiver_s *frame_receiver, AVFrame *frame) {
    int err;

    while(1) {
        if( (err = av_buffersink_get_frame(frame_receiver->bufferSinkContext,frame)) >= 0) {
            return err;
        }

        if(err != AVERROR(EAGAIN)) {
            return err;
        }

        err = luavideo_codec_receive(frame_receiver->codec_receiver,frame_receiver->rawFrame);
        if(err < 0) {
            return err;
        }

        frame_receiver->pts += frame_receiver->pts_diff;
        frame_receiver->pts_real = frame_receiver->rawFrame->pts;
        frame_receiver->rawFrame->pts = frame_receiver->pts;

        err = av_buffersrc_add_frame(frame_receiver->bufferSrcContext,frame_receiver->rawFrame);
        if(err < 0) {
            return err;
        }
    }
}

static int luavideo_format_seek(luavideo_seeker_s *seek_data, double ts) {
    /* handles seeking operations */
    AVFrame *frame = NULL;
    AVPacket *packet = NULL;
    int err = 0;

    ts = ts * seek_data->time_base_den / seek_data->time_base_num;

    /* drain all our buffered packets */
    while(thread_queue_count(seek_data->queue) > 0) {
        packet = thread_queue_consume(seek_data->queue,THREAD_QUEUE_WAIT_INFINITE);
        if(packet == NULL) {
            continue;
        }
        av_packet_free(&packet);
    }

    /* drain anything left in the codec */
    packet = av_packet_alloc();
    while( (err = av_read_frame(seek_data->avFormatContext,packet)) >= 0) {
        av_packet_unref(packet);
    }
    av_packet_free(&packet);

    /* drain anything left in the buffer */
    frame = av_frame_alloc();
    while( (err = av_buffersink_get_frame(seek_data->fr->bufferSinkContext,frame)) >= 0) {
        av_frame_unref(frame);
    }
    av_frame_free(&frame);

    return avformat_seek_file(seek_data->avFormatContext,seek_data->videoStreamIndex,0,(jpr_int64)ts,(jpr_int64)ts,0);
}

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
    luavideo_avformat_s av;
    luavideo_frame_receiver_s fr;
    luavideo_codec_receiver_s cr;
    luavideo_packet_receiver_s pr;
    luavideo_seeker_s seeker;
    luavideo_frame_t f;
    luavideo_queue_t *q;
    luavideo_packet_thread_s packet_thread_ctrl;
    luavideo_cmd_t *cmd;
    int err;
    AVFrame *frame;
    AVPacket *packet;

    thread_ptr_t packet_thread;
    thread_queue_t packet_queue;
    thread_signal_t packet_signal;
    thread_signal_t packet_ready_signal;
    void **packet_queue_storage;

    packet_thread = NULL;
    packet_queue_storage = NULL;
    frame = NULL;
    fr.rawFrame = NULL;
    packet_queue.values = NULL;

    q = (luavideo_queue_t *)userdata;

    if(luavideo_avformat_open(&av,q) < 0) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_process_cleanup;
    }

    packet_queue_storage = (void **)malloc(sizeof(void *) * q->buffer);
    if(packet_queue_storage == NULL) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_process_cleanup;
    }

    frame = av_frame_alloc();
    if(frame == NULL) {
        q->ctrl->status = LUAVIDEO_ERROR;
        goto luavideo_process_cleanup;
    }

    thread_queue_init(&packet_queue,q->buffer,packet_queue_storage,0);
    thread_signal_init(&packet_signal);
    thread_signal_init(&packet_ready_signal);

    packet_thread_ctrl.avFormatContext = av.avFormatContext;
    packet_thread_ctrl.queue = &packet_queue;
    packet_thread_ctrl.signal = &packet_signal;
    packet_thread_ctrl.outsignal = &packet_ready_signal;
    packet_thread_ctrl.queueSize = q->buffer;
    packet_thread_ctrl.running = 1;

    luavideo_frame_receiver_init(&fr,&av,&cr);
    luavideo_codec_receiver_init(&cr,&av,&pr,av.videoStreamIndex);
    luavideo_packet_receiver_init(&pr,&packet_signal,&packet_queue,&packet_ready_signal,&packet_thread_ctrl);

    seeker.avFormatContext = av.avFormatContext;
    seeker.queue = &packet_queue;
    seeker.fr = &fr;
    seeker.videoStreamIndex = av.videoStreamIndex;
    seeker.time_base_den = av.avStream->time_base.den;
    seeker.time_base_num = av.avStream->time_base.num;
    seeker.pts_prev = 0;

    /* start the packet-making thread */
    packet_thread = thread_create(luavideo_packet_feeder,&packet_thread_ctrl,"luavideo_packet_thread", THREAD_STACK_SIZE_DEFAULT);
    q->ctrl->status = LUAVIDEO_OK;

    f.channels = q->channels;
    f.timestamp = 0.0f;

    while(1) {
        while(thread_queue_count(&q->ctrl->cmd) > 0) {
            cmd = (luavideo_cmd_t *)thread_queue_consume(&q->ctrl->cmd,THREAD_QUEUE_WAIT_INFINITE);
            if(cmd == NULL) abort();

            switch(cmd->cmd) {
                case LUAVIDEO_STOP: {
                    q->ctrl->status = LUAVIDEO_DONE;
                    goto luavideo_process_cleanup;
                }

                case LUAVIDEO_FRAME: {
                    av_frame_unref(frame);
                    tryreceive:
                    if( (err = luavideo_frame_receive(&fr,frame)) < 0) {
                        if(err == AVERROR_EOF) {
                            q->ctrl->loops--;
                            if(q->ctrl->loops == 0) {
                                q->ctrl->status = LUAVIDEO_DONE;
                                goto luavideo_process_cleanup;
                            }
                            packet_thread_ctrl.running = 0;
                            thread_signal_raise(&packet_signal);
                            thread_join(packet_thread);
                            thread_destroy(packet_thread);
                            packet_thread = NULL;
                            avcodec_flush_buffers(av.avCodecContext);
#ifndef NDEBUG
                            packet_queue.id_produce_is_set.i = 0;
#endif
                            err = luavideo_format_seek(&seeker, cmd->dblparam);
#ifndef NDEBUG
                            packet_queue.id_produce_is_set.i = 0;
#endif
                            /* start the packet thread */
                            packet_thread_ctrl.running = 1;
                            packet_thread = thread_create(luavideo_packet_feeder,&packet_thread_ctrl,"luavideo_packet_thread", THREAD_STACK_SIZE_DEFAULT);
                            if(err >= 0) goto tryreceive;
                        }
                        goto luavideo_process_cleanup;

                    }
                    f.timestamp = (double)fr.pts_real * (double)av.avStream->time_base.num / (double)av.avStream->time_base.den;
                    f.width    = frame->width;
                    f.height   = frame->height;
                    f.data     = frame->data[0];
                    f.linesize = frame->linesize[0];
                    produce(queue,&f);
                    break;
                }

                case LUAVIDEO_SEEK: {
                    /* stop the packet thread */
                    packet_thread_ctrl.running = 0;
                    thread_signal_raise(&packet_signal);
                    thread_join(packet_thread);
                    thread_destroy(packet_thread);
                    packet_thread = NULL;
                    avcodec_flush_buffers(av.avCodecContext);

                    /* prevent errors from cropping up before we start the
                     * thread up again */
#ifndef NDEBUG
                    packet_queue.id_produce_is_set.i = 0;
#endif

                    luavideo_format_seek(&seeker, cmd->dblparam);

#ifndef NDEBUG
                    packet_queue.id_produce_is_set.i = 0;
#endif

                    /* start the packet thread */
                    packet_thread_ctrl.running = 1;
                    packet_thread = thread_create(luavideo_packet_feeder,&packet_thread_ctrl,"luavideo_packet_thread", THREAD_STACK_SIZE_DEFAULT);
                    break;
                }
                default: {
                    abort();
                }
            }
        }
        thread_signal_wait(&q->ctrl->cmd_signal,THREAD_SIGNAL_WAIT_INFINITE);
    }

    luavideo_process_cleanup:
    if(packet_thread != NULL) {
        packet_thread_ctrl.running = 0;
        thread_signal_raise(&packet_signal);
        thread_join(packet_thread);
        thread_destroy(packet_thread);
        packet_thread = NULL;
    }
    if(packet_queue.values != NULL) {
        while(thread_queue_count(&packet_queue) > 0) {
            packet = thread_queue_consume(&packet_queue,THREAD_QUEUE_WAIT_INFINITE);
            if(packet != NULL) av_packet_free(&packet);
        }
    }
    if(frame != NULL) av_frame_free(&frame);
    luavideo_avformat_close(&av);
    luavideo_frame_receiver_close(&fr);
    luavideo_queue_cleanup(NULL,q);
    if(packet_queue_storage != NULL) free(packet_queue_storage);
    thread_signal_term(&packet_signal);
    thread_queue_term(&packet_queue);
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
    int buffer = 10;

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

    lua_getfield(L,1,"buffer");
    buffer = luaL_optinteger(L,-1,10);
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
    v->filter_ctxs = filter_ctxs;
    v->filters = filters;
    v->ctrl = ctrl;
    v->channels = channels;
    v->buffer = buffer;

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
