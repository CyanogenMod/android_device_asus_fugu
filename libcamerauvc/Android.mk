ifeq ($(INTEL_USE_CAMERA_UVC),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	ControlThread.cpp \
	PreviewThread.cpp \
	PictureThread.cpp \
	VideoThread.cpp \
	PipeThread.cpp \
	CameraDriver.cpp \
	DebugFrameRate.cpp \
	Callbacks.cpp \
	CameraHAL.cpp \
	ColorConverter.cpp \
	EXIFFields.cpp \
	JpegCompressor.cpp \

LOCAL_C_INCLUDES += \
	system/media/camera/include\
	frameworks/base/include \
	frameworks/base/include/binder \
	frameworks/base/include/camera \
	external/jpeg \
	hardware/libhardware/include/hardware \
	external/skia/include/core \
	external/skia/include/images \
	device/asus/fugu/libs3cjpeg \

LOCAL_SHARED_LIBRARIES := \
	libcamera_client \
	libutils \
	libcutils \
	libbinder \
	libskia \
	libandroid \
	libui \
	libs3cjpeg \

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE := camera.$(TARGET_BOOTLOADER_BOARD_NAME)
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))

endif # ifeq ($(INTEL_USE_CAMERA_UVC),true)
