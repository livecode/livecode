LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := revzip

LOCAL_SRC_FILES := $(addprefix src/,revzip.cpp)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../libexternal/include \
	$(LOCAL_PATH)/../thirdparty/libzip/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/include

LOCAL_CPPFLAGS += -frtti -fexceptions

LOCAL_STATIC_LIBRARIES := libexternal libzip

LOCAL_LDLIBS += -lz \
	$(call host-path,$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/libstdc++.a)
	
# SN-2015-03-25: [[ Bug 14326 ]] Add the symbol to allow the mobile externals to access
#  the external interface version setting function
LOCAL_LDFLAGS += -Wl,-u,getXtable -Wl,-u,setExternalInterfaceVersion -Wl,-u,configureSecurity

include $(BUILD_SHARED_LIBRARY)