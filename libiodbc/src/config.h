#ifdef _LINUX
#include "config.linux.h"
#else
#define _MACX
#if defined(MAC_OS_X_VERSION_MIN_REQUIRED) && MAC_OS_X_VERSION_MIN_REQUIRED <= 1020
#define MACOSX102
#include "config.mac.ppc.h"
#else
#include "config.mac.x86.h"
#endif
#endif
