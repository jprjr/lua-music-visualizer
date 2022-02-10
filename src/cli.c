#include <string.h>
#include <stdarg.h>
#include "attr.h"
#ifndef _WIN32
#include <signal.h>
#endif
#include "audio-decoder.h"
#include "audio-resampler.h"
#include "audio-processor.h"
#include "video-generator.h"
#include "jpr_proc.h"
#include "str.h"
#include "scan.h"
#include "version.h"
#include "int.h"
#include "fmt.h"
#ifndef _WIN32
#include "thread.h"
#endif
#include "util.h"

#include <stdlib.h>

#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

#include "license.h"
#include "license_lua.h"
#include "license_libsamplerate.h"
#ifdef USE_FFTW3
#include "license_fftw3.h"
#else
#include "license_kissfft.h"
#endif

#if DECODE_SPC
#include "license_snes_spc.h"
#endif

#if DECODE_NSF
#include "license_nsfplay.h"
#endif

#if DECODE_VGM
#include "license_libvgm.h"
#endif

#ifndef _WIN32
static int signal_thread_proc(void *userdata) {
    sigset_t sigset;
    int *sig;
    thread_queue_t *queue = (thread_queue_t *)userdata;
    sigemptyset(&sigset);
#ifdef SIGINT
    sigaddset(&sigset,SIGINT);
#endif
#ifdef SIGTERM
    sigaddset(&sigset,SIGTERM);
#endif
#ifdef SIGHUP
    sigaddset(&sigset,SIGHUP);
#endif
#ifdef SIGUSR1
    sigaddset(&sigset,SIGUSR1);
#endif
    sig = (int *)malloc(sizeof(int));
    while( sigwait(&sigset,sig) == 0) {
        thread_queue_produce(queue,sig);
        switch(*sig) {
#ifdef SIGUSR1
            case SIGUSR1: {
                break;
            }
#endif
#ifdef SIGHUP
            case SIGHUP: {
                break;
            }
#endif
#ifdef SIGINT
            case SIGINT: {
                thread_exit(0);
                UNREACHABLE
                break;
            }
#endif
#ifdef SIGTERM
            case SIGTERM: {
                thread_exit(0);
                UNREACHABLE
                break;
            }
#endif
            default: {
                thread_exit(1);
                UNREACHABLE
                break;
            }
        }
        sig = (int *)malloc(sizeof(int));
    }
    thread_exit(1);
    UNREACHABLE
    return 1;
}

static void block_signals(void) {
    sigset_t sigset;
    sigemptyset(&sigset);
#ifdef SIGINT
    sigaddset(&sigset,SIGINT);
#endif
#ifdef SIGTERM
    sigaddset(&sigset,SIGTERM);
#endif
#ifdef SIGHUP
    sigaddset(&sigset,SIGHUP);
#endif
#ifdef SIGUSR1
    sigaddset(&sigset,SIGUSR1);
#endif
    sigprocmask(SIG_BLOCK,&sigset,NULL);
    return;
}
#endif

static int usage(const char *self, int e) {
    WRITE_STDERR("Usage: ");
    WRITE_STDERR(self);
    WRITE_STDERR(" [options] songfile scriptfile program ...\n");
    WRITE_STDERR("Options:\n");
    WRITE_STDERR("  -h\n");
    WRITE_STDERR("  --help\n");
    WRITE_STDERR("  --width=width\n");
    WRITE_STDERR("  --height=height\n");
    WRITE_STDERR("  --fps=fps\n");
    WRITE_STDERR("  --samplerate=samplerate (enables raw input)\n");
    WRITE_STDERR("  --channels (enables raw input)\n");
    WRITE_STDERR("  --resample=samplerate (force output audio sample rate)\n");
    WRITE_STDERR("  --version (prints version and exits)\n");
    WRITE_STDERR("  --plugins (prints enabled plugins and exits)\n");
    WRITE_STDERR("  --probe (lists file info)\n");
    WRITE_STDERR("  -l<module> -- calls require(<module>)\n");
    WRITE_STDERR("  -joff -- turns off JIT\n");
    return e;
}

static const char *separator =
    "----------"
    "----------"
    "----------"
    "----------"
    "----------"
    "----------"
    "----------"
    "--";

static void writetext(jpr_file *f,const char *l) {
    file_write(f,l,str_len(l));
}
static void writetextln(jpr_file *f,const char *l) {
    writetext(f,l);
#ifdef JPR_WINDOWS
    writetext(f,"\r\n");
#else
    writetext(f,"\n");
#endif
}

static int about(void) {
    jpr_file *f;
    const char * const *line;
    f = file_open("-","wb");
    if(UNLIKELY(f == NULL)) {
        WRITE_STDERR("about: unable to open stdout\n");
        return 1;
    }

    writetext(f,"lua-music-visualizer ");
    writetext(f,lua_music_vis_version);
    writetextln(f," (MIT)");

    writetextln(f,"Third-party libraries:");

    writetextln(f,"libsamplerate (MIT)");

    if(video_generator_using_luajit()) {
        writetextln(f,"LuaJIT (MIT)");
    } else {
        writetextln(f,"Lua (MIT)");
    }

#ifdef USE_FFTW3
    writetextln(f,"FFTW3 (GPL2)");
#else
    writetextln(f,"KISSFFT (BSD-3-Clause)");
#endif

#if DECODE_SPC
    writetextln(f,"SNES_SPC (LGPL-2)");
#endif

#if DECODE_NSF
    writetextln(f,"NSFPlay (NSFPlay License)");
#endif

#if DECODE_VGM
    writetextln(f,"libvgm (unknown)");
#endif

    writetextln(f,"");
    writetextln(f,separator);
    writetextln(f,"");

    writetextln(f,"[Begin lua-music-visualizer license]");
    line = license;
    while(*line != NULL) {
        writetextln(f,*line);
        line++;
    }
    writetextln(f,"[End lua-music-visualizer license]");

    writetextln(f,"");
    writetextln(f,separator);
    writetextln(f,"");

    writetextln(f,"[Begin libsamplerate license]");
    line = license_libsamplerate;
    while(*line != NULL) {
        writetextln(f,*line);
        line++;
    }
    writetextln(f,"[End libsamplerate license]");

    writetextln(f,"");
    writetextln(f,separator);
    writetextln(f,"");

    if(video_generator_using_luajit()) {
        writetextln(f,"[Begin LuaJIT license]");
    } else {
        writetextln(f,"[Begin Lua license]");
    }
    line = license_lua;
    while(*line != NULL) {
        writetextln(f,*line);
        line++;
    }
    if(video_generator_using_luajit()) {
        writetextln(f,"[End LuaJIT License]");
    } else {
        writetextln(f,"[End Lua License]");
    }

    writetextln(f,"");
    writetextln(f,separator);
    writetextln(f,"");

#ifdef USE_FFTW3
    writetextln(f,"[Begin FFTW3 license]");
#else
    writetextln(f,"[Begin KISSFFT license]");
#endif

    line = license_fft;
    while(*line != NULL) {
        writetextln(f,*line);
        line++;
    }

#ifdef USE_FFTW3
    writetextln(f,"[End FFTW3 license]");
#else
    writetextln(f,"[End KISSFFT license]");
#endif

#if DECODE_SPC
    writetextln(f,"");
    writetextln(f,separator);
    writetextln(f,"");
    writetextln(f,"[Begin SNES_SPC license]");

    line = license_snes_spc;
    while(*line != NULL) {
        writetextln(f,*line);
        line++;
    }
    writetextln(f,"[End SNES_SPC license]");
#endif

#if DECODE_NSF
    writetextln(f,"");
    writetextln(f,separator);
    writetextln(f,"");

    writetextln(f,"[Begin NSFPlay license]");

    line = license_nsfplay;
    while(*line != NULL) {
        writetextln(f,*line);
        line++;
    }
    writetextln(f,"[End NSFPlay license]");
#endif

#if DECODE_VGM
    writetextln(f,"");
    writetextln(f,separator);
    writetextln(f,"");

    writetextln(f,"[Begin libvgm license]");

    line = license_libvgm;
    while(*line != NULL) {
        writetextln(f,*line);
        line++;
    }
    writetextln(f,"[End libvgm license]");
#endif

    file_close(f);
    return 0;
}

static int probe(const char *filename) {
    audio_info *info;
    jpr_file *f;
    audio_decoder *decoder;
    unsigned int i;
    char tracknum[4];

    f = file_open("-","w");
    if(UNLIKELY(f == NULL)) {
        WRITE_STDERR("probe: unable to open stdout\n");
        return 1;
    }

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(UNLIKELY(decoder == NULL)) return 1;

    audio_decoder_init(decoder);

    info = audio_decoder_probe(decoder,filename);
    if(info == NULL) {
        return 1;
    }

    if(info->artist) {
        file_write(f,"Artist: ",8);
        file_write(f,info->artist,str_len(info->artist));
        file_write(f,"\n",1);
    }
    if(info->album) {
        file_write(f,"Album: ",8);
        file_write(f,info->album,str_len(info->album));
        file_write(f,"\n",1);
    }

    for(i=0;i<info->total;i++) {
        fmt_uint(tracknum,i+1);
        writetext(f,tracknum);
        if(info->tracks[i].title[0]) {
            writetext(f,": ");
        }
        writetextln(f,info->tracks[i].title);
    }

    audio_decoder_free_info(info);
    return 0;
}

static int plugins(void) {
    jpr_file *f;
    const audio_plugin* const* plugin;
    plugin = plugin_list;
    f = file_open("-","w");
    if(UNLIKELY(f == NULL)) return 1;
    while(*plugin != NULL) {
        file_write(f,(*plugin)->name,str_len((*plugin)->name));
        file_write(f,"\n",1);
        plugin++;
    }
    file_close(f);
    return 0;
}

static int version(void) {
    jpr_file *f;
    f = file_open("-","w");
    if(UNLIKELY(f == NULL)) return 1;
    writetextln(f,lua_music_vis_version);
    file_close(f);
    return 0;
}

attr_noreturn
static void quit(int e,...) {
    va_list ap;
    void *p = NULL;

    va_start(ap,e);
    do {
        p = va_arg(ap,void *);
        if(p != NULL) {
            free(p);
        }
    } while(p != NULL);

    JPR_EXIT(e);
}

int cli_start(int argc, char **argv) {
    const char *self;
    const char *songfile;
    const char *scriptfile;
    const char *s;
    const char *module;
    int jit;
    char *c;
    char *sf = NULL;

    audio_decoder *decoder = NULL;
    audio_resampler *resampler = NULL;;
    audio_processor *processor = NULL;;
    video_generator *generator = NULL;;
#ifndef _WIN32
    thread_ptr_t signal_thread = NULL;
    thread_queue_t queue;
    int sig_queue[10];
    int *sig;
#endif

    unsigned int width      = 0;
    unsigned int height     = 0;
    unsigned int fps        = 0;
    unsigned int samplerate = 0;
    unsigned int resample   = 0;
    unsigned int channels   = 0;
    unsigned int tracknum   = 1;
    int do_probe = 0;
    float progress       = 0.0f;

    jpr_proc_pipe f;
    jpr_proc_info i;
    int exitcode;
    int r = 0;

    self = *argv++;
    argc--;

    module = NULL;
    jit = 1;

    while(argc > 0) {
        if(str_equals(*argv,"--")) {
            argv++;
            argc--;
            break;
        }
        else if(str_iequals(*argv,"--version")) {
            return version();
        }
        else if(str_iequals(*argv,"--plugins")) {
            return plugins();
        }
        else if(str_iequals(*argv,"--about")) {
            return about();
        }
        else if(str_iequals(*argv,"--help")) {
            return usage(self,0);
        }
        else if(str_iequals(*argv,"-h")) {
            return usage(self,0);
        }
        else if(str_iequals(*argv,"--probe")) {
            do_probe = 1;
            argv++;
            argc--;
        }
        else if(str_iequals(*argv,"-joff")) {
            jit = 0;
            argv++;
            argc--;
        }
        else if(str_istarts(*argv,"-l") && str_len(&((*argv)[2])) > 0) {
            module = &((*argv)[2]);
            argv++;
            argc--;
        }
        else if(str_istarts(*argv,"--width")) {
            c = str_chr(*argv,'=');
            if(c != NULL) {
                s = &c[1];
            } else {
                argv++;
                argc--;
                s = *argv;
            }
            if(scan_uint(s,&width) == 0) {
                return usage(self,1);
            }
            argv++;
            argc--;
        }
        else if(str_istarts(*argv,"--height")) {
            c = str_chr(*argv,'=');
            if(c != NULL) {
                s = &c[1];
            } else {
                argv++;
                argc--;
                s = *argv;
            }
            if(scan_uint(s,&height) == 0) {
                return usage(self,1);
            }
            argv++;
            argc--;
        }
        else if(str_istarts(*argv,"--fps")) {
            c = str_chr(*argv,'=');
            if(c != NULL) {
                s = &c[1];
            } else {
                argv++;
                argc--;
                s = *argv;
            }
            if(scan_uint(s,&fps) == 0) {
                return usage(self,1);
            }
            argv++;
            argc--;
        }
        else if(str_istarts(*argv,"--channels")) {
            c = str_chr(*argv,'=');
            if(c != NULL) {
                s = &c[1];
            } else {
                argv++;
                argc--;
                s = *argv;
            }
            if(scan_uint(s,&channels) == 0) {
                return usage(self,1);
            }
            argv++;
            argc--;
        }
        else if(str_istarts(*argv,"--samplerate")) {
            c = str_chr(*argv,'=');
            if(c != NULL) {
                s = &c[1];
            } else {
                argv++;
                argc--;
                s = *argv;
            }
            if(scan_uint(s,&samplerate) == 0) {
                return usage(self,1);
            }
            argv++;
            argc--;
        }
        else if(str_istarts(*argv,"--resample")) {
            c = str_chr(*argv,'=');
            if(c != NULL) {
                s = &c[1];
            } else {
                argv++;
                argc--;
                s = *argv;
            }
            if(scan_uint(s,&resample) == 0) {
                return usage(self,1);
            }
            argv++;
            argc--;
        }
        else {
            break;
        }
    }

    if(do_probe && argc > 0) {
        return probe(*argv);
    }

    if(argc < 3) {
        return usage(self,1);
    }

    songfile   = *argv++;
    scriptfile = *argv++;

#ifndef _WIN32
    sig_queue[0] = 0;
    thread_queue_init(&queue,10,(void **)&sig_queue,0);

    block_signals();
    signal_thread = thread_create(signal_thread_proc,&queue,"signal thread",THREAD_STACK_SIZE_DEFAULT);
    if(UNLIKELY(signal_thread == NULL)) {
        r = 1;
        goto cli_cleanup;
    }
#endif

    if(jpr_proc_info_init(&i)) return 1;
    if(jpr_proc_pipe_init(&f)) return 1;

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(UNLIKELY(decoder == NULL)) {
        r = 1;
        goto cli_cleanup;
    }

    resampler = (audio_resampler *)malloc(sizeof(audio_resampler));
    if(UNLIKELY(resampler == NULL)) {
        r = 1;
        goto cli_cleanup;
    }

    processor = (audio_processor *)malloc(sizeof(audio_processor));
    if(UNLIKELY(processor == NULL)) {
        r = 1;
        goto cli_cleanup;
    }

    generator = (video_generator *)malloc(sizeof(video_generator));
    if(UNLIKELY(generator == NULL)) {
        r = 1;
        goto cli_cleanup;
    }

    sf = str_dup(songfile);
    if(UNLIKELY(sf == NULL)) {
        r = 1;
        goto cli_cleanup;
    }

    c = str_chr(sf,'#');
    if(c != NULL) {
        *c = '\0';
        if(!scan_uint(&c[1],&tracknum)) {
            tracknum = 1;
        }
    }

    if(width == 0) width     =       1280;
    if(height == 0) height   =        720;
    if(fps == 0) fps         =         30;

    generator->width         =      width;
    generator->height        =     height;
    generator->fps           =        fps;
    decoder->samplerate      = samplerate;
    decoder->channels        =   channels;
    decoder->track           =   tracknum;
    resampler->samplerate    =   resample;

    if(jpr_proc_spawn(&i,(const char * const *)argv,&f,NULL,NULL)) {
        LOG_ERROR("error spawning process");
        r = 1;
        goto cli_cleanup;
    }

    if(video_generator_init(generator,processor,resampler,decoder,jit,module,sf,scriptfile,&f)) {
        LOG_ERROR("error starting the video generator");
        r = 1;
        goto cli_cleanup;
    }

    free(sf);
    sf = NULL;

    while(video_generator_loop(generator,&progress) == 0) {
#ifndef _WIN32
        if(thread_queue_count(&queue) > 0) {
            sig = thread_queue_consume(&queue);
            switch(*sig) {
#ifdef SIGINT
                case SIGINT: free(sig); goto quitting; break;
#endif
#ifdef SIGTERM
                case SIGTERM: free(sig); goto quitting; break;
#endif
#ifdef SIGHUP
                case SIGHUP: free(sig); video_generator_reload(generator); break;
#endif
#ifdef SIGUSR1
                case SIGUSR1: free(sig); video_generator_reload(generator); break;
#endif
                default: break;
            }
        }
#endif
    }

    cli_cleanup:
#ifndef _WIN32
    quitting:
    /* signal thread may still be waiting on a signal
     * if we get here because of end-of-file */
    kill(getpid(),SIGINT);
#endif
    if(generator != NULL) video_generator_close(generator);

    jpr_proc_pipe_close(&f);

    /* wait up to 30 seconds for video encoder to finish */
    if(jpr_proc_info_wait(&i,&exitcode,30) == 2) {
        /* send a term signal, wait 5 seconds */
        jpr_proc_info_term(&i);
        if(jpr_proc_info_wait(&i,&exitcode,5) == 2) {
            /* send a kill signal, something's wrong */
            jpr_proc_info_kill(&i);
            jpr_proc_info_wait(&i,&exitcode,5);
        }
    }

    if(sf != NULL) free(sf);
    if(decoder != NULL) free(decoder);
    if(resampler != NULL) free(resampler);
    if(processor != NULL) free(processor);
    if(generator != NULL) free(generator);
#ifndef _WIN32
    if(signal_thread != NULL) {
        thread_join(signal_thread);
        thread_destroy(signal_thread);
    }

    while(thread_queue_count(&queue) > 0) {
        sig = thread_queue_consume(&queue);
        free(sig);
    }

    thread_queue_term(&queue);
#endif

    return r;
}
