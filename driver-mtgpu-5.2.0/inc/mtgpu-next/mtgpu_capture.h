/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     GPLv2
 */

#ifndef _MTGPU_CAPTURE_H_
#define _MTGPU_CAPTURE_H_

struct mtgpu_dispc;
struct mtdisp_layer_config;

struct mtgpu_capture_context {
	struct list_head job_list;
	struct mutex *lock;
	wait_queue_head_t *wq;
	struct dma_fence_array *fence_array;
	struct work_struct *work;
	struct device *dev;
	struct drm_crtc *crtc;
	struct drm_gem_object *obj;
	struct mtdisp_layer_config *layer_config;
};

struct mtgpu_capture_job {
	struct dma_fence *fence;
	struct list_head node;
	int wait_done;
	struct drm_gem_object *obj;
	struct mtdisp_layer_config *layer_config;
};

void mtgpu_capture_job_process(struct device *dev, struct mtgpu_capture_context *capture_ctx,
			       struct mtdisp_layer_config *layer, struct drm_gem_object *obj);
int mtgpu_capture_context_create(struct device *dev, struct drm_crtc *crtc,
				 struct mtgpu_capture_context **capture_ctx_out);
void mtgpu_capture_context_destroy(struct mtgpu_capture_context *capture_ctx);

#endif /* _MTGPU_CAPTURE_H_ */
