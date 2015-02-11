NAME=lcidlc
TYPE=application

SOURCES=\
	Error.cpp Interface.cpp InterfaceGenerate.cpp Main.cpp \
	NativeType.cpp Parser.cpp Position.cpp Scanner.cpp \
	Value.cpp EncodedSupport.c EncodedJavaSupport.c

CUSTOM_LIBS=core
CUSTOM_STATIC_LIBS=stdc++

EncodedSupport.c: Support.mm
	../prebuilt/bin/Revolution.lnx "./encode_source.rev" "$SRCROOT/src/Support.mm" "$SRCROOT/src/EncodedSupport.c" "g_support_template"

EncodedJavaSupport.c: Support.mm
	../prebuilt/bin/Revolution.lnx "./encode_source.rev" "$SRCROOT/src/Support.java" "$SRCROOT/src/EncodedJavaSupport.c" "g_java_support_template"

include $(dir $(lastword $(MAKEFILE_LIST)))/../rules/application.linux.makefile
