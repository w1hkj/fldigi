// ----------------------------------------------------------------------------
//	qrunner.h
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

#ifndef QRUNNER_H_
#define QRUNNER_H_

#ifndef NDEBUG
#	include <iostream>
#endif

#include <stdexcept>
#include <boost/bind.hpp>
#include <boost/bind/protect.hpp>

#include "threads.h"
#include "qrunner/fqueue.h"
#include "timeops.h"

class qexception : public std::exception
{
public:
        qexception(const char *msg_) : msg(msg_) { }
        qexception(int e) : msg(strerror(e)) { }
        ~qexception() throw() { }
        const char *what(void) const throw() { return msg.c_str(); }
private:
        std::string msg;
};

struct signal_after
{
        Fl_Mutex *m;
        Fl_Cond *c;

        signal_after(Fl_Mutex *m_, Fl_Cond *c_) : m(m_), c(c_) { }
        template <typename F>
        void operator()(const F& f) const
        {
                f();
                fl_lock(m);
                fl_cond_signal(c);
                fl_unlock(m);
        }
};

struct deadline
{
        struct timespec dl;

        deadline(const struct timespec &dl_) : dl(dl_) { }
        template <typename F>
        void operator()(const F& f) const
        {
                struct timespec now;
                if (clock_gettime(CLOCK_REALTIME, &now) == -1)
                        throw qexception(errno);

                if (dl > now)
                        f();
#ifndef NDEBUG
                else
                        std::cerr << "too late for event\n";
#endif
        }
};

#ifndef NDEBUG
struct timer
{
        struct timespec t;

        timer(const struct timespec &t_) : t(t_) { }
        template <typename F>
        void operator()(const F& f) const
        {
                struct timespec now;
                if (clock_gettime(CLOCK_REALTIME, &now) == -1)
                        throw qexception(errno);

                struct timespec diff = now - t;
                std::cout << "t: " << (double)diff.tv_sec + diff.tv_nsec/1e9 << '\n';

                f();
        }
};
#endif

class qrunner
{
public:
        qrunner(size_t npri_ = 1);
        ~qrunner();

        void attach(void);
        static void attach_cb(void *arg);
        void detach(void);

        template <typename F>
        bool request(const F &f, size_t pri = 0)
        {
                if (fifo->push(f, pri)) {
#ifdef NDEBUG
                        write(pfd[1], "", 1);
#else
                        assert(write(pfd[1], "", 1) == 1);
#endif
                        return true;
                }

#ifndef NDEBUG
                std::cerr << "fifo full\n";
#endif
                return false;
        }

        template <typename F>
        bool request_sync(const F &f, size_t pri = 0)
        {
                if (!attached)
                        return request(f, pri);

                Fl_Mutex m = PTHREAD_MUTEX_INITIALIZER;
                Fl_Cond c = PTHREAD_COND_INITIALIZER;

                fl_lock(&m);
                signal_after sa(&m, &c);
                for (;;) {
                        if (request(boost::bind<void>(sa, f), pri))
                                break;
                        sched_yield();
                }
                fl_cond_wait(&c, &m);
                fl_unlock(&m);

                return true;
        }

        template <typename F>
        bool request_dl(const F &f, double dtime, size_t pri = 0)
        {
                struct timespec now, dl;
                if (clock_gettime(CLOCK_REALTIME, &now) == -1)
                        throw qexception(errno);
                dl = now + dtime;

                deadline d(dl);
                return request(boost::bind<void>(d, f), pri);
        }

#ifndef NDEBUG
        template <typename F>
        bool request_time(const F &f, size_t pri = 0)
        {
                struct timespec now;
                if (clock_gettime(CLOCK_REALTIME, &now) == -1)
                        throw qexception(errno);
                timer t(now);
                return request(boost::bind<void>(t, f), pri);
        }
#endif

        static void execute(int fd, void *arg);

        size_t nprio(void) { return fifo->queues(); }
        void drop(size_t pri) { fifo->drop(pri); }
        size_t size(size_t pri) { return fifo->size(pri); }

protected:
        fqueue *fifo;
        int pfd[2];
        size_t npri;
        bool attached;
        char rbuf[64];
};


#define QUEUE QUEUE_ASYNC

#define CMP_CB(...) __VA_ARGS__
#define QUEUE_ASYNC(...)                                                \
        do {                                                            \
                if (GET_THREAD_ID() != FLMAIN_TID)                      \
                        cbq[GET_THREAD_ID()]->request(boost::bind(__VA_ARGS__)); \
                else                                                    \
                        boost::bind(__VA_ARGS__)();                     \
        } while (0)

#define QUEUE_SYNC(...)                                                 \
        do {                                                            \
                if (thread_id_ != FLMAIN_TID)                           \
                        cbq[GET_THREAD_ID()]->request_sync(boost::protect(boost::bind(__VA_ARGS__))); \
                else                                                    \
                        boost::bind(__VA_ARGS__)();                     \
        } while (0)

#define QUEUE_DL(d_, ...)                                               \
        do {                                                            \
                if (thread_id_ != FLMAIN_TID)                           \
                        cbq[GET_THREAD_ID()]->request_dl(boost::protect(boost::bind(__VA_ARGS__)), d_); \
                else                                                    \
                        boost::bind(__VA_ARGS__)();                     \
        } while (0)



extern qrunner *cbq[NUM_THREADS];

#endif // QRUNNER_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
