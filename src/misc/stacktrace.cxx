// ----------------------------------------------------------------------------
//	stacktrace.cxx: portable stack trace and error handlers
//
// Copyright (C) 2007
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
#include <iostream>
#include <cstdlib>

#if HAVE_EXECINFO_H
#  include <execinfo.h>
#endif

#include "stacktrace.h"

#define MAX_STACK_FRAMES 64


void pstack(int fd, unsigned skip)
{
#if HAVE_EXECINFO_H
        void* stack[MAX_STACK_FRAMES];

        ++skip;
        backtrace_symbols_fd(stack + skip, backtrace(stack, MAX_STACK_FRAMES) - skip, fd);
#endif
}

void pstack_maybe(void)
{
        static bool trace = getenv("TRACE_LOCKS");

        if (trace)
                pstack(STDERR_FILENO, 1);
}

void diediedie(void)
{
        std::cerr << "\nAborting " PACKAGE
                     " due to a fatal error.\nPlease report this to "
                     PACKAGE_BUGREPORT << "\n\n";
        pstack(STDERR_FILENO);
        extern void print_versions(std::ostream&);
        print_versions(std::cerr << "\nVersion information:\n");
        abort();
}

void handle_unexpected(void)
{
        std::cerr << "Uncaught exception. Not again!\n";
        std::terminate();
}

// this may not give us anything useful, but we can try...
void handle_signal(int s)
{
        std::cerr << "Caught signal " << s;
        diediedie();
}
