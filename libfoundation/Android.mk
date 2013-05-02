LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libfoundation

LOCAL_SRC_FILES := src/foundation-array.cpp src/foundation-core.cpp src/foundation-debug.cpp \
	src/foundation-list.cpp src/foundation-name.cpp src/foundation-nativechars.cpp \
	src/foundation-number.cpp src/foundation-set.cpp src/foundation-stream.cpp \
	src/foundation-string.cpp src/foundation-unicodechars.cpp src/foundation-value.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)
