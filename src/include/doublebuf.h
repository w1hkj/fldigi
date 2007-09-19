// ----------------------------------------------------------------------------
//      doublebuf.h
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

#ifndef DOUBLE_BUF_H
#define DOUBLE_BUF_H

template <typename T>
class double_buffer
{
public:
        double_buffer(void) : bufsize(0), data(0), sw(0) { }

        double_buffer(size_t n)
                : bufsize(2 * n), data(0), sw(0)
        {
                data = new T[bufsize];
                other = data + bufsize / 2;
        }

        double_buffer(const T* array, size_t n)
                : bufsize(2 * n), data(0), sw(0)
        {
                data = new T[bufsize];
                for (size_t i = 0; i < n; i++)
                        data[i] = array[i];
                other = data + bufsize / 2;
        }

        double_buffer(const double_buffer<T>& db)
        {
                bufsize = db.bufsize;
                data = new T[bufsize];
                for (size_t i = 0; i < bufsize; i++)
                        data[i] = db.data[i];
                other = data + bufsize / 2;
                sw = db.sw;
        }

        ~double_buffer() { delete [] data; }

        void alloc(size_t n)
        {
                delete [] data;

                bufsize = 2 * n;
                data = new T[bufsize];
                other = data + bufsize / 2;
                sw = 0;
        }

        double_buffer &operator=(const double_buffer<T>& db)
        {
                delete [] data;

                bufsize = db.bufsize;
                data = new T[bufsize];
                for (size_t i = 0; i < bufsize; i++)
                        data[i] = db.data[i];
                other = data + bufsize / 2;
                sw = db.sw;

                return *this;
        }

        size_t size(void)        { return bufsize / 2; }

        T*     first(void)	 { return data; }
        T*     second(void)      { return other; }
               operator T*(void) { return sw ? second() : first(); }

        double_buffer<T> &operator++(void) { sw ^= 1; return *this; } // prefix op. for swap
        double_buffer<T> &operator--(void) { sw  = 0; return *this; } // prefix op. for reset

protected:
        size_t bufsize;
        T* data;
        T* other;
        unsigned char sw;
};

#endif // DOUBLE_BUF_H

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
