
PRODUCT_NAME := kiwi

KERNEL_SRC_DIR := linux/kernel-fugu

ifeq (,$(wildcard build/core/combo/arch/x86/x86-slm.mk))
override TARGET_ARCH_VARIANT := silvermont
endif

LOCAL_PATH := device/intel/moorefield/mofd_v0
include device/intel/moorefield/mofd_v0/mofd_v0_64.mk
