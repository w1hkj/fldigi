// ----------------------------------------------------------------------------
// threads.h
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

#ifndef THREADS_H_
#define THREADS_H_

#include <config.h>

#include <pthread.h>
#include <stdint.h>

#include <semaphore.h>
#if !HAVE_SEM_TIMEDWAIT
#  include <time.h>
int sem_timedwait(sem_t* sem, const struct timespec* abs_timeout);
#endif

int sem_timedwait_rel(sem_t* sem, double rel_timeout);
int pthread_cond_timedwait_rel(pthread_cond_t* cond, pthread_mutex_t* mutex, double rel_timeout);

enum {
	INVALID_TID = -1,
	TRX_TID, QRZ_TID, RIGCTL_TID, NORIGCTL_TID, EQSL_TID, ADIF_RW_TID,
	XMLRPC_TID,
	ARQ_TID, ARQSOCKET_TID,
	FLMAIN_TID,
	NUM_THREADS, NUM_QRUNNER_THREADS = NUM_THREADS - 1
};

#ifdef __linux__
void linux_log_tid(void);
#  define LOG_THREAD_ID() linux_log_tid()
#else
#  define LOG_THREAD_ID()  /* nothing */
#endif

#if USE_TLS
#       define THREAD_ID_TYPE __thread intptr_t
#       define CREATE_THREAD_ID() thread_id_ = INVALID_TID
#	define SET_THREAD_ID(x)   do { thread_id_ = (x); LOG_THREAD_ID(); } while (0)
#	define GET_THREAD_ID()    thread_id_
#else
#       define THREAD_ID_TYPE pthread_key_t
#	define CREATE_THREAD_ID() pthread_key_create(&thread_id_, NULL)
#	define SET_THREAD_ID(x)   do { pthread_setspecific(thread_id_, (const void *)(x + 1)); LOG_THREAD_ID(); } while (0)
#	define GET_THREAD_ID()    ((intptr_t)pthread_getspecific(thread_id_) - 1)
#endif // USE_TLS
extern THREAD_ID_TYPE thread_id_;


#ifndef NDEBUG
#  include "debug.h"
bool thread_in_list(int id, const int* list);
#  define ENSURE_THREAD(...)						\
	do {								\
		int id_ = GET_THREAD_ID();				\
		int t_[] = { __VA_ARGS__, INVALID_TID };		\
		if (!thread_in_list(id_, t_))				\
			LOG_ERROR("bad thread context: %d", id_);	\
	} while (0)
#  define ENSURE_NOT_THREAD(...)					\
	do {								\
		int id_ = GET_THREAD_ID();				\
		int t_[] = { __VA_ARGS__, INVALID_TID };		\
		if (thread_in_list(id_, t_))				\
			LOG_ERROR("bad thread context: %d", id_);	\
	} while (0)
#else
#  define ENSURE_THREAD(...) ((void)0)
#  define ENSURE_NOT_THREAD(...) ((void)0)
#endif // ! NDEBUG


// On POSIX systems we cancel threads by sending them SIGUSR2,
// which will also interrupt blocking calls.  On woe32 we use
// pthread_cancel and there is no good/sane way to interrupt.
#ifndef __WOE32__
#  define SET_THREAD_CANCEL()					\
	do {							\
		sigset_t usr2;					\
		sigemptyset(&usr2);				\
		sigaddset(&usr2, SIGUSR2);			\
		pthread_sigmask(SIG_UNBLOCK, &usr2, NULL);	\
	} while (0)
#  define TEST_THREAD_CANCEL() /* nothing */
#  define CANCEL_THREAD(t__) pthread_kill(t__, SIGUSR2)
#else
// threads have PTHREAD_CANCEL_ENABLE, PTHREAD_CANCEL_DEFERRED when created
#  define SET_THREAD_CANCEL() /* nothing */
#  define TEST_THREAD_CANCEL() pthread_testcancel()
#  define CANCEL_THREAD(t__) pthread_cancel(t__);
#endif

#include "fl_lock.h"

/// This ensures that a mutex is always unlocked when leaving a function or block.
class guard_lock
{
public:
	guard_lock(pthread_mutex_t* m);
	~guard_lock(void);
private:
	pthread_mutex_t* mutex;
};

/// This wraps together a mutex and a condition variable which are used
/// together very often for queues etc...
class syncobj
{
	pthread_mutex_t m_mutex ;
	pthread_cond_t m_cond ;
public:
	syncobj();
	~syncobj();
	pthread_mutex_t * mtxp(void) { return & m_mutex; }
	void signal();
	bool wait( double seconds );
};

#endif // !THREADS_H_
