OBJ_DIR := $(PWD)
KERNELDIR := /lib/modules/${KERNELVER}/build
KERNELSRC := /lib/modules/${KERNELVER}/source

ifneq ($(wildcard $(KERNELSRC)),)
	ccflags-y += -I$(KERNELSRC)
	ccflags-$(CONFIG_X86) += -include $(KERNELSRC)/arch/x86/include/asm/cpufeatures.h
else
	ccflags-$(CONFIG_X86) += -include $(KERNELDIR)/arch/x86/include/asm/cpufeatures.h
endif

ifeq ($(ARCH),arm64)
ccflags-y += -ffixed-x18
endif

obj-m += mtgpu.o

MTGPU_BINARY := $(OBJ_DIR)/objs/$(ARCH)/mtgpu.core.o_binary
MTGPU_BINARY_O := $(OBJ_DIR)/objs/$(ARCH)/mtgpu.core.o

CONFTEST_H := $(OBJ_DIR)/inc/conftest.h
CONFTEST := $(OBJ_DIR)/conftest

MTGPU_DEPENDENCIES := $(MTGPU_BINARY_O)

ifneq ($(wildcard ${PWD}/src/mtgpu/ion/Makefile),)
	MTGPU_ION_DIR := ${PWD}/src/mtgpu/ion
	MTGPU_ION_O := ${PWD}/src/mtgpu/ion/mt_ion.o

	KBUILD_EXTRA_SYMBOLS  += $(MTGPU_ION_DIR)/Module.symvers

	MTGPU_DEPENDENCIES += $(MTGPU_ION_O)

	mtgpu-objs += src/mtgpu/ion_lma_heap.o \
		      src/mt/mt_ion_stats.o
else
	MTGPU_DEPENDENCIES += $(CONFTEST_H)
endif

ifneq ($(wildcard ${PWD}/src/mtgpu/vgpu/basic/Makefile),)
	MTGPU_VZ_BASIC_DIR := ${PWD}/src/mtgpu/vgpu/basic
	MTGPU_VZ_BASIC_O := ${PWD}/src/mtgpu/vgpu/basic/mtvgpu_basic.o

	KBUILD_EXTRA_SYMBOLS  += $(MTGPU_VZ_BASIC_DIR)/Module.symvers

	MTGPU_DEPENDENCIES += $(MTGPU_VZ_BASIC_O)
else
	MTGPU_DEPENDENCIES += $(CONFTEST_H)
endif

mtgpu-objs += src/mt/mt_dvfs_device.o

# build for audio
ifneq ($(filter y m,$(CONFIG_SND)),)
        ifneq ($(filter y m,$(CONFIG_SND_PCM)),)
                mtgpu-objs += \
                        src/mtsnd/eld.o \
                        src/mtsnd/mtsnd_codec.o \
                        src/mtsnd/mtsnd_drv.o \
                        src/mtsnd/mtsnd_pcm.o
        else
                $(info Audio modules will NOT be compiled due to missing dependencies: snd-pcm)
        endif
else
        $(info Audio modules will NOT be compiled due to missing dependencies: snd)
endif

# build for virtualization
mtgpu-$(CONFIG_VZ_MOORE_THREADS) += \
	src/mtgpu/vgpu/mtgpu_vgpu_common.o \
	src/mtgpu/vgpu/os-interface-vgpu.o

mtgpu-$(CONFIG_HUG_N_MOORE_THREADS) += \
	src/mtgpu/vgpu/basic/mtgpu_mdev_common.o \
	src/mtgpu/vgpu/basic/vgpu_types_basic.o

mtgpu-objs += src/common/os-interface.o \
	src/common/os-interface-drm.o \
	src/mtgpu/mtgpu_drv.o \
	src/mtgpu/mtgpu_module_param.o \
	src/mtgpu/drm_compatibility.o \
	src/mtgpu/mtgpu_drm_dsc.o \
	src/mtgpu/mtgpu_drm_dp.o \
	src/mtgpu/mtgpu_drm_dispc.o \
	src/mtgpu/mtgpu_drm_atomic.o \
	src/mtgpu/mtgpu_drm_utils.o \
	src/mtgpu/mtgpu_drm_dummy.o \
	src/mtgpu/mtgpu_drm_display.o \
	src/mtgpu/mtgpu_drm_drv.o \
	src/mtgpu/mtgpu_drm_dummy_connector.o \
	src/mtgpu/mtgpu_drm_dummy_crtc.o \
	src/mtgpu/mtgpu_drm_gem.o \
	src/mtgpu/mtgpu_drm_hdmi.o \
	src/mtgpu/mtgpu_drm_debugfs.o \
	src/mtgpu/phy-mthreads-dp.o \
	src/mtgpu/p2pdma.o \
	src/mtgpu/linux_display_debugfs.o \
	src/mtgpu/linux_data_convert.o \
	src/mtgpu/linux_dummy_drv.o \
	src/mtgpu/linux_dummy_crtc_drv.o \
	src/mtgpu/linux_dummy_connector_drv.o \
	src/mtgpu/linux_display_drv.o \
	src/mtgpu/linux_dispc_drv.o \
	src/mtgpu/linux_encoder_drv.o \
	src/mtgpu/linux_dp_drv.o \
	src/mtgpu/linux_dp_mst_drv.o \
	src/mtgpu/linux_hdmi_drv.o \
	src/mtvpu/mtvpu_api.o \
	src/mtvpu/mtvpu_drv.o \
	src/mtvpu/mtvpu_gem.o \
	src/mt/error_mapping.o \
	src/mt/interrupt_support.o \
	src/mt/module_common.o \
	src/mt/osfunc.o \
	src/mt/pci_support.o \
	src/mt/physmem_osmem_linux.o \
	src/mt/pmr_os.o \
	src/mt/mt_counting_timeline.o \
	src/mt/mt_debug.o \
	src/mt/mt_debugfs.o \
	src/mt/mt_drm.o \
	src/mt/mt_fence.o \
	src/mt/mt_gputrace.o \
	src/mt/mt_platform_drv.o \
	src/mt/mt_sw_fence.o \
	src/mt/mt_sync_file.o \
	src/mt/mt_sync_ioctl_common.o \
	src/mt/mt_sync_ioctl_drm.o \
	objs/$(ARCH)/mtgpu.core.o

mtgpu-$(CONFIG_RISCV) += src/mt/osfunc_riscv.o
mtgpu-$(CONFIG_ARM) += src/mt/osfunc_arm.o
mtgpu-$(CONFIG_ARM64) += src/mt/osfunc_arm64.o
mtgpu-$(CONFIG_LOONGARCH) += src/mt/osfunc_loongarch.o \
	src/mtgpu/dmi_loongarch.o
mtgpu-$(CONFIG_X86) += src/mt/osfunc_x86.o

ccflags-y := -D__linux__ -include config_kernel.h
ccflags-y += -include linux/version.h
ccflags-y += -include conftest.h -include arch_compat.h
ccflags-y += -I$(OBJ_DIR)/inc \
	-I$(OBJ_DIR)/inc/mt \
	-I$(OBJ_DIR)/inc/mt/generated \
	-I$(OBJ_DIR)/inc/mt/hwdefs \
	-I$(OBJ_DIR)/inc/mt/hwdefs/km \
	-I$(OBJ_DIR)/inc/mt/include \
	-I$(OBJ_DIR)/inc/mt/include/mt \
	-I$(OBJ_DIR)/inc/mt/services \
	-I$(OBJ_DIR)/inc/common \
	-I$(OBJ_DIR)/inc/mtgpu \
	-I$(OBJ_DIR)/inc/shared_include/linux/uapi/drm \
	-I$(OBJ_DIR)/inc/shared_include/fwif \
	-I$(OBJ_DIR)/inc/shared_include/ \
	-I$(OBJ_DIR)/inc/mtgpu-next \
	-I$(OBJ_DIR)/inc/mtgpu/vgpu \
	-I$(OBJ_DIR)/inc/mtgpu/vgpu/basic \
	-I$(OBJ_DIR)/inc/mtgpu/ion \
	-I$(OBJ_DIR)/inc/mtgpu/ion/ion \
	-I$(OBJ_DIR)/inc/mtgpu/ion/ion/heaps \
	-I$(OBJ_DIR)/inc/mtvpu \
	-I$(OBJ_DIR)/inc/mtvpu/linux \
	-I$(OBJ_DIR)/inc/mtvpu/linux/helper \
	-I$(OBJ_DIR)/inc/mtsnd \
	-I$(OBJ_DIR)/inc/imgtec

quiet_cmd_symlink = SYMLINK $@
cmd_symlink = ln -sf $< $@

all: $(MTGPU_DEPENDENCIES)
	 $(MAKE) -C $(KERNELDIR) M=$(OBJ_DIR) modules
	@echo "make all end"

# Plain-make copy for the outer `all` target (kbuild handles its own copy
# inside the KERNELRELEASE context below).
ifeq ($(KERNELRELEASE),)
$(MTGPU_BINARY_O): $(MTGPU_BINARY)
	cp -f $< $@
endif

# Use Kbuild bookkeeping for the precompiled vendor core object. Modern
# modpost reads every object's .cmd file even when the object was not
# compiled here, and `targets +=` is required for kbuild to run the rule.
ifneq ($(KERNELRELEASE),)
targets += objs/$(ARCH)/mtgpu.core.o
$(obj)/objs/$(ARCH)/mtgpu.core.o: $(src)/objs/$(ARCH)/mtgpu.core.o_binary FORCE
	$(call if_changed,copy)
endif

$(CONFTEST_H): $(CONFTEST)
	@$< -a $(ARCH) -k $(KERNELVER) -d $(KERNELDIR) -e $(KERNELSRC)
	@ln -sf $(OBJ_DIR)/conftest.h $@

$(MTGPU_ION_O): $(CONFTEST_H)
	 $(MAKE) -C $(KERNELDIR) M=$(MTGPU_ION_DIR) modules
	@cp $(MTGPU_ION_DIR)/mt_ion.ko $(PWD)

$(MTGPU_VZ_BASIC_O): $(CONFTEST_H)
	make  -C $(KERNELDIR)   M=$(MTGPU_VZ_BASIC_DIR) modules
	@cp $(MTGPU_VZ_BASIC_DIR)/mtvgpu_basic.ko $(PWD)/mtvgpu_basic.ko

clean:
	 $(MAKE) -C $(KERNELDIR) M=$(OBJ_DIR) clean
	@echo "make clean end"
