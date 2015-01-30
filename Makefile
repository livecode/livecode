###############################################################################
# Engine Targets

.PHONY: libopenssl liburlcache libstubs libfoundation libcore libscript
.PHONY: libexternal libexternalv1 libz libjpeg libpcre libpng libplugin libgraphics libskia
.PHONY: revsecurity libgif
.PHONY: kernel development standalone webruntime webplugin webplayer server
.PHONY: kernel-standalone kernel-development kernel-server
.PHONY: libireviam onrev-server

libcore:
	$(MAKE) -C ./libcore libcore

libexternal:
	$(MAKE) -C ./libexternal libexternal

libexternalv1:
	$(MAKE) -C ./libexternalv1 libexternalv1

libffi:
	$(MAKE) -C ./thirdparty/libffi libffi

libz:
	$(MAKE) -C ./thirdparty/libz libz

libjpeg:
	$(MAKE) -C ./thirdparty/libjpeg libjpeg

libpcre:
	$(MAKE) -C ./thirdparty/libpcre libpcre

libpng:
	$(MAKE) -C ./thirdparty/libpng libpng

libgif:
	$(MAKE) -C ./thirdparty/libgif libgif

libopenssl:
	$(MAKE) -C ./thirdparty/libopenssl libopenssl

libskia:
	$(MAKE) -C ./thirdparty/libskia libskia

libfoundation: libffi
	$(MAKE) -C ./libfoundation libfoundation

libscript:
	$(MAKE) -C ./libscript libscript

revsecurity:
	$(MAKE) -C ./thirdparty/libopenssl -f Makefile.revsecurity revsecurity

libgraphics: libskia
	$(MAKE) -C ./libgraphics libgraphics

kernel: libz libgif libjpeg libpcre libpng libopenssl libexternal libfoundation libstdscript libgraphics

	$(MAKE) -C ./engine -f Makefile.kernel libkernel

kernel-standalone: kernel
	$(MAKE) -C ./engine -f Makefile.kernel-standalone libkernel-standalone

kernel-development: kernel
	$(MAKE) -C ./engine -f Makefile.kernel-development libkernel-development

kernel-server: libz libgif libjpeg libpcre libpng libopenssl libexternal libfoundation libstdscript libgraphics
	$(MAKE) -C ./engine -f Makefile.kernel-server libkernel-server

development: libz libgif libjpeg libpcre libpng libopenssl libexternal libfoundation libstdscript kernel kernel-development revsecurity
	$(MAKE) -C ./engine -f Makefile.development engine-community

standalone: libz libgif libjpeg libpcre libpng libopenssl libfoundation libstdscript kernel revsecurity kernel-standalone revsecurity
	$(MAKE) -C ./engine -f Makefile.standalone standalone-community

installer: libz libgif libjpeg libpcre libpng libopenssl libexternal libfoundation libstdscript kernel revsecurity

	$(MAKE) -C ./engine -f Makefile.installer installer

server: libz libgif libjpeg libpcre libpng libopenssl libexternal libfoundation libstdscript libgraphics kernel-server revsecurity
	$(MAKE) -C ./engine -f Makefile.server server-community

###############################################################################
# revPDFPrinter Targets

.PHONY: libcairopdf revpdfprinter

libcairopdf:
	$(MAKE) -C ./thirdparty/libcairo libcairopdf

revpdfprinter: libcairopdf libcore
	$(MAKE) -C ./revpdfprinter revpdfprinter

###############################################################################
# revDB Targets

.PHONY: libpq libmysql libsqlite libiodbc

libpq:
	$(MAKE) -C ./thirdparty/libpq libpq

libmysql:
	$(MAKE) -C ./thirdparty/libmysql libmysql

libsqlite:
	$(MAKE) -C ./thirdparty/libsqlite libsqlite

libiodbc:
	$(MAKE) -C ./thirdparty/libiodbc libiodbc

#####

.PHONY: dbpostgresql dbmysql dbsqlite dbodbc server-dbpostgresql server-dbmysql server-dbodbc server-dbsqlite

dbpostgresql: libpq
	$(MAKE) -C ./revdb dbpostgresql

dbmysql: libmysql libz libopenssl
	$(MAKE) -C ./revdb dbmysql

dbsqlite: libsqlite libexternal
	$(MAKE) -C ./revdb dbsqlite

dbodbc: libiodbc libexternal
	$(MAKE) -C ./revdb dbodbc

server-dbpostgresql: libpq
	$(MAKE) -C ./revdb server-dbpostgresql

server-dbmysql: libmysql libz
	$(MAKE) -C ./revdb server-dbmysql

server-dbsqlite: libsqlite libexternal
	$(MAKE) -C ./revdb server-dbsqlite

server-dbodbc: libiodbc libexternal
	$(MAKE) -C ./revdb server-dbodbc

####

.PHONY: revdb server-revdb

revdb: libexternal
	$(MAKE) -C ./revdb revdb

server-revdb: libexternal
	$(MAKE) -C ./revdb server-revdb

###############################################################################
# revXML Targets

.PHONY: libxml libxslt revxml server-revxml

libxml:
	$(MAKE) -C ./thirdparty/libxml libxml

libxslt:
	$(MAKE) -C ./thirdparty/libxslt libxslt

revxml: libxml libxslt libexternal
	$(MAKE) -C ./revxml revxml

server-revxml: libxml libxslt libexternal
	$(MAKE) -C ./revxml server-revxml

###############################################################################
# revZip Targets

.PHONY: libzip revzip server-revzip

libzip:
	$(MAKE) -C ./thirdparty/libzip libzip

revzip: libzip libz libexternal
	$(MAKE) -C ./revzip revzip

server-revzip: libzip libz libexternal
	$(MAKE) -C ./revzip server-revzip

###############################################################################
# revAndroid Targets

.PHONY: revandroid

revandroid: libexternalv1
	$(MAKE) -C ./revmobile revandroid

###############################################################################
# MLC Targets

.PHONY: lc-compile lc-bootstrap-compile lc-compile-clean
.PHONY: libstdscript
.PHONY: lc-test

########## Standard script library
libstdscript: lc-compile
	$(MAKE) -C ./libscript libstdscript

########## Compiler
lc-compile: libscript libfoundation libffi
	$(MAKE) -C ./toolchain lc-compile

lc-bootstrap-compile: libscript libfoundation libffi
	$(MAKE) -C ./toolchain bootstrap

lc-compile-clean:
	$(MAKE) -C ./toolchain clean

########## Module runner

lc-run: libstdscript libfoundation
	$(MAKE) -C ./toolchain lc-run

########## Test runner
lc-test: libstdscript libfoundation
	$(MAKE) -C ./toolchain lc-test

########## Tests
lc-test-check: lc-test
	$(MAKE) -C ./toolchain lc-test-check
.PHONY: mlc-check

###############################################################################
# All Targets

.PHONY: all bootstrap clean
.DEFAULT_GOAL := all

all: revzip server-revzip
all: revxml server-revxml
all: revdb dbodbc dbsqlite dbmysql dbpostgresql
all: server-revdb server-dbodbc server-dbsqlite server-dbmysql server-dbpostgresql
all: development standalone installer server
all: revpdfprinter revandroid
all: lc-run lc-test

bootstrap: lc-bootstrap-compile

check: lc-test-check

clean:
	-rm -rf _build/linux _cache/linux
	-rm -rf `find . -type d -name _mlc`
clean: lc-compile-clean
