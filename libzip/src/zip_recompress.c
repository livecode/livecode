#include "zip.h"
#include "zipint.h"



int
zip_recompress(struct zip *za, int idx, int compression)
{
    if (idx >= za->nentry || idx < 0) {
	_zip_error_set(&za->error, ZIP_ER_INVAL, 0);
	return -1;
    }

    if (za -> entry[idx].state != ZIP_ST_ADDED && za -> entry[idx] . state != ZIP_ST_REPLACED)
        return -1;

    if (compression == za->entry[idx] . ch_method)
        return 0;

    za->entry[idx] . ch_method = compression;

    return 0;
}
