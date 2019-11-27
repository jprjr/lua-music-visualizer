#ifndef JPR_PCM_H
#define JPR_PCM_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

struct jprpcm_s {
    FILE *f;
    unsigned int totalPCMFrameCount;
    unsigned int samplerate;
    unsigned int channels;
};

typedef struct jprpcm_s jprpcm;

#ifdef __cplusplus
extern "C" {
#endif

jprpcm *jprpcm_open_file(const char *filename, unsigned int samplerate, unsigned int channels);
unsigned int jprpcm_read_pcm_frames_s16(jprpcm *pcm, unsigned int framecount, int16_t *buf);
void jprpcm_close(jprpcm *pcm);


#ifdef __cplusplus
}
#endif
#endif

#ifdef JPR_PCM_IMPLEMENTATION

jprpcm *jprpcm_open_file(const char *filename, unsigned int samplerate, unsigned int channels) {
    jprpcm *pcm;
    FILE *f;

    pcm = (jprpcm *)malloc(sizeof(jprpcm));
    if(pcm == NULL) {
        return NULL;
    }

    if(strcmp(filename,"-") == 0) {
        f = stdin;
    } else {
        f = fopen(filename,"rb");
    }

    if(f == NULL) {
        fprintf(stderr,"error opening raw pcm: %s\n",strerror(errno));
        free(pcm);
        return NULL;
    }

    pcm->f = f;

    if(fseek(pcm->f,0,SEEK_END) == 0) {
        pcm->totalPCMFrameCount = ftell(pcm->f) / samplerate / channels / sizeof(int16_t);
        fseek(pcm->f,0,SEEK_SET);
    }

    pcm->samplerate = samplerate;
    pcm->channels = channels;

    return pcm;
}

unsigned int jprpcm_read_pcm_frames_s16(jprpcm *pcm, unsigned int framecount, int16_t *buf) {
    return fread(buf,sizeof(int16_t) * pcm->channels,framecount,pcm->f);
}

void jprpcm_close(jprpcm *pcm) {
    fclose(pcm->f);
    free(pcm);
}

#endif

