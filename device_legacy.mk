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

# These should be in vendor/intel/fugu/device-vendor.mk
# These must be enabled to avoid duplicate module android_webview_java
-include vendor/google/PRIVATE/gms/products/gms.mk
-include vendor/google/PRIVATE/gms/products/gms_optional.mk

#--------------------------------------------------------------------------------------------------#
# Add packages required by modifications in AOSP
#--------------------------------------------------------------------------------------------------#

# This library is required for Intel's implementation of Dalvik
# libpcgdvmjit is a part of Dalvik JIT compiler
PRODUCT_PACKAGES += libpcgdvmjit

# This library is required for Intel's implementation of Dalvik
# libcrash is a library which provides recorded state of an applications
# which crashed while running on Dalvik VM
PRODUCT_PACKAGES += libcrash

#Houdini prebuilt
HOUDINI_ARM_PREBUILTS_DIR := vendor/intel/houdini/arm
houdini_prebuilt_stamp := $(HOUDINI_ARM_PREBUILTS_DIR)/stamp-prebuilt-done
houdini_prebuilt_done := $(wildcard $(houdini_prebuilt_stamp))
ifneq ($(houdini_prebuilt_done),)
INTEL_HOUDINI := true
#Houdini
PRODUCT_PACKAGES += libhoudini \
    houdini \
    enable_houdini \
    disable_houdini \
    check.xml \
    cpuinfo \
    cpuinfo.neon

#houdini arm libraries
-include vendor/intel/houdini/houdini.mk
endif

# CAM
PRODUCT_PACKAGES += \
    cam_mandatory
