/*
  $NiH: zip_stat_index.c,v 1.8 2004/12/22 16:32:00 dillo Exp $

  zip_stat_index.c -- get information about file by index
  Copyright (C) 1999, 2003, 2004 Dieter Baron and Thomas Klausner

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



int
zip_stat_index(struct zip *za, int index, int flags, struct zip_stat *st)
{
    const char *name;
    
    if (index < 0 || index >= za->nentry) {
	_zip_error_set(&za->error, ZIP_ER_INVAL, 0);
	return -1;
    }

    if ((name=zip_get_name(za, index, flags)) == NULL)
	return -1;
    

    if ((flags & ZIP_FL_UNCHANGED) == 0
	&& ZIP_ENTRY_DATA_CHANGED(za->entry+index)) {
	if (za->entry[index].source->f(za->entry[index].source->ud,
				     st, sizeof(*st), ZIP_SOURCE_STAT) < 0) {
	    _zip_error_set(&za->error, ZIP_ER_CHANGED, 0);
	    return -1;
	}
    }
    else {
	if (za->cdir == NULL || index >= za->cdir->nentry) {
	    _zip_error_set(&za->error, ZIP_ER_INVAL, 0);
	    return -1;
	}
	
	st->index = index;
	st->crc = za->cdir->entry[index].crc;
	st->size = za->cdir->entry[index].uncomp_size;
	st->mtime = za->cdir->entry[index].last_mod;
	st->comp_size = za->cdir->entry[index].comp_size;
	st->comp_method = za->cdir->entry[index].comp_method;
	/* st->bitflags = za->cdir->entry[index].bitflags; */
    }

    st->name = name;
    
    return 0;
}
