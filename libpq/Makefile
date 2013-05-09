NAME=libpq
TYPE=archive

include ../../rules/environment.linux.makefile

SOURCES= \
	fe-lobj.c pqexpbuffer.c pipe.c pgstrcasecmp.c pgsleep.c \
	sprompt.c fe-misc.c fseeko.c open.c noblock.c wchar.c \
	kill.c fe-exec.c fe-print.c dirmod.c encnames.c memcmp.c \
	md5.c isinf.c inet_aton.c qsort.c snprintf.c rint.c \
	random.c rand.c pqsignal.c fe-secure.c fe-protocol3.c \
	fe-protocol2.c fe-connect.c fe-auth.c ip.c thread.c \
	crypt.c

CUSTOM_DEFINES=HAVE_CONFIG_H

CUSTOM_INCLUDES=./src

CUSTOM_CCFLAGS=

include ../../rules/archive.linux.makefile
