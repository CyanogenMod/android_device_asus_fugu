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
#define LOG_TAG "Camera_ColorConverter"

#include <camera/CameraParameters.h>
#include <linux/videodev2.h>
#include "ColorConverter.h"
#include "LogHelper.h"

namespace android {

inline unsigned char clamp(int x){
    if (x < 0 )
        x = 0;
    else if (x > 255)
        x = 255;
    return (x & 0xFF);
}

void YUYVToNV21(int width, int height, void *src, void *dst)
{
    unsigned char *pSrcY = (unsigned char *) src;
    unsigned char *pSrcU = pSrcY + 1;
    unsigned char *pSrcV = pSrcY + 3;

    unsigned char *pDstY = (unsigned char *) dst;
    unsigned char *pDstUV = pDstY + width * height;

    // YUYV format is: yuyvyuyvyuyv...yuyv
    // NV12 format is: yyyy...yyyyuvuv...uvuvuv
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 2; j++) { // 2 y-pixels at a time
            *pDstY++ = *pSrcY;
            pSrcY += 2;
            *pDstY++ = *pSrcY;
            pSrcY += 2;

            // 4:2:2 chroma has 1/2 the horizontal and FULL vertical resolution of full image
            // 4:2:0 chroma has 1/2 the horizontal and 1/2 vertical resolution of full image
            // so skip odd numbered rows
            if ((i % 2) == 0) {
                *pDstUV++ = *pSrcV;
                *pDstUV++ = *pSrcU;
            }
            pSrcU += 4;
            pSrcV += 4;
        }
    }
}

void YUYVToNV12(int width, int height, void *src, void *dst)
{
    unsigned char *pSrcY = (unsigned char *) src;
    unsigned char *pSrcU = pSrcY + 1;
    unsigned char *pSrcV = pSrcY + 3;

    unsigned char *pDstY = (unsigned char *) dst;
    unsigned char *pDstUV = pDstY + width * height;

    // YUYV format is: yuyvyuyvyuyv...yuyv
    // NV12 format is: yyyy...yyyyuvuv...uvuvuv
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width / 2; j++) { // 2 y-pixels at a time
            *pDstY++ = *pSrcY;
            pSrcY += 2;
            *pDstY++ = *pSrcY;
            pSrcY += 2;

            // 4:2:2 chroma has 1/2 the horizontal and FULL vertical resolution of full image
            // 4:2:0 chroma has 1/2 the horizontal and 1/2 vertical resolution of full image
            // so skip odd numbered rows
            if ((i % 2) == 0) {
                *pDstUV++ = *pSrcU;
                *pDstUV++ = *pSrcV;
            }
            pSrcU += 4;
            pSrcV += 4;
        }
    }
}

void YUYVToRGB8888(int width, int height, void *src, void *dst)
{
    int len = width * height * 2;
    int i = 0;
    unsigned char *pYUV = (unsigned char *)src;   //four bytes of two pixels
    unsigned char *pRGB = (unsigned char *)dst;       //6 rgb bytes for two pixels
    int C, D, E;
    len -= len % 4;
    for (i = 0; i < len; i += 4) {
        unsigned char y1 = *(pYUV++);
        unsigned char u = *(pYUV++);
        unsigned char y2 = *(pYUV++);
        unsigned char v = *(pYUV++);
//calculate 1st pixel
        C = y1 - 16;
        D = u - 128;
        E = v -128;
        *(pRGB++) = clamp((C * 298 + E * 409 + 128) >> 8);
        *(pRGB++) = clamp((C * 298 - D * 100 - E * 208 + 128) >> 8);
        *(pRGB++) = clamp((C * 298 + D * 516 + 128) >> 8);
        //alpha
        *(pRGB++) = 0xFF;
//calculate 2nd pixel
        C = y2 -16;
        *(pRGB++) = clamp((C * 298 + E * 409 + 128) >> 8);
        *(pRGB++) = clamp((C * 298 - D * 100 - E * 208 + 128) >> 8);
        *(pRGB++) = clamp((C * 298 + D * 516 + 128) >> 8);
        //alpha
        *(pRGB++) = 0xFF;

    }
}

void YUYVToRGB565(int width, int height, void *src, void *dst)
{

    unsigned char *yuvs = (unsigned char *) src;
    unsigned char *rgbs = (unsigned char *) dst;

    //points to the next luminance value pair
    int lumPtr = 0;
    //points to the next chromiance value pair
    int chrPtr = 1;
    //points to the next byte output pair of RGB565 value
    int outPtr = 0;

    while (true) {

        if (lumPtr == width * height * 2) // our work is done here!
            break;

        //read the luminance
        int Y1 = yuvs[lumPtr] & 0xff;
        lumPtr += 2;
        int Y2 = yuvs[lumPtr] & 0xff;
        lumPtr += 2;

        //read the chroma
        int Cb = (yuvs[chrPtr] & 0xff) - 128;
        chrPtr += 2;
        int Cr = (yuvs[chrPtr] & 0xff) - 128;
        chrPtr += 2;
        int R, G, B;

        //generate first RGB components
        B = clamp(Y1 + ((454 * Cb) >> 8));
        G = clamp(Y1 - ((88 * Cb + 183 * Cr) >> 8));
        R = clamp(Y1 + ((359 * Cr) >> 8));
        //NOTE: this assume little-endian encoding
        rgbs[outPtr++]  = (unsigned char) (((G & 0x3c) << 3) | (B >> 3));
        rgbs[outPtr++]  = (unsigned char) ((R & 0xf8) | (G >> 5));

        //generate second RGB components
        B = clamp(Y2 + ((454 * Cb) >> 8));
        G = clamp(Y2 - ((88 * Cb + 183 * Cr) >> 8));
        R = clamp(Y2 + ((359 * Cr) >> 8));
        //NOTE: this assume little-endian encoding
        rgbs[outPtr++]  = (unsigned char) (((G & 0x3c) << 3) | (B >> 3));
        rgbs[outPtr++]  = (unsigned char) ((R & 0xf8) | (G >> 5));
    }
}

void NV12ToRGB565(int width, int height, void *src, void *dst)
{

    unsigned char *yuvs = (unsigned char *) src;
    unsigned char *rgbs = (unsigned char *) dst;

    //the end of the luminance data
    int lumEnd = width * height;
    //points to the next luminance value pair
    int lumPtr = 0;
    //points to the next chromiance value pair
    int chrPtr = lumEnd;
    //points to the next byte output pair of RGB565 value
    int outPtr = 0;
    //the end of the current luminance scanline
    int lineEnd = width;

    while (true) {
        //skip back to the start of the chromiance values when necessary
        if (lumPtr == lineEnd) {
            if (lumPtr == lumEnd) break; //we've reached the end
            //division here is a bit expensive, but's only done once per scanline
            chrPtr = lumEnd + ((lumPtr  >> 1) / width) * width;
            lineEnd += width;
        }
        //read the luminance and chromiance values
        int Y1 = yuvs[lumPtr++] & 0xff;
        int Y2 = yuvs[lumPtr++] & 0xff;
        int Cb = (yuvs[chrPtr++] & 0xff) - 128;
        int Cr = (yuvs[chrPtr++] & 0xff) - 128;
        int R, G, B;

        //generate first RGB components
        B = Y1 + ((454 * Cb) >> 8);
        if(B < 0) B = 0; else if(B > 255) B = 255;
        G = Y1 - ((88 * Cb + 183 * Cr) >> 8);
        if(G < 0) G = 0; else if(G > 255) G = 255;
        R = Y1 + ((359 * Cr) >> 8);
        if(R < 0) R = 0; else if(R > 255) R = 255;
        //NOTE: this assume little-endian encoding
        rgbs[outPtr++]  = (unsigned char) (((G & 0x3c) << 3) | (B >> 3));
        rgbs[outPtr++]  = (unsigned char) ((R & 0xf8) | (G >> 5));

        //generate second RGB components
        B = Y2 + ((454 * Cb) >> 8);
        if(B < 0) B = 0; else if(B > 255) B = 255;
        G = Y2 - ((88 * Cb + 183 * Cr) >> 8);
        if(G < 0) G = 0; else if(G > 255) G = 255;
        R = Y2 + ((359 * Cr) >> 8);
        if(R < 0) R = 0; else if(R > 255) R = 255;
        //NOTE: this assume little-endian encoding
        rgbs[outPtr++]  = (unsigned char) (((G & 0x3c) << 3) | (B >> 3));
        rgbs[outPtr++]  = (unsigned char) ((R & 0xf8) | (G >> 5));
    }
}

// covert NV12 (Y plane, interlaced UV bytes) to
// NV21 (Y plane, interlaced VU bytes)
void NV12ToNV21(int width, int height, void *src, void *dst)
{
    int planeSizeY = width * height;
    int planeSizeUV = planeSizeY / 2;
    int i = 0;
    unsigned char *srcPtr = (unsigned char *) src;
    unsigned char *dstPtr = (unsigned char *) dst;

    // copy the entire Y plane
    memcpy(dstPtr, src, planeSizeY);

    // byte swap the UV data
    for(i=planeSizeY; i<(planeSizeY+planeSizeUV); i=i+2)
    {
        dstPtr[i] = srcPtr[i + 1];
        dstPtr[i + 1] = srcPtr[i];
    }
}

// covert NV12 (Y plane, interlaced UV bytes) to
// YV12 (Y plane, V plane, U plane)
void NV12ToYV12(int width, int height, void *src, void *dst)
{
    int planeSizeY = width * height;
    int planeSizeUV = planeSizeY / 2;
    int planeUOffset = planeSizeUV / 2;
    int i = 0;
    unsigned char *srcPtr = (unsigned char *) src;
    unsigned char *dstPtr = (unsigned char *) dst;
    unsigned char *dstPtrV = (unsigned char *) dst + planeSizeY;
    unsigned char *dstPtrU = (unsigned char *) dst + planeSizeY + planeUOffset;

    // copy the entire Y plane
    memcpy(dstPtr, src, planeSizeY);

    // deinterlace the UV data
    for(i=planeSizeY; i<(planeSizeY+planeSizeUV); i=i+2)
    {
        *dstPtrV++ = srcPtr[i + 1];
        *dstPtrU++ = srcPtr[i];
    }
}

static status_t colorConvertYUYV(int dstFormat, int width, int height, void *src, void *dst)
{
    switch (dstFormat) {
    case V4L2_PIX_FMT_NV12:
        YUYVToNV12(width, height, src, dst);
        break;
    case V4L2_PIX_FMT_NV21:
        YUYVToNV21(width, height, src, dst);
        break;
    case V4L2_PIX_FMT_RGB565:
        YUYVToRGB565(width, height, src, dst);
        break;
    case V4L2_PIX_FMT_RGB32:
        YUYVToRGB8888(width, height, src, dst);
        break;
    default:
        ALOGE("Invalid color format (dest)");
        return BAD_VALUE;
    };

    return NO_ERROR;
}

static status_t colorConvertNV12(int dstFormat, int width, int height, void *src, void *dst)
{
    switch (dstFormat) {
    case V4L2_PIX_FMT_NV21:
        NV12ToNV21(width, height, src, dst);
        break;
    case V4L2_PIX_FMT_YUV420:
        NV12ToYV12(width, height, src, dst);
        break;
    case V4L2_PIX_FMT_RGB565:
        NV12ToRGB565(width, height, src, dst);
        break;
    default:
        ALOGE("Invalid color format (dest)");
        return BAD_VALUE;
    };

    return NO_ERROR;
}

status_t colorConvert(int srcFormat, int dstFormat, int width, int height, void *src, void *dst)
{
    if (srcFormat == dstFormat) {
        ALOGE("src format is the same as dst format");
        return BAD_VALUE;
    }

    switch (srcFormat) {
    case V4L2_PIX_FMT_YUYV:
        return colorConvertYUYV(dstFormat, width, height, src, dst);
    case V4L2_PIX_FMT_NV12:
        return colorConvertNV12(dstFormat, width, height, src, dst);
    default:
        ALOGE("invalid (source) color format");
        return BAD_VALUE;
    };
}

const char *cameraParametersFormat(int v4l2Format)
{
    switch (v4l2Format) {
    case V4L2_PIX_FMT_YUV420:
        return CameraParameters::PIXEL_FORMAT_YUV420P;
    case V4L2_PIX_FMT_NV21:
        return CameraParameters::PIXEL_FORMAT_YUV420SP;
    case V4L2_PIX_FMT_YUYV:
        return CameraParameters::PIXEL_FORMAT_YUV422I;
    case V4L2_PIX_FMT_JPEG:
        return CameraParameters::PIXEL_FORMAT_JPEG;
    default:
        ALOGE("failed to map format %x to a PIXEL_FORMAT\n", v4l2Format);
        return NULL;
    };
}

int V4L2Format(const char *cameraParamsFormat)
{
    LOG1("@%s", __FUNCTION__);
    if (!cameraParamsFormat) {
        ALOGE("null cameraParamsFormat");
        return -1;
    }

    int len = strlen(CameraParameters::PIXEL_FORMAT_YUV420SP);
    if (strncmp(cameraParamsFormat, CameraParameters::PIXEL_FORMAT_YUV420SP, len) == 0)
        return V4L2_PIX_FMT_NV21;

    len = strlen(CameraParameters::PIXEL_FORMAT_YUV420P);
    if (strncmp(cameraParamsFormat, CameraParameters::PIXEL_FORMAT_YUV420P, len) == 0)
        return V4L2_PIX_FMT_YUV420;

    len = strlen(CameraParameters::PIXEL_FORMAT_JPEG);
    if (strncmp(cameraParamsFormat, CameraParameters::PIXEL_FORMAT_JPEG, len) == 0)
        return V4L2_PIX_FMT_JPEG;

    len = strlen(CameraParameters::PIXEL_FORMAT_YUV422I);
    if (strncmp(cameraParamsFormat, CameraParameters::PIXEL_FORMAT_YUV422I, len) == 0)
        return V4L2_PIX_FMT_YUYV;

    ALOGE("invalid format %s", cameraParamsFormat);
    return -1;
}

} // namespace android
