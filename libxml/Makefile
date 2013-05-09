NAME=libxml
TYPE=archive

include ../../rules/environment.linux.makefile

SOURCES= c14n.c catalog.c chvalid.c debugXML.c dict.c DOCBparser.c encoding.c \
	entities.c error.c globals.c hash.c HTMLparser.c HTMLtree.c legacy.c list.c \
	parser.c parserInternals.c pattern.c relaxng.c SAX.c schematron.c threads.c \
	tree.c uri.c valid.c xinclude.c xlink.c xmlIO.c xmlmemory.c xmlmodule.c \
	xmlreader.c xmlregexp.c xmlsave.c xmlschemas.c xmlschemastypes.c xmlstring.c \
	xmlunicode.c xmlwriter.c xpath.c xpointer.c SAX2.c

CUSTOM_DEFINES=

CUSTOM_INCLUDES=./include/libxml ./src

CUSTOM_CCFLAGS=

include ../../rules/archive.linux.makefile
