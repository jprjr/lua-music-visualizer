#ifndef JPR_SPC_H
#define JPR_SPC_H

#include "int.h"

typedef size_t (* jprspc_read_proc)(void* userdata, void* buf, size_t len);
typedef jpr_uint32 (* jprspc_seek_proc)(void* userdata, int offset, unsigned int origin);
typedef void(* jprspc_meta_proc)(void *userdata, const char *key, const char *value);

struct jprspc_s {
    void *userdata;
    jprspc_read_proc readProc;
    jprspc_seek_proc seekProc;
    jpr_uint64 totalPCMFrameCount;
    unsigned int samplerate;
    unsigned int channels;
    void *priv;
};

typedef struct jprspc_s jprspc;

#ifdef __cplusplus
extern "C" {
#endif

jprspc *jprspc_open(jprspc_read_proc readProc, jprspc_seek_proc seekProc, jprspc_meta_proc metaProc, void *userdata, unsigned int samplerate, unsigned int channels);
jpr_uint64 jprspc_read_pcm_frames_s16(jprspc *spc, jpr_uint64 framecount, jpr_int16 *buf);
void jprspc_close(jprspc *spc);


#ifdef __cplusplus
}
#endif


#endif
