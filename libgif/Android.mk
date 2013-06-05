LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libgif

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	$(addprefix src/,dgif_lib.c egif_lib.c gif_err.c gif_font.c gif_hash.c gifalloc.c quantize.c)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)
