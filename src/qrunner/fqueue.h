// ----------------------------------------------------------------------------
//	fqueue.h
//
// Copyright (C) 2007-2008
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
        fqueue(size_t count = 2048, size_t blocksize_ = 128)
                : blocksize(blocksize_)
        {
		rb = new fqueue_ringbuffer_t(blocksize * count);
        }
        ~fqueue()
        {
		drop();
		delete rb;
        }

        bool empty(void) { return rb->read_space() == 0; }
        bool full(void)  { return rb->write_space() == 0; }
        size_t size(void) { return rb->read_space() / blocksize; }

        template <class T>
        bool push(const T& t)
        {
                // If we have any space left at all, it will be at least
                // a blocksize. It will not wrap around the end of the rb.
                if (unlikely(rb->get_wv(wvec, blocksize) < blocksize))
                        return false;

                assert(blocksize >= sizeof(func_wrap<T>));
                // we assume a no-throw ctor!
                new (wvec[0].buf) func_wrap<T>(t);
                rb->write_advance(blocksize);

                return true;
        }

        bool pop(bool exec = false)
        {
		if (rb->get_rv(rvec, blocksize) < blocksize)
			return false;
		reinterpret_cast<func_base *>(rvec[0].buf)->destroy(exec);
		rb->read_advance(blocksize);

		return true;
        }

        bool execute(void) { return pop(true); }

        size_t drop(void)
        {
                size_t n = 0;
                while (pop(false))
                        ++n;
                return n;
        }

protected:
        fqueue_ringbuffer_t* rb;
        fqueue_ringbuffer_t::vector_type rvec[2], wvec[2];
        size_t blocksize;
};

#endif // FQUEUE_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
