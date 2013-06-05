/* src/include/pg_config.h.in.  Generated from configure.in by autoheader.  */

/* Define to the type of arg 1 of 'accept' */
#define ACCEPT_TYPE_ARG1 int

/* Define to the type of arg 2 of 'accept' */
#define ACCEPT_TYPE_ARG2 struct sockaddr *

/* Define to the type of arg 3 of 'accept' */
#define ACCEPT_TYPE_ARG3 socklen_t *

/* Define to the return type of 'accept' */
#define ACCEPT_TYPE_RETURN int

/* The alignment requirement of a `double'. */
#undef ALIGNOF_DOUBLE 8

/* The alignment requirement of a `int'. */
#undef ALIGNOF_INT 4

/* The alignment requirement of a `long'. */
#undef ALIGNOF_LONG 4 

/* The alignment requirement of a `long long int'. */
#undef ALIGNOF_LONG_LONG_INT 8

/* The alignment requirement of a `short'. */
#undef ALIGNOF_SHORT 4

/* Define to the default TCP port number on which the server listens and to
   which clients will try to connect. This can be overridden at run-time, but
   it's convenient if your clients have the right default compiled in.
   (--with-pgport=PORTNUM) */
#undef DEF_PGPORT

/* Define to the default TCP port number as a string constant. */
#undef DEF_PGPORT_STR

/* Define to 1 if you want National Language Support. (--enable-nls) */
#undef ENABLE_NLS

/* Define to 1 to build client libraries as thread-safe code.
   (--enable-thread-safety) */
#undef ENABLE_THREAD_SAFETY

/* Define to 1 if getpwuid_r() takes a 5th argument. */
#undef GETPWUID_R_5ARG

/* Define to 1 if gettimeofday() takes only 1 argument. */
#undef GETTIMEOFDAY_1ARG

#ifdef GETTIMEOFDAY_1ARG
# define gettimeofday(a,b) gettimeofday(a)
#endif

/* Define to 1 if you have the `atexit' function. */
#define HAVE_ATEXIT

/* Define to 1 if you have the `cbrt' function. */
#undef HAVE_CBRT

/* Define to 1 if you have the `class' function. */
#undef HAVE_CLASS

/* Define to 1 if you have the `crypt' function. */
#define HAVE_CRYPT

/* Define to 1 if you have the <crypt.h> header file. */
#undef HAVE_CRYPT_H

/* Define to 1 if you have the declaration of `fdatasync', and to 0 if you
   don't. */
#undef HAVE_DECL_FDATASYNC

/* Define to 1 if you have the declaration of `F_FULLFSYNC', and to 0 if you
   don't. */
#undef HAVE_DECL_F_FULLFSYNC

/* Define to 1 if you have the declaration of `snprintf', and to 0 if you
   don't. */
#define HAVE_DECL_SNPRINTF 1

/* Define to 1 if you have the declaration of `vsnprintf', and to 0 if you
   don't. */
#define HAVE_DECL_VSNPRINTF 1

/* Define to 1 if you have the <dld.h> header file. */
#undef HAVE_DLD_H

/* Define to 1 if you have the `dlopen' function. */
#define HAVE_DLOPEN

/* Define to 1 if you have the <editline/history.h> header file. */
#undef HAVE_EDITLINE_HISTORY_H

/* Define to 1 if you have the <editline/readline.h> header file. */
#undef HAVE_EDITLINE_READLINE_H

/* Define to 1 if you have the <endian.h> header file. */
#define HAVE_ENDIAN_H

/* Define to 1 if you have the `fcvt' function. */
#define HAVE_FCVT

/* Define to 1 if you have the `fdatasync' function. */
#undef HAVE_FDATASYNC

/* Define to 1 if you have finite(). */
#undef HAVE_FINITE

/* Define to 1 if you have the `fpclass' function. */
#undef HAVE_FPCLASS

/* Define to 1 if you have the `fp_class' function. */
#undef HAVE_FP_CLASS

/* Define to 1 if you have the `fp_class_d' function. */
#undef HAVE_FP_CLASS_D

/* Define to 1 if you have the <fp_class.h> header file. */
#undef HAVE_FP_CLASS_H

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#undef HAVE_FSEEKO

/* Define to 1 if your compiler understands __func__. */
#undef HAVE_FUNCNAME__FUNC

/* Define to 1 if your compiler understands __FUNCTION__. */
#undef HAVE_FUNCNAME__FUNCTION

/* Define to 1 if you have the `getaddrinfo' function. */
#undef HAVE_GETADDRINFO

/* Define to 1 if you have the `gethostbyname_r' function. */
#undef HAVE_GETHOSTBYNAME_R

/* Define to 1 if you have the `getopt' function. */
#undef HAVE_GETOPT

/* Define to 1 if you have the <getopt.h> header file. */
#undef HAVE_GETOPT_H

/* Define to 1 if you have the `getopt_long' function. */
#undef HAVE_GETOPT_LONG

/* Define to 1 if you have the `getpeereid' function. */
#undef HAVE_GETPEEREID

/* Define to 1 if you have the `getpwuid_r' function. */
#undef HAVE_GETPWUID_R

/* Define to 1 if you have the `getrusage' function. */
#undef HAVE_GETRUSAGE

/* Define to 1 if you have the <history.h> header file. */
#undef HAVE_HISTORY_H

/* Define to 1 if you have the <ieeefp.h> header file. */
#undef HAVE_IEEEFP_H

/* Define to 1 if you have the `inet_aton' function. */
#undef HAVE_INET_ATON

/* Define to 1 if the system has the type `int64'. */
#undef HAVE_INT64

/* Define to 1 if the system has the type `int8'. */
#undef HAVE_INT8

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define to 1 if you have the global variable 'int optreset'. */
#undef HAVE_INT_OPTRESET

/* Define to 1 if you have the global variable 'int timezone'. */
#undef HAVE_INT_TIMEZONE

/* Define to 1 if you have support for IPv6. */
#undef HAVE_IPV6

/* Define to 1 if you have isinf(). */
#undef HAVE_ISINF

/* Define to 1 if you have the <kernel/image.h> header file. */
#undef HAVE_KERNEL_IMAGE_H

/* Define to 1 if you have the <kernel/OS.h> header file. */
#undef HAVE_KERNEL_OS_H

/* Define to 1 if `e_data' is member of `krb5_error'. */
#undef HAVE_KRB5_ERROR_E_DATA

/* Define to 1 if `text.data' is member of `krb5_error'. */
#undef HAVE_KRB5_ERROR_TEXT_DATA

/* Define to 1 if `client' is member of `krb5_ticket'. */
#undef HAVE_KRB5_TICKET_CLIENT

/* Define to 1 if `enc_part2' is member of `krb5_ticket'. */
#undef HAVE_KRB5_TICKET_ENC_PART2

/* Define to 1 if you have the <langinfo.h> header file. */
#undef HAVE_LANGINFO_H

/* Define to 1 if you have the `bind' library (-lbind). */
#undef HAVE_LIBBIND

/* Define to 1 if you have the `BSD' library (-lBSD). */
#undef HAVE_LIBBSD

/* Define to 1 if you have the `compat' library (-lcompat). */
#undef HAVE_LIBCOMPAT

/* Define to 1 if you have the `crypto' library (-lcrypto). */
#define HAVE_LIBCRYPTO

/* Define to 1 if you have the `cygipc' library (-lcygipc). */
#undef HAVE_LIBCYGIPC

/* Define to 1 if you have the `dl' library (-ldl). */
#define HAVE_LIBDL

/* Define to 1 if you have the `dld' library (-ldld). */
#undef HAVE_LIBDLD

/* Define to 1 if you have the `eay32' library (-leay32). */
#undef HAVE_LIBEAY32

/* Define to 1 if you have the `gen' library (-lgen). */
#undef HAVE_LIBGEN

/* Define to 1 if you have the `IPC' library (-lIPC). */
#undef HAVE_LIBIPC

/* Define to 1 if you have the `lc' library (-llc). */
#undef HAVE_LIBLC

/* Define to 1 if you have the `ld' library (-lld). */
#undef HAVE_LIBLD

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM

/* Define to 1 if you have the `nsl' library (-lnsl). */
#undef HAVE_LIBNSL

/* Define to 1 if you have the `pam' library (-lpam). */
#undef HAVE_LIBPAM

/* Define to 1 if you have the `PW' library (-lPW). */
#undef HAVE_LIBPW

/* Define if you have a function readline library */
#undef HAVE_LIBREADLINE

/* Define to 1 if you have the `resolv' library (-lresolv). */
#define HAVE_LIBRESOLV

/* Define to 1 if you have the `socket' library (-lsocket). */
#undef HAVE_LIBSOCKET

/* Define to 1 if you have the `ssl' library (-lssl). */
#undef HAVE_LIBSSL

/* Define to 1 if you have the `ssleay32' library (-lssleay32). */
#undef HAVE_LIBSSLEAY32

/* Define to 1 if you have the `unix' library (-lunix). */
#undef HAVE_LIBUNIX

/* Define to 1 if you have the `util' library (-lutil). */
#undef HAVE_LIBUTIL

/* Define to 1 if you have the `wsock32' library (-lwsock32). */
#undef HAVE_LIBWSOCK32

/* Define to 1 if you have the `z' library (-lz). */
#define HAVE_LIBZ

/* Define to 1 if constants of type 'long long int' should have the suffix LL.
   */
#undef HAVE_LL_CONSTANTS

/* Define to 1 if `long int' works and is 64 bits. */
#undef HAVE_LONG_INT_64

/* Define to 1 if `long long int' works and is 64 bits. */
#define HAVE_LONG_LONG_INT_64

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H

/* Define to 1 if you have the <netinet/in.h> header file. */
#undef HAVE_NETINET_IN_H

/* Define to 1 if you have the <netinet/tcp.h> header file. */
#undef HAVE_NETINET_TCP_H

/* Define to 1 if you have the `on_exit' function. */
#undef HAVE_ON_EXIT

/* Define to 1 if you have the <pam/pam_appl.h> header file. */
#undef HAVE_PAM_PAM_APPL_H

/* Define to 1 if you have the `poll' function. */
#undef HAVE_POLL

/* Define to 1 if you have the <poll.h> header file. */
#undef HAVE_POLL_H

/* Define to 1 if you have the POSIX signal interface. */
#undef HAVE_POSIX_SIGNALS

/* Define to 1 if you have the `pstat' function. */
#undef HAVE_PSTAT

/* Define to 1 if the PS_STRINGS thing exists. */
#undef HAVE_PS_STRINGS

/* Define if you have POSIX threads libraries and header files. */
#undef HAVE_PTHREAD

/* Define to 1 if you have the <pwd.h> header file. */
#undef HAVE_PWD_H

/* Define to 1 if you have the `random' function. */
#undef HAVE_RANDOM

/* Define to 1 if you have the <readline.h> header file. */
#undef HAVE_READLINE_H

/* Define to 1 if you have the <readline/history.h> header file. */
#undef HAVE_READLINE_HISTORY_H

/* Define to 1 if you have the <readline/readline.h> header file. */
#undef HAVE_READLINE_READLINE_H

/* Define to 1 if you have the `readlink' function. */
#undef HAVE_READLINK

/* Define to 1 if you have the `replace_history_entry' function. */
#undef HAVE_REPLACE_HISTORY_ENTRY

/* Define to 1 if you have the `rint' function. */
#undef HAVE_RINT

/* Define to 1 if you have the global variable
   'rl_completion_append_character'. */
#undef HAVE_RL_COMPLETION_APPEND_CHARACTER

/* Define to 1 if you have the `rl_completion_matches' function. */
#undef HAVE_RL_COMPLETION_MATCHES

/* Define to 1 if you have the `rl_filename_completion_function' function. */
#undef HAVE_RL_FILENAME_COMPLETION_FUNCTION

/* Define to 1 if you have the <security/pam_appl.h> header file. */
#undef HAVE_SECURITY_PAM_APPL_H

/* Define to 1 if you have the `setproctitle' function. */
#undef HAVE_SETPROCTITLE

/* Define to 1 if you have the `setsid' function. */
#undef HAVE_SETSID

/* Define to 1 if you have the `sigprocmask' function. */
#undef HAVE_SIGPROCMASK

/* Define to 1 if you have sigsetjmp(). */
#undef HAVE_SIGSETJMP

/* Define to 1 if the system has the type `sig_atomic_t'. */
#undef HAVE_SIG_ATOMIC_T

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF

/* Define to 1 if you have spinlocks. */
#undef HAVE_SPINLOCKS

/* Define to 1 if you have the `srandom' function. */
#define HAVE_SRANDOM

/* Define to 1 if you have the <stdint.h> header file. */
#undef HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR

/* Define to 1 if you have the `strerror_r' function. */
#undef HAVE_STRERROR_R

/* Define to 1 if cpp supports the ANSI # stringizing operator. */
#undef HAVE_STRINGIZE

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H

/* Define to 1 if you have the `strtol' function. */
#define HAVE_STRTOL

/* Define to 1 if you have the `strtoll' function. */
#undef HAVE_STRTOLL

/* Define to 1 if you have the `strtoq' function. */
#undef HAVE_STRTOQ

/* Define to 1 if you have the `strtoul' function. */
#undef HAVE_STRTOUL

/* Define to 1 if you have the `strtoull' function. */
#undef HAVE_STRTOULL

/* Define to 1 if you have the `strtouq' function. */
#undef HAVE_STRTOUQ

/* Define to 1 if the system has the type `struct addrinfo'. */
#define HAVE_STRUCT_ADDRINFO

/* Define to 1 if the system has the type `struct cmsgcred'. */
#undef HAVE_STRUCT_CMSGCRED

/* Define to 1 if the system has the type `struct fcred'. */
#undef HAVE_STRUCT_FCRED

/* Define to 1 if the system has the type `struct option'. */
#undef HAVE_STRUCT_OPTION

/* Define to 1 if `sa_len' is member of `struct sockaddr'. */
#define HAVE_STRUCT_SOCKADDR_SA_LEN

/* Define to 1 if the system has the type `struct sockaddr_storage'. */
#define HAVE_STRUCT_SOCKADDR_STORAGE

/* Define to 1 if `ss_family' is member of `struct sockaddr_storage'. */
#define HAVE_STRUCT_SOCKADDR_STORAGE_SS_FAMILY

/* Define to 1 if `ss_len' is member of `struct sockaddr_storage'. */
#undef HAVE_STRUCT_SOCKADDR_STORAGE_SS_LEN

/* Define to 1 if `__ss_family' is member of `struct sockaddr_storage'. */
#undef HAVE_STRUCT_SOCKADDR_STORAGE___SS_FAMILY

/* Define to 1 if `__ss_len' is member of `struct sockaddr_storage'. */
#undef HAVE_STRUCT_SOCKADDR_STORAGE___SS_LEN

/* Define to 1 if the system has the type `struct sockaddr_un'. */
#undef HAVE_STRUCT_SOCKADDR_UN

/* Define to 1 if the system has the type `struct sockcred'. */
#undef HAVE_STRUCT_SOCKCRED

/* Define to 1 if `tm_zone' is member of `struct tm'. */
#define HAVE_STRUCT_TM_TM_ZONE

/* Define to 1 if you have the <SupportDefs.h> header file. */
#undef HAVE_SUPPORTDEFS_H

/* Define to 1 if you have the `symlink' function. */
#undef HAVE_SYMLINK

/* Define to 1 if you have the `sysconf' function. */
#undef HAVE_SYSCONF

/* Define to 1 if you have the syslog interface. */
#undef HAVE_SYSLOG

/* Define to 1 if you have the <sys/ipc.h> header file. */
#undef HAVE_SYS_IPC_H

/* Define to 1 if you have the <sys/poll.h> header file. */
#undef HAVE_SYS_POLL_H

/* Define to 1 if you have the <sys/pstat.h> header file. */
#undef HAVE_SYS_PSTAT_H

/* Define to 1 if you have the <sys/select.h> header file. */
#undef HAVE_SYS_SELECT_H

/* Define to 1 if you have the <sys/sem.h> header file. */
#undef HAVE_SYS_SEM_H

/* Define to 1 if you have the <sys/shm.h> header file. */
#undef HAVE_SYS_SHM_H

/* Define to 1 if you have the <sys/socket.h> header file. */
#undef HAVE_SYS_SOCKET_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#undef HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/time.h> header file. */
#undef HAVE_SYS_TIME_H

/* Define to 1 if you have the <sys/types.h> header file. */
#undef HAVE_SYS_TYPES_H

/* Define to 1 if you have the <sys/un.h> header file. */
#undef HAVE_SYS_UN_H

/* Define to 1 if you have the <termios.h> header file. */
#undef HAVE_TERMIOS_H

/* Define to 1 if your `struct tm' has `tm_zone'. Deprecated, use
   `HAVE_STRUCT_TM_TM_ZONE' instead. */
#undef HAVE_TM_ZONE

/* Define to 1 if you have the `towlower' function. */
#undef HAVE_TOWLOWER

/* Define to 1 if you have the external array `tzname'. */
#undef HAVE_TZNAME

/* Define to 1 if the system has the type `uint64'. */
#undef HAVE_UINT64

/* Define to 1 if the system has the type `uint8'. */
#undef HAVE_UINT8

/* Define to 1 if the system has the type `union semun'. */
#undef HAVE_UNION_SEMUN

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H

/* Define to 1 if you have unix sockets. */
#undef HAVE_UNIX_SOCKETS

/* Define to 1 if you have the `unsetenv' function. */
#undef HAVE_UNSETENV

/* Define to 1 if you have the `utime' function. */
#undef HAVE_UTIME

/* Define to 1 if you have the `utimes' function. */
#undef HAVE_UTIMES

/* Define to 1 if you have the <utime.h> header file. */
#undef HAVE_UTIME_H

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF

/* Define to 1 if you have the `waitpid' function. */
#undef HAVE_WAITPID

/* Define to 1 if you have the <wchar.h> header file. */
#undef HAVE_WCHAR_H

/* Define to 1 if you have the `wcstombs' function. */
#undef HAVE_WCSTOMBS

/* Define to 1 if you have the <wctype.h> header file. */
#undef HAVE_WCTYPE_H

/* Define to the appropriate snprintf format for 64-bit ints, if any. */
#undef INT64_FORMAT

/* Define to build with Kerberos 5 support. (--with-krb5) */
#undef KRB5

/* Define to the location of locale files. */
#undef LOCALEDIR

/* Define as the maximum alignment requirement of any C data type. */
#undef MAXIMUM_ALIGNOF

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME

/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* Define to the name of the default PostgreSQL service principal in Kerberos.
   (--with-krb-srvnam=NAME) */
#undef PG_KRB_SRVNAM

/* PostgreSQL version */
#undef PG_VERSION

/* A string containing the version number, platform, and C compiler */
#undef PG_VERSION_STR

/* Define to the necessary symbol if this constant uses a non-standard name on
   your system. */
#undef PTHREAD_CREATE_JOINABLE

/* The size of a `size_t', as computed by sizeof. */
#undef SIZEOF_SIZE_T

/* The size of a `unsigned long', as computed by sizeof. */
#undef SIZEOF_UNSIGNED_LONG

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS

/* Define to 1 if strerror_r() returns a int. */
#undef STRERROR_R_INT

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
#undef TM_IN_SYS_TIME

/* Define to the appropriate snprintf format for unsigned 64-bit ints, if any.
   */
#undef UINT64_FORMAT

/* Define to 1 to build with assertion checks. (--enable-cassert) */
#undef USE_ASSERT_CHECKING

/* Define to 1 to build with Bonjour support. (--with-bonjour) */
#undef USE_BONJOUR

/* Define to 1 if you want 64-bit integer timestamp and interval support.
   (--enable-integer-datetimes) */
#undef USE_INTEGER_DATETIMES

/* Define to select named POSIX semaphores. */
#undef USE_NAMED_POSIX_SEMAPHORES

/* Define to 1 to build with PAM support. (--with-pam) */
#undef USE_PAM

/* Use replacement snprintf() functions. */
#undef USE_REPL_SNPRINTF

/* Define to build with (Open)SSL support. (--with-openssl) */
#undef USE_SSL

/* Define to select SysV-style semaphores. */
#undef USE_SYSV_SEMAPHORES

/* Define to select SysV-style shared memory. */
#undef USE_SYSV_SHARED_MEMORY

/* Define to select unnamed POSIX semaphores. */
#undef USE_UNNAMED_POSIX_SEMAPHORES

/* Number of bits in a file offset, on hosts where this is settable. */
#undef _FILE_OFFSET_BITS

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
#undef _LARGEFILE_SOURCE

/* Define for large files, on AIX-style hosts. */
#undef _LARGE_FILES

/* Define to empty if `const' does not conform to ANSI C. */
#undef const

/* Define as `__inline' if that's what the C compiler calls it, or to nothing
   if it is not supported. */
#undef inline

/* Define to empty if the C compiler does not understand signed types. */
#undef signed

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
#undef volatile
