#include <stdio.h>
#include <string.h>
#include "audio-decoder.h"
#include "audio-processor.h"
#include "video-generator.h"
#include "jpr_proc.h"
#include "str.h"
#include "scan.h"

int usage(const char *self, int e) {
    fprintf(stderr,"Usage: %s [options] songfile scriptfile program ..\n",self);
    fprintf(stderr,"Options:\n");
    fprintf(stderr,"  -h\n");
    fprintf(stderr,"  --help\n");
    fprintf(stderr,"  --width=width\n");
    fprintf(stderr,"  --height=height\n");
    fprintf(stderr,"  --fps=fps\n");
    fprintf(stderr,"  --bars=bars\n");
    return e;
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

    if(argc < 4) {
        return usage(self,1);
    }

    while(1) {
        if(str_equals(*argv,"--")) {
            argv++;
            break;
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
                s = *argv;
            }
            if(scan_uint(s,&width) == 0) {
                return usage(self,1);
            }
            argv++;
        }
        else if(str_istarts(*argv,"--height")) {
            c = str_chr(*argv,'=');
            if(c < str_len(*argv)) {
                s = *argv + c + 1;
            } else {
                argv++;
                s = *argv;
            }
            if(scan_uint(s,&height) == 0) {
                return usage(self,1);
            }
            argv++;
        }
        else if(str_istarts(*argv,"--fps")) {
            c = str_chr(*argv,'=');
            if(c < str_len(*argv)) {
                s = *argv + c + 1;
            } else {
                argv++;
                s = *argv;
            }
            if(scan_uint(s,&fps) == 0) {
                return usage(self,1);
            }
            argv++;
        }
        else if(str_istarts(*argv,"--bars")) {
            c = str_chr(*argv,'=');
            if(c < str_len(*argv)) {
                s = *argv + c + 1;
            } else {
                argv++;
                s = *argv;
            }
            if(scan_uint(s,&bars) == 0) {
                return usage(self,1);
            }
            argv++;
        }
        else if(str_istarts(*argv,"--channels")) {
            c = str_chr(*argv,'=');
            if(c < str_len(*argv)) {
                s = *argv + c + 1;
            } else {
                argv++;
                s = *argv;
            }
            if(scan_uint(s,&channels) == 0) {
                return usage(self,1);
            }
            argv++;
        }
        else if(str_istarts(*argv,"--samplerate")) {
            c = str_chr(*argv,'=');
            if(c < str_len(*argv)) {
                s = *argv + c + 1;
            } else {
                argv++;
                s = *argv;
            }
            if(scan_uint(s,&samplerate) == 0) {
                return usage(self,1);
            }
            argv++;
        }
        else {
            break;
        }
    }
    songfile   = *argv++;
    scriptfile = *argv++;

    if(jpr_proc_info_init(&i)) return 1;
    if(jpr_proc_pipe_init(&f)) return 1;

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(decoder == NULL) return 1;
    processor = (audio_processor *)malloc(sizeof(audio_processor));
    if(processor == NULL) return 1;
    generator = (video_generator *)malloc(sizeof(video_generator));
    if(generator == NULL) return 1;

    if(width == 0) width     =       1280;
    if(height == 0) height   =        720;
    if(fps == 0) fps         =         30;

    generator->width         =      width;
    generator->height        =     height;
    generator->fps           =        fps;
    processor->spectrum_bars =       bars;
    decoder->samplerate      = samplerate;
    decoder->channels        =   channels;

    if(jpr_proc_spawn(&i,argv,&f,NULL,NULL)) return 1;

    if(video_generator_init(generator,processor,decoder,songfile,scriptfile,&f)) {
        fprintf(stderr,"error starting the video generator\n");
        return 1;
    }

    while(video_generator_loop(generator) == 0);

    video_generator_close(generator);

    jpr_proc_pipe_close(&f);
    free(decoder);
    free(processor);
    free(generator);

    return 0;
}
