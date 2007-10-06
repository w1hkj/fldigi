// ----------------------------------------------------------------------------
//	stacktrace.cxx
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

#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>

void pstack(FILE *log)
{
#if (defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__))
        if (!log || !getenv("FLDIGI_TRACE_LOCKS"))
                return;

        register unsigned *ebp asm("ebp");
        register unsigned *base = ebp;

        Dl_info dli;

        fprintf(log, "\t");
        while (base) {
                dladdr((void *)*(base + 1), &dli);
                fprintf(log, "%s@0x%08x ", dli.dli_fname, *(base + 1));

                base = (unsigned *)*base;
        }
        fprintf(log, "\n");
        fflush(log);
#endif // x86
}
