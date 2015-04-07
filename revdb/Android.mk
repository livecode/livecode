LOCAL_PATH := $(call my-dir)

#########

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := revdb

LOCAL_SRC_FILES := $(addprefix src/,revdb.cpp unxsupport.cpp database.cpp dbdrivercommon.cpp)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../libexternal/include \
	$(LOCAL_PATH)/../thirdparty/libzip/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/include

LOCAL_CPPFLAGS += -frtti -fexceptions

LOCAL_STATIC_LIBRARIES := libexternal

LOCAL_LDLIBS += -lz -llog \
	 $(call host-path,$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/libstdc++.a)

# SN-2015-03-25: [[ Bug 14326 ]] Add the symbol to allow the mobile externals to access
#  the external interface version setting function
LOCAL_LDFLAGS += -Wl,-u,getXtable -Wl,-u,setExternalInterfaceVersion -Wl,-u,configureSecurity

include $(BUILD_SHARED_LIBRARY)

#########

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := dbsqlite

LOCAL_SRC_FILES := $(addprefix src/,dbdrivercommon.cpp database.cpp dbsqliteapi.cpp sqlite_connection.cpp sqlite_cursor.cpp)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../libexternal/include \
	$(LOCAL_PATH)/../thirdparty/libsqlite/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/include

LOCAL_CPPFLAGS += -frtti -fexceptions

LOCAL_STATIC_LIBRARIES := libexternal libsqlite libsqlitedataset

LOCAL_LDLIBS += -lz \
	$(call host-path,$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/libstdc++.a)

LOCAL_LDFLAGS += -Wl,-u,newdbconnectionref -Wl,-u,releasedbconnectionref -Wl,-u,setidcounterref

include $(BUILD_SHARED_LIBRARY)

#########

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := dbmysql

LOCAL_SRC_FILES := $(addprefix src/,dbdrivercommon.cpp database.cpp dbmysqlapi.cpp mysql_connection.cpp mysql_cursor.cpp)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../libexternal/include \
	$(LOCAL_PATH)/../thirdparty/libmysql/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/include \
	$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/include

LOCAL_CPPFLAGS += -frtti -fexceptions

LOCAL_STATIC_LIBRARIES := libexternal libmysql libopenssl

LOCAL_LDLIBS += -lz \
	$(call host-path,$(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_ARCH_ABI)/libstdc++.a)

LOCAL_LDFLAGS += -Wl,-u,newdbconnectionref -Wl,-u,releasedbconnectionref -Wl,-u,setidcounterref

include $(BUILD_SHARED_LIBRARY)

#########
