OBJ_DIR := $(if $(KBUILD_EXTMOD),$(KBUILD_EXTMOD),$(CURDIR))
KERNELVER ?= $(shell uname -r)
ARCH ?= x86_64
KERNELDIR ?= /lib/modules/${KERNELVER}/build
KERNELSRC ?= /lib/modules/${KERNELVER}/source

# ---- objtool wrapper (Fedora port) -----------------------------------------
# kbuild runs objtool on module objects/links when CONFIG_OBJTOOL is set. On
# Fedora (rawhide especially) kernel-devel can ship an objtool linked against
# a binutils snapshot SONAME the distro no longer carries, which breaks every
# out-of-tree module build at the objtool step. Hand kbuild the wrapper, which
# runs the real objtool unchanged when it can start and links without objtool
# (with a loud warning) when it cannot. See scripts/objtool-safe.sh.
MTGPU_REAL_OBJTOOL ?= $(KERNELDIR)/tools/objtool/objtool
# Kernel tree for the wrapper's fallback path (rebuilt objtool cache key).
MTGPU_KDIR ?= $(KERNELDIR)
export MTGPU_REAL_OBJTOOL MTGPU_KDIR
KBUILD_OBJTOOL := objtool="$(OBJ_DIR)/scripts/objtool-safe.sh"

ifneq ($(wildcard $(KERNELSRC)),)
	ccflags-y += -I$(KERNELSRC)
	ccflags-$(CONFIG_X86) += -include $(KERNELSRC)/arch/x86/include/asm/cpufeatures.h
else
	ccflags-$(CONFIG_X86) += -include $(KERNELDIR)/arch/x86/include/asm/cpufeatures.h
endif

include ${KERNELDIR}/.config

obj-m += mtgpu.o

ifneq ($(wildcard ${PWD}/src/mtgpu/ion/Makefile),)
	MTGPU_ION_DIR := ${PWD}/src/mtgpu/ion
	MTGPU_ION_O := ${PWD}/src/mtgpu/ion/mt_ion.o

	mtgpu-objs += src/mtgpu/ion_lma_heap.o \
		      src/pvr/pvr_ion_stats.o
endif

mtgpu-objs += src/common/os-interface.o \
	src/common/os-interface-drm.o \
	src/mtgpu/mtgpu_drv.o \
	src/mtgpu/mtgpu_ob_conf_fixup.o \
	src/mtgpu/mtgpu_module_param.o \
	src/mtgpu/drm_compatibility.o \
	src/mtgpu/mtgpu_drm_dsc.o \
	src/mtgpu/mtgpu_drm_dp.o \
	src/mtgpu/mtgpu_drm_dispc.o \
	src/mtgpu/mtgpu_drm_atomic.o \
	src/mtgpu/mtgpu_drm_utils.o \
	src/mtgpu/mtgpu_drm_drv.o \
	src/mtgpu/mtgpu_drm_dummy_connector.o \
	src/mtgpu/mtgpu_drm_dummy_crtc.o \
	src/mtgpu/mtgpu_drm_gem.o \
	src/mtgpu/mtgpu_drm_hdmi.o \
	src/mtgpu/mtgpu_drm_debugfs.o \
	src/mtgpu/phy-mthreads-dp.o \
	src/mtgpu/p2pdma.o \
	src/mtgpu/mtgpu_ipc_tty.o \
	src/mtgpu/mtgpu_fec_dbg.o \
	src/mtvpu/mtvpu_api.o \
	src/mtvpu/mtvpu_drv.o \
	src/mtvpu/mtvpu_gem.o \
	src/pvr/error_mapping.o \
	src/pvr/interrupt_support.o \
	src/pvr/module_common.o \
	src/pvr/osfunc.o \
	src/pvr/pci_support.o \
	src/pvr/physmem_osmem_linux.o \
	src/pvr/pmr_os.o \
	src/pvr/pvr_buffer_sync.o \
	src/pvr/pvr_counting_timeline.o \
	src/pvr/pvr_debug.o \
	src/pvr/pvr_debugfs.o \
	src/pvr/pvr_drm.o \
	src/pvr/pvr_fence.o \
	src/pvr/pvr_gputrace.o \
	src/pvr/pvr_platform_drv.o \
	src/pvr/pvr_sw_fence.o \
	src/pvr/pvr_sync_file.o \
	src/pvr/pvr_sync_ioctl_common.o \
	src/pvr/pvr_sync_ioctl_drm.o \
	src/pvr/trace_events.o \
	objs/$(ARCH)/mtgpu_core.o

mtgpu-objs += src/pvr/pvr_dvfs_device.o

# build for audio
mtgpu-objs += \
	src/mtsnd/eld.o \
	src/mtsnd/mtsnd_codec.o \
	src/mtsnd/mtsnd_drv.o \
	src/mtsnd/mtsnd_pcm.o

# build for virtualization
mtgpu-$(CONFIG_VZ_MOORE_THREADS) += \
	src/mtgpu/vgpu/mtgpu_mdev_common.o \
	src/mtgpu/vgpu/os-interface-vgpu.o

mtgpu-$(CONFIG_RISCV) += src/pvr/osfunc_riscv.o
mtgpu-$(CONFIG_ARM) += src/pvr/osfunc_arm.o
mtgpu-$(CONFIG_ARM64) += src/pvr/osfunc_arm64.o
mtgpu-$(CONFIG_LOONGARCH) += src/pvr/osfunc_loongarch.o \
	src/mtgpu/dmi_loongarch.o
mtgpu-$(CONFIG_X86) += src/pvr/osfunc_x86.o

ccflags-y := -D__linux__ -include config_kernel.h
ccflags-y += -include linux/version.h
ccflags-y += -include conftest.h -include arch_compat.h
ccflags-y += -I$(OBJ_DIR)/inc \
	-I$(OBJ_DIR)/inc/pvr \
	-I$(OBJ_DIR)/inc/pvr/generated \
	-I$(OBJ_DIR)/inc/pvr/hwdefs \
	-I$(OBJ_DIR)/inc/pvr/hwdefs/km \
	-I$(OBJ_DIR)/inc/pvr/include \
	-I$(OBJ_DIR)/inc/pvr/include/powervr \
	-I$(OBJ_DIR)/inc/pvr/services \
	-I$(OBJ_DIR)/inc/common \
	-I$(OBJ_DIR)/inc/mtgpu \
	-I$(OBJ_DIR)/inc/shared_include/linux/uapi/drm \
	-I$(OBJ_DIR)/inc/shared_include/fwif \
	-I$(OBJ_DIR)/inc/mtgpu-next \
	-I$(OBJ_DIR)/inc/mtgpu/vgpu \
	-I$(OBJ_DIR)/inc/mtgpu/ion \
	-I$(OBJ_DIR)/inc/mtgpu/ion/ion \
	-I$(OBJ_DIR)/inc/mtgpu/ion/ion/heaps \
	-I$(OBJ_DIR)/inc/mtvpu \
	-I$(OBJ_DIR)/inc/mtvpu/linux \
	-I$(OBJ_DIR)/inc/mtvpu/linux/helper \
	-I$(OBJ_DIR)/inc/mtsnd \
	-I$(OBJ_DIR)/inc/imgtec

CONFTEST_H := $(OBJ_DIR)/inc/conftest.h
CONFTEST := $(OBJ_DIR)/conftest

MTGPU_BINARY := $(OBJ_DIR)/objs/$(ARCH)/mtgpu_core.o_binary

all: $(CONFTEST_H) $(MTGPU_ION_O)
	$(MAKE) -C $(KERNELDIR)   M=$(OBJ_DIR) modules $(KBUILD_OBJTOOL)
	@echo "make all end"

# Use Kbuild bookkeeping for the precompiled vendor object. Modern modpost
# reads every object's .cmd file even when the object was not compiled here.
ifneq ($(KERNELRELEASE),)
targets += objs/$(ARCH)/mtgpu_core.o
$(obj)/objs/$(ARCH)/mtgpu_core.o: $(src)/objs/$(ARCH)/mtgpu_core.o_binary FORCE
	$(call if_changed,copy)
endif

$(CONFTEST_H): $(CONFTEST)
	@$< -a $(ARCH) -k $(KERNELVER) -d $(KERNELDIR) -e $(KERNELSRC)
	@ln -sf $(OBJ_DIR)/conftest.h $@

$(MTGPU_ION_O):
	$(MAKE) -C $(KERNELDIR)   M=$(MTGPU_ION_DIR) modules $(KBUILD_OBJTOOL)
	@cp $(MTGPU_ION_DIR)/Module.symvers  $(MTGPU_ION_DIR)/mt_ion.ko $(PWD)

clean:
	$(MAKE) -C $(KERNELDIR)   M=$(OBJ_DIR) clean
	@echo "make clean end"
