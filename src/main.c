#include <stdio.h>
#include <string.h>
#include "audio-decoder.h"
#include "audio-processor.h"
#include "video-generator.h"

int main(int argc, const char *argv[]) {
    int r = 0;
    audio_decoder *decoder;
    audio_processor *processor;
    video_generator *generator;

    decoder = (audio_decoder *)malloc(sizeof(audio_decoder));
    if(decoder == NULL) return 1;
    processor = (audio_processor *)malloc(sizeof(audio_processor));
    if(processor == NULL) return 1;
    generator = (video_generator *)malloc(sizeof(video_generator));
    if(generator == NULL) return 1;

    if(video_generator_init(generator,processor,decoder,argv[1],argv[2])) return 1;

    do {
        r = video_generator_loop(generator);
    } while (r == 0);

    video_generator_close(generator);
    return 0;
}
