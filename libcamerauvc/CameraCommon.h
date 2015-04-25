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

#ifndef ANDROID_LIBCAMERA_COMMON_H
#define ANDROID_LIBCAMERA_COMMON_H

#include <camera.h>
#include <linux/videodev2.h>
#include <cutils/atomic.h>
#include <stdio.h>
#include "LogHelper.h"


//This file define the general configuration for the camera driver

#define BPP 2 // bytes per pixel
#define MAX_PARAM_VALUE_LENGTH 32
#define MAX_BURST_BUFFERS 32

namespace android {

struct CameraBuffer;

enum BufferType {
    BUFFER_TYPE_PREVIEW = 0,
    BUFFER_TYPE_VIDEO,
    BUFFER_TYPE_SNAPSHOT,
    BUFFER_TYPE_THUMBNAIL,
    BUFFER_TYPE_INTERMEDIATE //used for intermediate conversion,
                             // no need to return to driver
};
class IBufferOwner
{
public:
    virtual void returnBuffer(CameraBuffer* buff1) = 0;
    virtual ~IBufferOwner(){};
};


class CameraDriver;
class ControlThread;
class CameraBuffer {
public:
    CameraBuffer() :
        mCamMem(0),
        mID(-1),
        mDriverPrivate(0),
        mOwner(0),
        mReaderCount(0),
        mType(BUFFER_TYPE_INTERMEDIATE),
        mFormat(0),
        mSize(-1)
    {}

    int getID() const
    {
        return mID;
    }

    void* getData()
    {
        if (mCamMem != 0)
            return mCamMem->data;
        else
            return 0;
    }

    void releaseMemory()
    {
        if (mCamMem != 0)
            mCamMem->release(mCamMem);
        mCamMem = NULL;
    }

    // TODO: encapsulate memory allocation and data access
    camera_memory_t* getCameraMem() const
    {
        return mCamMem;
    }

    void setCameraMemory(camera_memory_t* m)
    {
        if (mCamMem != 0)
            releaseMemory();
        mCamMem = m;
    }

    //readers should  decrement reader count
    // when buffer is no longer in use
    // buffer automatically returned to driver if
    // reader count goes to zero
    //!TODO rename this method to decrementProcessor
    void decrementReader()
    {
        android_atomic_dec(&mReaderCount);
        // if all decrements done and count is zero
        // return to driver
        int32_t rc = android_atomic_acquire_load(&mReaderCount);
        if(rc == 0)
            returnToOwner();
    }

    // Readers should increment reader count
    // as soon as it holds a reference before doing process.
    void incrementReader()
    {
        android_atomic_inc(&mReaderCount);
    }

    void setOwner(IBufferOwner* o)
    {
        if (mOwner == 0)
            mOwner = o;
        else
            ALOGE("taking ownership from previous owner is not allowed.");
    }

    void setFormat(int f)
    {
        mFormat = f;
    }

    int getFormat() const
    {
        return mFormat;
    }


private:
    //not allowed to pass buffer by value
    CameraBuffer(const CameraBuffer& other) :
        mCamMem(other.mCamMem),
        mID(other.mID),
        mDriverPrivate(other.mDriverPrivate),
        mOwner(other.mOwner),
        mReaderCount(other.mReaderCount),
        mType(other.mType),
        mFormat(other.mFormat),
        mSize(other.mSize)
    {
        ALOGW("CameraBuffers are not designed to pass by value.");
    }

    const CameraBuffer& operator=(const CameraBuffer& other)
    {
        ALOGW("CameraBuffers are not designed to pass by value.");
        if (this != &other) {
            this->mCamMem = other.mCamMem;
            this->mDriverPrivate = other.mDriverPrivate;
            this->mFormat = other.mFormat;
            this->mID = other.mID;
            this->mSize = other.mSize;
            this->mOwner = other.mOwner;
            this->mReaderCount = other.mReaderCount;
            this->mType = other.mType;
        }
        return *this;
    }

    void returnToOwner()
    {
        if (mOwner != 0)
            mOwner->returnBuffer(this);
    }

    camera_memory_t *mCamMem;
    int mID;                 // id for debugging data flow path
    int mDriverPrivate;      // Private to the CameraDriver class.
                            // No other classes should touch this
    IBufferOwner* mOwner;    // owner who is responsible to enqueue back
                            // to CameraDriver
    volatile int32_t mReaderCount;
    BufferType mType;
    int mFormat;
    int mSize;
    friend class CameraDriver;
    friend class ControlThread;
};

struct CameraWindow {
    int x_left;
    int x_right;
    int y_top;
    int y_bottom;
    int weight;
};

static int frameSize(int format, int width, int height)
{
    int size = 0;
    switch (format) {
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YVU420:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_YUV411P:
        case V4L2_PIX_FMT_YUV422P:
            size = (width * height * 3 / 2);
            break;
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_Y41P:
        case V4L2_PIX_FMT_UYVY:
            size = (width * height *  2);
            break;
        case V4L2_PIX_FMT_RGB565:
            size = (width * height * BPP);
            break;
        default:
            size = (width * height * 2);
    }

    return size;
}

static int paddingWidth(int format, int width, int height)
{
    int padding = 0;
    switch (format) {
    //64bit align for 1.5byte per pixel
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YVU420:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_YUV411P:
    case V4L2_PIX_FMT_YUV422P:
        padding = (width + 63) / 64 * 64;
        break;
    //32bit align for 2byte per pixel
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_Y41P:
    case V4L2_PIX_FMT_UYVY:
        padding = width;
        break;
    case V4L2_PIX_FMT_RGB565:
        padding = (width + 31) / 32 * 32;
        break;
    default:
        padding = (width + 63) / 64 * 64;
    }
    return padding;
}

static const char* v4l2Fmt2Str(int format)
{
    static char fourccBuf[5];
    memset(&fourccBuf[0], 0, sizeof(fourccBuf));
    char *fourccPtr = (char*) &format;
    snprintf(fourccBuf, sizeof(fourccBuf), "%c%c%c%c", *fourccPtr, *(fourccPtr+1), *(fourccPtr+2), *(fourccPtr+3));
    return &fourccBuf[0];
}

}
#endif // ANDROID_LIBCAMERA_COMMON_H
