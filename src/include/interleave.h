// ----------------------------------------------------------------------------
// interleave.h  --  MFSK (de)interleaver
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef _INTERLEAVE_H
#define _INTERLEAVE_H

#define INTERLEAVE_FWD  0
#define INTERLEAVE_REV  1
#define PUNCTURE        128

class interleave 
{
protected:
	int size;
	int depth;
	int len;
	int direction;
	unsigned char *table;
	unsigned char *tab(int i, int j, int k) {
		return &table[(size * size * i) + (size * j) + k];
	}

public:
	interleave(int _size, int depth, int dir);
	~interleave();
	void symbols (unsigned char *psyms);
	void bits (unsigned int *pbits);
	void flush();
};

// ----------------------------------------------------------------------------

#endif
