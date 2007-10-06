// ----------------------------------------------------------------------------
//      mbuffer.h
//
// Copyright (C) 2007
//              Stelios Bounanos, M0GLD
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


// A simple vector wrapper for fldigi's double-buffering needs.
// Most vector operations are provided for mbuffers by redirecting them to the
// current vector.

// The template arguments are
// 1) T - the type
// 2) S - the apparent mbuffer size, i.e., the size of each vector. Defaults to 0.
//    A mbuffer instantiated with S == 0 is not very useful until resized
//    with alloc().
// 3) N - the number of vectors that we can cycle between. Defaults to 1.

// Things to note:
// 1) There is a T* conversion operator
// 2) Operations that modify the length of the container are not provided
// 3) Comparison operators are not implemented (but see (1)!)
// 4) mbuffer<T, 0, N> is meant to be used when we don't know the size at
//    compile time, in which case we resize with alloc. The compiler will treat
//    mbuffers resized to different lengths in this way as objects of the same type.

#ifndef MBUFFER_H
#define MBUFFER_H

#include <vector>
#include <algorithm>

#ifndef NDEBUG
#include <iosfwd>
#include <iterator>
#endif

#ifndef NDEBUG
template <typename T, std::size_t S, std::size_t N>
class mbuffer;
template <typename T, std::size_t S, std::size_t N>
std::ostream& operator<<(std::ostream& o, const mbuffer<T, S, N>& b);
#endif // NDEBUG

template <typename T, std::size_t S = 0, std::size_t N = 1>
class mbuffer
{
protected:
        std::vector<T> data[N];
	mutable std::size_t cur;

public:
        explicit mbuffer(void)
        {
                alloc(S);
        }
        explicit mbuffer(std::size_t n)
        {
                alloc(n);
        }
        mbuffer(const std::vector<T>& v)
        {
                data[0] = v;
                // resize 1 to N only
                alloc(data[0].size(), 1);
        }
        mbuffer(const T* a, std::size_t n)
        {
                data[0].assign(a, a + n);
                // resize 1 to N only
                alloc(n, 1);
        }
        void alloc(std::size_t n, std::size_t start = 0)
        {
                for (size_t i = start; i < N; ++i)
                        data[i].resize(n);
                cur = 0;
        }

        typedef typename std::vector<T>::iterator 		iterator;
        typedef typename std::vector<T>::const_iterator 	const_iterator;

        iterator       begin(void)       { return data[cur].begin(); }
        const_iterator begin(void) const { return data[cur].begin(); }
        iterator       end(void)         { return data[cur].end(); }
        const_iterator end(void) const   { return data[cur].end(); }


        typedef typename std::vector<T>::reverse_iterator 	reverse_iterator;
        typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;

        reverse_iterator rbegin(void)             { return reverse_iterator(end()); }
        const_reverse_iterator rbegin(void) const { return const_reverse_iterator(end()); }
        reverse_iterator rend(void)               { return reverse_iterator(begin()); }
        const_reverse_iterator rend(void) const   { return const_reverse_iterator(begin()); }


        typedef typename std::vector<T>::value_type 		value_type;
        typedef typename std::vector<T>::reference 		reference;
        typedef typename std::vector<T>::const_reference	const_reference;
	typedef typename std::vector<T>::size_type		size_type;
	typedef typename std::vector<T>::difference_type	difference_type;

        // These should be the same for all vectors in data[]
        size_type size(void)     { return data[0].size(); }
        size_type max_size(void) { return data[0].max_size(); }
        size_type capacity(void) { return data[0].capacity(); }
        bool empty(void)         { return data[0].empty(); }

        // Instead of these, we provide a conversion operator for T*
        // reference operator[](size_type i)             { return data[cur][i]; }
        // const_reference operator[](size_type i) const { return data[cur][i]; }
        reference at(size_type i)                     { return data[cur].at(i); }
        const_reference at(size_type i) const         { return data[cur].at(i); }
        reference front(void)                         { return data[cur].front(); }
        const_reference front(void) const             { return data[cur].front(); }
        reference back(void)                          { return data[cur].back(); }
        const_reference back(void) const              { return data[cur].back(); }


        // Operations that modify the size of data[cur] might invalidate
        // pointers to internal buffers. The rest of the data vectors would need
        // to be resized, e.g. with check_size below. For this reason these
        // operations are not provided, but are included here for completeness

        // void check_size(void)
        // {
        //         for (size_t i = 0; i < N; ++i)
        //                 if (data[i].size() != data[cur].size())
        //                         data[i].resize(data[cur].size());
        // }

        // mbuffer<T, S, N>& operator=(const mbuffer<T, S, N>& o)
        // {
        //         for (int i = 0; i < N; ++i)
        //                 std::copy(o.data[i].begin(), o.data[i].end(), data[i].begin());
        //         return *this;
        // }
        // std::vector<T>& operator=(const std::vector<T>& o)
        // {
        //         std::copy(o.begin(), o.end(), data[cur].begin());
        //         check_size();
        //         return data[cur];
        // }

        // The methods below would modify the length of the vector.
        // We would need to check_size() before returning from them.

        // There is no vector::assign; the one provided here fills the vector
        // with copies of the same value without causing a resize.
        void assign(const_reference v) { std::fill_n(begin(), size(), v); }
        // template <typename input_iterator>
        // void assign(input_iterator first, input_iterator last)
        // {
        //         data[cur].assign(first, last);
        // }

        // void push_back(const_reference v) { data[cur].push_back(v); }
        // void pop_back(void) { data[cur].pop_back(); }

        // iterator insert(iterator pos, const_reference v)
        // {
        //         return data[cur].insert(pos, v);
        // }
        // iterator insert(iterator pos, size_type n, const_reference v)
        // {
        //         return data[cur].insert(pos, n, v);
        // }
        // template <typename input_iterator>
        // void insert(iterator pos, input_iterator first, input_iterator last)
        // {
        //         data[cur].insert(pos, first, last);
        // }

        // iterator erase(iterator pos)                  { return data[cur].erase(pos); }
        // iterator erase(iterator first, iterator last) { return data[cur].erase(first, last); }

        // void clear(void) { data[cur].clear(); }

        void swap(mbuffer<T, S, N>& o)
        {
                for (int i = 0; i < N; ++i)
                        std::swap(data[i].begin(), data[i].end(), o.data[i].begin());
        }
        // void swap(std::vector<T>& o)
        // {
        //         std::swap(data[cur].begin(), data[cur].end(), o.begin());
        //         check_size();
        //         o.check_size();
        // }


        // and now for something completely different

        void next(void) const  { if (++cur == N) cur = 0; }
        void prev(void) const  { if (cur > 0) --cur; }
        void reset(void) const { cur = 0; }

        T* c_array(void)              { return &data[cur][0]; }
        operator T*(void)             { return c_array(); }
        const T* c_array(void) const  { return &data[cur][0]; }
        operator const T*(void) const { return c_array(); }

        std::vector<T>& vec(void)             { return data[cur]; }
        const std::vector<T>& vec(void) const { return data[cur]; }
        // We also do not provide vector<T> conversions
        // operator std::vector<T>&(void)             { return vec(); }
        // operator const std::vector<T>&(void) const { return vec(); }

	std::size_t idx(void)		    { return cur; }
	std::size_t nvec(void)              { return N; }
        std::vector<T>* vecp(std::size_t i) { return &data[i]; }

#ifndef NDEBUG
        friend std::ostream& operator<<<>(std::ostream& o, const mbuffer<T, S, N>& b);
#endif // NDEBUG
};

#ifndef NDEBUG
template <typename T, std::size_t S, std::size_t N>
std::ostream& operator<<(std::ostream& o, const mbuffer<T, S, N>& b)
{
        for (std::size_t i = 0; i < N; ++i) {
                o << '<' << i << ">\n";
                copy(b.data[i].begin(), b.data[i].end(),
                     std::ostream_iterator<T>(o, "\n"));
        }

        return o;
}
#endif // NDEBUG

template <typename T, std::size_t S, std::size_t N>
inline
void swap(mbuffer<T, S, N>& a, mbuffer<T, S, N>& b) { a.swap(b); }

#endif // MBUFFER_H

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
