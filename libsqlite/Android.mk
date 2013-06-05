LOCAL_PATH := $(call my-dir)

#########

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libsqlite

LOCAL_SRC_FILES := \
	$(addprefix src/,sqlite3.c)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := -DSQLITE_ENABLE_FTS3 -DSQLITE_ENABLE_FTS3_PARANTHESIS -DSQLITE_OMIT_LOAD_EXTENSION -Dfdatasync=fsync

include $(BUILD_STATIC_LIBRARY)

#########

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libsqlitedataset

LOCAL_SRC_FILES := \
	$(addprefix src/,dataset.cpp qry_dat.cpp sqlitedataset.cpp sqlitedecode.cpp)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/include

LOCAL_CPPFLAGS += -frtti -fexceptions

include $(BUILD_STATIC_LIBRARY)
