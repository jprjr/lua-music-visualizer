#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include "audio-decoder.h"
#include "audio-processor.h"
#include "video-generator.h"
#include "jpr_proc.h"
#include "str.h"
#include "scan.h"
#include "version.h"

static int should_reload = 0;
static int should_quit = 0;

static void catch_sig(int signo) {
    signal(signo,catch_sig);
    switch(signo) {
        case SIGINT: should_reload = 1; return;
        case SIGTERM: should_quit = 1; return;
    }
}

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

    /* ignore signals while init-ing the video generator */
    signal(SIGINT,SIG_IGN);
    signal(SIGTERM,SIG_IGN);

    if(video_generator_init(generator,processor,decoder,songfile,scriptfile,&f)) {
        fprintf(stderr,"error starting the video generator\n");
        quit(1,decoder,processor,generator,NULL);
        return 1;
    }

    signal(SIGINT,catch_sig);
    signal(SIGTERM,catch_sig);

    while(video_generator_loop(generator) == 0) {
        if(should_reload) {
            if(video_generator_reload(generator) != 0) break;
            should_reload = 0;
        }
        if(should_quit) break;
    }

    video_generator_close(generator);

    jpr_proc_pipe_close(&f);
    free(decoder);
    free(processor);
    free(generator);

    return 0;
}
