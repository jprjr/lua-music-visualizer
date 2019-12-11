#ifndef ID3_H
#define ID3_H

#include "audio-decoder.h"
#include "file.h"

#ifdef __cplusplus
extern "C" {
#endif

void process_id3(audio_decoder *a, jpr_file *f);

#ifdef __cplusplus
}
#endif

#endif
