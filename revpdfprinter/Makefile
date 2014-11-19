NAME=revpdfprinter
TYPE=library

include ../rules/environment.linux.makefile

SOURCES=revpdfprinter.cpp revpdfprinter_lnx.cpp

CUSTOM_DEFINES=

CUSTOM_INCLUDES=./src ../libcore/include ../thirdparty/libcairo/src

CUSTOM_LIBS=core cairopdf foundation z
CUSTOM_DYNAMIC_LIBS=dl freetype fontconfig pango-1.0 pangoft2-1.0 gobject-2.0 m

CUSTOM_CCFLAGS=\
	-Wall -Wno-non-virtual-dtor -fno-exceptions -fno-rtti \
	-fmessage-length=0

CUSTOM_LDFLAGS=

include ../rules/library.linux.makefile
