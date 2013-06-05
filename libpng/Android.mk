LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libpng

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	$(addprefix src/,png.c pngerror.c pnggccrd.c pngget.c pngmem.c pngpread.c pngread.c \
	pngrio.c pngrtran.c pngrutil.c pngset.c pngtrans.c pngvcrd.c pngwio.c \
	pngwrite.c pngwtran.c pngwutil.c)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)
