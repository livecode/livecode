#ifdef WIN32
#include "pg_config.win32.h"
#elif defined(_LINUX)
#include "pg_config.linux.h"
#else
#ifdef __ppc
#include "pg_config.mac.ppc.h"
#else
#include "pg_config.mac.i386.h"
#endif
#endif
