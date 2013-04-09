#include "zip.h"
#include "zipint.h"

int zip_get_attributes(struct zip *p_archive, int p_index, unsigned char *r_madeby, unsigned int *r_attributes)
{
    char *s;
    int i;
    
    if (p_index < 0 || p_index >= p_archive -> nentry)
	{
		_zip_error_set(&p_archive -> error, ZIP_ER_INVAL, 0);
		return -1;
    }

	*r_madeby = p_archive -> entry[p_index] . made_by;
	*r_attributes = p_archive -> entry[p_index] . ext_attrib;

    return 0;
}
