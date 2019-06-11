#include "audio-decoder.h"
#include "utf.h"
#include "str.h"
#include "unpack.h"

#define AUDIO_MAX(a,b) ( a > b ? a : b)
#define AUDIO_MIN(a,b) ( a < b ? a : b)

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

static const uint8_t allzero[10] = "\0\0\0\0\0\0\0\0\0\0";

struct str_alloc_s {
    char *s;
    unsigned int a;
};

typedef struct str_alloc_s str_alloc;

#define STR_ALLOC_ZERO { .s = NULL, .a = 0 }

static int str_alloc_resize(str_alloc *s, unsigned int size) {
    char *t = NULL;
    if(s->a >= size) return 0;
    t = realloc(s->s,size);
    if(t == NULL) {
      if(s->s != NULL) {
        free(s->s);
        s->s = NULL;
      }
      return 1;
    }
    s->a = size;
    s->s = t;
    return 0;
}

static unsigned int utf8_len_or_copy(uint8_t *dest, const uint8_t *src, unsigned int max) {
    if(dest == NULL) return AUDIO_MIN(str_len((const char *)src),max);
    return str_ncpy((char *)dest,(const char *)src,max);
}

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
        strncpy(buf,comment,commentLength);
        r = str_chr(buf,'=');
        if(buf[r]) {
            buf[r] = 0;
            str_lower(buf,buf);
            a->onmeta(a->meta_ctx,buf,buf+r+1);
        }
    }
}

static void process_id3(audio_decoder *a, FILE *f) {
    /* assumption: the file has already read the 'ID3' bytes */
    /* does not close out the file, that's the responsibility of the
     * calling function */
    char buffer[10];
    buffer[0] = 0;
    str_alloc buffer2 = STR_ALLOC_ZERO;
    str_alloc buffer3 = STR_ALLOC_ZERO;
    unsigned int (*text_func)(uint8_t *, const uint8_t *, unsigned int) = NULL;

    unsigned int id3_size = 0;
    unsigned int header_size = 0;
    unsigned int frame_size = 0;
    unsigned int dec_len = 0;
    uint8_t id3_ver = 0;

    if(fread(buffer,1,10,f) != 10) return;
    if(str_ncmp(buffer,"ID3",3) != 0) return;
    id3_ver = (uint8_t)buffer[3];
    id3_size =
      (((unsigned int)buffer[6]) << 21) +
      (((unsigned int)buffer[7]) << 14) +
      (((unsigned int)buffer[8]) << 7 ) +
      (((unsigned int)buffer[9]));

    switch(id3_ver) {
        case 2: header_size = 6; break;
        case 3: header_size = 10; break;
        case 4: header_size = 10; break;
        default: return;
    }


    if(buffer[5] & 0x20) {
        if(fread(buffer,1,6,f) != 6) return;
        frame_size =
          (((unsigned int)buffer[0]) << 21) +
          (((unsigned int)buffer[1]) << 14) +
          (((unsigned int)buffer[2]) << 7 ) +
          (((unsigned int)buffer[3]));
        frame_size -= 6;
        fseek(f,frame_size,SEEK_CUR);
    }

    while(id3_size > 0) {
        if(fread(buffer,1,header_size,f) != header_size) goto id3_done;
        if(memcmp(buffer,allzero,header_size) == 0) goto id3_done;

        id3_size -= header_size;
        if(header_size == 10) {
            frame_size = unpack_uint32be((uint8_t *)buffer + 4);
            buffer[4] = 0;
        } else {
            frame_size = unpack_uint24be((uint8_t *)buffer+3);
            buffer[3] = 0;
        }
        id3_size -= frame_size;

        if(str_alloc_resize(&buffer2,frame_size)) goto id3_done;

        if(fread(buffer2.s,1,frame_size,f) != frame_size) goto id3_done;

        if(buffer[0] != 'T') {
            continue;
        }

        switch(buffer2.s[0]) {
            case 0: text_func = utf_conv_iso88591_utf8; break;
            case 1: text_func = utf_conv_utf16_utf8; break;
            case 2: text_func = utf_conv_utf16be_utf8; break;
            case 3: text_func = utf8_len_or_copy; break;
            default: text_func = NULL;
        }
        if(text_func == NULL) continue;
        dec_len = text_func(NULL,(uint8_t *)buffer2.s+1,frame_size-1) + 1;
        if(str_alloc_resize(&buffer3,dec_len)) goto id3_done;
        buffer3.s[text_func((uint8_t *)buffer3.s,(uint8_t *)buffer2.s+1,frame_size-1)] = 0;

        if(str_icmp(buffer,"tpe1") == 0) {
            a->onmeta(a->meta_ctx,"artist",buffer3.s);
        }
        else if(str_icmp(buffer,"tit2") == 0) {
            a->onmeta(a->meta_ctx,"title",buffer3.s);
        }
        else if(str_icmp(buffer,"talb") == 0) {
            a->onmeta(a->meta_ctx,"album",buffer3.s);
        }
        else if(str_icmp(buffer,"tt2") == 0) {
            a->onmeta(a->meta_ctx,"title",buffer3.s);
        }
        else if(str_icmp(buffer,"tp1") == 0) {
            a->onmeta(a->meta_ctx,"artist",buffer3.s);
        }
        else if(str_icmp(buffer,"tal") == 0) {
            a->onmeta(a->meta_ctx,"album",buffer3.s);
        }

    }

    id3_done:

    if(buffer2.s != NULL) free(buffer2.s);
    if(buffer3.s != NULL) free(buffer3.s);

    return;
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
    a->samplerate = 0;
    a->channels = 0;
    a->framecount = 0;
    a->meta_ctx = NULL;
    a->samples = NULL;
    a->frame_pos = 0;

    return 0;
}

int audio_decoder_open(audio_decoder *a, const char *filename) {
    drflac *flac = NULL;
    drmp3 *mp3 = NULL;
    drwav *wav = NULL;

    if(str_iends(filename,".flac")) {
        flac = drflac_open_file_with_metadata(filename,flac_meta,a);
        if(flac == NULL) return 1;
        a->samples = (int16_t *)malloc(flac->totalPCMFrameCount * flac->channels * sizeof(int16_t));
        if(a->samples == NULL) return 1;
        drflac_read_pcm_frames_s16(flac,flac->totalPCMFrameCount, a->samples);
        a->framecount = flac->totalPCMFrameCount;
        a->samplerate = flac->sampleRate;
        a->channels = flac->channels;
        drflac_close(flac);
    }
    else if(str_iends(filename,".mp3")) {
        mp3_id3(a,filename);
        mp3 = (drmp3 *)malloc(sizeof(drmp3));
        if(mp3 == NULL) {
            fprintf(stderr,"out of memory\n");
            return 1;
        }
        if(!drmp3_init_file(mp3,filename,NULL)) {
            fprintf(stderr,"error opening as mp3\n");
            free(mp3);
            mp3 = NULL;
            return 1;
        }
        a->framecount = drmp3_get_pcm_frame_count(mp3);
        a->samplerate = mp3->sampleRate;
        a->channels = mp3->channels;
        a->samples = (int16_t *)malloc(a->framecount * a->channels * sizeof(int16_t));
        if(a->samples == NULL) {
            free(mp3);
            mp3 = NULL;
            return 1;
        }
        drmp3_read_pcm_frames_s16(mp3,a->framecount,a->samples);
        drmp3_uninit(mp3);
        free(mp3);
    }
    else if(str_iends(filename,"wav")) {
        wav_id3(a,filename);
        wav = drwav_open_file(filename);
        if(wav == NULL) return 1;
        a->framecount = wav->totalPCMFrameCount;
        a->samplerate = wav->sampleRate;
        a->channels = wav->channels;
        a->samples = (int16_t *)malloc(a->framecount * a->channels * sizeof(int16_t));
        if(a->samples == NULL) {
            free(wav);
            wav = NULL;
            return 1;
        }
        drwav_read_pcm_frames_s16(wav,a->framecount,a->samples);
        drwav_close(wav);
    }
    else {
        fprintf(stderr,"sorry, I don't support that file format\n");
    }

    return 0;
}

unsigned int audio_decoder_decode(audio_decoder *a, unsigned int framecount, int16_t *buf) {
    unsigned int count = AUDIO_MIN(framecount,(a->framecount - a->frame_pos));
    if(count == 0) return count;
    memcpy((uint8_t *)buf,(uint8_t *)a->samples + (a->frame_pos * sizeof(int16_t) * a->channels),count * sizeof(int16_t) * a->channels);
    a->frame_pos += count;
    return count;
}

void audio_decoder_close(audio_decoder *a) {
    if(a->samples != NULL) {
        free(a->samples);
        a->samples = NULL;
    }
}

