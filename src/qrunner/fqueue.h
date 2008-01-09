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
#include "util.h"
// #include <iostream>
// #include <cstdio>
// #include <stacktrace.h>

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
        typedef ringbuffer<char> fqueue_ringbuffer_t;

public:
        fqueue(size_t count = 2048, size_t nqueues_ = 1, size_t blocksize_ = 128)
                : nqueues(nqueues_), blocksize(blocksize_)
        {
                rb = new fqueue_ringbuffer_t*[nqueues];
                for (size_t i = 0; i < nqueues; i++)
                        rb[i] = new fqueue_ringbuffer_t(blocksize * count);
        }
        ~fqueue()
        {
                for (size_t i = 0; i < nqueues; i++) {
                        drop(i);
                        delete rb[i];
                }
                delete [] rb;
        }

        bool empty(size_t q)
        {
                if (q != nqueues)
                        return rb[q]->read_space() == 0;

                for (size_t i = 0; i < nqueues; i++)
                        if (rb[i]->read_space() > 0)
                                return false;
                return true;
        }
        bool empty(void) { return empty(nqueues); }

        bool full(size_t q)
        {
                if (q != nqueues)
                        return rb[q]->write_space() == 0;

                for (size_t i = 0; i < nqueues; i++)
                        if (rb[i]->write_space() > 0)
                                return false;
                return true;
        }

        size_t size(size_t q)
        {
                if (q != nqueues)
                        return rb[q]->read_space() / blocksize;

                size_t n = 0;
                for (size_t i = 0; i < nqueues; i++)
                        n += rb[i]->read_space() / blocksize;
                return n;
        }
        size_t size(void) { return size(nqueues); }

        size_t queues(void) { return nqueues; }

        template <class T>
        bool push(const T &t, size_t q)
        {
                // If we have any space left at all, it will be at least
                // a blocksize. It will not wrap around the end of the rb.
                rb[q]->get_wv(wvec);
                if (unlikely(wvec[0].len < blocksize))
                        return false;

                assert(blocksize >= sizeof(func_wrap<T>));
                // we assume a no-throw ctor!
                new (wvec[0].buf) func_wrap<T>(t);
                rb[q]->write_advance(blocksize);

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
                        rb[i]->get_rv(rvec);
                        if (rvec[0].len < blocksize)
                                continue;
                        reinterpret_cast<func_base *>(rvec[0].buf)->destroy(exec);
                        rb[i]->read_advance(blocksize);

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
        fqueue_ringbuffer_t **rb;
        fqueue_ringbuffer_t::vector_type rvec[2], wvec[2];
        size_t nqueues, blocksize;
};

#endif // FQUEUE_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
