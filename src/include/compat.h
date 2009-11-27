#ifndef COMPAT_H
#define COMPAT_H

/* adapted from compat.h in git-1.6.1.2 */

#if !defined(__APPLE__) && !defined(__FreeBSD__)  && !defined(__USLC__) && !defined(_M_UNIX)
#  define _XOPEN_SOURCE 600 /* glibc2 and AIX 5.3L need 500, OpenBSD needs 600 for S_ISLNK() */
#  define _XOPEN_SOURCE_EXTENDED 1 /* AIX 5.3L needs this */
#endif
#define _ALL_SOURCE 1
#define _GNU_SOURCE 1
#define _BSD_SOURCE 1

#include <config.h>

#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <assert.h>

#include "compat/mingw.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__WOE32__) && (!defined(__GNUC__) || __GNUC__ < 4)
#  define SNPRINTF_RETURNS_BOGUS 1
#else
#  define SNPRINTF_RETURNS_BOGUS 0
#endif

#if SNPRINTF_RETURNS_BOGUS
#define snprintf git_snprintf
extern int git_snprintf(char *str, size_t maxsize,
			const char *format, ...);
#define vsnprintf git_vsnprintf
extern int git_vsnprintf(char *str, size_t maxsize,
			 const char *format, va_list ap);
#endif

#ifdef __cplusplus
}
#endif

#endif // MINGW32_H
