// ----------------------------------------------------------------------------
//      mingw.c
//
// The following routines were copied from git-1.6.1.2/compat/mingw.c:
//   sleep mingw_getcwd mingw_getenv mingw_rename
//
// The uname routine was adapted from libgw32c 0.4.
//
// The rest:
// Copyright (C) 2009
//              Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
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

#include <config.h>

#include <ctype.h>
#include "compat.h"

/* default mode for stdin, stdout and stderr */
unsigned int _CRT_fmode = _O_BINARY;

unsigned sleep(unsigned seconds)
{
	Sleep(seconds*1000);
	return 0;
}

#undef getcwd
char *mingw_getcwd(char *pointer, int len)
{
	int i;
	char *ret = getcwd(pointer, len);
	if (!ret)
		return ret;
	for (i = 0; pointer[i]; i++)
		if (pointer[i] == '\\')
			pointer[i] = '/';
	return ret;
}

#undef getenv
char *mingw_getenv(const char *name)
{
	char *result = getenv(name);
	if (!result && !strcmp(name, "TMPDIR")) {
		/* on Windows it is TMP and TEMP */
		result = getenv("TMP");
		if (!result)
			result = getenv("TEMP");
	}
	return result;
}

#undef rename
int mingw_rename(const char *pold, const char *pnew)
{
	DWORD attrs;

	/*
	 * Try native rename() first to get errno right.
	 * It is based on MoveFile(), which cannot overwrite existing files.
	 */
	if (!rename(pold, pnew))
		return 0;
	if (errno != EEXIST)
		return -1;
	if (MoveFileEx(pold, pnew, MOVEFILE_REPLACE_EXISTING))
		return 0;
	/* TODO: translate more errors */
	if (GetLastError() == ERROR_ACCESS_DENIED &&
	    (attrs = GetFileAttributes(pnew)) != INVALID_FILE_ATTRIBUTES) {
		if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
			errno = EISDIR;
			return -1;
		}
		if ((attrs & FILE_ATTRIBUTE_READONLY) &&
		    SetFileAttributes(pnew, attrs & ~FILE_ATTRIBUTE_READONLY)) {
			if (MoveFileEx(pold, pnew, MOVEFILE_REPLACE_EXISTING))
				return 0;
			/* revert file attributes on failure */
			SetFileAttributes(pnew, attrs);
		}
	}
	errno = EACCES;
	return -1;
}

/******************************************************************************/

__attribute__((constructor))
static void wsa_init(void)
{
	WSADATA wsa;

	static int wsa_init_ = 0;
	if (wsa_init_)
		return;

	if (WSAStartup(MAKEWORD(2, 2), &wsa)) {
		fprintf(stderr, "unable to initialize winsock: error %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	atexit((void(*)(void)) WSACleanup);
	wsa_init_ = 1;
}

int socketpair(int family, int type, int protocol, int *sv)
{
	struct sockaddr_in addr;
	SOCKET sfd;
	int err, len = sizeof(addr);

	if (sv == NULL || family != AF_INET || type != SOCK_STREAM || protocol) {
		WSASetLastError(WSAEINVAL);
		return SOCKET_ERROR;
	}

	sv[0] = sv[1] = INVALID_SOCKET;
	if ((sfd = socket(family, type, 0)) == INVALID_SOCKET)
		return SOCKET_ERROR;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = family;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = 0; /* any port */

	if ((err = bind(sfd, (const struct sockaddr*)&addr, sizeof(addr))) == SOCKET_ERROR) {
		err = WSAGetLastError();
		closesocket(sfd);
		WSASetLastError(err);
		return SOCKET_ERROR;
	}

	if ((err = getsockname(sfd, (struct sockaddr*)&addr, &len)) == SOCKET_ERROR) {
		err = WSAGetLastError();
		closesocket(sfd);
		WSASetLastError(err);
		return SOCKET_ERROR;
	}

	do {
		if (listen(sfd, 1) == SOCKET_ERROR)
			break;
		if ((sv[0] = WSASocket(family, type, 0, NULL, 0, 0)) == INVALID_SOCKET)
			break;
		if (connect(sv[0], (const struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
			break;
		if ((sv[1] = accept(sfd, NULL, NULL)) == INVALID_SOCKET)
			break;
		closesocket(sfd);
		return 0;
	} while (0);

	/* error */
	err = WSAGetLastError();
	closesocket(sfd);
	closesocket(sv[0]);
	closesocket(sv[1]);
	WSASetLastError(err);
	return SOCKET_ERROR;
}

/******************************************************************************/

int nanosleep(const struct timespec *req, struct timespec *rem)
{
	if (unlikely(req->tv_nsec < 0 || req->tv_nsec < 0L || req->tv_nsec > 999999999L)) {
		errno = EINVAL;
		return -1;
	}
	Sleep(req->tv_sec * 1000 + req->tv_nsec / 1000000L);
	if (unlikely(rem)) {
		rem->tv_sec = 0;
		rem->tv_nsec = 0L;
	}
	return 0;
}

BOOL GetOsInfo(LPSTR OsName, LPSTR Release, LPSTR Version);
BOOL GetMachInfo(LPSTR MachineName, LPSTR ProcessorName);
int uname(struct utsname *name)
{
	char processor[1024];

	if (name == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (gethostname(name->nodename, sizeof(name->nodename)) < 0) {
		name->nodename[0] = '\0';
		errno = ENOSYS;
		return -1;
	}

	if (!GetOsInfo(name->sysname, name->release, name->version)) {
		strncpy (name->sysname, "win32", sizeof (name->sysname));
		strncpy (name->release, "unknown", sizeof (name->release));
		strncpy (name->version, "unknown", sizeof (name->version));
	}
	/* "windows32" is as yet the only universal windows description allowed
	   by config.guess and config.sub */
	strncpy(name->sysname, "windows32", sizeof (name->sysname));
	if (!GetMachInfo(name->machine, processor))
		strncpy(name->machine, "i386", sizeof (name->machine));

	return 0;
}

int getrusage(int who, struct rusage *usage)
{
	FILETIME ct, et, kt, ut;
	ULARGE_INTEGER uli;

	if (who != RUSAGE_SELF) {
		errno = EINVAL;
		return -1;
	}
	if (!usage) {
		errno = EFAULT;
		return -1;
	}

	if (!GetProcessTimes(GetCurrentProcess(), &ct, &et, &kt, &ut)) {
		errno = ENOENT;
		return -1;
	}

	// FILETIMEs use 100-ns units
	memcpy(&uli, &kt, sizeof(FILETIME));
	usage->ru_stime.tv_sec  = uli.QuadPart / 10000000L;
	usage->ru_stime.tv_usec = uli.QuadPart % 10000000L;
	memcpy(&uli, &ut, sizeof(FILETIME));
	usage->ru_utime.tv_sec  = uli.QuadPart / 10000000L;
	usage->ru_utime.tv_usec = uli.QuadPart % 10000000L;

	return 0;
}
