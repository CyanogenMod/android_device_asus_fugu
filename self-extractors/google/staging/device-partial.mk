# Copyright 2014 The Android Open Source Project
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

# Google blob(s) necessary for Fugu hardware
PRODUCT_COPY_FILES := \
    vendor/google/fugu/proprietary/libiperf.so:system/lib/libiperf.so:google \
    vendor/google/fugu/proprietary/ADT-1_Remote.kl:system/usr/keylayout/ADT-1_Remote.kl:google \

