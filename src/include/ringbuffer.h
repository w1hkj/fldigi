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

/** 
 The class template ringbuffer is used to transport data elements between
 different execution contexts (threads, OS callbacks, interrupt handlers)
 without requiring the use of any locks. This only works when there is
 a single reader and a single writer (ie. one thread or callback writes
 to the ring buffer, another thread or callback reads from it).

 The ringbuffer class manages a first-in-first-out buffer containing N 
 elements, where N must be a power of two. An element may be any size 
 (specified in bytes).

 Buffer storage is instantiated when the class Type is created and
 released when the class Type is deleted.
*/

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <cassert>
#include <cstring>
#include "util.h"

template <typename T>
class ringbuffer
{
protected:
	size_t size,			// size of buffer, a power of two
	big_mask,				// Used for wrapping indices with extra bit to distinguish full/empty.
	small_mask;				// Used for fitting indices to buffer.
	T* buf;					// Pointer to the buffer containing the actual data.
	volatile size_t ridx;	// Index of next readable element.
	volatile size_t widx;	// Index of next writable element.
public:
	typedef T value_type;
	typedef struct { value_type* buf; size_t len; } vector_type;

public:

// Instantiate ringbuffer to empty state ready to have elements written to it.
// buffer size, s, must be a power of two
	ringbuffer(size_t s) : ridx(0), widx(0) {
		assert(powerof2(s));
		size = s;
		big_mask = size * 2 - 1;
		small_mask = size - 1;
		buf = new T[2 * size];
	}

// Delete ringbuffer and all internal storage
	~ringbuffer() {
		delete [] buf;
	}

// number of elements available for reading.
	size_t read_space(void) {
		read_memory_barrier();
		return (widx - ridx) & big_mask;
	}

// number of elements available for writing.
	size_t write_space(void) {
		return size - read_space();
	}

// advance read index by n number of elements
	void read_advance(size_t n) {
		write_memory_barrier();
		ridx = (ridx + n) & big_mask;
	}

// advance write index by n number of elements
	void write_advance(size_t n) {
		write_memory_barrier();
		widx = (widx + n) & big_mask;
	}

	size_t get_rv(vector_type v[2], size_t n = 0) {
		size_t rspace = read_space();
		size_t index = ridx & small_mask;

		if (n == 0 || n > rspace)
			n = rspace;

		if (index + n > size) { // two part vector
			v[0].buf = buf + index;
			v[0].len = size - index;
			v[1].buf = buf;
			v[1].len = n - v[0].len;
		} else {
			v[0].buf = buf + index;
			v[0].len = n;
			v[1].len = 0;
		}
		return n;
	}

// read n elements from buffer, constrained by read/write indices
	size_t read(T* dst, size_t n) {
		vector_type v[2] = { {0,0}, {0,0} };
		n = get_rv(v, n);

		memcpy(dst, v[0].buf, v[0].len * sizeof(T));
		if (v[1].len)
			memcpy(dst + v[0].len, v[1].buf, v[1].len * sizeof(T));

		read_advance(n);
		return n;
	}

	size_t peek(T* dst, size_t n) {
		vector_type v[2] = { {0,0}, {0,0} };
		n = get_rv(v, n);

		memcpy(dst, v[0].buf, v[0].len * sizeof(T));
		if (v[1].len)
			memcpy(dst + v[0].len, v[1].buf, v[1].len * sizeof(T));
		return n;
	}

	size_t get_wv(vector_type v[2], size_t n = 0) {
		size_t wspace = write_space();
		size_t index = widx & small_mask;

		if (n == 0 || n > wspace)
			n = wspace;

		if (index + n > size) { // two part vector
			v[0].buf = buf + index;
			v[0].len = size - index;
			v[1].buf = buf;
			v[1].len = n - v[0].len;
		} else {
			v[0].buf = buf + index;
			v[0].len = n;
			v[1].len = 0;
		}
		return n;
	}

// write n elements to buffer, constrained by read/write indices
	size_t write(const T* src, size_t n) {
		vector_type v[2] = { {0,0}, {0,0} };
		n = get_wv(v, n);

		memcpy(v[0].buf, src, v[0].len * sizeof(T));
		if (v[1].len)
			memcpy(v[1].buf, src + v[0].len, v[1].len * sizeof(T));

		write_advance(n);
		return n;
	}

// fill buffer with elements with value v        
	void fill(const value_type& v) {
		reset();
		for (size_t i = 0; i < size; i++)
			buf[i] = v;
		write_advance(size);
	}

// fill buffer with elements whose value is zero
	void zero(void) {
		reset();
		memset(buf, 0, size * sizeof(T));
		write_advance(size);
	}

// reset the read/write indices
	void reset(void) { 
		ridx = widx = 0;
	}

// return maximum number of T elements that buffer can contain
	size_t length(void) {
		return size;
	}

// return number of memory bytes used by ringbuffer buffer store
	size_t bytes(void) {
		return size * sizeof(T);
	}

	size_t rbr_index() { return ridx; }
	size_t rbw_index() { return widx; }
};

#endif // RINGBUFFER_H

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
