###############################################################################
# Engine Targets

.PHONY: libopenssl liburlcache libstubs
.PHONY: libexternal libexternalv1 libz libjpeg libpcre libpng libplugin libcore libgraphics libskia
.PHONY: revsecurity libgif
.PHONY: kernel development standalone webruntime webplugin webplayer server
.PHONY: kernel-standalone kernel-development kernel-server
.PHONY: libireviam onrev-server

libexternal:
	$(MAKE) -C ./libexternal libexternal

libexternalv1:
	$(MAKE) -C ./libexternalv1 libexternalv1

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

libcore:
	$(MAKE) -C ./libcore libcore

revsecurity:
	$(MAKE) -C ./thirdparty/libopenssl -f Makefile.revsecurity revsecurity
	
libgraphics: libskia
	$(MAKE) -C ./libgraphics libgraphics

kernel: libz libgif libjpeg libpcre libpng libopenssl libexternal libcore libgraphics
	$(MAKE) -C ./engine -f Makefile.kernel libkernel

kernel-standalone: kernel
	$(MAKE) -C ./engine -f Makefile.kernel-standalone libkernel-standalone

kernel-development: kernel
	$(MAKE) -C ./engine -f Makefile.kernel-development libkernel-development

kernel-server:
	$(MAKE) -C ./engine -f Makefile.kernel-server libkernel-server

development: libz libgif libjpeg libpcre libpng libopenssl libexternal libcore kernel kernel-development revsecurity
	$(MAKE) -C ./engine -f Makefile.development engine-community

standalone: libz libgif libjpeg libpcre libpng libopenssl libcore kernel revsecurity kernel-standalone revsecurity
	$(MAKE) -C ./engine -f Makefile.standalone standalone-community

installer: libz libgif libjpeg libpcre libpng libopenssl libexternal libcore kernel revsecurity
	$(MAKE) -C ./engine -f Makefile.installer installer

server: libz libgif libjpeg libpcre libpng libopenssl libexternal libcore kernel kernel-server revsecurity
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
# All Targets

.PHONY: all clean
.DEFAULT_GOAL := all

all: revzip server-revzip
all: revxml server-revxml
all: revpdfprinter revandroid
all: revdb dbodbc dbsqlite dbmysql dbpostgresql
all: server-revdb server-dbodbc server-dbsqlite server-dbmysql server-dbpostgresql
all: development standalone installer server

clean:
	@rm -r _build/linux _cache/linux
