// ----------------------------------------------------------------------------
//      cmd_debug.cxx  version which excludes fltk calls
//
// Copyright (C) 2008-2010
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

#ifdef __MINGW32__
#  include "compat.h"
#endif

#include <sstream>
#include <fstream>
#include <iostream>

#include <cstdio>
#include <cstring>
#include <cstdarg>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "debug.h"
#include "gettext.h"

using namespace std;

static FILE* wfile = 0;
static FILE* rfile = 0;
static int rfd;
static bool tty;

static string linebuf;

debug* debug::inst = 0;
debug::level_e debug::level = debug::INFO_LEVEL;
uint32_t debug::mask = ~0u;

static const char* prefix[] = {
	_("Quiet"), _("Error"), _("Warning"), _("Info"), _("Verbose"), _("Debug")
};

void debug::rotate_log(const char* filename)
{
	const int n = 5; // rename existing log files to keep up to 5 old versions
	ostringstream oldfn, newfn;
	ostringstream::streampos p;

	oldfn << filename << '.';
	newfn << filename << '.';
	p = oldfn.tellp();

	for (int i = n - 1; i > 0; i--) {
		oldfn.seekp(p);
		newfn.seekp(p);
		oldfn << i;
		newfn << i + 1;
		rename(oldfn.str().c_str(), newfn.str().c_str());
	}
	rename(filename, oldfn.str().c_str());
}

void debug::start(const char* filename)
{
	if (debug::inst)
		return;
	rotate_log(filename);
	inst = new debug(filename);
}

void debug::stop(void)
{
	if (inst) {
		delete inst;
		inst = 0;
	}
}

static char fmt[1024];

void debug::log(level_e level, const char* func, const char* srcf, int line, const char* format, ...)
{
	if (!inst)
		return;

	if (unlikely(debug::level == DEBUG_LEVEL)) {
		time_t t = time(NULL);
		struct tm stm;
		(void)localtime_r(&t, &stm);
		snprintf(fmt, sizeof(fmt), "%c: [%02d:%02d:%02d] %s:%d: %s\n",
			 *prefix[level], stm.tm_hour, stm.tm_min, stm.tm_sec, srcf, line, format);
	}
	else
		snprintf(fmt, sizeof(fmt), "%c: %s: %s\n", *prefix[level], func, format);
	va_list args;
	va_start(args, format);
	intptr_t nw = vfprintf(wfile, fmt, args);
	va_end(args);
	if (tty) {
		if (level <= DEBUG_LEVEL && level > QUIET_LEVEL) {
			va_start(args, format);
			vfprintf(stderr, fmt, args);
			va_end(args);
		}
	}

#ifdef __MINGW32__
	fflush(wfile);
#endif

	sync_text(&nw);
}

void debug::elog(const char* func, const char* srcf, int line, const char* text)
{
	log(ERROR_LEVEL, func, srcf, line, "%s: %s", text, strerror(errno));
}

static char buf[BUFSIZ+1];

void debug::sync_text(void* arg)
{
	intptr_t toread = (intptr_t)arg;
	size_t block = MIN((size_t)toread, sizeof(buf) - 1);
	ssize_t n;

	while (toread > 0) {
		if ((n = read(rfd, buf, block)) <= 0)
			break;
		buf[n] = '\0';
		linebuf = buf;
		if (linebuf[linebuf.length() - 1] != '\n')
			linebuf += '\n';
		size_t p1 = 0, p2 = linebuf.find("\n");
		while( p2 != string::npos) {
			p1 = p2 + 1;
			p2 = linebuf.find("\n", p1);
		}
		toread -= n;
	}
}

debug::debug(const char* filename)
{
	if ((wfile = fopen(filename, "w")) == NULL)
		throw strerror(errno);
	setvbuf(wfile, (char*)NULL, _IOLBF, 0);
	set_cloexec(fileno(wfile), 1);

	if ((rfile = fopen(filename, "r")) == NULL)
		throw strerror(errno);
	rfd = fileno(rfile);
	set_cloexec(rfd, 1);
#ifndef __MINGW32__
	int f;
	if ((f = fcntl(rfd, F_GETFL)) == -1)
		throw strerror(errno);
	if (fcntl(rfd, F_SETFL, f | O_NONBLOCK) == -1)
		throw strerror(errno);
#endif
	tty = isatty(fileno(stderr));
}

debug::~debug()
{
	if (wfile) fclose(wfile);
	if (rfile) fclose(rfile);
}

