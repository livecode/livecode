##
## Copyright (C) 2012 The Android Open Source Project
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

DERIVED_SRC = ../_cache/mac/Release/stdscript.build/DerivedSources

SCRIPT_SRC_FILES = \
	src/script-builder.cpp \
	src/script-instance.cpp \
	src/script-module.cpp \
	src/script-object.cpp \
	src/script-package.cpp \
	src/module-arithmetic.cpp \
	src/module-array.cpp \
	src/module-binary.cpp \
	src/module-bitwise.cpp \
	src/module-byte.cpp \
	src/module-char.cpp \
	src/module-date.cpp \
	src/module-encoding.cpp \
	src/module-foreign.cpp \
	src/module-file.cpp \
	src/module-list.cpp \
	src/module-logic.cpp \
	src/module-map.cpp \
	src/module-math_foundation.cpp \
	src/module-math.cpp \
	src/module-sort.cpp \
	src/module-stream.cpp \
	src/module-string.cpp \
	src/module-type_convert.cpp \
	src/module-type.cpp \
	src/module-url.cpp \
	$(DERIVED_SRC)/arithmetic.mlc.c \
	$(DERIVED_SRC)/array.mlc.c \
	$(DERIVED_SRC)/binary.mlc.c \
	$(DERIVED_SRC)/bitwise.mlc.c \
	$(DERIVED_SRC)/byte.mlc.c \
	$(DERIVED_SRC)/char.mlc.c \
	$(DERIVED_SRC)/date.mlc.c \
	$(DERIVED_SRC)/file.mlc.c \
	$(DERIVED_SRC)/foreign.mlc.c \
	$(DERIVED_SRC)/item.mlc.c \
	$(DERIVED_SRC)/line.mlc.c \
	$(DERIVED_SRC)/list.mlc.c \
	$(DERIVED_SRC)/logic.mlc.c \
	$(DERIVED_SRC)/math-foundation.mlc.c \
	$(DERIVED_SRC)/math.mlc.c \
	$(DERIVED_SRC)/segmentchunk.mlc.c \
	$(DERIVED_SRC)/sort.mlc.c \
	$(DERIVED_SRC)/stream.mlc.c \
	$(DERIVED_SRC)/string.mlc.c \
	$(DERIVED_SRC)/type-convert.mlc.c \
	$(DERIVED_SRC)/type.mlc.c

#############################################################
#   build the libscript static library
#

TARGET_PLATFORM=android-8

LOCAL_MODULE:= libscript

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
	$(SCRIPT_SRC_FILES)

LOCAL_CPP_EXTENSION     := .cpp

LOCAL_C_INCLUDES        := \
  $(LOCAL_PATH)/include \
  $(LOCAL_PATH)/src \
  $(LOCAL_PATH)/../thirdparty/libffi/android/include \
  $(LOCAL_PATH)/../libfoundation/include \
  $(LOCAL_PATH)/../libgraphics/include \
  $(LOCAL_PATH)/../engine/src
  
LOCAL_CFLAGS += -DHB_NO_MT -DHAVE_OT -DHAVE_UCDN -DHAVE_FREETYPE
LOCAL_EXPORT_LDLIBS := -L$(LOCAL_PATH)/../prebuilt/lib/android/armv6 -licui18n -licuio -licule -liculx -licuuc -licudata

include $(BUILD_STATIC_LIBRARY)

