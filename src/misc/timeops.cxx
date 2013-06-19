// ----------------------------------------------------------------------------
//	timeops.cxx
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

#include "timeops.h"
#ifdef __MINGW32__
#  include "compat.h"
#endif

#if !HAVE_CLOCK_GETTIME
#  ifdef __APPLE__
#    include <mach/mach_time.h>
#  endif
#  if TIME_WITH_SYS_TIME
#    include <sys/time.h>
#  endif
#  include <errno.h>
int clock_gettime(clockid_t clock_id, struct timespec* tp)
{
	if (clock_id == CLOCK_REALTIME) {
		struct timeval t;
		if (unlikely(gettimeofday(&t, NULL) != 0))
			return -1;
		tp->tv_sec = t.tv_sec;
		tp->tv_nsec = t.tv_usec * 1000;
	}
	else if (clock_id == CLOCK_MONOTONIC) {
#if defined(__WOE32__)
		int32_t msec = GetTickCount();
		tp->tv_sec = msec / 1000;
		tp->tv_nsec = (msec % 1000) * 1000000;
#elif defined(__APPLE__)
		static mach_timebase_info_data_t info = { 0, 0 };
		if (unlikely(info.denom == 0))
			mach_timebase_info(&info);
		uint64_t t = mach_absolute_time() * info.numer / info.denom;
		tp->tv_sec = t / 1000000000;
		tp->tv_nsec = t % 1000000000;
#endif
	}
	else {
		errno = EINVAL;
		return -1;
	}

	return 0;
}
#endif // !HAVE_CLOCK_GETTIME

struct timespec operator+(const struct timespec &t0, const double &t)
{
        struct timespec r;
        r.tv_sec = t0.tv_sec + static_cast<time_t>(t);
        r.tv_nsec = t0.tv_nsec + static_cast<long>((t - static_cast<time_t>(t)) * 1e9);
        if (r.tv_nsec > 1000000000) {
                r.tv_nsec -= 1000000000;
                r.tv_sec++;
        }
        return r;
}

struct timespec operator-(const struct timespec &t0, const struct timespec &t1)
{
        struct timespec r = t0;

        if (r.tv_nsec < t1.tv_nsec) {
                --r.tv_sec;
                r.tv_nsec += 1000000000L;
        }
        r.tv_sec -= t1.tv_sec;
        r.tv_nsec -= t1.tv_nsec;

        return r;
}

struct timespec& operator-=(struct timespec &t0, const struct timespec &t1)
{
        if (t0.tv_nsec < t1.tv_nsec) {
                --t0.tv_sec;
                t0.tv_nsec += 1000000000L;
        }
        t0.tv_sec -= t1.tv_sec;
        t0.tv_nsec -= t1.tv_nsec;

        return t0;
}

bool operator>(const struct timespec &t0, const struct timespec &t1)
{
        if (t0.tv_sec == t1.tv_sec)
                return t0.tv_nsec > t1.tv_nsec;
        else if (t0.tv_sec > t1.tv_sec)
                return true;
        else
                return false;
}

bool operator==(const struct timespec &t0, const struct timespec &t1)
{
	return t0.tv_sec == t1.tv_sec && t0.tv_nsec == t1.tv_nsec;
}


struct timeval operator+(const struct timeval &t0, const double &t)
{
        struct timeval r;
        r.tv_sec = t0.tv_sec + static_cast<time_t>(t);
        r.tv_usec = t0.tv_usec + static_cast<suseconds_t>((t - static_cast<time_t>(t)) * 1e9);
        if (r.tv_usec > 1000000) {
                r.tv_usec -= 1000000;
                r.tv_sec++;
        }
        return r;
}

struct timeval operator-(const struct timeval &t0, const struct timeval &t1)
{
        struct timeval r = t0;

        if (r.tv_usec < t1.tv_usec) {
                --r.tv_sec;
                r.tv_usec += 1000000;
        }
        r.tv_sec -= t1.tv_sec;
        r.tv_usec -= t1.tv_usec;

        return r;
}

struct timeval& operator-=(struct timeval &t0, const struct timeval &t1)
{
        if (t0.tv_usec < t1.tv_usec) {
                --t0.tv_sec;
                t0.tv_usec += 1000000L;
        }
        t0.tv_sec -= t1.tv_sec;
        t0.tv_usec -= t1.tv_usec;

        return t0;
}

bool operator>(const struct timeval &t0, const struct timeval &t1)
{
        if (t0.tv_sec == t1.tv_sec)
                return t0.tv_usec > t1.tv_usec;
        else if (t0.tv_sec > t1.tv_sec)
                return true;
        else
                return false;
}

bool operator==(const struct timeval &t0, const struct timeval &t1)
{
	return t0.tv_sec == t1.tv_sec && t0.tv_usec == t1.tv_usec;
}


#ifndef HAVE_GMTIME_R
#include "threads.h"

static pthread_mutex_t gmtime_r_mutex = PTHREAD_MUTEX_INITIALIZER;

struct tm *gmtime_r(const time_t *_Time, struct tm *_Tm)
{
  pthread_mutex_lock (&gmtime_r_mutex);
  struct tm *p = gmtime(_Time);
  if (p && _Tm) memcpy (_Tm, p, sizeof (struct tm));
  pthread_mutex_unlock (&gmtime_r_mutex);
  return p;
}

static pthread_mutex_t gmtime_local_mutex = PTHREAD_MUTEX_INITIALIZER;

struct tm *localtime_r(const time_t *_Time,struct tm *_Tm)
{
  pthread_mutex_lock (&gmtime_local_mutex);
  struct tm *p = localtime(_Time);
  if (p && _Tm) memcpy (_Tm, p, sizeof (struct tm));
  pthread_mutex_unlock (&gmtime_local_mutex);
  return p;
}

#endif
