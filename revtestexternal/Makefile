NAME=revtestexternal
TYPE=library

SOURCES=\
	revtestexternal.cpp \
	revtestexternalidl.cpp

CUSTOM_DEFINES=

CUSTOM_INCLUDES=./src

CUSTOM_LIBS=

CUSTOM_STATIC_LIBS=stdc++

CUSTOM_DYNAMIC_LIBS=pthread

CUSTOM_CCFLAGS=

CUSTOM_LDFLAGS=

revtestexternalidl.cpp: revtestexternal.lcidl
	../_build/linux/$(MODE)/lcidlc revtestexternal.lcidl src/revtestexternalidl.cpp

include ../rules/library.linux.makefile
