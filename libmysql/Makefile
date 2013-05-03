NAME=libmysql
TYPE=archive

include ../../rules/environment.linux.makefile

SOURCES= \
	array.c bchange.c bmove_upp.c charset.c charset-def.c client.c \
	ctype-big5.c ctype-bin.c ctype.c ctype-cp932.c ctype-czech.c \
	ctype-eucjpms.c ctype-euc_kr.c ctype-extra.c ctype-gb2312.c ctype-gbk.c \
	ctype-latin1.c ctype-mb.c ctype-simple.c ctype-sjis.c ctype-tis620.c \
	ctype-uca.c ctype-ucs2.c ctype-ujis.c ctype-utf8.c ctype-win1250ch.c \
	dbug.c default.c dtoa.c errmsg.c errors.c int2str.c is_prefix.c libmysql.c \
	list.c longlong2str.c mf_arr_appstr.c mf_dirname.c mf_fn_ext.c mf_format.c \
	mf_loadpath.c mf_pack.c mf_qsort.c mulalloc.c my_access.c my_alloc.c \
	my_compress.c my_div.c my_error.c my_fopen.c my_fstream.c my_getsystime.c \
	my_getwd.c \
	my_init.c my_lib.c my_malloc.c my_messnc.c my_once.c my_open.c \
	my_pthread.c my_read.c my_realloc.c my_rnd.c my_static.c my_strtoll10.c \
	my_symlink.c my_sync.c my_thr_init.c my_time.c my_vsnprintf.c net_serv.c pack.c \
	password.c sha1.c str2int.c str_alloc.c strcend.c strend.c strmake.c \
	strmov.c strnmov.c strxmov.c strxnmov.c typelib.c vio.c viosocket.c \
	viossl.c viosslfactories.c xml.c

CUSTOM_DEFINES=

CUSTOM_INCLUDES=./include ./src

CUSTOM_CCFLAGS=

include ../../rules/archive.linux.makefile

