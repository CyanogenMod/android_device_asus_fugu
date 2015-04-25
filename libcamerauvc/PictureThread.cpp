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
#define LOG_TAG "Camera_PictureThread"

#include "PictureThread.h"
#include "LogHelper.h"
#include "Callbacks.h"
#include "ColorConverter.h"
#include <utils/Timers.h>

namespace android {

static const int MAX_EXIF_SIZE = 0xFFFF;
static const unsigned char JPEG_MARKER_SOI[2] = {0xFF, 0xD8}; // JPEG StartOfImage marker
static const unsigned char JPEG_MARKER_EOI[2] = {0xFF, 0xD9}; // JPEG EndOfImage marker

PictureThread::PictureThread() :
    Thread(true) // callbacks may call into java
    ,mMessageQueue("PictureThread", MESSAGE_ID_MAX)
    ,mThreadRunning(false)
    ,mCallbacks(Callbacks::getInstance())
    ,mOutData(NULL)
    ,mExifBuf(NULL)
{
    LOG1("@%s", __FUNCTION__);
}

PictureThread::~PictureThread()
{
    LOG1("@%s", __FUNCTION__);
    if (mOutData != NULL) {
        delete[] mOutData;
    }
    if (mExifBuf != NULL) {
        delete[] mExifBuf;
    }
}

/*
 * encodeToJpeg: encodes the given buffer and creates the final JPEG file
 * Input:  mainBuf  - buffer containing the main picture image
 *         thumbBuf - buffer containing the thumbnail image (optional, can be NULL)
 * Output: destBuf  - buffer containing the final JPEG image including EXIF header
 *         Note that, if present, thumbBuf will be included in EXIF header
 */
status_t PictureThread::encodeToJpeg(CameraBuffer *mainBuf, CameraBuffer *thumbBuf, CameraBuffer *destBuf)
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    nsecs_t startTime = systemTime();
    nsecs_t endTime;

    // Convert and encode the thumbnail, if present and EXIF maker is initialized

    if (mConfig.exif.enableThumb) {

        LOG1("Encoding thumbnail");

        // setup the JpegCompressor input and output buffers
        mEncoderInBuf.clear();
        mEncoderInBuf.buf = (unsigned char*)thumbBuf->getData();
        mEncoderInBuf.width = mConfig.thumbnail.width;
        mEncoderInBuf.height = mConfig.thumbnail.height;
        mEncoderInBuf.format = mConfig.thumbnail.format;
        mEncoderInBuf.size = frameSize(mConfig.thumbnail.format,
                mConfig.thumbnail.width,
                mConfig.thumbnail.height);
        mEncoderOutBuf.clear();
        mEncoderOutBuf.buf = mOutData;
        mEncoderOutBuf.width = mConfig.thumbnail.width;
        mEncoderOutBuf.height = mConfig.thumbnail.height;
        mEncoderOutBuf.quality = mConfig.thumbnail.quality;
        mEncoderOutBuf.size = mMaxOutDataSize;
        endTime = systemTime();
        int size = compressor.encode(mEncoderInBuf, mEncoderOutBuf);
        LOG1("Thumbnail JPEG size: %d (time to encode: %ums)", size, (unsigned)((systemTime() - endTime) / 1000000));
        if (size > 0) {
            encoder.setThumbData(mEncoderOutBuf.buf, size);
        } else {
            // This is not critical, we can continue with main picture image
            ALOGE("Could not encode thumbnail stream!");
        }
    } else {
        LOG1("Skipping thumbnail");
    }
    int totalSize = 0;
    unsigned int exifSize = 0;
    // Copy the SOI marker
    unsigned char* currentPtr = mExifBuf;
    memcpy(currentPtr, JPEG_MARKER_SOI, sizeof(JPEG_MARKER_SOI));
    totalSize += sizeof(JPEG_MARKER_SOI);
    currentPtr += sizeof(JPEG_MARKER_SOI);
    if (encoder.makeExif(currentPtr, &mConfig.exif, &exifSize, false) != JPG_SUCCESS)
        ALOGE("Error making EXIF");
    currentPtr += exifSize;
    totalSize += exifSize;
    // Copy the EOI marker
    memcpy(currentPtr, (void*)JPEG_MARKER_EOI, sizeof(JPEG_MARKER_EOI));
    totalSize += sizeof(JPEG_MARKER_EOI);
    currentPtr += sizeof(JPEG_MARKER_EOI);
    exifSize = totalSize;

    // Convert and encode the main picture image
    // setup the JpegCompressor input and output buffers
    mEncoderInBuf.clear();
    mEncoderInBuf.buf = (unsigned char *) mainBuf->getData();

    mEncoderInBuf.width = mConfig.picture.width;
    mEncoderInBuf.height = mConfig.picture.height;
    mEncoderInBuf.format = mConfig.picture.format;
    mEncoderInBuf.size = frameSize(mConfig.picture.format,
            mConfig.picture.width,
            mConfig.picture.height);
    mEncoderOutBuf.clear();
    mEncoderOutBuf.buf = (unsigned char*)mOutData;
    mEncoderOutBuf.width = mConfig.picture.width;
    mEncoderOutBuf.height = mConfig.picture.height;
    mEncoderOutBuf.quality = mConfig.picture.quality;
    mEncoderOutBuf.size = mMaxOutDataSize;
    endTime = systemTime();
    int mainSize = compressor.encode(mEncoderInBuf, mEncoderOutBuf);
    LOG1("Picture JPEG size: %d (time to encode: %ums)", mainSize, (unsigned)((systemTime() - endTime) / 1000000));
    if (mainSize > 0) {
        // We will skip SOI marker from final file
        totalSize += (mainSize - sizeof(JPEG_MARKER_SOI));
    } else {
        ALOGE("Could not encode picture stream!");
        status = UNKNOWN_ERROR;
    }

    if (status == NO_ERROR) {
        mCallbacks->allocateMemory(destBuf, totalSize);
        if (destBuf->getData() == NULL) {
            ALOGE("No memory for final JPEG file!");
            status = NO_MEMORY;
        }
    }
    if (status == NO_ERROR) {
        // Copy EXIF (it will also have the SOI and EOI markers
        memcpy(destBuf->getData(), mExifBuf, exifSize);
        // Copy the final JPEG stream into the final destination buffer, but exclude the SOI marker
        char *copyTo = (char*)destBuf->getData() + exifSize;
        char *copyFrom = (char*)mOutData + sizeof(JPEG_MARKER_SOI);
        memcpy(copyTo, copyFrom, mainSize - sizeof(JPEG_MARKER_SOI));
    }
    LOG1("Total JPEG size: %d (time to encode: %ums)", totalSize, (unsigned)((systemTime() - startTime) / 1000000));
    return status;
}


status_t PictureThread::encode(CameraBuffer *snaphotBuf, CameraBuffer *postviewBuf)
{
    LOG1("@%s", __FUNCTION__);
    Message msg;
    msg.id = MESSAGE_ID_ENCODE;
    msg.data.encode.snaphotBuf = snaphotBuf;
    msg.data.encode.postviewBuf = postviewBuf;
    status_t ret = INVALID_OPERATION;

    if (snaphotBuf != 0)
        snaphotBuf->incrementReader();

    if ((ret = mMessageQueue.send(&msg)) != NO_ERROR) {
        if (snaphotBuf != 0)
            snaphotBuf->decrementReader();
    }
    return ret;
}

void PictureThread::getDefaultParameters(CameraParameters *params)
{
    LOG1("@%s", __FUNCTION__);
    if (!params) {
        ALOGE("null params");
        return;
    }

    params->setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);
    params->set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS,
            CameraParameters::PIXEL_FORMAT_JPEG);
    params->set(CameraParameters::KEY_JPEG_QUALITY, "80");
    params->set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, "50");
}

void PictureThread::setConfig(Config *config)
{
    mConfig = *config;
    if(mOutData != NULL)
        delete mOutData;
    mMaxOutDataSize = (mConfig.picture.width * mConfig.picture.height * 2);
    mOutData = new unsigned char[mMaxOutDataSize];


    if (mExifBuf != NULL)
        delete mExifBuf;
    mExifBuf = new unsigned char[MAX_EXIF_SIZE];
}

status_t PictureThread::flushBuffers()
{
    LOG1("@%s", __FUNCTION__);
    Message msg;
    msg.id = MESSAGE_ID_FLUSH;
    mMessageQueue.remove(MESSAGE_ID_ENCODE);
    return mMessageQueue.send(&msg, MESSAGE_ID_FLUSH);
}

status_t PictureThread::handleMessageExit()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    mThreadRunning = false;
    return status;
}

status_t PictureThread::handleMessageEncode(MessageEncode *msg)
{
    LOG1("@%s: snapshot ID = %d", __FUNCTION__, msg->snaphotBuf->getID());
    status_t status = NO_ERROR;
    int exifSize = 0;
    int totalSize = 0;
    CameraBuffer jpegBuf;

    if (mConfig.picture.width == 0 ||
        mConfig.picture.height == 0 ||
        mConfig.picture.format == 0) {
        ALOGE("Picture information not set yet!");
        if (msg->snaphotBuf != NULL)
            msg->snaphotBuf->decrementReader();
        return UNKNOWN_ERROR;
    }

    // Encode the image
    if ((status = encodeToJpeg(msg->snaphotBuf, msg->postviewBuf, &jpegBuf)) == NO_ERROR) {
        mCallbacks->compressedRawFrameDone(msg->snaphotBuf);
        mCallbacks->compressedFrameDone(&jpegBuf);
    } else {
        ALOGE("Error generating JPEG image!");
    }
    // When the encoding is done, send back the buffers to camera
    if (msg->snaphotBuf != NULL)
        msg->snaphotBuf->decrementReader();
    LOG1("Releasing jpegBuf @%p", jpegBuf.getData());
    jpegBuf.releaseMemory();

    return status;
}

status_t PictureThread::handleMessageFlush()
{
    LOG1("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    mMessageQueue.reply(MESSAGE_ID_FLUSH, status);
    return status;
}

status_t PictureThread::waitForAndExecuteMessage()
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;
    Message msg;
    mMessageQueue.receive(&msg);

    switch (msg.id) {

        case MESSAGE_ID_EXIT:
            status = handleMessageExit();
            break;

        case MESSAGE_ID_ENCODE:
            status = handleMessageEncode(&msg.data.encode);
            break;

        case MESSAGE_ID_FLUSH:
            status = handleMessageFlush();
            break;

        default:
            status = BAD_VALUE;
            break;
    };
    return status;
}

bool PictureThread::threadLoop()
{
    LOG2("@%s", __FUNCTION__);
    status_t status = NO_ERROR;

    mThreadRunning = true;
    while (mThreadRunning)
        status = waitForAndExecuteMessage();

    return false;
}

status_t PictureThread::requestExitAndWait()
{
    LOG1("@%s", __FUNCTION__);
    Message msg;
    msg.id = MESSAGE_ID_EXIT;

    // tell thread to exit
    // send message asynchronously
    mMessageQueue.send(&msg);

    // propagate call to base class
    return Thread::requestExitAndWait();
}

} // namespace android
