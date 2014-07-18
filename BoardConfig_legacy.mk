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
ifneq (,$(wildcard build/core/combo/arch/x86/x86-slm.mk))
TARGET_ARCH_VARIANT := x86-slm
# All boot class paths for extensions
PRODUCT_BOOT_JARS := $(PRODUCT_BOOT_JARS):com.intel.multidisplay:com.intel.config:com.intel.cam.api:com.intel.nfc.adapteraddon
# WIDI
INTEL_WIDI := true
INTEL_WIDI_MERRIFIELD := true
TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS := true
endif

#Extend the AOSP path includes
$(call add-path-map, stlport:external/stlport/stlport \
        alsa-lib:external/alsa-lib/include \
        libxml2:external/libxml2/include \
        webcore-icu:external/webkit/Source/WebCore/icu \
        tinyalsa:external/tinyalsa/include \
        core-jni:frameworks/base/core/jni \
        vss:frameworks/av/libvideoeditor/vss/inc \
        vss-common:frameworks/av/libvideoeditor/vss/common/inc \
        vss-mcs:frameworks/av/libvideoeditor/vss/mcs/inc \
        vss-stagefrightshells:frameworks/av/libvideoeditor/vss/stagefrightshells/inc \
        lvpp:frameworks/av/libvideoeditor/lvpp \
        osal:frameworks/av/libvideoeditor/osal/inc \
        frameworks-base-core:frameworks/base/core/jni \
        frameworks-av:frameworks/av/include \
        frameworks-openmax:frameworks/native/include/media/openmax \
        jpeg:external/jpeg \
        skia:external/skia/include \
        sqlite:external/sqlite/dist \
        opencv-cv:external/opencv/cv/include \
        opencv-cxcore:external/opencv/cxcore/include \
        opencv-ml:external/opencv/ml/include \
        libstagefright:frameworks/av/media/libstagefright/include \
        libstagefright-rtsp:frameworks/av/media/libstagefright/rtsp \
        libmediaplayerservice:frameworks/av/media/libmediaplayerservice \
        gtest:external/gtest/include \
        frameworks-base-libs:frameworks/base/libs \
        frameworks-av-services:frameworks/av/services \
        tinycompress:external/tinycompress/include \
        libnfc-nci:external/libnfc-nci/src/include \
        libnfc-nci_hal:external/libnfc-nci/src/hal/include \
        libnfc-nci_nfc:external/libnfc-nci/src/nfc/include \
        libnfc-nci_nfa:external/libnfc-nci/src/nfa/include \
        libnfc-nci_gki:external/libnfc-nci/src/gki \
        libc-private:bionic/libc/private \
        icu4c-common:external/icu4c/common \
        expat-lib:external/expat/lib \
        libvpx:external/libvpx \
        protobuf:external/protobuf/src \
        zlib:external/zlib \
        openssl:external/openssl/include \
        libnl-headers:external/libnl-headers \
        system-security:system/security/keystore/include/keystore \
        libpcap:external/libpcap \
        libsensorhub:vendor/intel/hardware/libsensorhub/src/include \
        icu4c-i18n:external/icu4c/i18n \
        bt-bluez:system/bluetooth/bluez-clean-headers \
        astl:external/astl/include \
        libusb:external/libusb/libusb \
        libc-kernel:bionic/libc/kernel \
        libc-x86:bionic/libc/arch-x86/include \
        strace:external/strace \
        bionic:bionic \
        opengl:frameworks/native/opengl/include \
        libstagefright-wifi-display:frameworks/av/media/libstagefright/wifi-display \
        libffi:external/libffi/include \
        libffi-x86:external/libffi/linux-x86)


# Needed to build PCG JIT compiler for dalvik.
USE_INTEL_IPP := true

INTEL_VA:=true

#Power_HAL
POWERHAL_MRFLD := true

#BCU HAL
BCUHAL_MRFLD := true

# WIFI
BOARD_HAVE_WIFI := true

BOARD_USES_48000_AUDIO_CAPTURE_SAMPLERATE_FOR_WIDI:= true
#Support background music playback for Widi Multitasking
ENABLE_BACKGROUND_MUSIC := true

# Sign bootimage
MKBOOTIMG := device/asus/fugu/legacy_sign
BOARD_CUSTOM_MKBOOTIMG := $(MKBOOTIMG)

# Kernel cmdline
BOARD_KERNEL_CMDLINE := init=/init pci=noearly vmalloc=256M ptrace.ptrace_can_access=1
BOARD_KERNEL_CMDLINE += console=logk0 console=ttyS0 earlyprintk=nologger loglevel=8
BOARD_KERNEL_CMDLINE += androidboot.hardware=fugu androidboot.serialno=01234567890123456789
BOARD_KERNEL_CMDLINE += snd_pcm.maximum_substreams=8
BOARD_KERNEL_CMDLINE += ip=50.0.0.2:50.0.0.1::255.255.255.0::usb0:on
BOARD_KERNEL_CMDLINE += allow_factory=1 selinux=0

# external release
PRODUCT_OUT := out/target/product/$(TARGET_DEVICE)
HOST_OUT := out/host/linux-x86
-include device/intel/common/external/external.mk
