//
// "$Id: threads.cxx 4748 2006-01-15 02:26:54Z mike $"
//
// Simple threading API for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2006 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

// FLTK has no multithreaded support unless the main thread calls Fl::lock().
// This main thread is the only thread allowed to call Fl::run() or Fl::wait().
// From then on FLTK will be locked except when the main thread is actually
// waiting for events from the user. Other threads must call Fl::lock() and
// Fl::unlock() to surround calls to FLTK (such as to change widgets or
// redraw them).

#include <config.h>

#include "threads.h"

int fl_create_thread(Fl_Thread & t, void *(*f) (void *), void* p) {
  return pthread_create((pthread_t*)&t, NULL, f, p);
}

int fl_mutex_init(Fl_Mutex * m) {
	return pthread_mutex_init( (pthread_mutex_t*) m, 0);
}

int fl_cond_init(Fl_Cond * c) {
	return pthread_cond_init( (pthread_cond_t *) c, 0);
}

int fl_cond_wait(Fl_Cond *c, Fl_Mutex *m) {
	return pthread_cond_wait( (pthread_cond_t *) c, (pthread_mutex_t *) m );
}

int fl_cond_signal(Fl_Cond *c) {
	return pthread_cond_signal( (pthread_cond_t *) c);
}

int fl_cond_bcast(Fl_Cond *c) {
	return pthread_cond_broadcast( (pthread_cond_t *) c);
}

int fl_lock(Fl_Mutex *m) {
	return pthread_mutex_lock((pthread_mutex_t*)m);
}

int fl_unlock(Fl_Mutex *m) {
	return pthread_mutex_unlock((pthread_mutex_t*)m);
}

int fl_join (Fl_Thread  t) {
	return pthread_join ((pthread_t) t, 0);
}

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


#if USE_TLS
	__thread  int thread_id_;
#else
	pthread_key_t thread_id_;
#endif
