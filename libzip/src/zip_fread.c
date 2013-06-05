/*
  $NiH: zip_fread.c,v 1.18 2005/07/14 16:20:56 wiz Exp $

  zip_fread.c -- read from file
  Copyright (C) 1999, 2004, 2005 Dieter Baron and Thomas Klausner

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



#include "zip.h"
#include "zipint.h"



ssize_t
zip_fread(struct zip_file *zf, void *outbuf, size_t toread)
{
    int out_before, ret;
    int len;

    if (!zf)
	return -1;

    if (zf->error.zip_err != 0)
	return -1;

    if ((zf->flags & ZIP_ZF_EOF) || (toread == 0))
	return 0;

    if (zf->bytes_left == 0) {
	zf->flags |= ZIP_ZF_EOF;
	if (zf->flags & ZIP_ZF_CRC) {
	    if (zf->crc != zf->crc_orig) {
		_zip_error_set(&zf->error, ZIP_ER_CRC, 0);
		return -1;
	    }
	}
	return 0;
    }
    
    if ((zf->flags & ZIP_ZF_DECOMP) == 0) {
	ret = _zip_file_fillbuf(outbuf, toread, zf);
	if (ret > 0) {
	    if (zf->flags & ZIP_ZF_CRC)
		zf->crc = crc32(zf->crc, outbuf, ret);
	    zf->bytes_left -= ret;
	}

	if (zf -> za -> callback != NULL)
	    zf -> za -> callback(zf -> za -> callback_context,
				 zf -> za,
				 zf -> za -> cdir -> entry[zf -> index] . filename,
				 0,
				 zf -> za -> cdir -> entry[zf -> index] . uncomp_size - zf -> bytes_left,
				 zf -> za -> cdir -> entry[zf -> index] . uncomp_size,
				 zf -> za -> cdir -> entry[zf -> index] . uncomp_size - zf -> bytes_left,
				 zf -> za -> cdir -> entry[zf -> index] . uncomp_size);

	return ret;
    }
    
    zf->zstr->next_out = outbuf;
    zf->zstr->avail_out = toread;
    out_before = zf->zstr->total_out;
    
    /* endless loop until something has been accomplished */
    for (;;) {
	ret = inflate(zf->zstr, Z_SYNC_FLUSH);

	switch (ret) {
	case Z_OK:
	case Z_STREAM_END:
	    /* all ok */
	    /* Z_STREAM_END probably won't happen, since we didn't
	       have a header */
	    len = zf->zstr->total_out - out_before;
	    if (len >= zf->bytes_left || len >= toread) {
		if (zf->flags & ZIP_ZF_CRC)
		    zf->crc = crc32(zf->crc, outbuf, len);
		zf->bytes_left -= len;

		if (zf -> za -> callback != NULL)
		    zf -> za -> callback(zf -> za -> callback_context,
					 zf -> za,
					 zf -> za -> cdir -> entry[zf -> index] . filename,
					 0,
					 zf -> za -> cdir -> entry[zf -> index] . uncomp_size - zf -> bytes_left,
					 zf -> za -> cdir -> entry[zf -> index] . uncomp_size,
					 zf -> za -> cdir -> entry[zf -> index] . uncomp_size - zf -> bytes_left,
					 zf -> za -> cdir -> entry[zf -> index] . uncomp_size);

	        return len;
	    }
	    break;

	case Z_BUF_ERROR:
	    if (zf->zstr->avail_in == 0) {
		len = _zip_file_fillbuf(zf->buffer, BUFSIZE, zf);
		if (len == 0) {
		    _zip_error_set(&zf->error, ZIP_ER_INCONS, 0);
		    return -1;
		}
		else if (len < 0)
		    return -1;
		zf->zstr->next_in = (Bytef *)zf->buffer;
		zf->zstr->avail_in = len;
		continue;
	    }
	    /* fallthrough */
	case Z_NEED_DICT:
	case Z_DATA_ERROR:
	case Z_STREAM_ERROR:
	case Z_MEM_ERROR:
	    _zip_error_set(&zf->error, ZIP_ER_ZLIB, ret);
	    return -1;
	}
    }
}
