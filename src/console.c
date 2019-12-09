#include <stdio.h>
#include <string.h>
#include <stdarg.h>
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
#ifndef _WIN32
#include "thread.h"
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
            case SIGUSR1: break;
#endif
#ifdef SIGHUP
            case SIGHUP: break;
#endif
#ifdef SIGINT
            case SIGINT: thread_exit(0); break;
#endif
#ifdef SIGTERM
            case SIGTERM: thread_exit(0); break;
#endif
            default: thread_exit(1); break;
        }
        sig = (int *)malloc(sizeof(int));
    }
    thread_exit(1);
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
    fprintf(stderr,"Usage: %s [options] songfile scriptfile program ..\n",self);
    fprintf(stderr,"Options:\n");
    fprintf(stderr,"  -h\n");
    fprintf(stderr,"  --help\n");
    fprintf(stderr,"  --width=width\n");
    fprintf(stderr,"  --height=height\n");
    fprintf(stderr,"  --fps=fps\n");
    fprintf(stderr,"  --bars=bars\n");
    fprintf(stderr,"  --samplerate=samplerate (enables raw input)\n");
    fprintf(stderr,"  --channels (enables raw input)\n");
    fprintf(stderr,"  --version (prints version and exits)\n");
    return e;
}

static int version(void) {
    fprintf(stdout,"%s\n",lua_music_vis_version);
    fflush(stdout);
    return 0;
}


__attribute__((noreturn))
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

    exit(e);
}

int main(int argc, const char * const* argv) {
    const char *self;
    const char *songfile;
    const char *scriptfile;
    const char *s;
    unsigned int c;

    audio_decoder *decoder;
    audio_processor *processor;
    video_generator *generator;
#ifndef _WIN32
    thread_ptr_t signal_thread;
    thread_queue_t queue;
    int sig_queue[10];
    int *sig;
#endif

    unsigned int width      = 0;
    unsigned int height     = 0;
    unsigned int fps        = 0;
    unsigned int bars       = 0;
    unsigned int samplerate = 0;
    unsigned int channels   = 0;

    jpr_proc_pipe f;
    jpr_proc_info i;

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
            if(c < str_len(*argv)) {
                s = *argv + c + 1;
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
            if(c < str_len(*argv)) {
                s = *argv + c + 1;
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
            if(c < str_len(*argv)) {
                s = *argv + c + 1;
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
        else if(str_istarts(*argv,"--bars")) {
            c = str_chr(*argv,'=');
            if(c < str_len(*argv)) {
                s = *argv + c + 1;
            } else {
                argv++;
                argc--;
                s = *argv;
            }
            if(scan_uint(s,&bars) == 0) {
                return usage(self,1);
            }
            argv++;
            argc--;
        }
        else if(str_istarts(*argv,"--channels")) {
            c = str_chr(*argv,'=');
            if(c < str_len(*argv)) {
                s = *argv + c + 1;
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
            if(c < str_len(*argv)) {
                s = *argv + c + 1;
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
    if(signal_thread == NULL) {
        quit(1,NULL);
    }
#endif

    if(jpr_proc_info_init(&i)) return 1;
    if(jpr_proc_pipe_init(&f)) return 1;

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(decoder == NULL) quit(1,NULL);
    processor = (audio_processor *)malloc(sizeof(audio_processor));
    if(processor == NULL) quit(1,decoder,NULL);
    generator = (video_generator *)malloc(sizeof(video_generator));
    if(generator == NULL) quit(1,decoder,processor,NULL);

    if(width == 0) width     =       1280;
    if(height == 0) height   =        720;
    if(fps == 0) fps         =         30;

    generator->width         =      width;
    generator->height        =     height;
    generator->fps           =        fps;
    processor->spectrum_bars =       bars;
    decoder->samplerate      = samplerate;
    decoder->channels        =   channels;

    if(jpr_proc_spawn(&i,argv,&f,NULL,NULL)) {
        fprintf(stderr,"error spawning process\n");
        quit(1,decoder,processor,generator,NULL);
        return 1;
    }

    if(video_generator_init(generator,processor,decoder,songfile,scriptfile,&f)) {
        fprintf(stderr,"error starting the video generator\n");
        fflush(stderr);
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
    kill(getpid(),SIGTERM);

    while(thread_queue_count(&queue) > 0) {
        sig = thread_queue_consume(&queue);
        free(sig);
    }
#endif

    video_generator_close(generator);

    jpr_proc_pipe_close(&f);
    free(decoder);
    free(processor);
    free(generator);
#ifndef _WIN32
    thread_join(signal_thread);
    thread_destroy(signal_thread);
    thread_queue_term(&queue);
#endif

    return 0;
}
