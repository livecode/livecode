LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libpcre

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	$(addprefix src/, chartables.c pcre_compile.c pcre_config.c pcre_dfa_exec.c pcre_exec.c pcre_fullinfo.c \
	pcre_get.c pcre_globals.c pcre_info.c pcre_maketables.c pcre_ord2utf8.c pcre_refcount.c \
	pcre_study.c pcre_tables.c pcre_try_flipped.c pcre_ucp_searchfuncs.c pcre_valid_utf8.c \
	pcre_version.c pcre_xclass.c)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)
