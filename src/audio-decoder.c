#include "audio-decoder.h"
#include "str.h"

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

static void flac_meta(void *ctx, drflac_metadata *pMetadata) {
    audio_decoder *a = (audio_decoder *)ctx;
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
        str_ncpy(buf,comment,commentLength);
        r = str_chr(buf,'=');
        if(buf[r]) {
            buf[r] = 0;
            str_strlower(buf);
            a->onmeta(a->meta_ctx,buf,buf+r+1);
        }
    }

}

int audio_decoder_init(audio_decoder *a) {
    a->samplerate = 0;
    a->channels = 0;
    a->framecount = 0;
    a->ctx.p = NULL;
    a->meta_ctx = NULL;
    a->type = -1;

    return 0;
}

int audio_decoder_open(audio_decoder *a, const char *filename) {
    if(str_iends(filename,".flac")) {
        a->ctx.pFlac = drflac_open_file_with_metadata(filename,flac_meta,a);
        if(a->ctx.pFlac == NULL) return 1;
        a->framecount = a->ctx.pFlac->totalPCMFrameCount;
        a->samplerate = a->ctx.pFlac->sampleRate;
        a->channels = a->ctx.pFlac->channels;
        a->type = 0;
    }

    if(a->ctx.p == NULL) return 1;
    return 0;
}

unsigned int audio_decoder_decode(audio_decoder *a, unsigned int framecount, int16_t *buf) {
    if(a->type == 0) {
        return (unsigned int)drflac_read_pcm_frames_s16(a->ctx.pFlac,framecount,buf);
    }
    return 0;
}

void audio_decoder_close(audio_decoder *a) {
    switch(a->type) {
        case 0: drflac_close(a->ctx.pFlac); break;
    }
}

