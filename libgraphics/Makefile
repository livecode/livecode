NAME=libgraphics
TYPE=archive

include ../rules/environment.linux.makefile

SOURCES=context.cpp image.cpp path.cpp utils.cpp blur.cpp spread.cpp lnxtext.cpp cachetable.cpp legacyblendmodes.cpp legacygradients.cpp region.cpp

CUSTOM_DEFINES=

CUSTOM_INCLUDES=./src \
		../libskia/include/core \
		../libskia/include/config \
		../libskia/include/effects

CUSTOM_CCFLAGS=-fno-exceptions -fno-rtti -pthread
ifeq ($(ARCH),i386)
	CUSTOM_CCFLAGS+= -march=i486
endif

include ../rules/archive.linux.makefile
