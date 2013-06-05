#ifndef _HAD_ZIP_H
#define _HAD_ZIP_H

/*
  $NiH: zip.h,v 1.50 2005/07/14 14:08:11 dillo Exp $

  zip.h -- exported declarations.
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



#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdio.h>
#include <time.h>

#ifdef _WIN32
typedef signed long ssize_t;
#endif

/* flags for zip_open */

#define ZIP_CREATE           1
#define ZIP_EXCL             2
#define ZIP_CHECKCONS        4


/* flags for zip_name_locate, zip_fopen, zip_stat, ... */

#define ZIP_FL_NOCASE		1 /* ignore case on name lookup */
#define ZIP_FL_NODIR		2 /* ignore directory component */
#define ZIP_FL_COMPRESSED	4 /* read compressed data */
#define ZIP_FL_UNCHANGED	8 /* use original data, ignoring changes */

/* libzip error codes */

#define ZIP_ER_OK             0  /* N No error */
#define ZIP_ER_MULTIDISK      1  /* N Multi-disk zip archives not supported */
#define ZIP_ER_RENAME         2  /* S Renaming temporary file failed */
#define ZIP_ER_CLOSE          3  /* S Closing zip archive failed */
#define ZIP_ER_SEEK           4  /* S Seek error */
#define ZIP_ER_READ           5  /* S Read error */
#define ZIP_ER_WRITE          6  /* S Write error */
#define ZIP_ER_CRC            7  /* N CRC error */
#define ZIP_ER_ZIPCLOSED      8  /* N Containing zip archive was closed */
#define ZIP_ER_NOENT          9  /* N No such file */
#define ZIP_ER_EXISTS        10  /* N File already exists */
#define ZIP_ER_OPEN          11  /* S Can't open file */
#define ZIP_ER_TMPOPEN       12  /* S Failure to create temporary file */
#define ZIP_ER_ZLIB          13  /* Z Zlib error */
#define ZIP_ER_MEMORY        14  /* N Malloc failure */
#define ZIP_ER_CHANGED       15  /* N Entry has been changed */
#define ZIP_ER_COMPNOTSUPP   16  /* N Compression method not supported */
#define ZIP_ER_EOF           17  /* N Premature EOF */
#define ZIP_ER_INVAL         18  /* N Invalid argument */
#define ZIP_ER_NOZIP         19  /* N Not a zip archive */
#define ZIP_ER_INTERNAL      20  /* N Internal error */
#define ZIP_ER_INCONS        21  /* N Zip archive inconsistent */
#define ZIP_ER_REMOVE        22  /* S Can't remove file */
#define ZIP_ER_DELETED       23  /* N Entry has been deleted */
#define ZIP_ER_CANCELLED     24  /* N Operation was cancelled */


/* type of system error value */

#define ZIP_ET_NONE	      0  /* sys_err unused */
#define ZIP_ET_SYS	      1  /* sys_err is errno */
#define ZIP_ET_ZLIB	      2  /* sys_err is zlib error code */

/* compression methods */

#define ZIP_CM_DEFAULT	      -1  /* better of deflate or store */
#define ZIP_CM_STORE	       0  /* stored (uncompressed) */
#define ZIP_CM_SHRINK	       1  /* shrunk */
#define ZIP_CM_REDUCE_1	       2  /* reduced with factor 1 */
#define ZIP_CM_REDUCE_2	       3  /* reduced with factor 2 */
#define ZIP_CM_REDUCE_3	       4  /* reduced with factor 3 */
#define ZIP_CM_REDUCE_4	       5  /* reduced with factor 4 */
#define ZIP_CM_IMPLODE	       6  /* imploded */
/* 7 - Reserved for Tokenizing compression algorithm */
#define ZIP_CM_DEFLATE	       8  /* deflated */
#define ZIP_CM_DEFLATE64       9  /* deflate64 */
#define ZIP_CM_PKWARE_IMPLODE 10  /* PKWARE imploding */



enum zip_source_cmd {
    ZIP_SOURCE_OPEN,	/* prepare for reading */
    ZIP_SOURCE_READ, 	/* read data */
    ZIP_SOURCE_CLOSE,	/* reading is done */
    ZIP_SOURCE_STAT,	/* get meta information */
    ZIP_SOURCE_ERROR,	/* get error information */
    ZIP_SOURCE_FREE	/* cleanup and free resources */
};

typedef ssize_t (*zip_source_callback)(void *state, void *data,
				       size_t len, enum zip_source_cmd cmd);


struct zip_stat {
    const char *name;			/* name of the file */
    int index;				/* index within archive */
    unsigned int crc;			/* crc of file data */
    time_t mtime;			/* modification time */
    off_t size;				/* size of file (uncompressed) */
    off_t comp_size;			/* size of file (compressed) */
    unsigned short comp_method;		/* compression method used */
};

struct zip;
struct zip_file;
struct zip_source;

/* Added 06-07-19 */
typedef int (*zip_progress_callback_t)(void *p_context, struct zip *p_archive, const char *p_item, 
						   int p_type, unsigned long p_item_progress, unsigned long p_item_total, 
						   unsigned long p_global_progress, unsigned long p_global_total);


int zip_add(struct zip *, const char *, struct zip_source *);
int zip_close(struct zip *);
int zip_delete(struct zip *, int);
const char *zip_get_path(const struct zip *);
void zip_error_get(struct zip *, int *, int *);
int zip_error_get_sys_type(int);
int zip_error_to_str(char *, size_t, int, int);
int zip_fclose(struct zip_file *);
void zip_file_error_get(struct zip_file *, int *, int *);
const char *zip_file_strerror(struct zip_file *);
struct zip_file *zip_fopen(struct zip *, const char *, int);
struct zip_file *zip_fopen_index(struct zip *, int, int);
ssize_t zip_fread(struct zip_file *, void *, size_t);
const char *zip_get_name(struct zip *, int, int);
int zip_get_num_files(struct zip *);
int zip_name_locate(struct zip *, const char *, int);
struct zip *zip_open(const char *, int, int *);
int zip_recompress(struct zip *, int, int);
int zip_rename(struct zip *, int, const char *);
int zip_set_attributes(struct zip *p_archive, int p_index, unsigned char p_madeby, unsigned int p_attributes);
int zip_get_attributes(struct zip *p_archive, int p_index, unsigned char *p_madeby, unsigned int *p_attributes);
int zip_replace(struct zip *, int, struct zip_source *);
struct zip_source *zip_source_buffer(struct zip *, const void *, off_t, int);
struct zip_source *zip_source_file(struct zip *, const char *, off_t, off_t);
struct zip_source *zip_source_filep(struct zip *, FILE *, off_t, off_t);
void zip_source_free(struct zip_source *);
struct zip_source *zip_source_function(struct zip *,
				       zip_source_callback, void *);
struct zip_source *zip_source_zip(struct zip *, struct zip *, int, int,
				  off_t, off_t);
int zip_stat(struct zip *, const char *, int, struct zip_stat *);
int zip_stat_index(struct zip *, int, int, struct zip_stat *);
const char *zip_strerror(struct zip *);
int zip_unchange(struct zip *, int);
int zip_unchange_all(struct zip *);

/* Added 06-07-19 */
struct zip_source *zip_source_filename(struct zip *, const char *, off_t, off_t);
/* Added 06-07-19 */
int zip_set_progress_callback(struct zip *p_archive, zip_progress_callback_t p_callback,  void *p_context);

#ifdef __cplusplus
}
#endif

#endif /* _HAD_ZIP_H */
