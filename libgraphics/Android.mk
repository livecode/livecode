LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libgraphics

LOCAL_SRC_FILES := $(addprefix src/,\
		context.cpp \
		image.cpp \
		path.cpp \
		utils.cpp \
		blur.cpp \
		mblandroidtext.cpp \
		cachetable.cpp \
		legacyblendmodes.cpp \
	)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../libcore/include \
	$(LOCAL_PATH)/../thirdparty/libskia/include/config \
	$(LOCAL_PATH)/../thirdparty/libskia/include/core \
	$(LOCAL_PATH)/../thirdparty/libskia/include/effects \

include $(BUILD_STATIC_LIBRARY)
