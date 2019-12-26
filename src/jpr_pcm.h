#ifndef JPR_PCM_H
#define JPR_PCM_H

#include <stddef.h>
#include <stdint.h>

typedef size_t (* jprpcm_read_proc)(void* userdata, void* buf, size_t len);
typedef uint32_t (* jprpcm_seek_proc)(void* userdata, int offset, unsigned int origin);

struct jprpcm_s {
    void *userdata;
    jprpcm_read_proc readProc;
    jprpcm_seek_proc seekProc;
    unsigned int totalPCMFrameCount;
    unsigned int samplerate;
    unsigned int channels;
};

typedef struct jprpcm_s jprpcm;

#ifdef __cplusplus
extern "C" {
#endif

jprpcm *jprpcm_open(jprpcm_read_proc readProc, jprpcm_seek_proc seekProc, void *userdata, unsigned int samplerate, unsigned int channels);
unsigned int jprpcm_read_pcm_frames_s16(jprpcm *pcm, unsigned int framecount, int16_t *buf);
void jprpcm_close(jprpcm *pcm);


#ifdef __cplusplus
}
#endif
#endif /* JPR_PCM_H */

#ifdef JPR_PCM_IMPLEMENTATION

jprpcm *jprpcm_open(jprpcm_read_proc readProc, jprpcm_seek_proc seekProc, void *userdata, unsigned int samplerate, unsigned int channels) {
    jprpcm *pcm;
    uint32_t bytes;

    pcm = (jprpcm *)mem_alloc(sizeof(jprpcm));
    if(pcm == NULL) {
        return NULL;
    }
    pcm->readProc = readProc;
    pcm->seekProc = seekProc;
    pcm->userdata = userdata;

    bytes = pcm->seekProc(pcm->userdata,0,2);
    if(bytes > 0) {
        pcm->totalPCMFrameCount = bytes / samplerate / channels / sizeof(int16_t);
        pcm->seekProc(pcm->userdata,0,0);
    }

    pcm->samplerate = samplerate;
    pcm->channels = channels;

    return pcm;
}

unsigned int jprpcm_read_pcm_frames_s16(jprpcm *pcm, unsigned int framecount, int16_t *buf) {
    return pcm->readProc(pcm->userdata,buf,sizeof(int16_t) * pcm->channels *framecount);
}

void jprpcm_close(jprpcm *pcm) {
    mem_free(pcm);
}

#endif

