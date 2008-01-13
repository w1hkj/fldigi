// ----------------------------------------------------------------------------
// interleave.cxx  --  MFSK (de)interleaver
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#include <config.h>

#include <cstring>

#include "interleave.h"

// ---------------------------------------------------------------------- 

interleave::interleave (int _size, int dir)
{
	size = _size;
	if (size == 4) depth = 10;
	else depth = 5;
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

