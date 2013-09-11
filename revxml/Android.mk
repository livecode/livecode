LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := revxml

LOCAL_SRC_FILES := $(addprefix src/,revxml.cpp xmlattribute.cpp xmldoc.cpp xmlelement.cpp)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../libexternal/include \
	$(LOCAL_PATH)/../thirdparty/libxml/include \
	$(LOCAL_PATH)/../thirdparty/libxslt/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/include

LOCAL_CPPFLAGS += -frtti -fexceptions

LOCAL_STATIC_LIBRARIES := libexternal libxslt libxml

LOCAL_LDLIBS += -lz \
	$(call host-path,$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/libstdc++.a)

LOCAL_LDFLAGS += -Wl,-u,getXtable

include $(BUILD_SHARED_LIBRARY)
