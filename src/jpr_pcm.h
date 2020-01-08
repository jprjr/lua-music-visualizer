#ifndef JPR_PCM_H
#define JPR_PCM_H

#include "int.h"
#include <stdlib.h>

typedef size_t (* jprpcm_read_proc)(void* userdata, void* buf, size_t len);
typedef jpr_uint32 (* jprpcm_seek_proc)(void* userdata, int offset, unsigned int origin);

struct jprpcm_s {
    void *userdata;
    jprpcm_read_proc readProc;
    jprpcm_seek_proc seekProc;
    jpr_uint64 totalPCMFrameCount;
    unsigned int samplerate;
    unsigned int channels;
};

typedef struct jprpcm_s jprpcm;

#ifdef __cplusplus
extern "C" {
#endif

jprpcm *jprpcm_open(jprpcm_read_proc readProc, jprpcm_seek_proc seekProc, void *userdata, unsigned int samplerate, unsigned int channels);
jpr_uint64 jprpcm_read_pcm_frames_s16(jprpcm *pcm, jpr_uint64 framecount, jpr_int16 *buf);
void jprpcm_close(jprpcm *pcm);


#ifdef __cplusplus
}
#endif
#endif /* JPR_PCM_H */

#ifdef JPR_PCM_IMPLEMENTATION

jprpcm *jprpcm_open(jprpcm_read_proc readProc, jprpcm_seek_proc seekProc, void *userdata, unsigned int samplerate, unsigned int channels) {
    jprpcm *pcm;
    jpr_uint64 bytes;

    pcm = (jprpcm *)malloc(sizeof(jprpcm));
    if(pcm == NULL) {
        return NULL;
    }
    pcm->readProc = readProc;
    pcm->seekProc = seekProc;
    pcm->userdata = userdata;

    bytes = pcm->seekProc(pcm->userdata,0,2);
    if(bytes > 0) {
        pcm->totalPCMFrameCount = bytes / samplerate / channels / sizeof(jpr_int16);
        pcm->seekProc(pcm->userdata,0,0);
    }

    pcm->samplerate = samplerate;
    pcm->channels = channels;

    return pcm;
}

jpr_uint64 jprpcm_read_pcm_frames_s16(jprpcm *pcm, jpr_uint64 framecount, jpr_int16 *buf) {
    size_t r;
	size_t req;
    req = sizeof(jpr_int16) * pcm->channels * (size_t)framecount;
    r = pcm->readProc(pcm->userdata,buf,req);
    return (jpr_uint64)r;
}

void jprpcm_close(jprpcm *pcm) {
    free(pcm);
}

#endif

