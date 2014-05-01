NAME=libfoundation
TYPE=archive

include ../rules/environment.linux.makefile

SOURCES=foundation-array.cpp foundation-core.cpp foundation-debug.cpp \
	foundation-list.cpp foundation-name.cpp foundation-nativechars.cpp \
        foundation-data.cpp foundation-number.cpp foundation-set.cpp \
        foundation-stream.cpp foundation-string.cpp foundation-unicodechars.cpp \
        foundation-value.cpp foundation-error.cpp foundation-unicode.cpp \
	foundation-locale.cpp foundation-bidi.cpp foundation-text.cpp

CUSTOM_DEFINES=

CUSTOM_INCLUDES=./src

CUSTOM_CCFLAGS=-fno-exceptions -fno-rtti


include ../rules/archive.linux.makefile
