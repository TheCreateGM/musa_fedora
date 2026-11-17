#ifndef _IOCTL_VPU_H_
#define _IOCTL_VPU_H_
#include "../config.h"

#define RETCODE(r) (ret == RETCODE_SUCCESS ? 0 : -OS_VAL(EIO))

struct ParamEncStartFrame;
struct ParamEncStartFrameV2;
struct drm_file;
struct mt_virm;
struct mt_chip;
struct mt_intr_map;
struct mt_vpu_cmd_queue;

u64 get_node_addr(struct drm_file *file, struct mt_virm *vm, u64 handle_or_addr,
                  struct mt_chip *chip);
u64 get_node_addr_local(struct drm_file *file, struct mt_virm *vm, u64 handle_or_addr,
                struct mt_chip *chip);
u64 get_node_addr_guest(struct drm_file *file, struct mt_virm *vm, u64 handle_or_addr, u32 type,
                 struct mt_chip *chip);
int add_opened(struct drm_file *file, struct mt_virm *vm, int coreIdx, int instIdx, int isDecoder,
               VpuHandle handle);
int del_opened(struct drm_file *file, struct mt_virm *vm, VpuHandle handle);

int mtvpu_job_dispatch(struct drm_device *drm, struct drm_file *file, struct mtgpu_codec_priv_data *data_priv, void *data, u32 cmdType);

int ioctl_gem_create(struct drm_device *dev, void *data, struct drm_file *file);
int ioctl_gem_map(struct drm_device *dev, void *data, struct drm_file *file);
int ioctl_gem_destroy(struct drm_device *dev, void *data, struct drm_file *file);
int ioctl_request_core(struct drm_device *drm, void *data, struct drm_file *file);
int ioctl_unref_core(struct drm_device *drm, void *data, struct drm_file *file);
int ioctl_gem_modify(struct drm_device *drm, void *data, struct drm_file *file);

int ioctl_dec_open(struct drm_device *drm, void *data, struct drm_file *file,
                   struct mtgpu_codec_priv_data *data_priv, struct mt_virm *vm);
int ioctl_dec_close(struct drm_device *drm, void *data, struct drm_file *file, struct mt_virm *vm);
int ioctl_dec_init_seq(struct drm_device *drm, void *data, struct drm_file *file,
                       struct mt_virm *vm);
int ioctl_dec_open_and_init(struct drm_device *drm, void *data, struct drm_file *file,
                            struct mtgpu_codec_priv_data *data_priv, struct mt_virm *vm);
int ioctl_dec_start_frame(struct drm_device *drm, void *data, struct drm_file *file,
                          struct mt_virm *vm);
int vpu_dec_seq_done(struct mt_chip *chip, struct mt_virm *vm, VpuHandle handle,
                     RenderTargetInfo *target);
int vpu_dec_pic_done(struct mt_chip *chip, struct mt_virm *vm, VpuHandle handle, struct mt_intr_map *intr_data);

int ioctl_enc_open(struct drm_device *drm, void *data, struct drm_file *file,
                   struct mtgpu_codec_priv_data *data_priv, struct mt_virm *vm);
int ioctl_enc_close(struct drm_device *drm, void *data, struct drm_file *file, struct mt_virm *vm);
int ioctl_enc_start_frame(struct drm_device *drm, void *data, struct drm_file *file,
                          struct mt_virm *vm);
int ioctl_enc_start_frame_v2(struct drm_device *drm, void *data, struct drm_file *file,
                            struct mt_virm *vm);
int vpu_enc_pic_done(struct mt_chip *chip, struct mt_virm *vm, VpuHandle handle,
                     struct ParamEncStartFrame *args);
int vpu_enc_pic_done_v2(struct mt_chip *chip, struct mt_virm *vm, VpuHandle handle,
                     struct ParamEncStartFrameV2 *args);
int clear_sync_addr(u64 addr, struct mt_vpu_cmd_queue *cmd_q);

int vid_ioctl_wait(struct drm_device *drm, void *data, struct drm_file *file);
int vid_ioctl_dma_trans(struct drm_device *drm, void *data, struct drm_file *file);

int ioctl_guest_dec_open(struct drm_device *drm, void *data, struct drm_file *file);
int ioctl_guest_dec_close(struct drm_device *drm, void *data, struct drm_file *file);
int ioctl_guest_dec_init_seq(struct drm_device *drm, void *data, struct drm_file *file);
int ioctl_guest_dec_start_frame(struct drm_device *drm, void *data, struct drm_file *file);
int ioctl_guest_enc_open(struct drm_device *drm, void *data, struct drm_file *file);
int ioctl_guest_enc_close(struct drm_device *drm, void *data, struct drm_file *file);
int ioctl_guest_enc_init_seq(struct drm_device *drm, void *data, struct drm_file *file);
int ioctl_guest_enc_start_frame(struct drm_device *drm, void *data, struct drm_file *file);
int ioctl_guest_enc_start_frame_v2(struct drm_device *drm, void *data, struct drm_file *file);
#endif
