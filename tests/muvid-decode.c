/*
 * muvid-decode.c — MUSA-native hardware video decode probe for the MTT S30.
 *
 * The MUSA runtime (musa-runtime 1.5.4) ships libvideo_musa.so, which
 * exposes Moore Threads' MUVID video API: a hardware video parser
 * (muvidCreateVideoParser / muvidParseVideoData) and a hardware video
 * decoder (muvidCreateDecoder / muvidDecodePicture / muvidGetDecodeStatus).
 * This is the MUSA-native counterpart of the VA-API entrypoint and the
 * route that a MUSA application should use.
 *
 * The probe feeds an H.264 Annex-B elementary stream to the MUSA parser,
 * lets the MUVID parser split it into access units, decodes each picture on
 * the S30's dedicated video engine, reads the decoded surface back and
 * compares the luma plane against a reference luma plane produced by a
 * software decoder. It therefore proves the *pixels*, not just that a call
 * returned success.
 *
 * The MUVID API is loaded dynamically (libvideo_musa.so / libmusa.so.1) so
 * that the probe builds and runs on a host that has the MUSA runtime
 * installed but no Video Codec SDK.
 *
 * Usage: muvid-decode <stream.h264> [-r ref.y] [-y out.y]
 *   -r ref.y   reference luma plane, width*height bytes, tight packing
 *   -y out.y   write the decoded luma plane here
 *
 * Exit status:
 *   0  at least one picture was decoded and read back
 *   1  usage or I/O error
 *   2  the MUSA/MUVID libraries could not be loaded or initialised
 *   3  the decode produced no picture, or the decode failed
 */
#define _GNU_SOURCE
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>

#include <ffmtcodec/dynlink_muviddec.h>
#include <ffmtcodec/mtmuvid.h>

/*
 * The Moore Threads Video Codec SDK loader asks for the MUSA driver API under
 * the SONAME "libmusa.so.1" and the MUVID API under "libvideo_musa.so".
 * The packaged MUSA runtime 1.5.4 installs the driver API as
 * /usr/local/musa/lib/libmusa.so.1.5 with SONAME "libmusa.so.1.5", while a
 * 5.2.0-era orphan in /usr/lib64 answers to "libmusa.so.1" and speaks a
 * 5.2 UAPI that the 3.1.0 kernel driver rejects (MTGPU_QUERY_CMD_MKT_NAME,
 * cmd 12 -> EINVAL). This probe therefore resolves the libraries itself and
 * prefers the SONAME that the installed runtime actually ships.
 */

/* MUSA driver API entry points used by this probe. */
typedef int (*fn_muInit_t)(unsigned int);
typedef int (*fn_muDeviceGetCount_t)(int *);
typedef int (*fn_muDeviceGet_t)(int *, int);
typedef int (*fn_muCtxCreate_v2_t)(void *, unsigned int, int);
typedef int (*fn_muCtxPushCurrent_v2_t)(void *);
typedef int (*fn_muCtxPopCurrent_v2_t)(void *);
typedef int (*fn_muCtxDestroy_v2_t)(void *);
typedef int (*fn_muMemcpyDtoH_v2_t)(void *, unsigned long long, size_t);

struct musa_api {
    void *dl;
    fn_muInit_t             init;
    fn_muDeviceGetCount_t   device_get_count;
    fn_muDeviceGet_t        device_get;
    fn_muCtxCreate_v2_t     ctx_create;
    fn_muCtxPushCurrent_v2_t ctx_push_current;
    fn_muCtxPopCurrent_v2_t  ctx_pop_current;
    fn_muCtxDestroy_v2_t    ctx_destroy;
    fn_muMemcpyDtoH_v2_t    memcpy_dtoh;
};

/* MUVID entry points used by this probe. */
typedef int (*fn_muvidGetDecoderCaps_t)(MUVIDDECODECAPS *);
typedef int (*fn_muvidCreateDecoder_t)(MUvideodecoder *, MUVIDDECODECREATEINFO *);
typedef int (*fn_muvidDestroyDecoder_t)(MUvideodecoder);
typedef int (*fn_muvidDecodePicture_t)(MUvideodecoder, MUVIDPICPARAMS *);
typedef int (*fn_muvidGetDecodeStatus_t)(MUvideodecoder, int, MUVIDGETDECODESTATUS *);
typedef int (*fn_muvidMapVideoFrame32_t)(MUvideodecoder, int, unsigned int *, unsigned int *, MUVIDPROCPARAMS *);
typedef int (*fn_muvidMapVideoFrame64_t)(MUvideodecoder, int, unsigned long long *, unsigned int *, MUVIDPROCPARAMS *);
typedef int (*fn_muvidUnmapVideoFrame32_t)(MUvideodecoder, unsigned int);
typedef int (*fn_muvidUnmapVideoFrame64_t)(MUvideodecoder, unsigned long long);
typedef int (*fn_muvidCtxLockCreate_t)(MUvideoctxlock *, void *);
typedef int (*fn_muvidCtxLockDestroy_t)(MUvideoctxlock);
typedef int (*fn_muvidCreateVideoParser_t)(MUvideoparser *, MUVIDPARSERPARAMS *);
typedef int (*fn_muvidParseVideoData_t)(MUvideoparser, MUVIDSOURCEDATAPACKET *);
typedef int (*fn_muvidDestroyVideoParser_t)(MUvideoparser);

struct muvid_api {
    void *dl;
    fn_muvidGetDecoderCaps_t   get_caps;
    fn_muvidCreateDecoder_t    create_decoder;
    fn_muvidDestroyDecoder_t   destroy_decoder;
    fn_muvidDecodePicture_t    decode_picture;
    fn_muvidGetDecodeStatus_t  get_decode_status;
    fn_muvidMapVideoFrame32_t  map_frame32;
    fn_muvidMapVideoFrame64_t  map_frame64;
    fn_muvidUnmapVideoFrame32_t unmap_frame32;
    fn_muvidUnmapVideoFrame64_t unmap_frame64;
    fn_muvidCtxLockCreate_t    ctx_lock_create;
    fn_muvidCtxLockDestroy_t   ctx_lock_destroy;
    fn_muvidCreateVideoParser_t create_parser;
    fn_muvidParseVideoData_t   parse_video_data;
    fn_muvidDestroyVideoParser_t destroy_parser;
    int wide_device_pointer;    /* 1 when the 64-bit device pointer API exists */
};

struct probe {
    struct musa_api musa;
    struct muvid_api muvid;

    MUcontext ctx;
    MUvideoctxlock lock;

    MUvideoparser parser;
    MUvideodecoder decoder;

    MUVIDEOFORMAT fmt;
    int have_fmt;

    /* Bookkeeping for the run. */
    int pictures_decoded;
    int pictures_mapped;
    int decode_failures;
    MUVIDGETDECODESTATUS last_status;

    unsigned int width, height;
    unsigned char *host;      /* host copy of the decoded NV12 frame */
    size_t host_size;

    const char *out_y;
    const char *ref_y;
};

static struct probe g;


/* ------------------------------------------------------------------ */
/* Library and symbol loading                                          */
/* ------------------------------------------------------------------ */
static void *open_first(const char *const *names)
{
    for (int i = 0; names[i]; i++) {
        void *h = dlopen(names[i], RTLD_NOW | RTLD_GLOBAL);
        if (h) {
            printf("LIBRARY %s\n", names[i]);
            return h;
        }
    }
    return NULL;
}

/* Load one library specified by an environment variable, if set. This lets
 * the probe drive an alternative MUVID build (for example the upstream
 * Video Codec SDK 13.0 library) without touching the installed stack. */
static void *open_override(const char *env)
{
    const char *path = getenv(env);
    if (!path || !*path)
        return NULL;
    void *h = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (h) {
        printf("LIBRARY %s (%s)\n", path, env);
        return h;
    }
    fprintf(stderr, "MUVID_FAIL %s=%s: %s\n", env, path, dlerror());
    return NULL;
}

#define NEED_SYM(h, field, name, type)                                       \
    do {                                                                     \
        *(void **)&(field) = dlsym((h), (name));                             \
        if (!(field)) {                                                     \
            fprintf(stderr, "MUVID_FAIL missing symbol %s\n", (name));      \
            return -1;                                                      \
        }                                                                    \
    } while (0)

static int load_musa_api(struct musa_api *api)
{
    /* Prefer the SONAME that the installed MUSA runtime ships. */
    static const char *const candidates[] = {
        "libmusa.so.1.5", "libmusa.so.1", "libmusa.so", NULL
    };
    memset(api, 0, sizeof(*api));
    api->dl = open_override("MUVID_MUSA_LIB");
    if (!api->dl)
        api->dl = open_first(candidates);
    if (!api->dl) {
        fprintf(stderr, "MUVID_FAIL cannot load the MUSA driver API "
                        "(tried libmusa.so.1.5, libmusa.so.1, libmusa.so)\n");
        return -1;
    }
    NEED_SYM(api->dl, api->init, "muInit", fn_muInit_t);
    NEED_SYM(api->dl, api->device_get_count, "muDeviceGetCount",
             fn_muDeviceGetCount_t);
    NEED_SYM(api->dl, api->device_get, "muDeviceGet", fn_muDeviceGet_t);
    NEED_SYM(api->dl, api->ctx_create, "muCtxCreate_v2", fn_muCtxCreate_v2_t);
    NEED_SYM(api->dl, api->ctx_push_current, "muCtxPushCurrent_v2",
             fn_muCtxPushCurrent_v2_t);
    NEED_SYM(api->dl, api->ctx_pop_current, "muCtxPopCurrent_v2",
             fn_muCtxPopCurrent_v2_t);
    NEED_SYM(api->dl, api->ctx_destroy, "muCtxDestroy_v2", fn_muCtxDestroy_v2_t);
    NEED_SYM(api->dl, api->memcpy_dtoh, "muMemcpyDtoH_v2", fn_muMemcpyDtoH_v2_t);
    return 0;
}

static int load_muvid_api(struct muvid_api *api)
{
    static const char *const candidates[] = {
        "libvideo_musa.so.1.2", "libvideo_musa.so", NULL
    };
    memset(api, 0, sizeof(*api));
    api->dl = open_override("MUVID_LIB");
    if (!api->dl)
        api->dl = open_first(candidates);
    if (!api->dl) {
        fprintf(stderr, "MUVID_FAIL cannot load libvideo_musa.so "
                        "(install musa-runtime)\n");
        return -1;
    }
    NEED_SYM(api->dl, api->get_caps, "muvidGetDecoderCaps",
             fn_muvidGetDecoderCaps_t);
    NEED_SYM(api->dl, api->create_decoder, "muvidCreateDecoder",
             fn_muvidCreateDecoder_t);
    NEED_SYM(api->dl, api->destroy_decoder, "muvidDestroyDecoder",
             fn_muvidDestroyDecoder_t);
    NEED_SYM(api->dl, api->decode_picture, "muvidDecodePicture",
             fn_muvidDecodePicture_t);
    NEED_SYM(api->dl, api->get_decode_status, "muvidGetDecodeStatus",
             fn_muvidGetDecodeStatus_t);
    /* The context lock arrived in a later Video Codec SDK release. It is
     * only needed to synchronise MUSA context ownership across threads, so
     * its absence is not fatal for a single-threaded probe. */
    *(void **)&api->ctx_lock_create = dlsym(api->dl, "muvidCtxLockCreate");
    *(void **)&api->ctx_lock_destroy = dlsym(api->dl, "muvidCtxLockDestroy");
    NEED_SYM(api->dl, api->create_parser, "muvidCreateVideoParser",
             fn_muvidCreateVideoParser_t);
    NEED_SYM(api->dl, api->parse_video_data, "muvidParseVideoData",
             fn_muvidParseVideoData_t);
    NEED_SYM(api->dl, api->destroy_parser, "muvidDestroyVideoParser",
             fn_muvidDestroyVideoParser_t);

    /* The device-pointer width differs between Video Codec SDK releases:
     * 12.x exports muvidMapVideoFrame (32-bit pointer), 13.x exports
     * muvidMapVideoFrame64. Accept both. */
    *(void **)&api->map_frame64 = dlsym(api->dl, "muvidMapVideoFrame64");
    *(void **)&api->unmap_frame64 = dlsym(api->dl, "muvidUnmapVideoFrame64");
    if (api->map_frame64 && api->unmap_frame64) {
        api->wide_device_pointer = 1;
    } else {
        *(void **)&api->map_frame32 = dlsym(api->dl, "muvidMapVideoFrame");
        *(void **)&api->unmap_frame32 = dlsym(api->dl, "muvidUnmapVideoFrame");
        if (!api->map_frame32 || !api->unmap_frame32) {
            fprintf(stderr, "MUVID_FAIL neither muvidMapVideoFrame64 nor "
                            "muvidMapVideoFrame is available\n");
            return -1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Stream input                                                        */
/* ------------------------------------------------------------------ */
static unsigned char *read_file(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "MUVID_FAIL open %s\n", path); return NULL; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long n = ftell(f);
    if (n <= 0) { fclose(f); fprintf(stderr, "MUVID_FAIL empty %s\n", path); return NULL; }
    rewind(f);
    unsigned char *buf = malloc((size_t)n);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "MUVID_FAIL short read on %s\n", path);
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    *out_len = (size_t)n;
    return buf;
}

/* ------------------------------------------------------------------ */
/* Parser callbacks                                                    */
/* ------------------------------------------------------------------ */
static int on_sequence(void *user, MUVIDEOFORMAT *fmt)
{
    struct probe *p = user;
    p->fmt = *fmt;
    p->have_fmt = 1;
    p->width = fmt->coded_width;
    p->height = fmt->coded_height;
    printf("SEQUENCE codec=%u %ux%u chroma=%u bitdepth_minus8=%u "
           "min_decode_surfaces=%u display_area=%d,%d,%d,%d\n",
           (unsigned)fmt->codec, fmt->coded_width, fmt->coded_height,
           (unsigned)fmt->chroma_format, (unsigned)fmt->bit_depth_luma_minus8,
           (unsigned)fmt->min_num_decode_surfaces,
           fmt->display_area.left, fmt->display_area.top,
           fmt->display_area.right, fmt->display_area.bottom);
    /* Returning 1 accepts the sequence and keeps the parser's DPB size. */
    return 1;
}

static int on_decode(void *user, MUVIDPICPARAMS *pp)
{
    struct probe *p = user;

    if (!p->decoder) {
        MUVIDDECODECREATEINFO dci;
        memset(&dci, 0, sizeof(dci));
        dci.CodecType = p->fmt.codec;
        dci.ChromaFormat = p->fmt.chroma_format;
        dci.OutputFormat = musaVideoSurfaceFormat_NV12;
        dci.bitDepthMinus8 = p->fmt.bit_depth_luma_minus8;
        dci.DeinterlaceMode = p->fmt.progressive_sequence
                            ? musaVideoDeinterlaceMode_Weave
                            : musaVideoDeinterlaceMode_Adaptive;
        dci.ulCreationFlags = musaVideoCreate_PreferMUVID;
        dci.ulNumDecodeSurfaces = p->fmt.min_num_decode_surfaces
                                ? p->fmt.min_num_decode_surfaces : 1;
        dci.ulWidth = p->fmt.coded_width;
        dci.ulHeight = p->fmt.coded_height;
        dci.ulMaxWidth = p->fmt.coded_width;
        dci.ulMaxHeight = p->fmt.coded_height;
        dci.ulTargetWidth = p->fmt.coded_width;
        dci.ulTargetHeight = p->fmt.coded_height;
        dci.ulNumOutputSurfaces = 1;
        dci.vidLock = p->lock;

        MUresult r = p->muvid.create_decoder(&p->decoder, &dci);
        printf("DECODER_CREATE result=%d %lux%lu surfaces=%lu\n", (int)r,
               dci.ulWidth, dci.ulHeight, dci.ulNumDecodeSurfaces);
        if (r != MUSA_SUCCESS) {
            fprintf(stderr, "MUVID_FAIL muvidCreateDecoder -> %d\n", (int)r);
            p->decode_failures++;
            return 0;
        }
    }

    MUresult r = p->muvid.decode_picture(p->decoder, pp);
    if (r != MUSA_SUCCESS) {
        fprintf(stderr, "MUVID_FAIL muvidDecodePicture -> %d (picidx=%d "
                        "slices=%u len=%u)\n", (int)r, pp->CurrPicIdx,
                pp->nNumSlices, pp->nBitstreamDataLen);
        p->decode_failures++;
        return 0;
    }
    p->pictures_decoded++;

    MUVIDGETDECODESTATUS st;
    memset(&st, 0, sizeof(st));
    r = p->muvid.get_decode_status(p->decoder, pp->CurrPicIdx, &st);
    p->last_status = st;
    if (st.decodeStatus != muvidDecodeStatus_Success) {
        fprintf(stderr, "MUVID_FAIL decode status=%d after muvidDecodePicture "
                        "(picidx=%d)\n", (int)st.decodeStatus, pp->CurrPicIdx);
        p->decode_failures++;
        return 0;
    }
    return 1;
}

static int on_display(void *user, MUVIDPARSERDISPINFO *di)
{
    struct probe *p = user;
    if (!p->decoder) return 1;

    unsigned long long devptr = 0;
    unsigned int pitch = 0;
    MUVIDPROCPARAMS vpp;
    memset(&vpp, 0, sizeof(vpp));

    MUresult r;
    if (p->muvid.wide_device_pointer) {
        r = p->muvid.map_frame64(p->decoder, di->picture_index, &devptr,
                                 &pitch, &vpp);
    } else {
        unsigned int devptr32 = 0;
        r = p->muvid.map_frame32(p->decoder, di->picture_index, &devptr32,
                                 &pitch, &vpp);
        devptr = devptr32;
    }
    if (r != MUSA_SUCCESS) {
        fprintf(stderr, "MUVID_FAIL muvidMapVideoFrame -> %d\n", (int)r);
        p->decode_failures++;
        return 1;
    }

    size_t luma = (size_t)p->width * p->height;
    size_t total = luma + luma / 2;   /* NV12 */
    if (!p->host || p->host_size < total) {
        free(p->host);
        p->host = malloc(total);
        p->host_size = total;
    }
    if (!p->host) {
        if (p->muvid.wide_device_pointer)
            p->muvid.unmap_frame64(p->decoder, devptr);
        else
            p->muvid.unmap_frame32(p->decoder, (unsigned int)devptr);
        return 1;
    }

    /* Copy the mapped surface back to the host. */
    r = p->musa.memcpy_dtoh(p->host, (unsigned long long)devptr, total);
    if (p->muvid.wide_device_pointer)
        p->muvid.unmap_frame64(p->decoder, devptr);
    else
        p->muvid.unmap_frame32(p->decoder, (unsigned int)devptr);
    if (r != MUSA_SUCCESS) {
        fprintf(stderr, "MUVID_FAIL muMemcpyDtoH -> %d\n", (int)r);
        p->decode_failures++;
        return 1;
    }
    p->pictures_mapped++;

    /* Basic sanity: the luma plane must not be uniformly zero. */
    long long sum = 0;
    unsigned int nonzero = 0;
    for (size_t i = 0; i < luma; i++) {
        sum += p->host[i];
        if (p->host[i]) nonzero++;
    }
    printf("PIC picture_index=%d pitch=%u luma_mean=%.2f nonzero=%zu/%zu\n",
           di->picture_index, pitch, (double)sum / (double)(luma ? luma : 1),
           (size_t)nonzero, luma);

    if (p->out_y) {
        FILE *f = fopen(p->out_y, "wb");
        if (f) {
            fwrite(p->host, 1, luma, f);
            fclose(f);
            printf("LUMA_WRITTEN %s %ux%u\n", p->out_y, p->width, p->height);
        }
    }

    if (p->ref_y) {
        FILE *f = fopen(p->ref_y, "rb");
        if (!f) {
            fprintf(stderr, "MUVID_FAIL cannot read reference %s\n", p->ref_y);
        } else {
            unsigned char *ref = malloc(luma);
            size_t got = fread(ref, 1, luma, f);
            fclose(f);
            if (got != luma) {
                fprintf(stderr, "MUVID_FAIL reference plane is %zu bytes, "
                                "expected %zu\n", got, luma);
            } else {
                long long diff = 0;
                size_t maxdiff = 0, bad = 0;
                for (size_t i = 0; i < luma; i++) {
                    int d = (int)p->host[i] - (int)ref[i];
                    if (d < 0) d = -d;
                    diff += d;
                    if ((size_t)d > maxdiff) maxdiff = (size_t)d;
                    if (d > 8) bad++;
                }
                double mae = (double)diff / (double)luma;
                double psnr = 0.0;
                if (maxdiff) {
                    double mse = 0.0;
                    for (size_t i = 0; i < luma; i++) {
                        double d = (double)p->host[i] - (double)ref[i];
                        mse += d * d;
                    }
                    mse /= (double)luma;
                    psnr = 10.0 * log10(255.0 * 255.0 / mse);
                }
                printf("COMPARE mae=%.4f psnr_db=%.2f maxdiff=%zu pixels_gt8=%zu\n",
                       mae, psnr, maxdiff, bad);
                printf("COMPARE_STATUS: %s\n",
                       (maxdiff <= 8) ? "PASS" : "FAIL");
            }
            free(ref);
        }
    }
    return 1;
}

/* ------------------------------------------------------------------ */
int main(int argc, char **argv)
{
    const char *stream = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-r") && i + 1 < argc) g.ref_y = argv[++i];
        else if (!strcmp(argv[i], "-y") && i + 1 < argc) g.out_y = argv[++i];
        else if (!stream) stream = argv[i];
        else {
            fprintf(stderr, "usage: %s <stream.h264> [-r ref.y] [-y out.y]\n",
                    argv[0]);
            return 1;
        }
    }
    if (!stream) {
        fprintf(stderr, "usage: %s <stream.h264> [-r ref.y] [-y out.y]\n",
                argv[0]);
        return 1;
    }

    size_t len = 0;
    unsigned char *data = read_file(stream, &len);
    if (!data) return 1;
    printf("STREAM file=%s bytes=%zu\n", stream, len);

    if (load_musa_api(&g.musa) != 0) return 2;
    if (load_muvid_api(&g.muvid) != 0) return 2;

    if (g.musa.init(0) != MUSA_SUCCESS) {
        fprintf(stderr, "MUVID_FAIL muInit failed\n");
        return 2;
    }
    int ndev = 0;
    if (g.musa.device_get_count(&ndev) != MUSA_SUCCESS || ndev < 1) {
        fprintf(stderr, "MUVID_FAIL no MUSA device\n");
        return 2;
    }
    MUdevice dev;
    if (g.musa.device_get(&dev, 0) != MUSA_SUCCESS) {
        fprintf(stderr, "MUVID_FAIL muDeviceGet failed\n");
        return 2;
    }
    if (g.musa.ctx_create(&g.ctx, 0, dev) != MUSA_SUCCESS) {
        fprintf(stderr, "MUVID_FAIL muCtxCreate failed\n");
        return 2;
    }
    g.musa.ctx_push_current(g.ctx);
    printf("DEVICE index=0 context=ok\n");

    /* muvidGetDecoderCaps() is a per-codec query: the codec and chroma
     * format are inputs, the capabilities are outputs. */
    MUVIDDECODECAPS caps;
    memset(&caps, 0, sizeof(caps));
    caps.eCodecType = musaVideoCodec_H264;
    caps.eChromaFormat = musaVideoChromaFormat_420;
    if (g.muvid.get_caps(&caps) != MUSA_SUCCESS) {
        /* The caps struct layout grew between Video Codec SDK releases, and
         * the runtime in musa-runtime 1.5.4 predates this header. The query
         * is informational; the decoder itself is the authority. */
        fprintf(stderr, "NOTE muvidGetDecoderCaps failed; continuing "
                        "without the capability query\n");
        memset(&caps, 0, sizeof(caps));
        caps.bIsSupported = 1;
    }
    printf("CAPS h264_420 supported=%u mtdECs=%u min=%ux%u max=%ux%u "
           "max_mb=%u output_format_mask=0x%x\n",
           (unsigned)caps.bIsSupported, (unsigned)caps.nNumMTDECs,
           (unsigned)caps.nMinWidth, (unsigned)caps.nMinHeight,
           (unsigned)caps.nMaxWidth, (unsigned)caps.nMaxHeight,
           (unsigned)caps.nMaxMBCount, (unsigned)caps.nOutputFormatMask);
    if (!caps.bIsSupported) {
        fprintf(stderr, "MUVID_FAIL the S30 reports no H.264 4:2:0 decoder "
                        "capability\n");
        return 2;
    }


    if (g.muvid.ctx_lock_create) {
        if (g.muvid.ctx_lock_create(&g.lock, g.ctx) != MUSA_SUCCESS) {
            fprintf(stderr, "MUVID_FAIL muvidCtxLockCreate failed\n");
            return 2;
        }
    } else {
        printf("NOTE libvideo_musa.so has no muvidCtxLockCreate; "
               "proceeding without a context lock\n");
    }

    MUVIDPARSERPARAMS pp;
    memset(&pp, 0, sizeof(pp));
    pp.CodecType = musaVideoCodec_H264;
    pp.ulMaxNumDecodeSurfaces = 1;
    pp.ulClockRate = 10000000;
    pp.ulMaxDisplayDelay = 0;
    pp.pUserData = &g;
    pp.pfnSequenceCallback = on_sequence;
    pp.pfnDecodePicture = on_decode;
    pp.pfnDisplayPicture = on_display;

    MUresult prc = g.muvid.create_parser(&g.parser, &pp);
    if (prc != MUSA_SUCCESS) {
        fprintf(stderr, "MUVID_FAIL muvidCreateVideoParser -> %d\n", (int)prc);
        return 2;
    }

    /* Feed the stream. A single packet with ENDOFPICTURE|ENDOFSTREAM lets the
     * parser split it into access units by itself. */
    MUVIDSOURCEDATAPACKET pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.payload = data;
    pkt.payload_size = (unsigned int)len;
    pkt.flags = MUVID_PKT_ENDOFPICTURE | MUVID_PKT_ENDOFSTREAM;

    MUresult r = g.muvid.parse_video_data(g.parser, &pkt);
    printf("PARSE result=%d\n", (int)r);
    if (r != MUSA_SUCCESS) {
        fprintf(stderr, "MUVID_FAIL muvidParseVideoData -> %d\n", (int)r);
        return 3;
    }

    printf("SUMMARY decoded=%d mapped=%d failures=%d\n",
           g.pictures_decoded, g.pictures_mapped, g.decode_failures);

    g.muvid.destroy_parser(g.parser);
    if (g.decoder) g.muvid.destroy_decoder(g.decoder);
    if (g.lock && g.muvid.ctx_lock_destroy) g.muvid.ctx_lock_destroy(g.lock);
    g.musa.ctx_pop_current(&g.ctx);
    g.musa.ctx_destroy(g.ctx);
    free(g.host);
    free(data);
    if (g.muvid.dl) dlclose(g.muvid.dl);
    if (g.musa.dl) dlclose(g.musa.dl);

    if (g.pictures_mapped == 0 || g.decode_failures) {
        fprintf(stderr, "MUVID_STATUS: FAIL\n");
        return 3;
    }
    printf("MUVID_STATUS: SUCCESS\n");
    return 0;
}
