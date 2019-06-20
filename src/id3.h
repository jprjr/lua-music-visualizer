#ifndef ID3_H
#define ID3_H

#include <stdio.h>
#include "audio-decoder.h"

#ifdef __cplusplus
extern "C" {
#endif

void process_id3(audio_decoder *a, FILE *f);

#ifdef __cplusplus
}
#endif

#endif
