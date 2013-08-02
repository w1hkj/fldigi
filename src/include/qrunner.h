// ----------------------------------------------------------------------------
//	qrunner.h
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

#ifndef QRUNNER_H_
#define QRUNNER_H_

#ifndef NDEBUG
#    include <inttypes.h>
#    include "debug.h"
#endif

#include <unistd.h>
#include <cerrno>
#include <stdexcept>
#include <cstring>

#if HAVE_STD_BIND
#   include <functional>
namespace qrbind {
    using std::bind;
};
#elif HAVE_STD_TR1_BIND
#   include <tr1/functional>
namespace qrbind {
    using std::tr1::bind;
};
#else
#   error need std::bind or std::tr1::bind
#endif

#include "threads.h"
#include "qrunner/fqueue.h"

#ifndef __MINGW32__
#  define QRUNNER_READ(fd__, buf__, len__) read(fd__, buf__, len__)
#  define QRUNNER_WRITE(fd__, buf__, len__) write(fd__, buf__, len__)
#else
#  define QRUNNER_READ(fd__, buf__, len__) recv(fd__, (char*)buf__, len__, 0)
#  define QRUNNER_WRITE(fd__, buf__, len__) send(fd__, (const char*)buf__, len__, 0)
#endif

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

struct fsignal
{
        typedef void result_type;
        pthread_mutex_t* m;
        pthread_cond_t* c;

        fsignal(pthread_mutex_t* m_, pthread_cond_t* c_) : m(m_), c(c_) { }
        void operator()(void) const
        {
                pthread_mutex_lock(m);
                pthread_cond_signal(c);
                pthread_mutex_unlock(m);
        }
};

struct nop
{
        typedef void result_type;
        void operator()(void) const { }
};

class qrunner
{
public:
        qrunner();
        ~qrunner();

        void attach(void);
        void detach(void);

        template <typename F>
        bool request(const F& f)
        {
                if (fifo->push(f)) {
#ifdef NDEBUG
                        if (unlikely(QRUNNER_WRITE(pfd[1], "", 1) != 1))
                                throw qexception(errno);
#else
                        assert(QRUNNER_WRITE(pfd[1], "", 1) == 1);
#endif
                        return true;
                }

#ifndef NDEBUG
//Remi's extra debugging info		LOG_ERROR("qrunner: thread %" PRIdPTR " fifo full!", GET_THREAD_ID());
		LOG_ERROR("qrunner: thread %" PRIdPTR " fifo full at %s!", GET_THREAD_ID(),typeid(F).name() );

#endif
                return false;
        }

        template <typename F>
        bool request_sync(const F& f)
        {
                if (!attached)
                        return request(f);

                for (;;) {
                        if (request(f))
                                break;
                        sched_yield();
                }
                static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
                static pthread_cond_t c = PTHREAD_COND_INITIALIZER;
                fsignal s(&m, &c);
                pthread_mutex_lock(&m);
                for (;;) {
                        if (request(s))
                                break;
                        sched_yield();
                }
                pthread_cond_wait(&c, &m);
                pthread_mutex_unlock(&m);

                return true;
        }

        static void execute(int fd, void *arg);
        void flush(void);

        void drop(void) { fifo->drop(); }
        size_t size(void) { return fifo->size(); }

protected:
        fqueue *fifo;
        int pfd[2];
        bool attached;
	bool inprog;
public:
	bool drop_flag;
};


extern qrunner *cbq[NUM_QRUNNER_THREADS];

#if BENCHMARK_MODE
#define REQ(...) ((void)0)
#define REQ_DROP(...) ((void)0)
#define REQ_SYNC(...) ((void)0)
#define REQ_FLUSH(...) ((void)0)
#define QRUNNER_DROP(...) ((void)0)
#else

#define REQ REQ_ASYNC
#define REQ_DROP REQ_ASYNC_DROP

#define REQ_ASYNC(...)							\
	do {								\
		if (GET_THREAD_ID() != FLMAIN_TID)			\
			cbq[GET_THREAD_ID()]->request(qrbind::bind(__VA_ARGS__)); \
		else							\
			qrbind::bind(__VA_ARGS__)();			\
	} while (0)
#define REQ_SYNC(...)							\
	do {								\
		if (GET_THREAD_ID() != FLMAIN_TID)			\
			cbq[GET_THREAD_ID()]->request_sync(qrbind::bind(__VA_ARGS__)); \
		else							\
			qrbind::bind(__VA_ARGS__)();			\
	} while (0)

#define REQ_ASYNC_DROP(...)						\
        do {                                                            \
                if (GET_THREAD_ID() != FLMAIN_TID) {			\
			if (unlikely(cbq[GET_THREAD_ID()]->drop_flag))	\
				break;					\
                        cbq[GET_THREAD_ID()]->request(qrbind::bind(__VA_ARGS__)); \
		}							\
                else                                                    \
                        qrbind::bind(__VA_ARGS__)();			\
        } while (0)
#define REQ_SYNC_DROP(...)						\
        do {                                                            \
                if (GET_THREAD_ID() != FLMAIN_TID) {			\
			if (unlikely(cbq[GET_THREAD_ID()]->drop_flag))	\
				break;					\
                        cbq[GET_THREAD_ID()]->request_sync(qrbind::bind(__VA_ARGS__)); \
		}							\
                else                                                    \
                        qrbind::bind(__VA_ARGS__)();			\
        } while (0)

#define REQ_FLUSH(t_)                                                   \
        do {                                                            \
		if (GET_THREAD_ID() != FLMAIN_TID)			\
			cbq[GET_THREAD_ID()]->request_sync(nop());	\
		else if (t_ < NUM_QRUNNER_THREADS)			\
			cbq[t_]->flush();				\
		else							\
			for (int i = 0; i < NUM_QRUNNER_THREADS; i++)	\
				cbq[i]->flush();			\
        } while (0)

#define QRUNNER_DROP(v_)					\
	do {							\
		if ((GET_THREAD_ID() != FLMAIN_TID))		\
			cbq[GET_THREAD_ID()]->drop_flag = v_;	\
	} while (0)
#endif // BENCHMARK_MODE

#endif // QRUNNER_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
