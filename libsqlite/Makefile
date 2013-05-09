NAME=libsqlite
TYPE=archive

include ../../rules/environment.linux.makefile

SOURCES=dataset.cpp qry_dat.cpp sqlite3.c sqlitedataset.cpp sqlitedecode.cpp

CUSTOM_DEFINES=SQLITE_ENABLE_FTS3 SQLITE_ENABLE_FTS3_PARANTHESIS SQLITE_OMIT_LOAD_EXTENSION

CUSTOM_INCLUDES=

CUSTOM_CCFLAGS=

include ../../rules/archive.linux.makefile
