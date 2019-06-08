#include <stdio.h>
#include <string.h>
#include "audio-decoder.h"
#include "audio-processor.h"
#include "video-generator.h"
#include "str.h"

static audio_decoder *decoder;
static audio_processor *processor;
static video_generator *generator;

int main(int argc, char **argv) {

    if(argc < 4) {
        fprintf(stderr,"Usage: %s songfile scriptfile output\n",argv[0]);
        return 1;
    }
    FILE *f = NULL;
    const char *songfile = argv[1];
    const char *scriptfile = argv[2];
    const char *output = argv[3];
    unsigned int r = 0;

    if(str_cmp(output,"-") == 0) {
        f = stdout;
    } else {
        f = fopen(output,"wb");
        if(f == NULL) {
            fprintf(stderr,"error opening %s for output\n",output);
            return 1;
        }
    }

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(decoder == NULL) return 1;
    processor = (audio_processor *)malloc(sizeof(audio_processor));
    if(processor == NULL) return 1;
    generator = (video_generator *)malloc(sizeof(video_generator));
    if(generator == NULL) return 1;

    if(video_generator_init(generator,processor,decoder,songfile,scriptfile,f)) {
        fprintf(stderr,"error starting the video generator\n");
        return 1;
    }

    do {
        r = video_generator_loop(generator);
    } while (r == 0);

    video_generator_close(generator);

    fclose(f);
    free(decoder);
    free(processor);
    free(generator);

    return 0;
}
