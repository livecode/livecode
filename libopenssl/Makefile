NAME=libopenssl
TYPE=archive

include ../../rules/environment.linux.makefile

SOURCES=\
	sslstubs.cpp

CUSTOM_DEFINES=

CUSTOM_INCLUDES=./include ./src

CUSTOM_CCFLAGS=

sslstubs.cpp: ssl.stubs
	mkdir -p ./src
	../../prebuilt/bin/Revolution.lnx "../../tools/weak_stub_maker.lc" <./ssl.stubs >./src/sslstubs.cpp

include ../../rules/archive.linux.makefile
