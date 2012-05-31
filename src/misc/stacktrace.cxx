// ----------------------------------------------------------------------------
//	stacktrace.cxx: portable stack trace and error handlers
//
// Copyright (C) 2007-2009
//		Stelios Bounanos, M0GLD
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

#include <unistd.h>
#if HAVE_DBG_STACK
#  include <fstream>
#endif
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <csignal>

#ifdef BUILD_FLDIGI
#  include "main.h"
#else
#  include "flarq.h"
#endif

using namespace std;


static volatile sig_atomic_t signum = 0;
#if !HAVE_DBG_STACK
static void pstack(int fd, unsigned skip = 0);
#else
static void pstack(ostream& out, unsigned skip = 0);
#endif


void diediedie(void)
{
#ifndef __MINGW32__
	// If this environment variable is set, creates a core dump.
	if( getenv("FLDIGI_COREDUMP") )
	{
		signal(SIGSEGV, SIG_DFL);
		kill(getpid(), SIGSEGV);
	}
#endif

	static bool print_trace = true;

	if (!print_trace)
		exit(128 + (signum ? signum : SIGABRT));

#define CRASH_HEADER "\nAborting " PACKAGE_TARNAME " due to a fatal error.\n" \
        "Please report this to:   " PACKAGE_BUGREPORT                         \
        "\nor file a bug report at: " PACKAGE_NEWBUG                          \
        "\n\n****** Stack trace:\n"

#ifndef __MINGW32__
	if (isatty(STDERR_FILENO))
#endif
	{
		if (signum)
			cerr << "\nCaught signal " << signum;
		cerr << CRASH_HEADER;
#if !HAVE_DBG_STACK
		pstack(STDERR_FILENO);
#else
		pstack(cerr);
#endif

		extern string version_text, build_text;
		cerr << "\n****** Version information:\n" << version_text
		     << "\n****** Build information:\n" << build_text;

		string stfname;
#ifdef BUILD_FLDIGI
		stfname.assign(HomeDir).append("stacktrace.txt");
#else
		stfname = Logfile;
#endif

#if !HAVE_DBG_STACK
		FILE* stfile = fopen(stfname.c_str(), "w");
		if (stfile) {
			pstack(fileno(stfile), 1);
			fprintf(stfile, "%s\n****** Version information:\n%s\n****** Build information:%s\n",
				CRASH_HEADER, version_text.c_str(), build_text.c_str());
		}
#else
		ofstream stfile(stfname.c_str());
		if (stfile) {
			stfile << CRASH_HEADER;
			pstack(stfile, 1);
			stfile << "\n****** Version information:\n" << version_text;
			stfile << "\n****** Build information:\n" << build_text;
		}
#endif
	}

	print_trace = false;
	exit(128 + (signum ? signum : SIGABRT));
}


#if !HAVE_DBG_STACK
#  if HAVE_EXECINFO_H
#    include <execinfo.h>
#    define MAX_STACK_FRAMES 64

void pstack(int fd, unsigned skip)
{
        void* stack[MAX_STACK_FRAMES];

        ++skip;
        backtrace_symbols_fd(stack + skip, backtrace(stack, MAX_STACK_FRAMES) - skip, fd);
}
#  else
void pstack(int fd, unsigned skip) { }
#  endif
#else
#  include <algorithm>
#  include <iterator>
#  include "stack.h"

static void pstack(ostream& out, unsigned skip)
{
	dbg::stack s;
	dbg::stack::const_iterator start = s.begin(), end = s.end();

	while (skip-- && ++start != end);
	copy(start, end, ostream_iterator<dbg::stack_frame>(out, "\n"));
}
#endif

void pstack_maybe(void)
{
        static bool trace = getenv("FLDIGI_TRACE_LOCKS");

        if (trace)
#if !HAVE_DBG_STACK
                pstack(STDERR_FILENO, 1);
#else
		pstack(cerr, 1);
#endif
}

void handle_unexpected(void)
{
        cerr << "Uncaught exception. Not again!\n";
        abort();
}

// this may not give us anything useful, but we can try...
void handle_signal(int s)
{
	if (s != SIGUSR2) {
		signum = s;
		diediedie();
	}
}
