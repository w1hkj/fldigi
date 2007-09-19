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

#ifdef NDEBUG

#define FL_LOCK(x)   Fl::lock(x)
#define FL_UNLOCK(x) Fl::unlock(x)
#define FL_AWAKE(x)  Fl::awake(x)

#else // debugging
#	include <cstdio>
	extern FILE *locklog;
	extern FILE *awakelog;
	void pstack(FILE *log);

#	ifdef TRACE_LOCKS
#		define FL_LOCK(x)     do { Fl::lock(); pstack(locklog); } while (0)
#		define FL_UNLOCK(x)   do { Fl::unlock(); } while (0)
#		define FL_AWAKE(x)    do { pstack(awakelog); Fl::awake(); } while (0)
#		define FL_LOCK_D(x)   FL_LOCK(x)
#		define FL_UNLOCK_D(x) FL_UNLOCK(x)
#		define FL_AWAKE_D(x)  FL_AWAKE(x)
#	endif // TRACE_LOCKS

#	ifndef NO_LOCKS
#		define FL_LOCK(x)                                       \
                do {                                                    \
                        switch (GET_THREAD_ID()) {                      \
                        case TRX_TID:                                   \
                                printf("E: trx lock in %s at %s:%d\n",  \
                                       __func__, __FILE__, __LINE__);   \
                                break;                                  \
                        case FLMAIN_TID:                                \
                                printf("W: flrun lock in %s at %s:%d\n",\
                                       __func__, __FILE__, __LINE__);   \
                                break;                                  \
                        default:                                        \
                                printf("I: lock in %s at %s:%d\n",      \
                                       __func__, __FILE__, __LINE__);   \
                        }                                               \
                        printf("\t"); pstack(stdout);                   \
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
