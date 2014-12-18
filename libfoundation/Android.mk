LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libfoundation

LOCAL_SRC_FILES := src/foundation-array.cpp src/foundation-core.cpp src/foundation-debug.cpp \
	src/foundation-list.cpp src/foundation-name.cpp src/foundation-nativechars.cpp \
	src/foundation-number.cpp src/foundation-set.cpp src/foundation-stream.cpp \
	src/foundation-data.cpp src/foundation-string.cpp src/foundation-unicodechars.cpp src/foundation-value.cpp \
	src/foundation-error.cpp src/foundation-unicode.cpp src/foundation-locale.cpp src/foundation-text.cpp src/foundation-bidi.cpp \
	src/foundation-chunk.cpp src/foundation-filters.cpp src/foundation-foreign.cpp src/foundation-math.cpp \
	src/foundation-pickle.cpp src/foundation-proper-list.cpp src/foundation-record.cpp src/foundation-typeconvert.cpp \
	src/foundation-typeinfo.cpp src/foundation-handler.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../prebuilt/include \
	$(LOCAL_PATH)/../thirdparty/libffi/android/include

LOCAL_EXPORT_LDLIBS := -L$(LOCAL_PATH)/../prebuilt/lib/android/armv6 -licui18n -licuio -licule -liculx -licuuc -licudata

include $(BUILD_STATIC_LIBRARY)
