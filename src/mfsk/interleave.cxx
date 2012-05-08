// ----------------------------------------------------------------------------
// interleave.cxx  --  MFSK (de)interleaver
//
// Copyright (C) 2006-2008
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

#include <config.h>

#include <cstring>

#include "interleave.h"

// ---------------------------------------------------------------------- 

interleave::interleave (int _size, int dir)
{
	size = _size;
	if (size == -1) { // dominoEX interleaver
		size = 4;
		depth = 4;
//BPSK+FEC+interleaver. First digit is size, then number of concatenated square interleavers
	} else if (size == -220) { // BPSK FEC + Interleaver 2x2x20
		size = 2;
		depth = 20;
	} else if (size == -240) { // BPSK FEC + Interleaver 2x2x40
		size = 2;
		depth = 40;
	} else if (size == -280) { // BPSK FEC + Interleaver 2x2x80
		size = 2;
		depth = 80;
	} else if (size == -2160) { // BPSK FEC + Interleaver 2x2x160
		size = 2;
		depth = 160;
	} else if (size == -488) { // THOR 44/88 Interleaver 4x4x88
		size = 4;
		depth = 88;
	} else if (size == 5)
		depth = 5;
	else
		depth = 10;
	direction = dir;
	table = new unsigned char [depth * size * size];
	memset(table, 0, depth * size * size);
}

interleave::~interleave ()
{
	delete [] table;
}


void interleave::symbols(unsigned char *psyms)
{
	int i, j, k;

	for (k = 0; k < depth; k++) {
		for (i = 0; i < size; i++)
			for (j = 0; j < size - 1; j++)
				*tab(k, i, j) = *tab(k, i, j + 1);

		for (i = 0; i < size; i++)
			*tab(k, i, size - 1) = psyms[i];

		for (i = 0; i < size; i++) {
			if (direction == INTERLEAVE_FWD)
				psyms[i] = *tab(k, i, size - i - 1);
			else
				psyms[i] = *tab(k, i, i);
		}
	}
}

void interleave::bits(unsigned int *pbits)
{
	unsigned char syms[size];
	int i;

	for (i = 0; i < size; i++)
		syms[i] = (*pbits >> (size - i - 1)) & 1;

	symbols(syms);

	for (*pbits = i = 0; i < size; i++)
		*pbits = (*pbits << 1) | syms[i];
}

// ---------------------------------------------------------------------- 

