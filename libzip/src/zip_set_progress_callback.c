#include "zip.h"
#include "zipint.h"

int zip_set_progress_callback(struct zip *p_archive, zip_progress_callback_t p_callback, void *p_context)
{
	if (p_archive != NULL)
	{
		p_archive->callback = p_callback;
		p_archive->callback_context = p_context;
		return 0;
	}
	
	return 1;
}
