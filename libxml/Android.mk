LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

TARGET_PLATFORM=android-8

LOCAL_MODULE := libxml

LOCAL_SRC_FILES := \
	$(addprefix src/,c14n.c catalog.c chvalid.c debugXML.c dict.c DOCBparser.c encoding.c \
	entities.c error.c globals.c hash.c HTMLparser.c HTMLtree.c legacy.c list.c \
	parser.c parserInternals.c pattern.c relaxng.c SAX.c schematron.c threads.c \
	tree.c uri.c valid.c xinclude.c xlink.c xmlIO.c xmlmemory.c xmlmodule.c \
	xmlreader.c xmlregexp.c xmlsave.c xmlschemas.c xmlschemastypes.c xmlstring.c \
	xmlunicode.c xmlwriter.c xpath.c xpointer.c SAX2.c)

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)
