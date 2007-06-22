// ----------------------------------------------------------------------------
// sfft.cxx  --  Sliding FFT
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

#include <stdlib.h>
#include <iostream>
#include <math.h>

#include "sfft.h"
#include "misc.h"

#define	K1	0.9999
// ----------------------------------------------------------------------------


sfft::sfft(int len, int _first, int _last)
{
	vrot = new complex[len];
	delay  = new complex[len];
	bins     = new complex[len];
	fftlen = len;
	first = _first;
	last = _last;
	ptr = 0;
	double phi = 0.0, tau = 2.0 * M_PI/ len;
	for (int i = 0; i < len; i++) {
		vrot[i].re = K1 * cos (phi);
		vrot[i].im = K1 * sin (phi);
		phi += tau;
		delay[i] = bins[i] = 0.0;
	}
	k2 = pow(K1, len);
}

sfft::~sfft()
{
	delete [] vrot;
	delete [] delay;
	delete [] bins;
}

// Sliding FFT, complex input, complex output
// returns address of first component in array

complex *sfft::run(complex input)
{
	complex z;// = input - delay[ptr] * k2;
	complex y;
	z.re = - k2 * delay[ptr].re + input.re;
	z.im = - k2 * delay[ptr].im + input.im;
	delay[ptr++] = input;
	ptr %= fftlen;
	
	for (int i = first; i < last; i++) {
		y = bins[i] + z;
		bins[i] = y * vrot[i];
	}
	return &bins[first];
}

// ----------------------------------------------------------------------------
