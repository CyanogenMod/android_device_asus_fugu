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

#ifndef ANDROID_LIBCAMERA_CAMERA_DRIVER
#define ANDROID_LIBCAMERA_CAMERA_DRIVER

#include <utils/Timers.h>
#include <utils/Errors.h>
#include <utils/Vector.h>
#include <utils/Errors.h>
#include <utils/threads.h>
#include <camera/CameraParameters.h>
#include "CameraCommon.h"

namespace android {

class Callbacks;

class CameraDriver {

// public types
public:

// constructor/destructor
public:
    CameraDriver(int cameraId);
    ~CameraDriver();

// public types
    enum Mode {
        MODE_NONE,
        MODE_PREVIEW,
        MODE_CAPTURE,
        MODE_VIDEO,
    };

    enum Effect {
        EFFECT_NONE,
        EFFECT_MONO,
        EFFECT_NEGATIVE,
        EFFECT_SOLARIZE,
        EFFECT_SEPIA,
        EFFECT_POSTERIZE,
        EFFECT_WHITEBOARD,
        EFFECT_BLACKBOARD,
        EFFECT_AQUA,
    };

    enum FlashMode {
        FLASH_MODE_OFF,
        FLASH_MODE_AUTO,
        FLASH_MODE_ON,
        FLASH_MODE_RED_EYE,
        FLASH_MODE_TORCH,
    };

    enum SceneMode {
        SCENE_MODE_AUTO,
        SCENE_MODE_ACTION,
        SCENE_MODE_PORTRAIT,
        SCENE_MODE_LANDSCAPE,
        SCENE_MODE_NIGHT,
        SCENE_MODE_NIGHT_PORTRAIT,
        SCENE_MODE_THEATRE,
        SCENE_MODE_BEACH,
        SCENE_MODE_SNOW,
        SCENE_MODE_SUNSET,
        SCENE_MODE_STEADYPHOTO,
        SCENE_MODE_FIREWORKS,
        SCENE_MODE_SPORTS,
        SCENE_MODE_PARTY,
        SCENE_MODE_CANDLELIGHT,
        SCENE_MODE_BARCODE,
    };

    enum FocusMode {
        FOCUS_DISTANCE_INFINITY,
        FOCUS_MODE_AUTO,
        FOCUS_MODE_INFINITY,
        FOCUS_MODE_MACRO,
        FOCUS_MODE_FIXED,
        FOCUS_MODE_EDOF,
        FOCUS_MODE_CONTINUOUS_VIDEO,
        FOCUS_MODE_CONTINUOUS_PICTURE,
    };

    enum WhiteBalanceMode {
        WHITE_BALANCE_AUTO,
        WHITE_BALANCE_INCANDESCENT,
        WHITE_BALANCE_FLUORESCENT,
        WHITE_BALANCE_WARM_FLUORESCENT,
        WHITE_BALANCE_DAYLIGHT,
        WHITE_BALANCE_CLOUDY_DAYLIGHT,
        WHITE_BALANCE_TWILIGHT,
        WHITE_BALANCE_SHADE,
    };

// public methods
public:

    void getDefaultParameters(CameraParameters *params);

    status_t start(Mode mode);
    status_t stop();

    inline int getNumBuffers() { return NUM_DEFAULT_BUFFERS; }

    status_t getPreviewFrame(CameraBuffer **buff);
    status_t putPreviewFrame(CameraBuffer *buff);

    status_t getRecordingFrame(CameraBuffer **buff, nsecs_t *timestamp);
    status_t putRecordingFrame(CameraBuffer *buff);

    status_t setSnapshotBuffers(void *buffs, int numBuffs);
    status_t getSnapshot(CameraBuffer **buff);
    status_t putSnapshot(CameraBuffer *buff);

    status_t getThumbnail(CameraBuffer *buff,CameraBuffer **inputBuffer,
        int width, int height, int thumb_w, int thumb_h);
    status_t putThumbnail(CameraBuffer *buff);
    CameraBuffer* findBuffer(void* findMe) const;

    bool dataAvailable();
    bool isBufferValid(const CameraBuffer * buffer) const;

    status_t setPreviewFrameSize(int width, int height);
    status_t setPostviewFrameSize(int width, int height);
    status_t setSnapshotFrameSize(int width, int height);
    status_t setVideoFrameSize(int width, int height);

    // The camera sensor YUV format
    inline int getFormat() { return mFormat; }

    void getVideoSize(int *width, int *height);

    void getZoomRatios(Mode mode, CameraParameters *params);
    void computeZoomRatios(char *zoom_ratio, int max_count);
    void getFocusDistances(CameraParameters *params);
    status_t setZoom(int zoom);

    // EXIF params
    status_t getFNumber(unsigned int *fNumber); // format is: (numerator << 16) | denominator
    status_t getExposureInfo(CamExifExposureProgramType *exposureProgram,
                             CamExifExposureModeType *exposureMode,
                             int *exposureTime,
                             float *exposureBias,
                             int *aperture);
    status_t getBrightness(float *brightness);
    status_t getIsoSpeed(int *isoSpeed);
    status_t getMeteringMode(CamExifMeteringModeType *meteringMode);
    status_t getAWBMode(CamExifWhiteBalanceType *wbMode);
    status_t getSceneMode(CamExifSceneCaptureType *sceneMode);

    // camera hardware information
    static int getNumberOfCameras();
    static status_t getCameraInfo(int cameraId, camera_info *cameraInfo);

    float getFrameRate() { return mConfig.fps; }

    status_t autoFocus();
    status_t cancelAutoFocus();

    status_t setEffect(Effect effect);
    status_t setFlashMode(FlashMode flashMode);
    status_t setSceneMode(SceneMode sceneMode);
    status_t setFocusMode(FocusMode focusModei,
                          CameraWindow *windows = 0,    // optional
                          int numWindows = 0);          // optional
    status_t setWhiteBalanceMode(WhiteBalanceMode wbMode);
    status_t setAeLock(bool lock);
    status_t setAwbLock(bool lock);
    status_t setMeteringAreas(CameraWindow *windows, int numWindows);

// private types
private:

    static const int MAX_CAMERAS         = 8;
    static const int NUM_DEFAULT_BUFFERS = 4;

    struct FrameInfo {
        int width;      // Frame width
        int height;     // Frame height
        int padding;    // Frame padding width
        int maxWidth;   // Frame maximum width
        int maxHeight;  // Frame maximum height
        int size;       // Frame size in bytes
    };

    struct Config {
        FrameInfo preview;    // preview
        FrameInfo recording;  // recording
        FrameInfo snapshot;   // snapshot
        FrameInfo postview;   // postview (thumbnail for capture)
        float fps;            // preview/recording (shared)
        int num_snapshot;     // number of snapshots to take
        int zoom;             // zoom value
    };

    struct CameraSensor {
        char *devName;              // device node's name, e.g. /dev/video0
        struct camera_info info;    // camera info defined by Android
        int fd;                     // the file descriptor of device at run time

        /* more fields will be added when we find more 'per camera' data*/
    };

    struct DriverBuffer {
        CameraBuffer camBuff;
        struct v4l2_buffer vBuff;
    };

    struct DriverBufferPool {
        int numBuffers;
        int numBuffersQueued;
        CameraBuffer *thumbnail;
        DriverBuffer *bufs;
    };

    struct DriverSupportedControls {
        bool zoomAbsolute;
        bool focusAuto;
        bool focusAbsolute;
        bool tiltAbsolute;
        bool panAbsolute;
        bool exposureAutoPriority;
        bool exposureAbsolute;
        bool exposureAuto;
        bool backlightCompensation;
        bool sharpness;
        bool whiteBalanceTemperature;
        bool powerLineFrequency;
        bool gain;
        bool whiteBalanceTemperatureAuto;
        bool saturation;
        bool contrast;
        bool brightness;
        bool hue;
    };

// private methods
private:

    status_t startPreview();
    status_t stopPreview();
    status_t startRecording();
    status_t stopRecording();
    status_t startCapture();
    status_t stopCapture();

    static int enumerateCameras();
    static void cleanupCameras();
    const char* getMaxSnapShotResolution();

    // Open, Close, Configure methods
    int openDevice();
    void closeDevice();
    int configureDevice(Mode deviceMode, int w, int h, int numBuffers);
    int deconfigureDevice();
    int startDevice();
    void stopDevice();

    // Buffer methods
    status_t allocateBuffer(int fd, int index);
    status_t allocateBuffers(int numBuffers);
    status_t freeBuffer(int index);
    status_t freeBuffers();
    status_t queueBuffer(CameraBuffer *buff, bool init = false);
    status_t dequeueBuffer(CameraBuffer **buff, nsecs_t *timestamp = 0);

    status_t v4l2_capture_open(const char *devName);
    status_t v4l2_capture_close(int fd);
    status_t v4l2_capture_querycap(int fd, struct v4l2_capability *cap);
    status_t v4l2_capture_queryctrl(int fd, int attribute_num);
    status_t querySupportedControls();
    status_t getZoomMaxMinValues();
    int detectDeviceResolutions();
    int set_capture_mode(Mode deviceMode);
    int v4l2_capture_try_format(int fd, int *w, int *h);
    int v4l2_capture_g_framerate(int fd, float * framerate, int width, int height);
    int v4l2_capture_s_format(int fd, int w, int h);
    int set_attribute (int fd, int attribute_num,
                               const int value, const char *name);
    int set_zoom (int fd, int zoom);
    int xioctl(int fd, int request, void *arg);

    // private members
private:

    static int numCameras;
    static Mutex mCameraSensorLock;                             // lock to access mCameraSensor
    static struct CameraSensor *mCameraSensor[MAX_CAMERAS];     // all camera sensors in CameraDriver Class.

    Mode mMode;
    Callbacks *mCallbacks;

    Config mConfig;

    struct DriverBufferPool mBufferPool;

    int mSessionId; // uniquely identify each session

    int mCameraId;

    int mFormat;

    struct DriverSupportedControls mSupportedControls;

    int mZoomMax;

    int mZoomMin;
    char *mDetectedRes;

}; // class CameraDriver

}; // namespace android

#endif // ANDROID_LIBCAMERA_CAMERA_DRIVER
