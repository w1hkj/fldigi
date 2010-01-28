// ----------------------------------------------------------------------------
//	fl_lock.h
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

#ifndef FL_LOCK_H_
#define FL_LOCK_H_

// disabled calls
#define FL_LOCK_D(x)   ((void)0)
#define FL_UNLOCK_D(x) ((void)0)
#define FL_AWAKE_D(x)  ((void)0)
// enabled calls
#define FL_LOCK_E(x)   FL_LOCK(x)
#define FL_UNLOCK_E(x) FL_UNLOCK(x)
#define FL_AWAKE_E(x)  FL_AWAKE(x)

#if !defined(NDEBUG) && !defined(NO_LOCKS)
#	include <FL/Fl.H>
#endif

#ifdef NDEBUG
#	define FL_LOCK(x)   Fl::lock(x)
#	define FL_UNLOCK(x) Fl::unlock(x)
#	define FL_AWAKE(x)  Fl::awake(x)
#else // debugging
#	include <stacktrace.h>

#	ifndef NO_LOCKS
#               include "debug.h"
#		define FL_LOCK(x)                                       \
                do {                                                    \
                        switch (GET_THREAD_ID()) {                      \
                        case TRX_TID:                                   \
                                LOG_ERROR("trx lock");			\
                                break;                                  \
                        case FLMAIN_TID:                                \
                                LOG_WARN("flrun lock");			\
                                break;                                  \
                        default:                                        \
                                LOG_VERBOSE("lock");			\
                        }                                               \
                        pstack_maybe();                                 \
                        Fl::lock(x);                                    \
                } while (0);

#		define FL_UNLOCK(x) Fl::unlock(x)
#		define FL_AWAKE(x)  Fl::awake(x)
#	else // no locks
#		define FL_LOCK(x)   ((void)0)
#		define FL_UNLOCK(x) ((void)0)
#		define FL_AWAKE(x)  ((void)0)
#	endif // NO_LOCKS
#endif // NDEBUG

#endif // FL_LOCK_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
