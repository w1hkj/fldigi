// ----------------------------------------------------------------------------
// viterbi.cxx  --  Viterbi decoder
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// Adapted from code contained in gmfsk source code distribution.
//
// This file is part of fldigi.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "viterbi.h"
#include "misc.h"

/* ---------------------------------------------------------------------- */
viterbi::viterbi( int poly1, int poly2, unsigned int * output, int outsize )
{
	for (int i = 0; i < outsize; i++) {
		output[i] = parity(poly1 & i) | (parity(poly2 & i) << 1);
	}
	
	for (int i = 0; i < 256; i++) {
		mettab[0][i] = 128 - i;
		mettab[1][i] = i - 128;
	}
}

viterbi::~viterbi() {}

/* ---------------------------------------------------------------------- */
encoder::encoder(int k, int poly1, int poly2)
{
	int size = 1 << k;	/* size of the output table */

	output = new int[size];
// output contains 2 bits in positions 0 and 1 describing the state machine
// for each bit delay, ie: for k = 7 there are 128 possible state pairs.
// the modulo-2 addition for polynomial 1 is in bit 0
// the modulo-2 addition for polynomial 2 is in bit 1
// the allowable state outputs are 0, 1, 2 and 3
	for (int i = 0; i < size; i++) {
		output[i] = parity(poly1 & i) | (parity(poly2 & i) << 1);
	}
	shreg = 0;
	shregmask = size - 1;
}

encoder::~encoder()
{
	delete [] output;
}

int encoder::encode(int bit)
{
	shreg = (shreg << 1) | !!bit;

	return output[shreg & shregmask];
}

