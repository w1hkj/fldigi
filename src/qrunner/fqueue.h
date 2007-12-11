// ----------------------------------------------------------------------------
//	fqueue.h
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

#ifndef FQUEUE_H_
#define FQUEUE_H_

#include <stdexcept>
#include <cassert>
#include "ringbuffer.h"
// #include <iostream>
// #include <cstdio>
// #include <stacktrace.h>

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

class func_base
{
public:
        virtual void destroy(bool run) = 0;
        virtual ~func_base() { }
};

template <typename F>
class func_wrap : public func_base
{
public:
        explicit func_wrap(const F &f_) : f(f_) { }
        virtual void destroy(bool run)
        {
                if (run) f();
                this->~func_wrap();
        }
private:
        F f;
};


class fqueue
{
public:
        fqueue(size_t count = 2048, size_t nqueues_ = 1, size_t blocksize_ = 128)
                : nqueues(nqueues_), blocksize(blocksize_)
        {
                rb = new jack_ringbuffer_t*[nqueues];
                for (size_t i = 0; i < nqueues; i++)
                        if ((rb[i] = jack_ringbuffer_create(blocksize * count)) == 0)
                                throw std::bad_alloc();
        }
        ~fqueue()
        {
                for (size_t i = 0; i < nqueues; i++) {
                        drop(i);
                        jack_ringbuffer_free(rb[i]);
                }
                delete [] rb;
        }

        bool empty(size_t q)
        {
                if (q != nqueues)
                        return jack_ringbuffer_read_space(rb[q]) == 0;

                for (size_t i = 0; i < nqueues; i++)
                        if (jack_ringbuffer_read_space(rb[i]) > 0)
                                return false;
                return true;
        }
        bool empty(void) { return empty(nqueues); }

        bool full(size_t q)
        {
                if (q != nqueues)
                        return jack_ringbuffer_write_space(rb[q]) == 0;

                for (size_t i = 0; i < nqueues; i++)
                        if (jack_ringbuffer_write_space(rb[i]) > 0)
                                return false;
                return true;
        }

        size_t size(size_t q)
        {
                if (q != nqueues)
                        return jack_ringbuffer_read_space(rb[q]) / blocksize;

                size_t n = 0;
                for (size_t i = 0; i < nqueues; i++)
                        n += jack_ringbuffer_read_space(rb[i]) / blocksize;
                return n;
        }
        size_t size(void) { return size(nqueues); }

        size_t queues(void) { return nqueues; }

        template <class T>
        bool push(const T &t, size_t q)
        {
                // If we have any space left at all, it will be at least
                // a blocksize. It will not wrap around the end of the rb.
                jack_ringbuffer_get_write_vector(rb[q], wvec);
                if (unlikely(wvec[0].len < blocksize))
                        return false;

                assert(blocksize >= sizeof(func_wrap<T>));
                // we assume a no-throw ctor!
                new (wvec[0].buf) func_wrap<T>(t);
                // std::cout << time(0) << " push " << typeid(*reinterpret_cast<func_base *>(wvec[0].buf)).name() << std::endl;
                //pstack(1);
                jack_ringbuffer_write_advance(rb[q], blocksize);

                return true;
        }

        bool pop(size_t q, bool exec = false)
        {
                size_t start, end;
                if (q != nqueues) // pull from named queue
                        start = end = q;
                else { // pull first available element
                        start = 0;
                        end = nqueues - 1;
                }

                for (size_t i = start; i <= end; i++) {
                        jack_ringbuffer_get_read_vector(rb[i], rvec);
                        if (rvec[0].len < blocksize)
                                continue;
                        // std::cout << time(0) << " pop " << typeid(*reinterpret_cast<func_base *>(rvec[0].buf)).name() << std::endl;
                        reinterpret_cast<func_base *>(rvec[0].buf)->destroy(exec);
                        jack_ringbuffer_read_advance(rb[i], blocksize);

                        return true;
                }

                return false;
        }

        bool execute(void) { return pop(nqueues, true); }

        size_t drop(size_t q)
        {
                size_t n = 0;
                while (pop(q, false))
                        ++n;
                return n;
        }

protected:
        jack_ringbuffer_t **rb;
        jack_ringbuffer_data_t rvec[2], wvec[2];
        size_t nqueues, blocksize;
};

#endif // FQUEUE_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
