#ifndef _HAD_ZIPINT_H
#define _HAD_ZIPINT_H

/*
  $NiH: zipint.h,v 1.41 2005/06/18 00:54:08 wiz Exp $

  zipint.h -- internal declarations.
  Copyright (C) 1999, 2003, 2004, 2005 Dieter Baron and Thomas Klausner

  This file is part of libzip, a library to manipulate ZIP archives.
  The authors can be contacted at <nih@giga.or.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The names of the authors may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <zlib.h>

#include <zip.h>

#ifdef _WIN32
#include "zipintw32.h"
#endif

#define CENTRAL_MAGIC "PK\1\2"
#define LOCAL_MAGIC   "PK\3\4"
#define EOCD_MAGIC    "PK\5\6"
#define DATADES_MAGIC "PK\7\8"
#define CDENTRYSIZE         46
#define LENTRYSIZE          30
#define MAXCOMLEN        65536
#define EOCDLEN             22
#define CDBUFSIZE       (MAXCOMLEN+EOCDLEN)
#define BUFSIZE		8192



/* state of change of a file in zip archive */

enum zip_state { ZIP_ST_UNCHANGED, ZIP_ST_DELETED, ZIP_ST_REPLACED,
		 ZIP_ST_ADDED, ZIP_ST_RENAMED };

/* constants for struct zip_file's member flags */

#define ZIP_ZF_EOF	1 /* EOF reached */
#define ZIP_ZF_DECOMP	2 /* decompress data */
#define ZIP_ZF_CRC	4 /* compute and compare CRC */

/* error information */

struct zip_error {
    int zip_err;	/* libzip error code (ZIP_ER_*) */
    int sys_err;	/* copy of errno (E*) or zlib error code */
    char *str;		/* string representation or NULL */
};

/* zip archive, part of API */

struct zip {
	/* Added 06-07-19 */
	void *callback_context;
	zip_progress_callback_t callback;

	
    char *zn;			/* file name */
    FILE *zp;			/* file */
    struct zip_error error;	/* error information */

    struct zip_cdir *cdir;	/* central directory */
    int nentry;			/* number of entries */
    int nentry_alloc;		/* number of entries allocated */
    struct zip_entry *entry;	/* entries */
    int nfile;			/* number of opened files within archive */
    int nfile_alloc;		/* number of files allocated */
    struct zip_file **file;	/* opened files within archive */
};

/* file in zip archive, part of API */

struct zip_file {
    struct zip *za;		/* zip archive containing this file */
    struct zip_error error;	/* error information */
    int flags;			/* -1: eof, >0: error */
    int index;			/* index of the file */

    int method;			/* compression method */
    long fpos;			/* position within zip file (fread/fwrite) */
    unsigned long bytes_left;	/* number of bytes left to read */
    unsigned long cbytes_left;  /* number of bytes of compressed data left */
    
    unsigned long crc;		/* CRC so far */
    unsigned long crc_orig;	/* CRC recorded in archive */

    char *buffer;
    z_stream *zstr;
};

/* zip archive directory entry (central or local) */

struct zip_dirent {
    unsigned short version_madeby;	/* (c)  version of creator */
    unsigned short version_needed;	/* (cl) version needed to extract */
    unsigned short bitflags;		/* (cl) general purposee bit flag */
    unsigned short comp_method;		/* (cl) compression method used */
    time_t last_mod;			/* (cl) time of last modification */
    unsigned int crc;			/* (cl) CRC-32 of uncompressed data */
    unsigned int comp_size;		/* (cl) size of commpressed data */
    unsigned int uncomp_size;		/* (cl) size of uncommpressed data */
    char *filename;			/* (cl) file name (NUL-terminated) */
    unsigned short filename_len;	/* (cl) length of filename (w/o NUL) */
    char *extrafield;			/* (cl) extra field */
    unsigned short extrafield_len;	/* (cl) length of extra field */
    char *comment;			/* (c)  file comment */
    unsigned short comment_len;		/* (c)  length of file comment */
    unsigned short disk_number;		/* (c)  disk number start */
    unsigned short int_attrib;		/* (c)  internal file attributes */
    unsigned int ext_attrib;		/* (c)  external file attributes */
    unsigned int offset;		/* (c)  offest of local header  */
};

/* zip archive central directory */

struct zip_cdir {
    struct zip_dirent *entry;	/* directory entries */
    int nentry;			/* number of entries */

    unsigned int size;		/* size of central direcotry */
    unsigned int offset;	/* offset of central directory in file */
    char *comment;		/* zip archive comment */
    unsigned short comment_len;	/* length of zip archive comment */
};



struct zip_source {
    zip_source_callback f;
    void *ud;
};

/* entry in zip archive directory */

struct zip_entry {
    enum zip_state state;
    struct zip_source *source;
    int ch_method;
    char *ch_filename;

	// Add made by (unsigned char)
	// Add external attrs (unsigned int)
	unsigned char made_by;
	unsigned int ext_attrib;
};



extern const char * const _zip_err_str[];
extern const int _zip_nerr_str;
extern const int _zip_err_type[];



#define ZIP_ENTRY_DATA_CHANGED(x)	\
			((x)->state == ZIP_ST_REPLACED  \
			 || (x)->state == ZIP_ST_ADDED)



void _zip_cdir_free(struct zip_cdir *);
struct zip_cdir *_zip_cdir_new(int, struct zip_error *);
int _zip_cdir_write(struct zip_cdir *, FILE *, struct zip_error *);

void _zip_dirent_finalize(struct zip_dirent *);
void _zip_dirent_init(struct zip_dirent *);
int _zip_dirent_read(struct zip_dirent *, FILE *,
		     unsigned char **, unsigned int, int, struct zip_error *);
int _zip_dirent_write(struct zip_dirent *, FILE *, int, struct zip_error *);

void _zip_entry_free(struct zip_entry *);
void _zip_entry_init(struct zip *, int);
struct zip_entry *_zip_entry_new(struct zip *);

void _zip_error_copy(struct zip_error *, struct zip_error *);
void _zip_error_fini(struct zip_error *);
void _zip_error_get(struct zip_error *, int *, int *);
void _zip_error_init(struct zip_error *);
void _zip_error_set(struct zip_error *, int, int);
const char *_zip_error_strerror(struct zip_error *);

int _zip_file_fillbuf(void *, size_t, struct zip_file *);
unsigned int _zip_file_get_offset(struct zip *, int);

void _zip_free(struct zip *);
const char *_zip_get_name(struct zip *, int, int, struct zip_error *);
int _zip_local_header_read(struct zip *, int);
int _zip_name_locate(struct zip *, const char *, int, struct zip_error *);
struct zip *_zip_new(struct zip_error *);
unsigned short _zip_read2(unsigned char **);
unsigned int _zip_read4(unsigned char **);
int _zip_replace(struct zip *, int, const char *, struct zip_source *);
int _zip_set_name(struct zip *, int, const char *);
int _zip_unchange(struct zip *, int, int);
void _zip_unchange_data(struct zip_entry *);

#endif /* zipint.h */
