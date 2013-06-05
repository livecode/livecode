/* Copyright (C) 2000-2008 MySQL AB, 2008-2009 Sun Microsystems, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/*
  Static variables for mysys library. All definied here for easy making of
  a shared library
*/

#include "mysys_priv.h"
#include "my_static.h"
#include "my_alarm.h"

my_bool timed_mutexes= 0;

	/* from my_init */
char *	home_dir=0;
const char      *my_progname= NULL, *my_progname_short= NULL;
char		NEAR curr_dir[FN_REFLEN]= {0},
		NEAR home_dir_buff[FN_REFLEN]= {0};
ulong		my_stream_opened=0,my_file_opened=0, my_tmp_file_created=0;
ulong           my_file_total_opened= 0;
int		NEAR my_umask=0664, NEAR my_umask_dir=0777;
#ifndef THREAD
int		NEAR my_errno=0;
#endif
struct st_my_file_info my_file_info_default[MY_NFILE];
uint   my_file_limit= MY_NFILE;
struct st_my_file_info *my_file_info= my_file_info_default;

	/* From mf_brkhant */
int			NEAR my_dont_interrupt=0;
volatile int		_my_signals=0;
struct st_remember _my_sig_remember[MAX_SIGNALS]={{0,0}};
#ifdef THREAD
sigset_t my_signals;			/* signals blocked by mf_brkhant */
#endif

	/* from mf_reccache.c */
ulong my_default_record_cache_size=RECORD_CACHE_SIZE;

	/* from soundex.c */
				/* ABCDEFGHIJKLMNOPQRSTUVWXYZ */
				/* :::::::::::::::::::::::::: */
const char *soundex_map=	  "01230120022455012623010202";

	/* from my_malloc */
USED_MEM* my_once_root_block=0;			/* pointer to first block */
uint	  my_once_extra=ONCE_ALLOC_INIT;	/* Memory to alloc / block */

	/* from my_largepage.c */
#ifdef HAVE_LARGE_PAGES
my_bool my_use_large_pages= 0;
uint    my_large_page_size= 0;
#endif

/* ERROR INJECTION: Described in my_sys.h. */
IF_DBUG(int my_malloc_error_inject= 0);

	/* from safe_malloc */
uint sf_malloc_prehunc=0,		/* If you have problem with core- */
     sf_malloc_endhunc=0,		/* dump when malloc-message.... */
					/* set theese to 64 or 128  */
     sf_malloc_quick=0;			/* set if no calls to sanity */
size_t sf_malloc_cur_memory= 0L;		/* Current memory usage */
size_t sf_malloc_max_memory= 0L;		/* Maximum memory usage */
uint  sf_malloc_count= 0;		/* Number of times NEW() was called */
uchar *sf_min_adress= (uchar*) ~(unsigned long) 0L,
     *sf_max_adress= (uchar*) 0L;
/* Root of the linked list of struct st_irem */
struct st_irem *sf_malloc_root = NULL;

	/* from my_alarm */
int volatile my_have_got_alarm=0;	/* declare variable to reset */
ulong my_time_to_wait_for_lock=2;	/* In seconds */

	/* from errors.c */
#ifdef SHARED_LIBRARY
char * NEAR globerrs[GLOBERRS];		/* my_error_messages is here */
#endif
void (*my_abort_hook)(int) = (void(*)(int)) exit;
void (*error_handler_hook)(uint error,const char *str,myf MyFlags)=
    my_message_no_curses;
void (*fatal_error_handler_hook)(uint error,const char *str,myf MyFlags)=
  my_message_no_curses;

static const char *proc_info_dummy(void *a __attribute__((unused)),
                                   const char *b __attribute__((unused)),
                                   const char *c __attribute__((unused)),
                                   const char *d __attribute__((unused)),
                                   const unsigned int e __attribute__((unused)))
{
  return 0;
}

/* this is to be able to call set_thd_proc_info from the C code */
const char *(*proc_info_hook)(void *, const char *, const char *, const char *,
                              const unsigned int)= proc_info_dummy;

#if defined(ENABLED_DEBUG_SYNC)
/**
  Global pointer to be set if callback function is defined
  (e.g. in mysqld). See sql/debug_sync.cc.
*/
void (*debug_sync_C_callback_ptr)(const char *, size_t);
#endif /* defined(ENABLED_DEBUG_SYNC) */

#ifdef __WIN__
/* from my_getsystime.c */
ulonglong query_performance_frequency, query_performance_offset;
#endif

	/* How to disable options */
my_bool NEAR my_disable_locking=0;
my_bool NEAR my_disable_async_io=0;
my_bool NEAR my_disable_flush_key_blocks=0;
my_bool NEAR my_disable_symlinks=0;
my_bool NEAR mysys_uses_curses=0;
