// This file contains a weakly-linked implementation of gethostuuid,
// which just returns an all 0 uuid on Tiger. This allows SQLite to
// work on all platforms, with reduced features on 10.4.

#include <unistd.h>
#include <dlfcn.h>
#include <string.h>

typedef int (*gethostuuid_ptr_t)(uuid_t id, const struct timespec *wait);
static gethostuuid_ptr_t s_gethostuuid = NULL;
static unsigned int s_gethostuuid_initialized = 0;

int gethostuuid(uuid_t id, const struct timespec *wait)
{
	if (!s_gethostuuid_initialized)
	{
		void *t_module;
		t_module = dlopen("libSystem.dylib", RTLD_NOLOAD);
		if (t_module != NULL)
		{
			s_gethostuuid = (gethostuuid_ptr_t)dlsym(t_module, "gethostuuid");
			dlclose(t_module);
		}
		s_gethostuuid_initialized = 1;
	}
	
	if (s_gethostuuid == NULL)
	{
		memset(id, 0, sizeof(id));
		return 0;
	}
	
	return s_gethostuuid(id, wait);
}
