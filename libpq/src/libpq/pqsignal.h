/*-------------------------------------------------------------------------
 *
 * pqsignal.h
 *	  prototypes for the reliable BSD-style signal(2) routine.
 *
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/libpq/pqsignal.h,v 1.29 2004/12/31 22:03:32 pgsql Exp $
 *
 * NOTES
 *	  This shouldn't be in libpq, but the monitor and some other
 *	  things need it...
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQSIGNAL_H
#define PQSIGNAL_H

#include <signal.h>

#ifdef HAVE_SIGPROCMASK
extern sigset_t UnBlockSig,
			BlockSig,
			AuthBlockSig;

#define PG_SETMASK(mask)	sigprocmask(SIG_SETMASK, mask, NULL)
#else
extern int	UnBlockSig,
			BlockSig,
			AuthBlockSig;

#ifndef WIN32
#define PG_SETMASK(mask)	sigsetmask(*((int*)(mask)))
#else
#define PG_SETMASK(mask)		pqsigsetmask(*((int*)(mask)))
int			pqsigsetmask(int mask);
#endif
#endif

typedef void (*pqsigfunc) (int);

extern void pqinitmask(void);

extern pqsigfunc pqsignal(int signo, pqsigfunc func);

#endif   /* PQSIGNAL_H */
