#include "audio-decoder.h"
#include "id3.h"
#include "utf.h"
#include "str.h"
#include "file.h"
#include "unpack.h"
#include "util.h"
#include "jpr_proc.h"
#include "int.h"

#include <stdlib.h>

#ifdef CHECK_LEAKS
#include "stb_leakcheck.h"
#endif

#define AUDIO_MAX(a,b) ( a > b ? a : b)
#define AUDIO_MIN(a,b) ( a < b ? a : b)

#if DECODE_PCM
#include "jpr_pcm.h"
#endif

#if DECODE_FLAC
#include "jpr_flac.h"
#endif

#if DECODE_MP3
#include "jpr_mp3.h"
#endif

#if DECODE_WAV
#include "jpr_wav.h"
#endif

#if DECODE_SPC
#include "jpr_spc.h"
#endif

#if DECODE_NEZ
#include "jpr_nez.h"
#endif

#if DECODE_NSF
#include "jpr_nsf.h"
#endif

#if DECODE_VGM
#include "jpr_vgm.h"
#endif

const audio_plugin* const plugin_list[] = {
#if DECODE_FLAC
    &jprflac_plugin,
#endif
#if DECODE_MP3
    &jprmp3_plugin,
#endif
#if DECODE_WAV
    &jprwav_plugin,
#endif
#if DECODE_SPC
    &jprspc_plugin,
#endif
#if DECODE_NSF
    &jprnsf_plugin,
#endif
#if DECODE_NEZ
    &jprnez_plugin,
#endif
#if DECODE_VGM
    &jprvgm_plugin,
#endif
#if DECODE_PCM
    &jprpcm_plugin,
#endif
    NULL
};

static jpr_uint64 read_proc(audio_decoder *a, void *buf, jpr_uint64 bytes) {
    return file_read(a->file,buf,bytes);
}

static jpr_int64 seek_proc(audio_decoder *a, jpr_int64 offset, enum JPR_FILE_POS whence) {
    return file_seek(a->file,offset,whence);
}

static jpr_int64 tell_proc(audio_decoder *a) {
    return file_tell(a->file);
}

static jpr_uint8 *slurp_proc(audio_decoder *a, size_t *size) {
    return file_slurp_fh(a->file,size);
}


int audio_decoder_init(audio_decoder *a) {
    a->file = NULL;
    a->plugin_ctx = NULL;
    a->meta_ctx = NULL;
    a->framecount = 0;
    a->read = read_proc;
    a->seek = seek_proc;
    a->tell = tell_proc;
    a->slurp = slurp_proc;

    return 0;
}

int audio_decoder_open(audio_decoder *a, const char *filename) {
    const char * const *ext;
    const audio_plugin* const* plugin = plugin_list;

    a->file = file_open(filename,"rb");
    if(a->file == NULL) return 1;

    while(*plugin != NULL) {
        ext = (*plugin)->extensions;
        while(*ext != NULL) {
            if(str_iends(filename,*ext)) {
                a->plugin_ctx = (*plugin)->open(a);
                if(a->plugin_ctx != NULL) {
                    a->plugin = *plugin;
                    return 0;
                }
            }
            ext++;
        }
        plugin++;
    }

    file_close(a->file);
    return 1;

}

jpr_uint32 audio_decoder_decode(audio_decoder *a, jpr_uint32 framecount, jpr_int16 *buf) {
    mem_set(buf,0,a->channels * sizeof(jpr_int16)*framecount);
    return a->plugin->decode(a->plugin_ctx,framecount,buf);
}

void audio_decoder_close(audio_decoder *a) {
    a->plugin->close(a->plugin_ctx);
    file_close(a->file);
    a->plugin = NULL;
    a->plugin_ctx = NULL;
    a->file = NULL;
}

audio_info *audio_decoder_probe(audio_decoder *a, const char *filename) {
    audio_info *info;
    const char * const *ext;
    const audio_plugin* const* plugin = plugin_list;

    info = NULL;

    a->file = file_open(filename,"rb");
    if(a->file == NULL) return NULL;


    while(*plugin != NULL) {
        if((*plugin)->probe != NULL) {
            ext = (*plugin)->extensions;
            while(*ext != NULL) {
                if(str_iends(filename,*ext)) {
                    info = (*plugin)->probe(a);
                    if(info != NULL) goto probe_cleanup;
                }
                ext++;
            }
        }
        plugin++;
    }

    probe_cleanup:

    file_close(a->file);
    a->file = NULL;
    return info;

}
void audio_decoder_free_info(audio_info *info) {
    unsigned int i = 0;
    if(info == NULL) return;
    if(info->artist) free(info->artist);
    if(info->album) free(info->album);
    if(info->tracks == NULL) return;
    for(i=0;i<info->total;i++) {
        if(info->tracks[i].title != NULL) free(info->tracks[i].title);
    }
    free(info->tracks);
    free(info);
}

