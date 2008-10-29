#ifndef THREADS_H_
#define THREADS_H_

#include <config.h>

#include <pthread.h>

#include <semaphore.h>
#if !HAVE_SEM_TIMEDWAIT
#  include <time.h>
int sem_timedwait(sem_t* sem, const struct timespec* abs_timeout);
#endif

int sem_timedwait_rel(sem_t* sem, double rel_timeout);
int pthread_cond_timedwait_rel(pthread_cond_t* cond, pthread_mutex_t* mutex, double rel_timeout);

// 3 threads use qrunner
enum { INVALID_TID = -1, TRX_TID, QRZ_TID, RIGCTL_TID, NORIGCTL_TID,
#if USE_XMLRPC
       XMLRPC_TID,
#endif
       ARQ_TID, ARQSOCKET_TID,
       FLMAIN_TID, NUM_THREADS,
       NUM_QRUNNER_THREADS = NUM_THREADS - 1 };

#if USE_TLS
#       define THREAD_ID_TYPE __thread int
#	define CREATE_THREAD_ID() thread_id_ = INVALID_TID
#	define SET_THREAD_ID(x)   thread_id_ = (x)
#	define GET_THREAD_ID()    thread_id_
#else
#       define THREAD_ID_TYPE pthread_key_t
#	define CREATE_THREAD_ID() pthread_key_create(&thread_id_, 0)
#	define SET_THREAD_ID(x)   pthread_setspecific(thread_id_, (void *)(x))
#	define GET_THREAD_ID()    (int)pthread_getspecific(thread_id_)
#endif // USE_TLS


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

extern THREAD_ID_TYPE thread_id_;

#include "fl_lock.h"

#endif // !THREADS_H_
