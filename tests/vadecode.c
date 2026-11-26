/*
 * vaprobe.c's sibling: exercise the MTT S30's VA-API *video decode* path
 * (VAEntrypointVLD) end to end and verify the decoded pixels on the host.
 *
 * The driver advertises VLD profiles for H.264/HEVC/VP9/VP8/AV1/MPEG-2/VC-1,
 * but "advertised" is not "working". This program feeds a real H.264 Annex-B
 * elementary stream through the hardware decoder, reads the decoded surface
 * back out of the driver, and writes the luma plane to a file so the caller
 * can compare it against a software decode of the same stream.
 *
 * It is a self-contained probe: it does not link against libva at compile
 * time (the driver host installs libva, not libva-devel), so the ABI typedefs
 * below mirror va/va.h and va/va_drm.h from libva 2.24 (the version in
 * Fedora 45). Only the entry points that exist in libva >= 1.0 are used.
 *
 * Usage: vaprobe-style decode probe
 *   vadecode <render-node> <stream.h264> [-w WIDTH -h HEIGHT] [-y out.y]
 *
 * Exit status:
 *   0  hardware decode produced a picture and it was read back
 *   1  usage error
 *   2  the driver/display could not be set up, or no VLD entrypoint exists
 *   3  the decode itself failed (driver reported an error)
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* libva ABI (mirrors va/va.h, libva 2.24)                             */
/* ------------------------------------------------------------------ */
typedef void *VADisplay;
typedef int VAStatus;
typedef unsigned int VAGenericID;

typedef VAGenericID VAConfigID;
typedef VAGenericID VAContextID;
typedef VAGenericID VASurfaceID;
typedef VAGenericID VABufferID;
typedef VAGenericID VAImageID;

typedef uint32_t VAProfile;
typedef uint32_t VAEntrypoint;

#define VA_STATUS_SUCCESS                 0
#define VA_RT_FORMAT_YUV420               0x00000001u

/* VABufferType (libva 2.24 va/va.h) */
#define VA_BUFFER_TYPE_SLICE_DATA         5   /* VASliceDataBufferType */
#define VA_BUFFER_TYPE_PICTURE             0   /* VAPictureParameterBufferType */

/* VAProfile */
#define VA_PROFILE_H264_BASELINE          2
#define VA_PROFILE_H264_MAIN              3
#define VA_PROFILE_H264_HIGH              4

/* VAEntrypoint */
#define VA_ENTRYPOINT_VLD                 1
#define VA_ENTRYPOINT_ENC_SLICE           6
#define VA_ENTRYPOINT_ENC_PICTURE         7
#define VA_ENTRYPOINT_ENC_SLICE_LP        8

/* VAConfigAttribType */
#define VA_CONFIG_ATTRIB_RT_FORMAT        0
#define VA_CONFIG_ATTRIB_DEC_SLICE_MODE   6

/* VAConfigAttribDecSliceMode values */
#define VA_DEC_SLICE_MODE_NORMAL          0x00000001u
#define VA_DEC_SLICE_MODE_BASE            0x00000002u

typedef struct {
    int type;
    uint32_t value;
} VAConfigAttrib;

typedef struct {
    uint32_t fourcc;
    uint32_t byte_order;
    uint32_t bits_per_pixel;
    uint32_t depth;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t alpha_mask;
    uint32_t va_reserved[2];
} VAImageFormat;

typedef struct {
    VAImageID    image_id;
    VAImageFormat format;
    VABufferID   buf;
    uint16_t     width;
    uint16_t     height;
    uint32_t     data_size;
    uint32_t     num_planes;
    uint32_t     pitches[3];
    uint32_t     offsets[3];
    int32_t      num_palette_entries;
    int32_t      entry_bytes;
    int8_t       component_order[4];
    uint32_t     va_reserved[2];
} VAImage;

/* ------------------------------------------------------------------ */
/* libva entry points                                                   */
/* ------------------------------------------------------------------ */
typedef VADisplay (*vaGetDisplayDRM_t)(int fd);
typedef VAStatus (*vaInitialize_t)(VADisplay, int *, int *);
typedef VAStatus (*vaTerminate_t)(VADisplay);
typedef const char *(*vaErrorStr_t)(VAStatus);
typedef VAStatus (*vaQueryConfigEntrypoints_t)(VADisplay, VAProfile, int *, int *);
typedef VAStatus (*vaCreateConfig_t)(VADisplay, VAProfile, VAEntrypoint,
                                     VAConfigAttrib *, int, VAConfigID *);
typedef VAStatus (*vaDestroyConfig_t)(VADisplay, VAConfigID);
typedef VAStatus (*vaCreateSurfaces_t)(VADisplay, unsigned int, unsigned int,
                                       unsigned int, VASurfaceID *, unsigned int,
                                       void *, unsigned int);
typedef VAStatus (*vaDestroySurfaces_t)(VADisplay, VASurfaceID *, int);
typedef VAStatus (*vaCreateContext_t)(VADisplay, VAConfigID, int, int, int,
                                      VASurfaceID *, int, VAContextID *);
typedef VAStatus (*vaDestroyContext_t)(VADisplay, VAContextID);
typedef VAStatus (*vaCreateBuffer_t)(VADisplay, VAContextID, int, unsigned int,
                                     unsigned int, void *, VABufferID *);
typedef VAStatus (*vaDestroyBuffer_t)(VADisplay, VABufferID);
typedef VAStatus (*vaBeginPicture_t)(VADisplay, VAContextID, VASurfaceID);
typedef VAStatus (*vaRenderPicture_t)(VADisplay, VAContextID, VABufferID *, int);
typedef VAStatus (*vaEndPicture_t)(VADisplay, VAContextID);
typedef VAStatus (*vaSyncSurface_t)(VADisplay, VASurfaceID);
typedef VAStatus (*vaDeriveImage_t)(VADisplay, VASurfaceID, VAImage *);
typedef VAStatus (*vaMapBuffer_t)(VADisplay, VABufferID, void **);
typedef VAStatus (*vaUnmapBuffer_t)(VADisplay, VABufferID);

struct va_api {
    void *dl;
    vaGetDisplayDRM_t         get_display;
    vaInitialize_t            init;
    vaTerminate_t             term;
    vaErrorStr_t              errstr;
    vaQueryConfigEntrypoints_t query_eps;
    vaCreateConfig_t          create_config;
    vaDestroyConfig_t         destroy_config;
    vaCreateSurfaces_t        create_surfaces;
    vaDestroySurfaces_t       destroy_surfaces;
    vaCreateContext_t         create_context;
    vaDestroyContext_t        destroy_context;
    vaCreateBuffer_t          create_buffer;
    vaDestroyBuffer_t         destroy_buffer;
    vaBeginPicture_t          begin_picture;
    vaRenderPicture_t         render_picture;
    vaEndPicture_t            end_picture;
    vaSyncSurface_t           sync_surface;
    vaDeriveImage_t           derive_image;
    vaMapBuffer_t             map_buffer;
    vaUnmapBuffer_t           unmap_buffer;
};

static int load_symbol(void *dl, const char *name, void **out)
{
    void *sym = dlsym(dl, name);
    if (!sym) {
        fprintf(stderr, "VADECODE_FAIL dlsym %s: %s\n", name, dlerror());
        return -1;
    }
    *out = sym;
    return 0;
}

#define LOAD(field, name)                                                      \
    do {                                                                       \
        if (load_symbol(api->dl, (name), (void **)&api->field) != 0)            \
            return -1;                                                         \
    } while (0)

static int va_api_load(struct va_api *api)
{
    memset(api, 0, sizeof(*api));
    api->dl = dlopen("libva.so.2", RTLD_NOW | RTLD_LOCAL);
    if (!api->dl) {
        fprintf(stderr, "VADECODE_FAIL dlopen libva.so.2: %s\n", dlerror());
        return -1;
    }
    /* vaGetDisplayDRM lives in libva-drm, not libva. */
    void *hd = dlopen("libva-drm.so.2", RTLD_NOW | RTLD_LOCAL);
    if (!hd) {
        fprintf(stderr, "VADECODE_FAIL dlopen libva-drm.so.2: %s\n", dlerror());
        return -1;
    }
    if (load_symbol(hd, "vaGetDisplayDRM", (void **)&api->get_display) != 0)
        return -1;

    LOAD(init, "vaInitialize");
    LOAD(term, "vaTerminate");
    LOAD(errstr, "vaErrorStr");
    LOAD(query_eps, "vaQueryConfigEntrypoints");
    LOAD(create_config, "vaCreateConfig");
    LOAD(destroy_config, "vaDestroyConfig");
    LOAD(create_surfaces, "vaCreateSurfaces");
    LOAD(destroy_surfaces, "vaDestroySurfaces");
    LOAD(create_context, "vaCreateContext");
    LOAD(destroy_context, "vaDestroyContext");
    LOAD(create_buffer, "vaCreateBuffer");
    LOAD(destroy_buffer, "vaDestroyBuffer");
    LOAD(begin_picture, "vaBeginPicture");
    LOAD(render_picture, "vaRenderPicture");
    LOAD(end_picture, "vaEndPicture");
    LOAD(sync_surface, "vaSyncSurface");
    LOAD(derive_image, "vaDeriveImage");
    LOAD(map_buffer, "vaMapBuffer");
    LOAD(unmap_buffer, "vaUnmapBuffer");
    return 0;
}

#define VADECODE_OK(msg_fmt, ...)                                              \
    do {                                                                       \
        printf(msg_fmt "\n", ##__VA_ARGS__);                                   \
    } while (0)

/* ------------------------------------------------------------------ */
/* H.264 elementary stream handling                                     */
/* ------------------------------------------------------------------ */
static uint8_t *read_file(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "VADECODE_FAIL open %s\n", path);
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long n = ftell(f);
    if (n <= 0) { fclose(f); fprintf(stderr, "VADECODE_FAIL empty %s\n", path); return NULL; }
    rewind(f);
    uint8_t *buf = malloc((size_t)n);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "VADECODE_FAIL short read on %s\n", path);
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    *out_len = (size_t)n;
    return buf;
}

/* Bit reader with Exp-Golomb support, operating on a NAL payload. */
struct bitreader {
    const uint8_t *p;
    size_t nbits;
    size_t pos;
};

static int br_bit(struct bitreader *br, unsigned *out)
{
    if (br->pos >= br->nbits) return -1;
    unsigned byte = br->p[br->pos >> 3];
    unsigned bit = (byte >> (7 - (br->pos & 7))) & 1u;
    br->pos++;
    *out = bit;
    return 0;
}

static int br_bits(struct bitreader *br, int n, uint32_t *out)
{
    uint32_t v = 0;
    for (int i = 0; i < n; i++) {
        unsigned b;
        if (br_bit(br, &b) != 0) return -1;
        v = (v << 1) | b;
    }
    *out = v;
    return 0;
}

static int br_ue(struct bitreader *br, uint32_t *out)
{
    int leading = 0;
    unsigned b;
    while (1) {
        if (br_bit(br, &b) != 0) return -1;
        if (b) break;
        leading++;
        if (leading > 32) return -1;
    }
    if (leading == 0) { *out = 0; return 0; }
    uint32_t rest = 0;
    if (br_bits(br, leading, &rest) != 0) return -1;
    *out = ((1u << leading) - 1u) + rest;
    return 0;
}

static int br_se(struct bitreader *br, int32_t *out)
{
    uint32_t ue;
    if (br_ue(br, &ue) != 0) return -1;
    if (ue & 1u) *out = (int32_t)((ue + 1u) / 2u);
    else *out = -(int32_t)(ue / 2u);
    return 0;
}

/*
 * Split an Annex-B stream into NAL units. Returns the payload of the first
 * SPS (nal_unit_type 7) with emulation prevention bytes removed, or NULL.
 */
static uint8_t *find_sps(const uint8_t *data, size_t len, size_t *sps_len)
{
    size_t i = 0;
    while (i + 3 < len) {
        /* Locate the next start code. */
        size_t sc = 0;
        if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1)
            sc = 3;
        else if (i + 4 < len && data[i] == 0 && data[i + 1] == 0 &&
                 data[i + 2] == 0 && data[i + 3] == 1)
            sc = 4;
        if (sc == 0) { i++; continue; }

        size_t start = i + sc;
        size_t j = start;
        while (j + 3 < len) {
            if (data[j] == 0 && data[j + 1] == 0 &&
                (data[j + 2] == 1 || (data[j + 2] == 0 && data[j + 3] == 1)))
                break;
            j++;
        }
        size_t end = j;
        if (end > start) {
            unsigned nal_type = data[start] & 0x1fu;
            if (nal_type == 7) {
                /* Copy the payload, dropping emulation prevention bytes. */
                size_t n = end - start - 1;
                uint8_t *out = malloc(n ? n : 1);
                if (!out) return NULL;
                size_t o = 0;
                for (size_t k = 1; k < end - start; k++) {
                    if (k + 2 < end - start &&
                        data[start + k] == 0x00 && data[start + k + 1] == 0x00 &&
                        data[start + k + 2] == 0x03) {
                        k += 2; /* skip the escaped 0x03 */
                        continue;
                    }
                    out[o++] = data[start + k];
                }
                *sps_len = o;
                return out;
            }
        }
        i = start;
    }
    return NULL;
}

/*
 * Parse the coded picture size out of an H.264 SPS payload.
 * Returns 0 on success.
 */
static int parse_sps_dimensions(const uint8_t *sps, size_t len,
                                int *out_w, int *out_h, int *out_profile)
{
    struct bitreader br = { sps, len * 8, 0 };
    uint32_t v;

    if (br_bits(&br, 8, &v) != 0) return -1;
    uint32_t profile_idc = v;
    if (br_bits(&br, 8, &v) != 0) return -1; /* constraint flags + reserved */
    if (br_bits(&br, 8, &v) != 0) return -1; /* level_idc */
    if (br_ue(&br, &v) != 0) return -1;       /* seq_parameter_set_id */

    uint32_t chroma_format_idc = 1;
    const int high_profiles[] = { 100, 110, 122, 244, 44, 83, 86, 118, 128,
                                  138, 139, 134, 135 };
    for (size_t k = 0; k < sizeof(high_profiles) / sizeof(high_profiles[0]); k++) {
        if (profile_idc == (uint32_t)high_profiles[k]) {
            if (br_ue(&br, &chroma_format_idc) != 0) return -1;
            if (chroma_format_idc == 3) {
                if (br_bits(&br, 1, &v) != 0) return -1; /* separate_colour_plane_flag */
            }
            if (br_ue(&br, &v) != 0) return -1; /* bit_depth_luma_minus8 */
            if (br_ue(&br, &v) != 0) return -1; /* bit_depth_chroma_minus8 */
            if (br_bits(&br, 1, &v) != 0) return -1; /* qpprime bypass */
            if (br_bits(&br, 1, &v) != 0) return -1; /* seq_scaling_matrix_present_flag */
            if (v) {
                int lists = (chroma_format_idc != 3) ? 8 : 12;
                for (int l = 0; l < lists; l++) {
                    if (br_bits(&br, 1, &v) != 0) return -1; /* seq_scaling_list_present */
                    if (v) {
                        int size = (l < 6) ? 16 : 64;
                        int last = 8, next = 8;
                        for (int s = 0; s < size; s++) {
                            if (next != 0) {
                                int32_t delta;
                                if (br_se(&br, &delta) != 0) return -1;
                                next = (last + delta + 256) % 256;
                            }
                            last = (next == 0) ? last : next;
                        }
                    }
                }
            }
            break;
        }
    }

    if (br_ue(&br, &v) != 0) return -1;       /* log2_max_frame_num_minus4 */
    uint32_t poc_type;
    if (br_ue(&br, &poc_type) != 0) return -1;
    if (poc_type == 0) {
        if (br_ue(&br, &v) != 0) return -1;
    } else if (poc_type == 1) {
        if (br_bits(&br, 1, &v) != 0) return -1;
        int32_t s;
        if (br_se(&br, &s) != 0) return -1;
        if (br_se(&br, &s) != 0) return -1;
        uint32_t cycle;
        if (br_ue(&br, &cycle) != 0) return -1;
        for (uint32_t c = 0; c < cycle; c++)
            if (br_se(&br, &s) != 0) return -1;
    } else if (poc_type > 2) {
        return -1;
    }
    if (br_ue(&br, &v) != 0) return -1;       /* max_num_ref_frames */
    if (br_bits(&br, 1, &v) != 0) return -1; /* gaps_in_frame_num_allowed */
    uint32_t w_mbs, h_units;
    if (br_ue(&br, &w_mbs) != 0) return -1;
    if (br_ue(&br, &h_units) != 0) return -1;
    if (br_bits(&br, 1, &v) != 0) return -1; /* frame_mbs_only_flag */
    uint32_t frame_mbs_only = v;
    if (!frame_mbs_only) {
        if (br_bits(&br, 1, &v) != 0) return -1; /* mb_adaptive_frame_field_flag */
    }
    if (br_bits(&br, 1, &v) != 0) return -1;   /* direct_8x8_inference_flag */
    uint32_t crop_present;
    if (br_bits(&br, 1, &crop_present) != 0) return -1;

    uint32_t crop_l = 0, crop_r = 0, crop_t = 0, crop_b = 0;
    if (crop_present) {
        if (br_ue(&br, &crop_l) != 0) return -1;
        if (br_ue(&br, &crop_r) != 0) return -1;
        if (br_ue(&br, &crop_t) != 0) return -1;
        if (br_ue(&br, &crop_b) != 0) return -1;
    }

    uint32_t sub_w = (chroma_format_idc == 1 || chroma_format_idc == 0) ? 2 : 1;
    uint32_t sub_h = (chroma_format_idc == 1) ? 2 : 1;
    uint32_t crop_x = sub_w;
    uint32_t crop_y = sub_h * (2 - frame_mbs_only);

    int width = (int)((w_mbs + 1) * 16) - (int)((crop_l + crop_r) * crop_x);
    int height = (int)((2 - frame_mbs_only) * (h_units + 1) * 16) -
                 (int)((crop_t + crop_b) * crop_y);

    if (width <= 0 || height <= 0) return -1;
    *out_w = width;
    *out_h = height;
    if (out_profile) *out_profile = (int)profile_idc;
    return 0;
}

/* ------------------------------------------------------------------ */
int main(int argc, char **argv)
{
    const char *node = NULL;
    const char *stream = NULL;
    const char *out_y = NULL;
    int req_w = 0, req_h = 0;
    long req_elems = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-w") && i + 1 < argc) req_w = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-h") && i + 1 < argc) req_h = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-n") && i + 1 < argc) req_elems = atol(argv[++i]);
        else if (!strcmp(argv[i], "-y") && i + 1 < argc) out_y = argv[++i];
        else if (!node) node = argv[i];
        else if (!stream) stream = argv[i];
        else { fprintf(stderr, "usage: %s <render-node> <stream.h264> "
                               "[-w W -h H] [-y out.y]\n", argv[0]); return 1; }
    }
    if (!node || !stream) {
        fprintf(stderr, "usage: %s <render-node> <stream.h264> "
                        "[-w W -h H] [-y out.y]\n", argv[0]);
        return 1;
    }

    size_t len = 0;
    uint8_t *data = read_file(stream, &len);
    if (!data) return 2;

    int width = 0, height = 0, profile_idc = 0;
    size_t sps_len = 0;
    uint8_t *sps = find_sps(data, len, &sps_len);
    if (sps) {
        if (parse_sps_dimensions(sps, sps_len, &width, &height, &profile_idc) != 0)
            width = height = 0;
        free(sps);
    }
    if (req_w > 0 && req_h > 0) { width = req_w; height = req_h; }
    if (width <= 0 || height <= 0) {
        fprintf(stderr, "VADECODE_FAIL could not determine the picture size "
                        "(pass -w/-h)\n");
        free(data);
        return 1;
    }
    VADECODE_OK("STREAM bytes=%zu profile_idc=%d width=%d height=%d",
                len, profile_idc, width, height);

    struct va_api api;
    if (va_api_load(&api) != 0) { free(data); return 2; }

    int fd = open(node, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "VADECODE_FAIL open %s: %s\n", node, strerror(errno));
        free(data);
        return 2;
    }
    VADisplay dpy = api.get_display(fd);
    if (!dpy) {
        fprintf(stderr, "VADECODE_FAIL vaGetDisplayDRM returned NULL\n");
        free(data);
        close(fd);
        return 2;
    }
    int major = -1, minor = -1;
    VAStatus st = api.init(dpy, &major, &minor);
    if (st != VA_STATUS_SUCCESS) {
        fprintf(stderr, "VADECODE_FAIL vaInitialize -> %d (%s)\n", st,
                api.errstr ? api.errstr(st) : "?");
        free(data);
        close(fd);
        return 2;
    }

    /* Pick the first H.264 profile that actually offers a VLD entrypoint. */
    VAProfile profiles[3] = { VA_PROFILE_H264_HIGH, VA_PROFILE_H264_MAIN,
                              VA_PROFILE_H264_BASELINE };
    const char *pnames[3] = { "High", "Main", "Baseline" };
    VAProfile chosen = 0;
    for (int i = 0; i < 3; i++) {
        int n = 0, eps[16];
        if (api.query_eps(dpy, profiles[i], eps, &n) != VA_STATUS_SUCCESS)
            continue;
        for (int j = 0; j < n; j++) {
            if (eps[j] == VA_ENTRYPOINT_VLD) { chosen = profiles[i]; break; }
        }
        if (chosen) {
            VADECODE_OK("PROFILE %s advertises VAEntrypointVLD", pnames[i]);
            break;
        }
    }
    if (!chosen) {
        fprintf(stderr, "VADECODE_FAIL no H.264 profile advertises "
                        "VAEntrypointVLD\n");
        api.term(dpy);
        free(data);
        close(fd);
        return 2;
    }

    /*
     * The config must carry the slice-decode mode explicitly. The driver
     * keeps the attribute in its per-config table and, when the attribute is
     * absent, its stored slice mode stays 0; the decode path then rejects
     * the picture with "VpuApiDecPic: slices number was wrong!". libva's
     * specification says the mode defaults to VA_DEC_SLICE_MODE_NORMAL when
     * the user does not set it, but this driver does not apply that default.
     */
    VAConfigAttrib attrs[2];
    attrs[0].type = VA_CONFIG_ATTRIB_DEC_SLICE_MODE;
    attrs[0].value = VA_DEC_SLICE_MODE_NORMAL;
    attrs[1].type = VA_CONFIG_ATTRIB_RT_FORMAT;
    attrs[1].value = VA_RT_FORMAT_YUV420;

    VAConfigID config = 0;
    st = api.create_config(dpy, chosen, VA_ENTRYPOINT_VLD, attrs,
                           (int)(sizeof(attrs) / sizeof(attrs[0])), &config);
    if (st != VA_STATUS_SUCCESS) {
        fprintf(stderr, "VADECODE_FAIL vaCreateConfig(%u,VLD) -> %d (%s)\n",
                chosen, st, api.errstr ? api.errstr(st) : "?");
        api.term(dpy);
        free(data);
        close(fd);
        return 2;
    }

    VASurfaceID surf = 0;
    st = api.create_surfaces(dpy, VA_RT_FORMAT_YUV420, (unsigned)width,
                             (unsigned)height, &surf, 1, NULL, 0);
    if (st != VA_STATUS_SUCCESS) {
        fprintf(stderr, "VADECODE_FAIL vaCreateSurfaces(%dx%d) -> %d (%s)\n",
                width, height, st, api.errstr ? api.errstr(st) : "?");
        api.destroy_config(dpy, config);
        api.term(dpy);
        free(data);
        close(fd);
        return 2;
    }

    VAContextID ctx = 0;
    st = api.create_context(dpy, config, width, height, 0x1 /* VA_PROGRESSIVE */,
                             &surf, 1, &ctx);
    if (st != VA_STATUS_SUCCESS) {
        fprintf(stderr, "VADECODE_FAIL vaCreateContext -> %d (%s)\n", st,
                api.errstr ? api.errstr(st) : "?");
        api.destroy_surfaces(dpy, &surf, 1);
        api.destroy_config(dpy, config);
        api.term(dpy);
        free(data);
        close(fd);
        return 2;
    }

    /*
     * One access unit = one picture. The stream is fed as slice data.
     *
     * NOTE: libva 2.24 changed the vaCreateBuffer() prototype: it now takes
     * the VAContextID as its second argument (it was VABufferType there in
     * libva 1.x). Calling the 1.x signature against a 2.24 library makes the
     * driver receive the buffer type as its context id, which the vendor
     * driver reports as "DdiMedia_GetContextFromContextID: Invalid context".
     */
    VABufferID buf_id = 0;
    st = api.create_buffer(dpy, ctx, VA_BUFFER_TYPE_SLICE_DATA,
                           (unsigned int)len,
                           req_elems > 0 ? (unsigned int)req_elems : 1u,
                           data, &buf_id);
    if (st != VA_STATUS_SUCCESS) {
        fprintf(stderr, "VADECODE_FAIL vaCreateBuffer -> %d (%s)\n", st,
                api.errstr ? api.errstr(st) : "?");
        api.destroy_context(dpy, ctx);
        api.destroy_surfaces(dpy, &surf, 1);
        api.destroy_config(dpy, config);
        api.term(dpy);
        free(data);
        close(fd);
        return 2;
    }

    st = api.begin_picture(dpy, ctx, surf);
    if (st == VA_STATUS_SUCCESS)
        st = api.render_picture(dpy, ctx, &buf_id, 1);
    if (st == VA_STATUS_SUCCESS)
        st = api.end_picture(dpy, ctx);
    if (st != VA_STATUS_SUCCESS) {
        fprintf(stderr, "VADECODE_FAIL render -> %d (%s)\n", st,
                api.errstr ? api.errstr(st) : "?");
        api.destroy_buffer(dpy, buf_id);
        api.destroy_context(dpy, ctx);
        api.destroy_surfaces(dpy, &surf, 1);
        api.destroy_config(dpy, config);
        api.term(dpy);
        free(data);
        close(fd);
        return 3;
    }

    st = api.sync_surface(dpy, surf);
    if (st != VA_STATUS_SUCCESS) {
        fprintf(stderr, "VADECODE_FAIL vaSyncSurface -> %d (%s)\n", st,
                api.errstr ? api.errstr(st) : "?");
        api.destroy_buffer(dpy, buf_id);
        api.destroy_context(dpy, ctx);
        api.destroy_surfaces(dpy, &surf, 1);
        api.destroy_config(dpy, config);
        api.term(dpy);
        free(data);
        close(fd);
        return 3;
    }

    /* Read the decoded picture back out of the driver's surface. */
    VAImage img;
    memset(&img, 0, sizeof(img));
    st = api.derive_image(dpy, surf, &img);
    if (st != VA_STATUS_SUCCESS) {
        fprintf(stderr, "VADECODE_FAIL vaDeriveImage -> %d (%s)\n", st,
                api.errstr ? api.errstr(st) : "?");
        api.destroy_buffer(dpy, buf_id);
        api.destroy_context(dpy, ctx);
        api.destroy_surfaces(dpy, &surf, 1);
        api.destroy_config(dpy, config);
        api.term(dpy);
        free(data);
        close(fd);
        return 3;
    }

    void *mapped = NULL;
    st = api.map_buffer(dpy, img.buf, &mapped);
    if (st != VA_STATUS_SUCCESS || !mapped) {
        fprintf(stderr, "VADECODE_FAIL vaMapBuffer -> %d (%s)\n", st,
                api.errstr ? api.errstr(st) : "?");
        api.destroy_buffer(dpy, buf_id);
        api.destroy_context(dpy, ctx);
        api.destroy_surfaces(dpy, &surf, 1);
        api.destroy_config(dpy, config);
        api.term(dpy);
        free(data);
        close(fd);
        return 3;
    }

    VADECODE_OK("SURFACE fourcc=0x%08x width=%u height=%u planes=%u "
                "pitch0=%u offset0=%u",
                img.format.fourcc, img.width, img.height, img.num_planes,
                img.pitches[0], img.offsets[0]);

    /* Plane 0 is luma for every planar/packed YUV 4:2:0 layout, so the
     * comparison against a software decode is layout independent. */
    const uint8_t *y = (const uint8_t *)mapped + img.offsets[0];
    uint32_t pitch = img.pitches[0];
    uint32_t yw = img.width, yh = img.height;

    /* Cheap sanity check: a decoded picture must not be uniformly constant. */
    long long sum = 0;
    uint32_t nonzero = 0;
    for (uint32_t yrow = 0; yrow < yh; yrow++) {
        const uint8_t *row = y + (size_t)yrow * pitch;
        for (uint32_t x = 0; x < yw; x++) { sum += row[x]; if (row[x]) nonzero++; }
    }
    double mean = yw && yh ? (double)sum / ((double)yw * yh) : 0.0;
    VADECODE_OK("LUMA mean=%.2f nonzero=%u/%u", mean, nonzero, yw * yh);

    int wrote = 0;
    if (out_y) {
        FILE *f = fopen(out_y, "wb");
        if (f) {
            for (uint32_t yrow = 0; yrow < yh; yrow++)
                fwrite(y + (size_t)yrow * pitch, 1, yw, f);
            wrote = 1;
            fclose(f);
            VADECODE_OK("LUMA_WRITTEN %s %ux%u", out_y, yw, yh);
        } else {
            fprintf(stderr, "VADECODE_FAIL cannot write %s\n", out_y);
        }
    }

    api.unmap_buffer(dpy, img.buf);
    api.destroy_buffer(dpy, buf_id);
    api.destroy_context(dpy, ctx);
    api.destroy_surfaces(dpy, &surf, 1);
    api.destroy_config(dpy, config);
    api.term(dpy);
    close(fd);
    free(data);

    if (!nonzero) {
        fprintf(stderr, "VADECODE_FAIL decoded luma plane is entirely zero\n");
        return 3;
    }
    VADECODE_OK("VADECODE_STATUS: SUCCESS");
    (void)wrote;
    return 0;
}
