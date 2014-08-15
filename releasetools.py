# Copyright (C) 2012 The Android Open Source Project
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

"""Emit extra commands needed for Group during OTA installation
(installing the bootloader)."""

import struct
import common


def WriteIfwi(info):
  info.script.AppendExtra('package_extract_file("ifwi.bin", "/tmp/ifwi.bin");')
  info.script.AppendExtra("""fugu.flash_ifwi("/tmp/ifwi.bin");""")

def WriteDroitboot(info):
  info.script.WriteRawImage("/fastboot", "droidboot.img")

def WriteBootloader(info, bootloader):
  header_fmt = "<8sHHII"
  header_size = struct.calcsize(header_fmt)
  magic, revision, reserved, ifwi_size, droidboot_size = struct.unpack(
    header_fmt, bootloader[:header_size])

  assert magic == "BOOTLDR!", "bootloader.img bad magic value"

  ifwi = bootloader[header_size:header_size+ifwi_size]
  droidboot = bootloader[header_size+ifwi_size:]

  common.ZipWriteStr(info.output_zip, "droidboot.img", droidboot)
  common.ZipWriteStr(info.output_zip, "ifwi.bin", ifwi)

  WriteIfwi(info)
  WriteDroitboot(info)

def FullOTA_InstallEnd(info):
  try:
    bootloader_img = info.input_zip.read("RADIO/bootloader.img")
  except KeyError:
    print "no bootloader.img in target_files; skipping install"
  else:
    WriteBootloader(info, bootloader_img)


def IncrementalOTA_InstallEnd(info):
  try:
    target_bootloader_img = info.target_zip.read("RADIO/bootloader.img")
    try:
      source_bootloader_img = info.source_zip.read("RADIO/bootloader.img")
    except KeyError:
      source_bootloader_img = None

    if source_bootloader_img == target_bootloader_img:
      print "bootloader unchanged; skipping"
    else:
      print "bootloader changed; adding it"
      WriteBootloader(info, target_bootloader_img)
  except KeyError:
    print "no bootloader.img in target target_files; skipping install"
