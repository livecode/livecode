#ifdef _WINDOWS

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>

int mkstemp(char* p_template)
{
	char *t_filename;

	t_filename = _mktemp(p_template);
	if (t_filename == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	return _open(t_filename, _O_RDWR|_O_BINARY|_O_CREAT|_O_EXCL, 0600);
}

#endif //_WINDOWS
