#include <config.h>

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

#ifdef __MINGW32__
#include <stdlib.h>
static void ptw32_cleanup(void)
{
       (void)pthread_win32_process_detach_np();
}
void ptw32_init(void)
{
       (void)pthread_win32_process_attach_np();
       atexit(ptw32_cleanup);
}
#endif // __MINGW32__

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
