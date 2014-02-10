#ifndef __MC_THREAD__
#define __MC_THREAD__

#ifndef __MC_CORE__
#include "core.h"
#endif

////////////////////////////////////////////////////////////////////////////////

// A thread event is a simple primitive designed to be used by two (distinct)
// threads to communicate. The first thread 'waits' on the event, and then the
// second thread can trigger it at any time. Note that it is probably best that
// the waiting thread 'Reset' the event rather than the triggering thread. Indeed
// a usage pattern of:
//   Reset();
//   <do something to cause other thread to trigger>
//   Wait();
// Is the best way to use this object.

typedef struct MCThreadEvent *MCThreadEventRef;

bool MCThreadEventCreate(MCThreadEventRef& r_event);
void MCThreadEventDestroy(MCThreadEventRef event);

void MCThreadEventTrigger(MCThreadEventRef event);
void MCThreadEventReset(MCThreadEventRef event);
void MCThreadEventWait(MCThreadEventRef event);

////////////////////////////////////////////////////////////////////////////////

#endif
