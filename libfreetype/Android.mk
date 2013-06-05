LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libfreetype

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
	$(addprefix src/,autofit/autofit.c \
		bdf/bdf.c \
		cff/cff.c \
		base/ftbase.c \
		base/ftbbox.c \
		base/ftbitmap.c \
		cache/ftcache.c \
		base/ftdebug.c \
		base/ftfstype.c \
		base/ftgasp.c \
		base/ftglyph.c \
		base/ftgxval.c \
		base/ftinit.c \
		base/ftlcdfil.c \
		lzw/ftlzw.c \
		gzip/ftgzip.c \
		base/ftmm.c \
		base/ftotval.c \
		base/ftpatent.c \
		base/ftpfr.c \
		base/ftstroke.c \
		base/ftsynth.c \
		base/ftsystem.c \
		base/fttype1.c \
		base/ftxf86.c \
		pcf/pcf.c \
		pfr/pfr.c \
		psaux/psaux.c \
		pshinter/pshinter.c \
		psnames/psmodule.c \
		raster/raster.c \
		sfnt/sfnt.c \
		smooth/smooth.c \
		truetype/truetype.c \
		type1/type1.c \
		cid/type1cid.c \
		type42/type42.c \
		winfonts/winfnt.c \
		)

LOCAL_C_INCLUDES := \
		$(LOCAL_PATH)/include

LOCAL_CFLAGS += -DFT2_BUILD_LIBRARY

include $(BUILD_STATIC_LIBRARY)
