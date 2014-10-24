LOCAL_PATH := $(call my-dir)

# only build if run from the revtestexternal/build-android.sh script
ifeq ($(NAME),revtestexternal)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := revtestexternal

LOCAL_SRC_FILES := \
	$(addprefix src/,$(LOCAL_MODULE).cpp) \
	$(addprefix derived_src/,$(LOCAL_MODULE).lcidl.cpp)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../lcidlc/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/include

# Uncomment this line if you want to use C++ exceptions
# LOCAL_CPPFLAGS += -frtti -fexceptions

LOCAL_STATIC_LIBRARIES :=

LOCAL_LDLIBS += -llog
LOCAL_LDLIBS += $(call host-path,$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/libstdc++.a)

# Make sure the entry points into the external are referenced.
LOCAL_LDFLAGS += \
	-Wl,-u,MCExternalDescribe \
	-Wl,-u,MCExternalInitialize \
 	-Wl,-u,MCExternalFinalize

include $(BUILD_SHARED_LIBRARY)

endif