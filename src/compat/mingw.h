// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef MINGW_H_
#define MINGW_H_

#include "config.h"

#include <sys/types.h>
#include <pthread.h>

#undef _WINSOCKAPI_
#include <winsock2.h>

#undef EADDRINUSE
#define EADDRINUSE WSAEADDRINUSE

#undef EISCONN
#define EISCONN WSAEISCONN

#undef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK

#undef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS

#undef EALREADY
#define EALREADY WSAEALREADY

//======================================================================

#ifdef __cplusplus
extern "C" {
#endif

typedef long suseconds_t;
#define hstrerror strerror

#ifndef SIGUSR2
#  define SIGUSR2 100
#endif

extern void was_init(void);
extern int was_init_state(void);
extern WSADATA * was_data(void);

/*
 * simple adaptors
 */

static inline int mingw_mkdir(const char *path, int mode)
{
	return mkdir(path);
}
#define mkdir mingw_mkdir

static inline int mingw_unlink(const char *pathname)
{
	/* read-only files cannot be removed */
	chmod(pathname, 0666);
	return unlink(pathname);
}
#define unlink mingw_unlink

/*
 * implementations of missing functions
 */

unsigned int sleep (unsigned int seconds);
char *mingw_getcwd(char *pointer, int len);
#define getcwd mingw_getcwd
char *mingw_getenv(const char *name);
#define getenv mingw_getenv
int mingw_rename(const char*, const char*);
#define rename mingw_rename

#ifndef SHUT_WR
#  define SHUT_WR SD_SEND
#endif
#ifndef SHUT_RD
#  define SHUT_RD SD_RECEIVE
#endif
#ifndef SHUT_RDWR
#  define SHUT_RDWR SD_BOTH
#endif

int nanosleep (const struct timespec *req, struct timespec *rem);
int socketpair(int family, int type, int protocol, SOCKET *sv);

/* uname */
#define UTSNAME_MAX_ 257
struct utsname
{
	char sysname[UTSNAME_MAX_];
	char nodename[UTSNAME_MAX_];
	char release[UTSNAME_MAX_];
	char version[UTSNAME_MAX_];
	char machine[UTSNAME_MAX_];
};
int uname(struct utsname *name);

/* getrusage */
#define RUSAGE_SELF     0
#define RUSAGE_CHILDREN (-1)
struct rusage
{
	struct timeval ru_utime;
	struct timeval ru_stime;
};
int getrusage(int who, struct rusage *usage);

/* fsync, fdatasync */
#include <io.h>
#define fsync _commit
#define fdatasync fsync

#ifdef __cplusplus
}
#endif

#endif
