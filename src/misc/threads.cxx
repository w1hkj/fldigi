// ----------------------------------------------------------------------------
// threads.cxx
//
// Copyright (C) 2007-2009
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>
#include <stdexcept>
#include <string.h>
#include <errno.h>
#include "threads.h"

THREAD_ID_TYPE thread_id_;

#include "timeops.h"

#if !HAVE_SEM_TIMEDWAIT
#  include <semaphore.h>
#  include <time.h>
#  include <errno.h>
#  include <unistd.h>
int sem_timedwait(sem_t* sem, const struct timespec* abs_timeout)
{
	int r;

	for (;;) {
		r = sem_trywait(sem);
		if (r == 0 || (r == -1 && errno != EAGAIN))
			return r;

		if (abs_timeout->tv_nsec < 0 || abs_timeout->tv_nsec >= 1000000000L) {
			errno = EINVAL;
			return -1;
		}

		struct timespec now;
		clock_gettime(CLOCK_REALTIME, &now);
		if (now == *abs_timeout || now > *abs_timeout) {
			errno = ETIMEDOUT;
			return -1;
		}

		usleep(100);
	}
}
#endif // !HAVE_SEM_TIMEDWAIT

int sem_timedwait_rel(sem_t* sem, double rel_timeout)
{
        struct timespec t;
        clock_gettime(CLOCK_REALTIME, &t);
        t = t + rel_timeout;

        return sem_timedwait(sem, &t);
}

int pthread_cond_timedwait_rel(pthread_cond_t* cond, pthread_mutex_t* mutex, double rel_timeout)
{
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &t);
	t = t + rel_timeout;

	return pthread_cond_timedwait(cond, mutex, &t);
}

#ifndef NDEBUG
bool thread_in_list(int id, const int* list)
{
	while (*list != INVALID_TID)
		if (id == *list++)
			return true;
	return false;
}
#endif

#ifdef __linux__
#  ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#  endif
#  include <unistd.h>
#  include <sys/syscall.h>
#  include "debug.h"
void linux_log_tid(void)
{
	LOG_DEBUG(PACKAGE_TARNAME " thread %" PRIdPTR " is LWP %ld", GET_THREAD_ID(), syscall(SYS_gettid));
}
#endif

// Synchronization objects.

guard_lock::guard_lock(pthread_mutex_t* m) : mutex(m)
{
	pthread_mutex_lock(mutex);
}

guard_lock::~guard_lock(void)
{
	pthread_mutex_unlock(mutex);
}

syncobj::syncobj()
{
	pthread_mutex_init( & m_mutex, NULL );
	pthread_cond_init( & m_cond, NULL );
}

syncobj::~syncobj()
{
	pthread_mutex_destroy( & m_mutex );
	pthread_cond_destroy( & m_cond );
}

void syncobj::signal()
{
	int rc = pthread_cond_signal( &m_cond );
	if( rc )
	{
		throw std::runtime_error(strerror(rc));
	}
}

bool syncobj::wait( double seconds )
{
	int rc = pthread_cond_timedwait_rel( &m_cond, &m_mutex, seconds );
	switch( rc )
	{
	case 0 : return true ;
	default : throw std::runtime_error(strerror(rc));
	case ETIMEDOUT: return false ;
	}
}



