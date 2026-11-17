/*
 * @Copyright   Copyright (c) Moorethreads Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 */

#ifndef __MTGPU_SND_H__
#define __MTGPU_SND_H__

int mtsnd_init(void);
void mtsnd_deinit(void);

int disable_audio __attribute__((weak)) = 1;
int __attribute__((weak)) mtsnd_init(void)
{
	return 0;
}

void __attribute__((weak)) mtsnd_deinit(void)
{
}

#endif /* __MTGPU_SND_H__ */
