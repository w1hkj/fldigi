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
viterbi::viterbi(int k, int poly1, int poly2)
{
	int outsize = 1 << k;
	_traceback = PATHMEM - 1;
	_chunksize = 8;
	nstates = 1 << (k - 1);
	
	output = new int[outsize];
	
	for (int i = 0; i < outsize; i++) {
		output[i] = parity(poly1 & i) | (parity(poly2 & i) << 1);
	}
	
	for (int i = 0; i < PATHMEM; i++) {
		metrics[i] = new int[nstates];
		history[i] = new int[nstates];
		sequence[i] = 0;
		for (int j = 0; j < nstates; j++)
			metrics[i][j] = history[i][j] = 0;
	}
	for (int i = 0; i < 256; i++) {
		mettab[0][i] = 128 - i;
		mettab[1][i] = i - 128;
	}
	reset();
}

viterbi::~viterbi()
{
	if (output) delete [] output;
	for (int i = 0; i < PATHMEM; i++) {
		if (metrics[i]) delete [] metrics[i];
		if (history[i]) delete [] history[i];
	}
}

void viterbi::reset()
{
	for (int i = 0; i < PATHMEM; i++) {
		memset(metrics[i], 0, nstates * sizeof(int));
		memset(history[i], 0, nstates * sizeof(int));
	}
	ptr = 0;
}

int viterbi::settraceback(int trace) {
	if (trace < 0 || trace > PATHMEM - 1)
	return -1;
	_traceback = trace;
	return 0;
}

int viterbi::setchunksize(int chunk) {
	if (chunk < 1 || chunk > _traceback)
	return -1;
	_chunksize = chunk;
	return 0;
}

int viterbi::traceback(int *metric)
{
	int bestmetric, beststate;
	unsigned int p, c = 0;

	p = (ptr - 1) % PATHMEM;

// Find the state with the best metric
	bestmetric = INT_MIN;
	beststate = 0;

	for (int i = 0; i < nstates; i++) {
		if (metrics[p][i] > bestmetric) {
			bestmetric = metrics[p][i];
			beststate = i;
		}
	}

// Trace back 'traceback' steps, starting from the best state
	sequence[p] = beststate;

	for (int i = 0; i < _traceback; i++) {
		unsigned int prev = (p - 1) % PATHMEM;

		sequence[prev] = history[p][sequence[p]];
		p = prev;
	}

	if (metric)
		*metric = metrics[p][sequence[p]];

// Decode 'chunksize' bits
	for (int i = 0; i < _chunksize; i++) {
// low bit of state is the previous input bit
		c = (c << 1) | (sequence[p] & 1);
		p = (p + 1) % PATHMEM;
	}

	if (metric)
		*metric = metrics[p][sequence[p]] - *metric;

	return c;
}

int viterbi::decode(unsigned char *sym, int *metric)
{
	unsigned int currptr, prevptr;
	int met[4];
	
	currptr = ptr;
	prevptr = (currptr - 1) % PATHMEM;
//	if (prevptr < 0) prevptr = PATHMEM - 1;

	met[0] = mettab[0][sym[1]] + mettab[0][sym[0]];
	met[1] = mettab[0][sym[1]] + mettab[1][sym[0]];
	met[2] = mettab[1][sym[1]] + mettab[0][sym[0]];
	met[3] = mettab[1][sym[1]] + mettab[1][sym[0]];

//	met[0] = 256 - sym[1] - sym[0];
//	met[1] = sym[0] - sym[1];
//	met[2] = sym[1] - sym[0];
//	met[3] = sym[0] + sym[1] - 256;

	for (int n = 0; n < nstates; n++) {
		int p0, p1, s0, s1, m0, m1;

		m0 = 0;
		m1 = 0;
		s0 = n;
		s1 = n + nstates;

		p0 = s0 >> 1;
		p1 = s1 >> 1;

		m0 = metrics[prevptr][p0] + met[output[s0]];
		m1 = metrics[prevptr][p1] + met[output[s1]];

		if (m0 > m1) {
			metrics[currptr][n] = m0;
			history[currptr][n] = p0;
		} else {
			metrics[currptr][n] = m1;
			history[currptr][n] = p1;
		}
	}

	ptr = (ptr + 1) % PATHMEM;

	if ((ptr % _chunksize) == 0)
		return traceback(metric);

	if (metrics[currptr][0] > INT_MAX / 2) {
		for (int i = 0; i < PATHMEM; i++)
			for (int j = 0; j < nstates; j++)
				metrics[i][j] -= INT_MAX / 2;
	}
	if (metrics[currptr][0] < INT_MIN / 2) {
		for (int i = 0; i < PATHMEM; i++)
			for (int j = 0; j < nstates; j++)
				metrics[i][j] += INT_MIN / 2;
	}

	return -1;
}

/* ---------------------------------------------------------------------- */
#include <iostream>
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

