/*-------------------------------------------------------------------------
 *
 * libpq-int.h
 *	  This file contains internal definitions meant to be used only by
 *	  the frontend libpq library, not by applications that call it.
 *
 *	  An application can include this file if it wants to bypass the
 *	  official API defined by libpq-fe.h, but code that does so is much
 *	  more likely to break across PostgreSQL releases than code that uses
 *	  only the official API.
 *
 * Portions Copyright (c) 1996-2005, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/interfaces/libpq/libpq-int.h,v 1.108.2.1 2005/11/22 18:23:30 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */

#ifndef LIBPQ_INT_H
#define LIBPQ_INT_H

/* We assume libpq-fe.h has already been included. */
#include "postgres_fe.h"

#include <time.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/time.h>
#endif

#ifdef ENABLE_THREAD_SAFETY
#ifdef WIN32
#include "pthread-win32.h"
#else
#include <pthread.h>
#endif
#include <signal.h>
#endif

#ifdef WIN32_CLIENT_ONLY
typedef int ssize_t;			/* ssize_t doesn't exist in VC (at least not
								 * VC6) */
#endif

/* include stuff common to fe and be */
#include "getaddrinfo.h"
#include "libpq/pqcomm.h"
/* include stuff found in fe only */
#include "pqexpbuffer.h"

#ifdef USE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

/*
 * POSTGRES backend dependent Constants.
 */
#define PQERRORMSG_LENGTH 1024
#define CMDSTATUS_LEN 40

/*
 * PGresult and the subsidiary types PGresAttDesc, PGresAttValue
 * represent the result of a query (or more precisely, of a single SQL
 * command --- a query string given to PQexec can contain multiple commands).
 * Note we assume that a single command can return at most one tuple group,
 * hence there is no need for multiple descriptor sets.
 */

/* Subsidiary-storage management structure for PGresult.
 * See space management routines in fe-exec.c for details.
 * Note that space[k] refers to the k'th byte starting from the physical
 * head of the block --- it's a union, not a struct!
 */
typedef union pgresult_data PGresult_data;

union pgresult_data
{
	PGresult_data *next;		/* link to next block, or NULL */
	char		space[1];		/* dummy for accessing block as bytes */
};

/* Data about a single attribute (column) of a query result */

typedef struct pgresAttDesc
{
	char	   *name;			/* column name */
	Oid			tableid;		/* source table, if known */
	int			columnid;		/* source column, if known */
	int			format;			/* format code for value (text/binary) */
	Oid			typid;			/* type id */
	int			typlen;			/* type size */
	int			atttypmod;		/* type-specific modifier info */
} PGresAttDesc;

/*
 * Data for a single attribute of a single tuple
 *
 * We use char* for Attribute values.
 *
 * The value pointer always points to a null-terminated area; we add a
 * null (zero) byte after whatever the backend sends us.  This is only
 * particularly useful for text values ... with a binary value, the
 * value might have embedded nulls, so the application can't use C string
 * operators on it.  But we add a null anyway for consistency.
 * Note that the value itself does not contain a length word.
 *
 * A NULL attribute is a special case in two ways: its len field is NULL_LEN
 * and its value field points to null_field in the owning PGresult.  All the
 * NULL attributes in a query result point to the same place (there's no need
 * to store a null string separately for each one).
 */

#define NULL_LEN		(-1)	/* pg_result len for NULL value */

typedef struct pgresAttValue
{
	int			len;			/* length in bytes of the value */
	char	   *value;			/* actual value, plus terminating zero byte */
} PGresAttValue;

/* Typedef for message-field list entries */
typedef struct pgMessageField
{
	struct pgMessageField *next;	/* list link */
	char		code;			/* field code */
	char		contents[1];	/* field value (VARIABLE LENGTH) */
} PGMessageField;

/* Fields needed for notice handling */
typedef struct
{
	PQnoticeReceiver noticeRec; /* notice message receiver */
	void	   *noticeRecArg;
	PQnoticeProcessor noticeProc;		/* notice message processor */
	void	   *noticeProcArg;
} PGNoticeHooks;

struct pg_result
{
	int			ntups;
	int			numAttributes;
	PGresAttDesc *attDescs;
	PGresAttValue **tuples;		/* each PGresTuple is an array of
								 * PGresAttValue's */
	int			tupArrSize;		/* allocated size of tuples array */
	ExecStatusType resultStatus;
	char		cmdStatus[CMDSTATUS_LEN];		/* cmd status from the query */
	int			binary;			/* binary tuple values if binary == 1,
								 * otherwise text */

	/*
	 * These fields are copied from the originating PGconn, so that operations
	 * on the PGresult don't have to reference the PGconn.
	 */
	PGNoticeHooks noticeHooks;
	int			client_encoding;	/* encoding id */

	/*
	 * Error information (all NULL if not an error result).  errMsg is the
	 * "overall" error message returned by PQresultErrorMessage.  If we have
	 * per-field info then it is stored in a linked list.
	 */
	char	   *errMsg;			/* error message, or NULL if no error */
	PGMessageField *errFields;	/* message broken into fields */

	/* All NULL attributes in the query result point to this null string */
	char		null_field[1];

	/*
	 * Space management information.  Note that attDescs and error stuff, if
	 * not null, point into allocated blocks.  But tuples points to a
	 * separately malloc'd block, so that we can realloc it.
	 */
	PGresult_data *curBlock;	/* most recently allocated block */
	int			curOffset;		/* start offset of free space in block */
	int			spaceLeft;		/* number of free bytes remaining in block */
};

/* PGAsyncStatusType defines the state of the query-execution state machine */
typedef enum
{
	PGASYNC_IDLE,				/* nothing's happening, dude */
	PGASYNC_BUSY,				/* query in progress */
	PGASYNC_READY,				/* result ready for PQgetResult */
	PGASYNC_COPY_IN,			/* Copy In data transfer in progress */
	PGASYNC_COPY_OUT			/* Copy Out data transfer in progress */
} PGAsyncStatusType;

/* PGQueryClass tracks which query protocol we are now executing */
typedef enum
{
	PGQUERY_SIMPLE,				/* simple Query protocol (PQexec) */
	PGQUERY_EXTENDED,			/* full Extended protocol (PQexecParams) */
	PGQUERY_PREPARE				/* Parse only (PQprepare) */
} PGQueryClass;

/* PGSetenvStatusType defines the state of the PQSetenv state machine */
/* (this is used only for 2.0-protocol connections) */
typedef enum
{
	SETENV_STATE_OPTION_SEND,	/* About to send an Environment Option */
	SETENV_STATE_OPTION_WAIT,	/* Waiting for above send to complete */
	SETENV_STATE_QUERY1_SEND,	/* About to send a status query */
	SETENV_STATE_QUERY1_WAIT,	/* Waiting for query to complete */
	SETENV_STATE_QUERY2_SEND,	/* About to send a status query */
	SETENV_STATE_QUERY2_WAIT,	/* Waiting for query to complete */
	SETENV_STATE_IDLE
} PGSetenvStatusType;

/* Typedef for the EnvironmentOptions[] array */
typedef struct PQEnvironmentOption
{
	const char *envName,		/* name of an environment variable */
			   *pgName;			/* name of corresponding SET variable */
} PQEnvironmentOption;

/* Typedef for parameter-status list entries */
typedef struct pgParameterStatus
{
	struct pgParameterStatus *next;		/* list link */
	char	   *name;			/* parameter name */
	char	   *value;			/* parameter value */
	/* Note: name and value are stored in same malloc block as struct is */
} pgParameterStatus;

/* large-object-access data ... allocated only if large-object code is used. */
typedef struct pgLobjfuncs
{
	Oid			fn_lo_open;		/* OID of backend function lo_open		*/
	Oid			fn_lo_close;	/* OID of backend function lo_close		*/
	Oid			fn_lo_creat;	/* OID of backend function lo_creat		*/
	Oid			fn_lo_create;	/* OID of backend function lo_create	*/
	Oid			fn_lo_unlink;	/* OID of backend function lo_unlink	*/
	Oid			fn_lo_lseek;	/* OID of backend function lo_lseek		*/
	Oid			fn_lo_tell;		/* OID of backend function lo_tell		*/
	Oid			fn_lo_read;		/* OID of backend function LOread		*/
	Oid			fn_lo_write;	/* OID of backend function LOwrite		*/
} PGlobjfuncs;

/*
 * PGconn stores all the state data associated with a single connection
 * to a backend.
 */
struct pg_conn
{
	/* Saved values of connection options */
	char	   *pghost;			/* the machine on which the server is running */
	char	   *pghostaddr;		/* the IPv4 address of the machine on which
								 * the server is running, in IPv4
								 * numbers-and-dots notation. Takes precedence
								 * over above. */
	char	   *pgport;			/* the server's communication port */
	char	   *pgunixsocket;	/* the Unix-domain socket that the server is
								 * listening on; if NULL, uses a default
								 * constructed from pgport */
	char	   *pgtty;			/* tty on which the backend messages is
								 * displayed (OBSOLETE, NOT USED) */
	char	   *connect_timeout;	/* connection timeout (numeric string) */
	char	   *pgoptions;		/* options to start the backend with */
	char	   *dbName;			/* database name */
	char	   *pguser;			/* Postgres username and password, if any */
	char	   *pgpass;
	char	   *sslmode;		/* SSL mode (require,prefer,allow,disable) */
#ifdef KRB5
	char	   *krbsrvname;		/* Kerberos service name */
#endif

	/* Optional file to write trace info to */
	FILE	   *Pfdebug;

	/* Callback procedures for notice message processing */
	PGNoticeHooks noticeHooks;

	/* Status indicators */
	ConnStatusType status;
	PGAsyncStatusType asyncStatus;
	PGTransactionStatusType xactStatus;
	/* note: xactStatus never changes to ACTIVE */
	PGQueryClass queryclass;
	bool		nonblocking;	/* whether this connection is using nonblock
								 * sending semantics */
	char		copy_is_binary; /* 1 = copy binary, 0 = copy text */
	int			copy_already_done;		/* # bytes already returned in COPY
										 * OUT */
	PGnotify   *notifyHead;		/* oldest unreported Notify msg */
	PGnotify   *notifyTail;		/* newest unreported Notify msg */

	/* Connection data */
	int			sock;			/* Unix FD for socket, -1 if not connected */
	SockAddr	laddr;			/* Local address */
	SockAddr	raddr;			/* Remote address */
	ProtocolVersion pversion;	/* FE/BE protocol version in use */
	int			sversion;		/* server version, e.g. 70401 for 7.4.1 */

	/* Transient state needed while establishing connection */
	struct addrinfo *addrlist;	/* list of possible backend addresses */
	struct addrinfo *addr_cur;	/* the one currently being tried */
	int			addrlist_family;	/* needed to know how to free addrlist */
	PGSetenvStatusType setenv_state;	/* for 2.0 protocol only */
	const PQEnvironmentOption *next_eo;

	/* Miscellaneous stuff */
	int			be_pid;			/* PID of backend --- needed for cancels */
	int			be_key;			/* key of backend --- needed for cancels */
	char		md5Salt[4];		/* password salt received from backend */
	char		cryptSalt[2];	/* password salt received from backend */
	pgParameterStatus *pstatus; /* ParameterStatus data */
	int			client_encoding;	/* encoding id */
	PGVerbosity verbosity;		/* error/notice message verbosity */
	PGlobjfuncs *lobjfuncs;		/* private state for large-object access fns */

	/* Buffer for data received from backend and not yet processed */
	char	   *inBuffer;		/* currently allocated buffer */
	int			inBufSize;		/* allocated size of buffer */
	int			inStart;		/* offset to first unconsumed data in buffer */
	int			inCursor;		/* next byte to tentatively consume */
	int			inEnd;			/* offset to first position after avail data */

	/* Buffer for data not yet sent to backend */
	char	   *outBuffer;		/* currently allocated buffer */
	int			outBufSize;		/* allocated size of buffer */
	int			outCount;		/* number of chars waiting in buffer */

	/* State for constructing messages in outBuffer */
	int			outMsgStart;	/* offset to msg start (length word); if -1,
								 * msg has no length word */
	int			outMsgEnd;		/* offset to msg end (so far) */

	/* Status for asynchronous result construction */
	PGresult   *result;			/* result being constructed */
	PGresAttValue *curTuple;	/* tuple currently being read */

#ifdef USE_SSL
	bool		allow_ssl_try;	/* Allowed to try SSL negotiation */
	bool		wait_ssl_try;	/* Delay SSL negotiation until after
								 * attempting normal connection */
	SSL		   *ssl;			/* SSL status, if have SSL connection */
	X509	   *peer;			/* X509 cert of server */
	char		peer_dn[256 + 1];		/* peer distinguished name */
	char		peer_cn[SM_USER + 1];	/* peer common name */
#endif

	/* Buffer for current error message */
	PQExpBufferData errorMessage;		/* expansible string */

	/* Buffer for receiving various parts of messages */
	PQExpBufferData workBuffer; /* expansible string */
};

/* PGcancel stores all data necessary to cancel a connection. A copy of this
 * data is required to safely cancel a connection running on a different
 * thread.
 */
struct pg_cancel
{
	SockAddr	raddr;			/* Remote address */
	int			be_pid;			/* PID of backend --- needed for cancels */
	int			be_key;			/* key of backend --- needed for cancels */
};


/* String descriptions of the ExecStatusTypes.
 * direct use of this array is deprecated; call PQresStatus() instead.
 */
extern char *const pgresStatus[];

/* ----------------
 * Internal functions of libpq
 * Functions declared here need to be visible across files of libpq,
 * but are not intended to be called by applications.  We use the
 * convention "pqXXX" for internal functions, vs. the "PQxxx" names
 * used for application-visible routines.
 * ----------------
 */

/* === in fe-connect.c === */

extern int pqPacketSend(PGconn *conn, char pack_type,
			 const void *buf, size_t buf_len);
extern bool pqGetHomeDirectory(char *buf, int bufsize);

#ifdef ENABLE_THREAD_SAFETY
extern pgthreadlock_t pg_g_threadlock;

#define pglock_thread()		pg_g_threadlock(true)
#define pgunlock_thread()	pg_g_threadlock(false)
#else
#define pglock_thread()		((void) 0)
#define pgunlock_thread()	((void) 0)
#endif


/* === in fe-exec.c === */

extern void pqSetResultError(PGresult *res, const char *msg);
extern void pqCatenateResultError(PGresult *res, const char *msg);
extern void *pqResultAlloc(PGresult *res, size_t nBytes, bool isBinary);
extern char *pqResultStrdup(PGresult *res, const char *str);
extern void pqClearAsyncResult(PGconn *conn);
extern void pqSaveErrorResult(PGconn *conn);
extern PGresult *pqPrepareAsyncResult(PGconn *conn);
extern void
pqInternalNotice(const PGNoticeHooks *hooks, const char *fmt,...)
/* This lets gcc check the format string for consistency. */
__attribute__((format(printf, 2, 3)));
extern int	pqAddTuple(PGresult *res, PGresAttValue *tup);
extern void pqSaveMessageField(PGresult *res, char code,
				   const char *value);
extern void pqSaveParameterStatus(PGconn *conn, const char *name,
					  const char *value);
extern void pqHandleSendFailure(PGconn *conn);

/* === in fe-protocol2.c === */

extern PostgresPollingStatusType pqSetenvPoll(PGconn *conn);

extern char *pqBuildStartupPacket2(PGconn *conn, int *packetlen,
					  const PQEnvironmentOption *options);
extern void pqParseInput2(PGconn *conn);
extern int	pqGetCopyData2(PGconn *conn, char **buffer, int async);
extern int	pqGetline2(PGconn *conn, char *s, int maxlen);
extern int	pqGetlineAsync2(PGconn *conn, char *buffer, int bufsize);
extern int	pqEndcopy2(PGconn *conn);
extern PGresult *pqFunctionCall2(PGconn *conn, Oid fnid,
				int *result_buf, int *actual_result_len,
				int result_is_int,
				const PQArgBlock *args, int nargs);

/* === in fe-protocol3.c === */

extern char *pqBuildStartupPacket3(PGconn *conn, int *packetlen,
					  const PQEnvironmentOption *options);
extern void pqParseInput3(PGconn *conn);
extern int	pqGetErrorNotice3(PGconn *conn, bool isError);
extern int	pqGetCopyData3(PGconn *conn, char **buffer, int async);
extern int	pqGetline3(PGconn *conn, char *s, int maxlen);
extern int	pqGetlineAsync3(PGconn *conn, char *buffer, int bufsize);
extern int	pqEndcopy3(PGconn *conn);
extern PGresult *pqFunctionCall3(PGconn *conn, Oid fnid,
				int *result_buf, int *actual_result_len,
				int result_is_int,
				const PQArgBlock *args, int nargs);

/* === in fe-misc.c === */

 /*
  * "Get" and "Put" routines return 0 if successful, EOF if not. Note that for
  * Get, EOF merely means the buffer is exhausted, not that there is
  * necessarily any error.
  */
extern int	pqCheckOutBufferSpace(int bytes_needed, PGconn *conn);
extern int	pqCheckInBufferSpace(int bytes_needed, PGconn *conn);
extern int	pqGetc(char *result, PGconn *conn);
extern int	pqPutc(char c, PGconn *conn);
extern int	pqGets(PQExpBuffer buf, PGconn *conn);
extern int	pqPuts(const char *s, PGconn *conn);
extern int	pqGetnchar(char *s, size_t len, PGconn *conn);
extern int	pqPutnchar(const char *s, size_t len, PGconn *conn);
extern int	pqGetInt(int *result, size_t bytes, PGconn *conn);
extern int	pqPutInt(int value, size_t bytes, PGconn *conn);
extern int	pqPutMsgStart(char msg_type, bool force_len, PGconn *conn);
extern int	pqPutMsgEnd(PGconn *conn);
extern int	pqReadData(PGconn *conn);
extern int	pqFlush(PGconn *conn);
extern int	pqWait(int forRead, int forWrite, PGconn *conn);
extern int pqWaitTimed(int forRead, int forWrite, PGconn *conn,
			time_t finish_time);
extern int	pqReadReady(PGconn *conn);
extern int	pqWriteReady(PGconn *conn);

/* === in fe-secure.c === */

extern int	pqsecure_initialize(PGconn *);
extern void pqsecure_destroy(void);
extern PostgresPollingStatusType pqsecure_open_client(PGconn *);
extern void pqsecure_close(PGconn *);
extern ssize_t pqsecure_read(PGconn *, void *ptr, size_t len);
extern ssize_t pqsecure_write(PGconn *, const void *ptr, size_t len);

#if defined(ENABLE_THREAD_SAFETY) && !defined(WIN32)
extern int	pq_block_sigpipe(sigset_t *osigset, bool *sigpipe_pending);
extern void pq_reset_sigpipe(sigset_t *osigset, bool sigpipe_pending,
				 bool got_epipe);
#endif

/*
 * this is so that we can check if a connection is non-blocking internally
 * without the overhead of a function call
 */
#define pqIsnonblocking(conn)	((conn)->nonblocking)

#ifdef ENABLE_NLS
extern char *
libpq_gettext(const char *msgid)
__attribute__((format_arg(1)));
#else
#define libpq_gettext(x) (x)
#endif

/*
 * These macros are needed to let error-handling code be portable between
 * Unix and Windows.  (ugh)
 */
#ifdef WIN32
#define SOCK_ERRNO (WSAGetLastError())
#define SOCK_STRERROR winsock_strerror
#define SOCK_ERRNO_SET(e) WSASetLastError(e)
#else
#define SOCK_ERRNO errno
#define SOCK_STRERROR pqStrerror
#define SOCK_ERRNO_SET(e) (errno = (e))
#endif

#endif   /* LIBPQ_INT_H */
