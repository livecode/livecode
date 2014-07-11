LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libgraphics

LOCAL_SRC_FILES := $(addprefix src/,\
		context.cpp \
		image.cpp \
		path.cpp \
		utils.cpp \
		blur.cpp spread.cpp \
		mblandroidtext.cpp \
		cachetable.cpp \
		legacyblendmodes.cpp \
		legacygradients.cpp \
		region.cpp \
		hb-sk.cpp \
	)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../libfoundation/include \
	$(LOCAL_PATH)/../thirdparty/libskia/include/config \
	$(LOCAL_PATH)/../thirdparty/libskia/include/core \
	$(LOCAL_PATH)/../thirdparty/libskia/include/effects \
	$(LOCAL_PATH)/../thirdparty/libskia/include/ports \
	$(LOCAL_PATH)/../thirdparty/libfreetype/include \
	$(LOCAL_PATH)/../thirdparty/libharfbuzz/src \
	$(LOCAL_PATH)/../prebuilt/include \

include $(BUILD_STATIC_LIBRARY)
