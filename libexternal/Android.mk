LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libexternal

LOCAL_SRC_FILES := \
	$(addprefix src/,external.c unxsupport.cpp)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)
