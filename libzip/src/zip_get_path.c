#include "zip.h"
#include "zipint.h"

const char *
zip_get_path(const struct zip *za)
{
    if (za == NULL)
	return NULL;

    return za->zn;
}
