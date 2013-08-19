NAME=libgraphics
TYPE=archive

include ../rules/environment.linux.makefile

SOURCES=context.cpp image.cpp path.cpp utils.cpp

CUSTOM_DEFINES=

CUSTOM_INCLUDES=./src \
		../libskia/include/core \
		../libskia/include/config \
		../libskia/include/effects

CUSTOM_CCFLAGS=-fno-exceptions -fno-rtti


include ../rules/archive.linux.makefile
