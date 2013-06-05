/*
  $NiH: zip_source_filep.c,v 1.6 2005/06/09 19:57:10 dillo Exp $

  zip_source_filep.c -- create data source from FILE *
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



#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zip.h"
#include "zipint.h"

struct read_file {
    FILE *f;		/* file to copy from */
    off_t off;		/* start offset of */
    off_t len;		/* lengt of data to copy */
    off_t remain;	/* bytes remaining to be copied */
    int e[2];		/* error codes */
};

static ssize_t read_file(void *state, void *data, size_t len,
		     enum zip_source_cmd cmd);



struct zip_source *
zip_source_filep(struct zip *za, FILE *file, off_t start, off_t len)
{
    struct read_file *f;
    struct zip_source *zs;

    if (za == NULL)
	return NULL;

    if (file == NULL || start < 0 || len < -1) {
	_zip_error_set(&za->error, ZIP_ER_INVAL, 0);
	return NULL;
    }

    if ((f=(struct read_file *)malloc(sizeof(struct read_file))) == NULL) {
	_zip_error_set(&za->error, ZIP_ER_MEMORY, 0);
	return NULL;
    }

    f->f = file;
    f->off = start;
    f->len = (len ? len : -1);
    
    if ((zs=zip_source_function(za, read_file, f)) == NULL) {
	free(f);
	return NULL;
    }

    return zs;
}



static ssize_t
read_file(void *state, void *data, size_t len, enum zip_source_cmd cmd)
{
    struct read_file *z;
    char *buf;
    int i, n;

    z = (struct read_file *)state;
    buf = (char *)data;

    switch (cmd) {
    case ZIP_SOURCE_OPEN:
	if (fseeko(z->f, z->off, SEEK_SET) < 0) {
	    z->e[0] = ZIP_ER_SEEK;
	    z->e[1] = errno;
	    return -1;
	}
	z->remain = z->len;
	return 0;
	
    case ZIP_SOURCE_READ:
	if (z->remain != -1)
	    n = len > z->remain ? z->remain : len;
	else
	    n = len;
	
	if ((i=fread(buf, 1, n, z->f)) < 0) {
	    z->e[0] = ZIP_ER_READ;
	    z->e[1] = errno;
	    return -1;
	}

	if (z->remain != -1)
	    z->remain -= i;

	return i;
	
    case ZIP_SOURCE_CLOSE:
	return 0;

    case ZIP_SOURCE_STAT:
        {
	    struct zip_stat *st;
	    struct stat fst;
	    
	    if (len < sizeof(*st))
		return -1;

	    st = (struct zip_stat *)data;

	    if (fstat(fileno(z->f), &fst) != 0) {
		z->e[0] = ZIP_ER_READ; /* best match */
		z->e[1] = errno;
		return -1;
	    }

	    st->mtime = fst.st_mtime;
	    st->crc = 0;
	    if (z->len != -1)
		st->size = z->len;
	    else if ((fst.st_mode&S_IFMT) == S_IFREG)
		st->size = fst.st_size;
	    else
		st->size = -1;
	    st->comp_size = -1;
	    st->comp_method = ZIP_CM_STORE;

	    return sizeof(*st);
	}

    case ZIP_SOURCE_ERROR:
	if (len < sizeof(int)*2)
	    return -1;

	memcpy(data, z->e, sizeof(int)*2);
	return sizeof(int)*2;

    case ZIP_SOURCE_FREE:
	fclose(z->f);
	free(z);
	return 0;

    default:
	;
    }

    return -1;
}
