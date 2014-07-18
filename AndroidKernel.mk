ifeq ($(TARGET_KERNEL_BUILT_FROM_SOURCE),true)

# Force using bash as a shell, otherwise, on Ubuntu, dash will break some
# dependency due to its bad handling of echo \1
MAKE += SHELL=/bin/bash

ifeq ($(KERNEL_CFG_NAME),)
$(error cannot build kernel, config not specified)
endif

ifeq ($(TARGET_KERNEL_ARCH),x86_64)
KERNEL_TOOLCHAIN_ARCH := $(TARGET_KERNEL_ARCH)
else
KERNEL_TOOLCHAIN_ARCH := i686
endif
KERNEL_EXTRA_FLAGS := ANDROID_TOOLCHAIN_FLAGS="-mno-android -Werror"
KERNEL_CROSS_COMP := \
  $(patsubst %gcc,%,$(firstword \
    $(wildcard $(ANDROID_BUILD_TOP)/prebuilts/gcc/$(HOST_PREBUILT_TAG)/host/$(KERNEL_TOOLCHAIN_ARCH)-linux-glibc2.7-4.6/bin/$(KERNEL_TOOLCHAIN_ARCH)-linux-gcc) \
    $(wildcard $(ANDROID_BUILD_TOP)/prebuilts/gcc/$(HOST_PREBUILT_TAG)/host/$(KERNEL_TOOLCHAIN_ARCH)-linux-glibc2.11-4.6/bin/$(KERNEL_TOOLCHAIN_ARCH)-linux-gcc)))

KERNEL_CCACHE :=$(firstword $(TARGET_CC))
KERNEL_PATH := $(ANDROID_BUILD_TOP)/vendor/intel/support
ifeq ($(notdir $(KERNEL_CCACHE)),ccache)
KERNEL_CROSS_COMP := "$(realpath $(KERNEL_CCACHE)) $(KERNEL_CROSS_COMP)"
KERNEL_PATH := $(KERNEL_PATH):$(ANDROID_BUILD_TOP)/$(dir $(KERNEL_CCACHE))
endif

#remove time_macros from ccache options, it breaks signing process
KERNEL_CCSLOP := $(filter-out time_macros,$(subst $(comma), ,$(CCACHE_SLOPPINESS)))
KERNEL_CCSLOP := $(subst $(space),$(comma),$(KERNEL_CCSLOP))

KERNEL_OUT_DIR := $(PRODUCT_OUT)/linux/kernel
KERNEL_CONFIG := $(KERNEL_OUT_DIR)/.config
KERNEL_BLD_FLAGS := \
    ARCH=$(TARGET_KERNEL_ARCH) \
    $(KERNEL_EXTRA_FLAGS)

KERNEL_BLD_FLAGS :=$(KERNEL_BLD_FLAGS) \
     O=../../$(KERNEL_OUT_DIR) \

KERNEL_BLD_ENV := CROSS_COMPILE=$(KERNEL_CROSS_COMP) \
    PATH=$(KERNEL_PATH):$(PATH) \
    CCACHE_SLOPPINESS=$(KERNEL_CCSLOP)

KERNEL_DEFCONFIG ?= $(KERNEL_SRC_DIR)/arch/x86/configs/$(TARGET_KERNEL_ARCH)_$(KERNEL_CFG_NAME)_defconfig
KERNEL_DIFFCONFIG ?= $(TARGET_DEVICE_DIR)/$(TARGET_DEVICE)_diffconfig
KERNEL_VERSION_FILE := $(KERNEL_OUT_DIR)/include/config/kernel.release
KERNEL_BZIMAGE := $(PRODUCT_OUT)/kernel

HOST_OPENSSL := $(HOST_OUT_EXECUTABLES)/openssl

$(KERNEL_CONFIG): $(KERNEL_DEFCONFIG) $(wildcard $(KERNEL_DIFFCONFIG))
	@echo Regenerating kernel config $(KERNEL_OUT_DIR)
	@mkdir -p $(KERNEL_OUT_DIR)
	@cat $^ > $@
	@! $(KERNEL_BLD_ENV) $(MAKE) -C $(KERNEL_SRC_DIR) $(KERNEL_BLD_FLAGS) listnewconfig | grep -q CONFIG_ ||  \
		(echo "There are errors in defconfig $^, please run cd $(KERNEL_SRC_DIR) && ./scripts/updatedefconfigs.sh" ; exit 1)

ifeq (,$(filter build_kernel-nodeps,$(MAKECMDGOALS)))
$(KERNEL_BZIMAGE): $(HOST_OPENSSL) $(MINIGZIP)
endif

$(KERNEL_BZIMAGE): $(KERNEL_CONFIG)
	@$(KERNEL_BLD_ENV) $(MAKE) -C $(KERNEL_SRC_DIR) $(KERNEL_BLD_FLAGS)
	@cp -f $(KERNEL_OUT_DIR)/arch/x86/boot/bzImage $@

clean_kernel:
	@$(KERNEL_BLD_ENV) $(MAKE) -C $(KERNEL_SRC_DIR) $(KERNEL_BLD_FLAGS) clean

menuconfig xconfig gconfig: $(KERNEL_CONFIG)
	@$(KERNEL_BLD_ENV) $(MAKE) -C $(KERNEL_SRC_DIR) $(KERNEL_BLD_FLAGS) $@
ifeq ($(wildcard $(KERNEL_DIFFCONFIG)),)
	@cp -f $(KERNEL_CONFIG) $(KERNEL_DEFCONFIG)
	@echo ===========
	@echo $(KERNEL_DEFCONFIG) has been modified !
	@echo ===========
else
	@./$(KERNEL_SRC_DIR)/scripts/diffconfig -m $(KERNEL_DEFCONFIG) $(KERNEL_CONFIG) > $(KERNEL_DIFFCONFIG)
	@echo ===========
	@echo $(KERNEL_DIFFCONFIG) has been modified !
	@echo ===========
endif

TAGS_files := TAGS
tags_files := tags
gtags_files := GTAGS GPATH GRTAGS GSYMS
cscope_files := $(addprefix cscope.,files out out.in out.po)

TAGS tags gtags cscope: $(KERNEL_CONFIG)
	@$(KERNEL_BLD_ENV) $(MAKE) -C $(KERNEL_SRC_DIR) $(KERNEL_BLD_FLAGS) $@
	@rm -f $(KERNEL_SRC_DIR)/$($@_files)
	@cp -fs $(addprefix `pwd`/$(KERNEL_OUT_DIR)/,$($@_files)) $(KERNEL_SRC_DIR)/


define build_kernel_module
$(error Use of external Kernel modules is not allowed)
endef

.PHONY: menuconfig xconfig gconfig
.PHONY: $(KERNEL_BZIMAGE)
.PHONY: build_kernel build_kernel-nodeps

$(PRODUCT_OUT)/boot.img: build_kernel

endif #TARGET_KERNEL_BUILT_FROM_SOURCE
