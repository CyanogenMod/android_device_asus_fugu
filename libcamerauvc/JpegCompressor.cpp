/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "Camera_JpegCompressor"

#define JPEG_BLOCK_SIZE 4096

#include "JpegCompressor.h"
#include "ColorConverter.h"
#include "LogHelper.h"
#include "SkBitmap.h"
#include "SkStream.h"
#include <string.h>

extern "C" {
    #include "jpeglib.h"
}

namespace android {
/*
 * START: jpeglib interface functions
 */

// jpeg destination manager structure
struct JpegDestinationManager {
    struct jpeg_destination_mgr pub; // public fields
    JSAMPLE *encodeBlock;            // encode block buffer
    JSAMPLE *outJpegBuf;             // JPEG output buffer
    JSAMPLE *outJpegBufPos;          // JPEG output buffer current ptr
    int outJpegBufSize;              // JPEG output buffer size
    int *dataCount;                  // JPEG output buffer data written count
};

// initialize the jpeg compression destination buffer (passed to libjpeg as function pointer)
static void init_destination(j_compress_ptr cinfo)
{
    LOG1("@%s", __FUNCTION__);
    JpegDestinationManager *dest = (JpegDestinationManager*) cinfo->dest;
    dest->encodeBlock = (JSAMPLE *)(*cinfo->mem->alloc_small) \
            ((j_common_ptr) cinfo, JPOOL_IMAGE, JPEG_BLOCK_SIZE * sizeof(JSAMPLE));
    dest->pub.next_output_byte = dest->encodeBlock;
    dest->pub.free_in_buffer = JPEG_BLOCK_SIZE;
}

// handle the jpeg output buffers (passed to libjpeg as function pointer)
static boolean empty_output_buffer(j_compress_ptr cinfo)
{
    LOG2("@%s", __FUNCTION__);
    JpegDestinationManager *dest = (JpegDestinationManager*) cinfo->dest;
    if(dest->outJpegBufSize < *(dest->dataCount) + JPEG_BLOCK_SIZE)
    {
        ALOGE("JPEGLIB: empty_output_buffer overflow!");
        *(dest->dataCount) = 0;
        return FALSE;
    }
    memcpy(dest->outJpegBufPos, dest->encodeBlock, JPEG_BLOCK_SIZE);
    dest->outJpegBufPos += JPEG_BLOCK_SIZE;
    *(dest->dataCount) += JPEG_BLOCK_SIZE;
    dest->pub.next_output_byte = dest->encodeBlock;
    dest->pub.free_in_buffer = JPEG_BLOCK_SIZE;
    return TRUE;
}

// terminate the compression destination buffer (passed to libjpeg as function pointer)
static void term_destination(j_compress_ptr cinfo)
{
    LOG1("@%s", __FUNCTION__);
    JpegDestinationManager *dest = (JpegDestinationManager*) cinfo->dest;
    int dataCount = JPEG_BLOCK_SIZE - dest->pub.free_in_buffer;
    if(dest->outJpegBufSize < dataCount || dataCount < 0)
    {
        if (dataCount < 0)
            ALOGE("jpeg overrun. this should not happen");

        *(dest->dataCount) = 0;
        return;
    }
    memcpy(dest->outJpegBufPos, dest->encodeBlock, dataCount);
    dest->outJpegBufPos += dataCount;
    *(dest->dataCount) += dataCount;
}

// setup the destination manager in j_compress_ptr handle
static int setup_jpeg_destmgr(j_compress_ptr cinfo, JSAMPLE *outBuf, int jpegBufSize, int *jpegSizePtr)
{
    LOG1("@%s", __FUNCTION__);
    JpegDestinationManager *dest;

    if(outBuf == NULL || jpegBufSize <= 0 )
        return -1;

    LOG1("Setting up JPEG destination manager...");
    dest = (JpegDestinationManager*) cinfo->dest;
    if (cinfo->dest == NULL) {
        LOG1("Create destination manager...");
        cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(JpegDestinationManager));
        dest = (JpegDestinationManager*) cinfo->dest;
        dest->pub.init_destination = init_destination;
        dest->pub.empty_output_buffer = empty_output_buffer;
        dest->pub.term_destination = term_destination;
        dest->outJpegBuf = outBuf;
    }
    LOG1("Out: bufPos = %p, bufSize = %d, dataCount = %d", outBuf, jpegBufSize, *jpegSizePtr);
    dest->outJpegBufSize = jpegBufSize;
    dest->outJpegBufPos = outBuf;
    dest->dataCount = jpegSizePtr;
    return 0;
}

/*
 * END: jpeglib interface functions
 */

JpegCompressor::JpegCompressor() :
    mVaInputSurfacesNum(0)
    ,mVaSurfaceWidth(0)
    ,mVaSurfaceHeight(0)
    ,mJpegCompressStruct(NULL)
    ,mStartSharedBuffersEncode(false)
#ifndef ANDROID_1998
    ,mStartCompressDone(false)
#endif
{
    LOG1("@%s", __FUNCTION__);
    mJpegEncoder = SkImageEncoder::Create(SkImageEncoder::kJPEG_Type);
    if (mJpegEncoder == NULL) {
        ALOGE("No memory for Skia JPEG encoder!");
    }
    memset(mVaInputSurfacesPtr, 0, sizeof(mVaInputSurfacesPtr));
    mJpegSize = -1;
}

JpegCompressor::~JpegCompressor()
{
    LOG1("@%s", __FUNCTION__);
    if (mJpegEncoder != NULL) {
        LOG1("Deleting Skia JPEG encoder...");
        delete mJpegEncoder;
    }
}

bool JpegCompressor::convertRawImage(void* src, void* dst, int width, int height, int format)
{
    LOG1("@%s", __FUNCTION__);
    return colorConvert(format, V4L2_PIX_FMT_RGB565, width, height, src, dst) == NO_ERROR;
}

// Takes YUV data (NV12 or YUV420) and outputs JPEG encoded stream
int JpegCompressor::encode(const InputBuffer &in, const OutputBuffer &out)
{
    LOG1("@%s:\n\t IN  = {buf:%p, w:%u, h:%u, sz:%u, f:%s}" \
             "\n\t OUT = {buf:%p, w:%u, h:%u, sz:%u, q:%d}",
            __FUNCTION__,
            in.buf, in.width, in.height, in.size, v4l2Fmt2Str(in.format),
            out.buf, out.width, out.height, out.size, out.quality);
    // For SW path
    SkBitmap skBitmap;
    SkDynamicMemoryWStream skStream;
    // For HW path
    struct jpeg_compress_struct cinfo;
    struct jpeg_compress_struct* pCinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];

    if (in.width == 0 || in.height == 0 || in.format == 0) {
        ALOGE("Invalid input received!");
        mJpegSize = -1;
        goto exit;
    }
    {
        // Choose Skia
        LOG1("Choosing Skia for JPEG encoding");
        if (mJpegEncoder == NULL) {
            ALOGE("Skia JpegEncoder not created, cannot encode to JPEG!");
            mJpegSize = -1;
            goto exit;
        }
        bool success = convertRawImage((void*)in.buf, (void*)out.buf, in.width, in.height, in.format);
        if (!success) {
            ALOGE("Could not convert the raw image!");
            mJpegSize = -1;
            goto exit;
        }
       SkColorType ct = SkBitmapConfigToColorType(SkBitmap::kRGB_565_Config);
       SkImageInfo info = SkImageInfo::Make(in.width, in.height, ct, kPremul_SkAlphaType);
       skBitmap.installPixels(info, out.buf, 0);
       // skBitmap.setConfig(SkBitmap::kRGB_565_Config, in.width, in.height);
      //  skBitmap.setPixels(out.buf, NULL);
        LOG1("Encoding stream using Skia...");
        if (mJpegEncoder->encodeStream(&skStream, skBitmap, out.quality)) {
            mJpegSize = skStream.getOffset();
            skStream.copyTo(out.buf);
        } else {
            ALOGE("Skia could not encode the stream!");
            mJpegSize = -1;
            goto exit;
        }
    }
exit:
    return mJpegSize;
}

}
