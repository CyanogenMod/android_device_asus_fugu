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
LOCAL_KERNEL := device/asus/fugu-kernel/bzImage
else
LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

PRODUCT_COPY_FILES := \
    $(LOCAL_KERNEL):kernel

endif #TARGET_KERNEL_BUILT_FROM_SOURCE

DEVICE_PACKAGE_OVERLAYS := \
    device/asus/fugu/overlay

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
    persist.sys.usb.config=mtp

PRODUCT_COPY_FILES += \
    device/asus/fugu/fstab.fugu:root/fstab.fugu \
    device/asus/fugu/init.fugu.rc:root/init.fugu.rc \
    device/asus/fugu/init.fugu.usb.rc:root/init.fugu.usb.rc \
    device/asus/fugu/ueventd.fugu.rc:root/ueventd.fugu.rc

# ia_watchdog - temporary prebuilt
PRODUCT_COPY_FILES += \
    device/asus/fugu/ia_watchdogd:root/usr/bin/ia_watchdogd

# Use partlink block devices
PRODUCT_PACKAGES += \
    partlink

# Add kernel watchdog daemon
#PRODUCT_PACKAGES += \
#    ia_watchdogd

# Bluetooth
PRODUCT_PACKAGES += \
    bt_bcm4339 \
    bt_bcm43241 \
    bt_bcm4354

# IMG graphics
PRODUCT_PACKAGES += \
    IMG_graphics \
    hwcomposer.moorefield

#Video
PRODUCT_COPY_FILES += \
        device/asus/fugu/media_profiles.xml:system/etc/media_profiles.xml \
        device/asus/fugu/wrs_omxil_components.list:system/etc/wrs_omxil_components.list \
        device/asus/fugu/media_codecs.xml:system/etc/media_codecs.xml

# psb video
xxxPRODUCT_PACKAGES += \
    pvr_drv_video

#video firmware
PRODUCT_PACKAGES += \
    msvdx.bin.0008.0000.0000 \
    msvdx.bin.0008.0000.0001 \
    msvdx.bin.0008.0002.0001 \
    msvdx.bin.0008.0000.0002 \
    msvdx.bin.000c.0001.0001 \
    topaz.bin.0008.0000.0000 \
    topaz.bin.0008.0000.0001 \
    topaz.bin.0008.0000.0002 \
    topaz.bin.0008.0002.0001 \
    topaz.bin.000c.0001.0001 \
    vsp.bin.0008.0000.0000 \
    vsp.bin.0008.0000.0001 \
    vsp.bin.0008.0000.0002 \
    vsp.bin.0008.0002.0001 \
    vsp.bin.000c.0001.0001
# libva
PRODUCT_PACKAGES += \
    libva \
    libva-android \
    libva-tpi \
    vainfo

#libstagefrighthw
PRODUCT_PACKAGES += \
    libstagefrighthw

# libmix
xxxPRODUCT_PACKAGES += \
    libmixvbp_mpeg4 \
    libmixvbp_h264 \
    libmixvbp_h264secure \
    libmixvbp_vc1 \
    libmixvbp_vp8 \
    libmixvbp \
    libva_videodecoder \
    libva_videoencoder

xxxPRODUCT_PACKAGES += \
    libwrs_omxil_common \
    libwrs_omxil_core_pvwrapped \
    libOMXVideoDecoderAVC \
    libOMXVideoDecoderH263 \
    libOMXVideoDecoderMPEG4 \
    libOMXVideoDecoderWMV \
    libOMXVideoDecoderVP8 \
    libOMXVideoEncoderAVC \
    libOMXVideoEncoderH263 \
    libOMXVideoEncoderMPEG4 \
    libOMXVideoEncoderVP8


# pvr
PRODUCT_PACKAGES += \
    libpvr2d

# libdrm
PRODUCT_PACKAGES += \
    libdrm \
    dristat \
    drmstat

$(call inherit-product-if-exists, vendor/intel/fugu/device-vendor.mk)

#PRODUCT_CHARACTERISTICS := tablet
