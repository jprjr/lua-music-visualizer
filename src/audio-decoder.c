#include "audio-decoder.h"
#include "id3.h"
#include "utf.h"
#include "str.h"
#include "unpack.h"
#include "jpr_proc.h"

#define AUDIO_MAX(a,b) ( a > b ? a : b)
#define AUDIO_MIN(a,b) ( a < b ? a : b)

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define JPR_PCM_IMPLEMENTATION
#include "jpr_pcm.h"

static void flac_meta(void *ctx, drflac_metadata *pMetadata) {
    audio_decoder *a = ctx;
    drflac_vorbis_comment_iterator iter;
    char buf[4096];
    int r = 0;
    const char *comment = NULL;
    unsigned int commentLength = 0;

    if(pMetadata->type != DRFLAC_METADATA_BLOCK_TYPE_VORBIS_COMMENT) return;
    if(a->onmeta == NULL) return;

    drflac_init_vorbis_comment_iterator(&iter,pMetadata->data.vorbis_comment.commentCount, pMetadata->data.vorbis_comment.pComments);
    while( (comment = drflac_next_vorbis_comment(&iter,&commentLength)) != NULL) {
        if(commentLength > 4095) continue;
        strncpy(buf,comment,commentLength);
        buf[commentLength] = 0;
        r = str_chr(buf,'=');
        if(buf[r]) {
            buf[r] = 0;
            str_lower(buf,buf);
            a->onmeta(a->meta_ctx,buf,buf+r+1);
        }
    }
}

static void wav_id3(audio_decoder *a, const char *filename) {
    if(a->onmeta == NULL) return;
    FILE *f = fopen(filename,"rb");
    uint32_t bytes = 0;
    uint32_t cbytes = 0;
    uint8_t buffer[12];
    if(f == NULL) {
        return;
    }
    if(fread(buffer,1,12,f) != 12) goto closereturn;
    if(str_ncmp((char *)buffer,"RIFF",4)) goto closereturn;
    if(str_ncmp((char *)buffer+8,"WAVE",4)) goto closereturn;
    bytes = unpack_uint32le(buffer+4);
    bytes -= 4;

    while(bytes > 0) {
        if(fread(buffer,1,12,f) != 12) goto closereturn;
        bytes -= 8;
        cbytes = unpack_uint32le(buffer+4);
        cbytes -= 4;
        if(str_incmp((char *)buffer,"id3 ",4) == 0) {
            process_id3(a,f);
            goto closereturn;
        }
        fseek(f,cbytes,SEEK_CUR);
        bytes -= cbytes;
    }

closereturn:
    fclose(f);
    return;
}

static void mp3_id3(audio_decoder *a, const char *filename) {
    if(a->onmeta == NULL) return;
    FILE *f = fopen(filename,"rb");
    if(f == NULL) {
        return;
    }
    process_id3(a,f);
    fclose(f);

}

int audio_decoder_init(audio_decoder *a) {
    a->framecount = 0;
    a->ctx.p = NULL;
    a->meta_ctx = NULL;
    a->type = -1;

    return 0;
}

int audio_decoder_open(audio_decoder *a, const char *filename) {
    drmp3 *mp3 = NULL;

    if(str_iends(filename,".flac")) {
        a->ctx.pFlac = drflac_open_file_with_metadata(filename,flac_meta,a);
        if(a->ctx.pFlac == NULL) return 1;

        a->framecount = a->ctx.pFlac->totalPCMFrameCount;
        a->samplerate = a->ctx.pFlac->sampleRate;
        a->channels = a->ctx.pFlac->channels;
        a->type = 0;
    }
    else if(str_iends(filename,".mp3")) {
        mp3_id3(a,filename);

        mp3 = (drmp3 *)malloc(sizeof(drmp3));
        if(mp3 == NULL) {
            fprintf(stderr,"out of memory\n");
            return 1;
        }
        a->ctx.pMp3 = mp3;
        if(!drmp3_init_file(a->ctx.pMp3,filename,NULL)) {
            fprintf(stderr,"error opening as mp3\n");
            free(mp3);
            a->ctx.pMp3 = NULL;
            return 1;
        }
        a->framecount = drmp3_get_pcm_frame_count(mp3);
        a->samplerate = mp3->sampleRate;
        a->channels = mp3->channels;
        a->type = 1;
    }
    else if(str_iends(filename,"wav")) {
        wav_id3(a,filename);

        a->ctx.pWav = drwav_open_file(filename);

        if(a->ctx.pWav == NULL) return 1;
        a->framecount = a->ctx.pWav->totalPCMFrameCount;
        a->samplerate = a->ctx.pWav->sampleRate;
        a->channels = a->ctx.pWav->channels;
        a->type = 2;
    }
    else if(a->samplerate != 0 && a->channels != 0) {
        a->ctx.pPcm = jprpcm_open_file(filename,a->samplerate,a->channels);
        if(a->ctx.pPcm == NULL) return 1;
        a->framecount = a->ctx.pPcm->totalPCMFrameCount;
        a->type = 3;
    }
    else {
        fprintf(stderr,"sorry, I don't support that file format\n");
    }

    if(a->ctx.p == NULL) return 1;
    return 0;
}

unsigned int audio_decoder_decode(audio_decoder *a, unsigned int framecount, int16_t *buf) {
    switch(a->type) {
        case 0: return (unsigned int)drflac_read_pcm_frames_s16(a->ctx.pFlac,framecount,buf);
        case 1: return (unsigned int)drmp3_read_pcm_frames_s16(a->ctx.pMp3,framecount,buf);
        case 2: return (unsigned int)drwav_read_pcm_frames_s16(a->ctx.pWav,framecount,buf);
        case 3: return jprpcm_read_pcm_frames_s16(a->ctx.pPcm,framecount,buf);
    }
    return 0;
}

void audio_decoder_close(audio_decoder *a) {
    switch(a->type) {
        case 0: {
            drflac_close(a->ctx.pFlac);
            a->ctx.pFlac = NULL;
            break;
        }
        case 1: {
            drmp3_uninit(a->ctx.pMp3);
            free(a->ctx.pMp3);
            a->ctx.pMp3 = NULL;
            break;
        }
        case 2: {
            drwav_close(a->ctx.pWav);
            a->ctx.pWav = NULL;
            break;
        }
        case 3: {
            jprpcm_close(a->ctx.pPcm);
            a->ctx.pPcm = NULL;
            break;
        }
    }
}

