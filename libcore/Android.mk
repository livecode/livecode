LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libcore

LOCAL_SRC_FILES := src/core.cpp src/binary.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)
