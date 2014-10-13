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

// this tests depends on a modified FL/filename.H in the Fltk-1.3.0
// change
//#  if defined(WIN32) && !defined(__CYGWIN__) && !defined(__WATCOMC__)
// to
//#  if defined(WIN32) && !defined(__CYGWIN__) && !defined(__WATCOMC__) && !defined(__WOE32__)

#ifdef __MINGW32__
#	if FLDIGI_FLTK_API_MAJOR == 1 && FLDIGI_FLTK_API_MINOR < 3
#		undef dirent
#		include <dirent.h>
#	else
#		include <dirent.h>
#	endif
#define WSA_MAJOR 2
#define WSA_MINOR 0
#else
#	include <dirent.h>
#endif

#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <assert.h>

#include "compat/mingw.h"

#endif // MINGW32_H
