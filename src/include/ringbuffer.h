// ----------------------------------------------------------------------------
//	ringbuffer.h
//
// Copyright (C) 2007-2009
//		Stelios Bounanos, M0GLD
//
// C++ version of PortAudio's ringbuffer code. The copying read/write methods
// use memcpy, so it generally safe to use them only for POD types. Thread safe
// for one reader and one writer.
//
// Licensed according to original copyright notice:
//
// Author: Phil Burk, http://www.softsynth.com
// modified for SMP safety on OS X by Bjorn Roche.
// also allowed for const where possible.
// Note that this is safe only for a single-thread reader
// and a single-thread writer.
//
// This program is distributed with the PortAudio Portable Audio Library.
// For more information see: http://www.portaudio.com
// Copyright (c) 1999-2000 Ross Bencina and Phil Burk
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files
// (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
// CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <cassert>
#include <cstring>
#include "util.h"

template <typename T>
class ringbuffer
{
protected:
        size_t size, big_mask, small_mask;
        T* buf;
        volatile size_t ridx, widx;
public:
        typedef T value_type;
        typedef struct { value_type* buf; size_t len; } vector_type;

public:
        ringbuffer(size_t s)
                : ridx(0), widx(0)
        {
                assert(powerof2(s));

                size = s;
                big_mask = size * 2 - 1;
                small_mask = size - 1;
                buf = new T[size];
        }
        ~ringbuffer()
        {
                delete [] buf;
        }


        size_t read_space(void)
        {
                read_memory_barrier();
                return (widx - ridx) & big_mask;
        }
        size_t write_space(void)
        {
                return size - read_space();
        }

        void read_advance(size_t n)
        {
                write_memory_barrier();
                ridx = (ridx + n) & big_mask;
        }
        void write_advance(size_t n)
        {
                write_memory_barrier();
                widx = (widx + n) & big_mask;
        }

        size_t get_rv(vector_type v[2], size_t n = 0)
        {
                size_t rspace = read_space();
                size_t index = ridx & small_mask;

                if (n == 0 || n > rspace)
                        n = rspace;

                if (index + n > size) { // two part vector
                        v[0].buf = buf + index;
                        v[0].len = size - index;
                        v[1].buf = buf;
                        v[1].len = n - v[0].len;
                }
                else {
                        v[0].buf = buf + index;
                        v[0].len = n;
                        v[1].len = 0;
                }

                return n;
        }
        size_t read(T* dst, size_t n)
        {
                vector_type v[2];
                n = get_rv(v, n);

                memcpy(dst, v[0].buf, v[0].len * sizeof(T));
                if (v[1].len)
                        memcpy(dst + v[0].len, v[1].buf, v[1].len * sizeof(T));

                read_advance(n);
                return n;
        }
        size_t peek(T* dst, size_t n)
        {
                vector_type v[2];
                n = get_rv(v, n);

                memcpy(dst, v[0].buf, v[0].len * sizeof(T));
                if (v[1].len)
                        memcpy(dst + v[0].len, v[1].buf, v[1].len * sizeof(T));

                return n;
        }

        size_t get_wv(vector_type v[2], size_t n = 0)
        {
                size_t wspace = write_space();
                size_t index = widx & small_mask;

                if (n == 0 || n > wspace)
                        n = wspace;

                if (index + n > size) { // two part vector
                        v[0].buf = buf + index;
                        v[0].len = size - index;
                        v[1].buf = buf;
                        v[1].len = n - v[0].len;
                }
                else {
                        v[0].buf = buf + index;
                        v[0].len = n;
                        v[1].len = 0;
                }

                return n;
        }
        size_t write(const T* src, size_t n)
        {
                vector_type v[2];
                n = get_wv(v, n);

                memcpy(v[0].buf, src, v[0].len * sizeof(T));
                if (v[1].len)
                        memcpy(v[1].buf, src + v[0].len, v[1].len * sizeof(T));

                write_advance(n);
                return n;
        }
        void fill(const value_type& v)
        {
                reset();
                for (size_t i = 0; i < size; i++)
                        buf[i] = v;
                write_advance(size);

        }
        void zero(void)
        {
                reset();
                memset(buf, 0, size * sizeof(T));
                write_advance(size);
        }

        void reset(void) { ridx = widx = 0; }
        size_t length(void) { return size; }
        size_t bytes(void) { return size * sizeof(T); }
};

#endif // RINGBUFFER_H

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
