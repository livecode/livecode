#include "zip.h"
#include "zipint.h"

int zip_set_attributes(struct zip *p_archive, int p_index, unsigned char p_madeby, unsigned int p_attributes)
{
    char *s;
    int i;
    
    if (p_index < 0 || p_index >= p_archive -> nentry)
	{
		_zip_error_set(&p_archive -> error, ZIP_ER_INVAL, 0);
		return -1;
    }
    
	// Set the state to renamed when changing attributes as this will cause the
	// central directory to be rewritten to the archive, which is what we want.
    if (p_archive -> entry[p_index] . state == ZIP_ST_UNCHANGED) 
		p_archive -> entry[p_index]. state = ZIP_ST_RENAMED;

	p_archive -> entry[p_index] . made_by = p_madeby;
	p_archive -> entry[p_index] . ext_attrib = p_attributes;

    return 0;
}
