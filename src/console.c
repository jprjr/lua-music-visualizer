#include <stdio.h>
#include <string.h>
#include "audio-decoder.h"
#include "audio-processor.h"
#include "video-generator.h"
#include "jpr_proc.h"
#include "str.h"

static audio_decoder *decoder;
static audio_processor *processor;
static video_generator *generator;

int main(int argc, char **argv) {

    if(argc < 4) {
        fprintf(stderr,"Usage: %s songfile scriptfile output\n",argv[0]);
        return 1;
    }
    jpr_proc_pipe f;
    const char *songfile = argv[1];
    const char *scriptfile = argv[2];
    const char *output = argv[3];

    if(jpr_proc_pipe_open_file(&f,output,"wb")) return 1;

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(decoder == NULL) return 1;
    processor = (audio_processor *)malloc(sizeof(audio_processor));
    if(processor == NULL) return 1;
    generator = (video_generator *)malloc(sizeof(video_generator));
    if(generator == NULL) return 1;

    generator->width         =  1280;
    generator->height        =   720;
    generator->fps           =    30;
    processor->spectrum_bars =    24;
    decoder->samplerate      = 48000;
    decoder->channels        =     2;

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
