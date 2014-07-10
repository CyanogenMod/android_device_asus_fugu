#
# Copyright 2013 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

KERNEL_SRC_DIR ?= linux/kernel-fugu
KERNEL_CFG_NAME ?= fugu
TARGET_KERNEL_ARCH ?= x86_64

# Check for availability of kernel source
ifneq ($(wildcard $(KERNEL_SRC_DIR)/Makefile),)
  # Give precedence to TARGET_PREBUILT_KERNEL
  ifeq ($(TARGET_PREBUILT_KERNEL),)
    TARGET_KERNEL_BUILT_FROM_SOURCE := true
  endif
endif

ifneq ($(TARGET_KERNEL_BUILT_FROM_SOURCE), true)
# Use prebuilt kernel
ifeq ($(TARGET_PREBUILT_KERNEL),)
LOCAL_KERNEL := device/asus/fugu-kernel/kernel
else
LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

PRODUCT_COPY_FILES := \
	$(LOCAL_KERNEL):kernel

endif #TARGET_KERNEL_BUILT_FROM_SOURCE


PRODUCT_COPY_FILES := \
    device/asus/fugu/fstab.fugu:root/fstab.fugu \
    device/asus/fugu/init.fugu.rc:root/init.fugu.rc \
    device/asus/fugu/init.fugu.usb.rc:root/init.fugu.usb.rc \
    device/asus/fugu/ueventd.fugu.rc:root/ueventd.fugu.rc

# Use partlink block devices
PRODUCT_PACKAGES += \
    partlink

# Add kernel watchdog daemon
PRODUCT_PACKAGES += \
    ia_watchdogd

# IMG graphics
PRODUCT_PACKAGES += \
    IMG_graphics \
    hwcomposer.moorefield

ifneq (,$(wildcard build/core/combo/arch/x86/x86-slm.mk))
# psb video
PRODUCT_PACKAGES += \
    pvr_drv_video \
    msvdx_bin \
    topaz_bin
endif

# pvr
PRODUCT_PACKAGES += \
    libpvr2d

# libdrm
PRODUCT_PACKAGES += \
    libdrm \
    dristat \
    drmstat

$(call inherit-product-if-exists, vendor/intel/fugu/device-vendor.mk)
$(call inherit-product, device/asus/fugu/device_legacy.mk)

PRODUCT_CHARACTERISTICS := tablet
