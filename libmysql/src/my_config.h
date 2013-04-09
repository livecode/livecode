#if defined(_LINUX)
#include "config-lnx.h"
#elif defined(_MACOSX)
#include "config-osx.h"
#elif defined(TARGET_SUBPLATFORM_IPHONE)
#include "config-ios.h"
#elif defined(TARGET_SUBPLATFORM_ANDROID)
#include "config-android.h"
#endif
