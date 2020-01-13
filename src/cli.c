#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "attr.h"
#ifndef _WIN32
#include <signal.h>
#endif
#include "audio-decoder.h"
#include "audio-processor.h"
#include "video-generator.h"
#include "jpr_proc.h"
#include "str.h"
#include "scan.h"
#include "version.h"
#include "int.h"
#ifndef _WIN32
#include "thread.h"
#endif
#include "util.h"

#include <stdlib.h>
#ifndef NDEBUG
#include "stb_leakcheck.h"
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
    WRITE_STDERR("  --bars=bars\n");
    WRITE_STDERR("  --samplerate=samplerate (enables raw input)\n");
    WRITE_STDERR("  --channels (enables raw input)\n");
    WRITE_STDERR("  --version (prints version and exits)\n");
    return e;
}

static int version(void) {
    jpr_file *f;
    f = file_open("-","w");
    if(UNLIKELY(f == NULL)) return 1;
    file_write(f,lua_music_vis_version,lua_music_vis_version_len);
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
    char *c;

    audio_decoder *decoder;
    audio_processor *processor;
    video_generator *generator;
#ifndef _WIN32
    thread_ptr_t signal_thread;
    thread_queue_t queue;
    int sig_queue[10];
    int *sig;
#endif

    jpr_uint64 temp_width      = 0;
    jpr_uint64 temp_height     = 0;
    jpr_uint64 temp_fps        = 0;
    jpr_uint64 temp_bars       = 0;
    jpr_uint64 temp_samplerate = 0;
    jpr_uint64 temp_channels   = 0;

    unsigned int width      = 0;
    unsigned int height     = 0;
    unsigned int fps        = 0;
    unsigned int bars       = 0;
    unsigned int samplerate = 0;
    unsigned int channels   = 0;

    jpr_proc_pipe f;
    jpr_proc_info i;
    int exitcode;

    self = *argv++;
    argc--;

    while(argc > 0) {
        if(str_equals(*argv,"--")) {
            argv++;
            argc--;
            break;
        }
        else if(str_equals(*argv,"--version")) {
            return version();
        }
        else if(str_equals(*argv,"--help")) {
            return usage(self,0);
        }
        else if(str_equals(*argv,"-h")) {
            return usage(self,0);
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
            if(scan_uint(s,&temp_width) == 0) {
                return usage(self,1);
            }
            width = (unsigned int)temp_width;
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
            if(scan_uint(s,&temp_height) == 0) {
                return usage(self,1);
            }
            height = (unsigned int)temp_height;
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
            if(scan_uint(s,&temp_fps) == 0) {
                return usage(self,1);
            }
            fps = (unsigned int)temp_fps;
            argv++;
            argc--;
        }
        else if(str_istarts(*argv,"--bars")) {
            c = str_chr(*argv,'=');
            if(c != NULL) {
                s = &c[1];
            } else {
                argv++;
                argc--;
                s = *argv;
            }
            if(scan_uint(s,&temp_bars) == 0) {
                return usage(self,1);
            }
            bars = (unsigned int)temp_bars;
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
            if(scan_uint(s,&temp_channels) == 0) {
                return usage(self,1);
            }
            channels = (unsigned int)temp_channels;
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
            if(scan_uint(s,&temp_samplerate) == 0) {
                return usage(self,1);
            }
            samplerate = (unsigned int)temp_samplerate;
            argv++;
            argc--;
        }
        else {
            break;
        }
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
    signal_thread = thread_create(signal_thread_proc,&queue,NULL,THREAD_STACK_SIZE_DEFAULT);
    if(UNLIKELY(signal_thread == NULL)) {
        quit(1,NULL);
    }
#endif

    if(jpr_proc_info_init(&i)) return 1;
    if(jpr_proc_pipe_init(&f)) return 1;

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(UNLIKELY(decoder == NULL)) quit(1,NULL);
    processor = (audio_processor *)malloc(sizeof(audio_processor));
    if(UNLIKELY(processor == NULL)) quit(1,decoder,NULL);
    generator = (video_generator *)malloc(sizeof(video_generator));
    if(UNLIKELY(generator == NULL)) quit(1,decoder,processor,NULL);

    if(width == 0) width     =       1280;
    if(height == 0) height   =        720;
    if(fps == 0) fps         =         30;

    generator->width         =      width;
    generator->height        =     height;
    generator->fps           =        fps;
    processor->spectrum_bars =       bars;
    decoder->samplerate      = samplerate;
    decoder->channels        =   channels;

    if(jpr_proc_spawn(&i,(const char * const *)argv,&f,NULL,NULL)) {
        LOG_ERROR("error spawning process");
        quit(1,decoder,processor,generator,NULL);
        return 1;
    }

    if(video_generator_init(generator,processor,decoder,songfile,scriptfile,&f)) {
        LOG_ERROR("error starting the video generator");
        quit(1,decoder,processor,generator,NULL);
        return 1;
    }

    while(video_generator_loop(generator) == 0) {
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
#ifndef _WIN32
    quitting:
    /* signal thread may still be waiting on a signal
     * if we get here because of end-of-file */
    kill(getpid(),SIGINT);
#endif
    video_generator_close(generator);
    jpr_proc_pipe_close(&f);
    jpr_proc_info_kill(&i);
    jpr_proc_info_wait(&i,&exitcode);
    free(decoder);
    free(processor);
    free(generator);
#ifndef _WIN32
    while(thread_queue_count(&queue) > 0) {
        sig = thread_queue_consume(&queue);
        free(sig);
    }

    thread_join(signal_thread);
    thread_destroy(signal_thread);
    thread_queue_term(&queue);
#endif

    return 0;
}
