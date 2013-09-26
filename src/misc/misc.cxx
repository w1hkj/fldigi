// ----------------------------------------------------------------------------
// misc.cxx  --  Miscellaneous helper functions
//
// Copyright (C) 2006-2007
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  These filters were adapted from code contained
// in the gmfsk source code distribution.
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

#include "misc.h"
#include <time.h>

// ----------------------------------------------------------------------------

/*
 * Hamming weight (number of bits that are ones).
 */
unsigned long hweight32(unsigned long w) 
{
	w = (w & 0x55555555) + ((w >>  1) & 0x55555555);
	w = (w & 0x33333333) + ((w >>  2) & 0x33333333);
	w = (w & 0x0F0F0F0F) + ((w >>  4) & 0x0F0F0F0F);
	w = (w & 0x00FF00FF) + ((w >>  8) & 0x00FF00FF);
	w = (w & 0x0000FFFF) + ((w >> 16) & 0x0000FFFF);
	return w;
}

unsigned short int hweight16(unsigned short int w)
{
	w = (w & 0x5555) + ((w >> 1) & 0x5555);
	w = (w & 0x3333) + ((w >> 2) & 0x3333);
	w = (w & 0x0F0F) + ((w >> 4) & 0x0F0F);
	w = (w & 0x00FF) + ((w >> 8) & 0x00FF);
	return w;
}

unsigned char hweight8(unsigned char w)
{
	w = (w & 0x55) + ((w >> 1) & 0x55);
	w = (w & 0x33) + ((w >> 2) & 0x33);
	w = (w & 0x0F) + ((w >> 4) & 0x0F);
	return w;
}

// ----------------------------------------------------------------------------

/*
 * Parity function. Return one if `w' has odd number of ones, zero otherwise.
 */

int parity(unsigned long w)
{
	return hweight32(w) & 1;
}

// ----------------------------------------------------------------------------

/*
 * Reverse order of bits.
 */
unsigned long rbits32(unsigned long w)
{
	w = ((w >>  1) & 0x55555555) | ((w <<  1) & 0xAAAAAAAA);
	w = ((w >>  2) & 0x33333333) | ((w <<  2) & 0xCCCCCCCC);
	w = ((w >>  4) & 0x0F0F0F0F) | ((w <<  4) & 0xF0F0F0F0);
	w = ((w >>  8) & 0x00FF00FF) | ((w <<  8) & 0xFF00FF00);
	w = ((w >> 16) & 0x0000FFFF) | ((w << 16) & 0xFFFF0000);
	return w;
}

unsigned short int rbits16(unsigned short int w)
{
	w = ((w >> 1) & 0x5555) | ((w << 1) & 0xAAAA);
	w = ((w >> 2) & 0x3333) | ((w << 2) & 0xCCCC);
	w = ((w >> 4) & 0x0F0F) | ((w << 4) & 0xF0F0);
	w = ((w >> 8) & 0x00FF) | ((w << 8) & 0xFF00);
	return w;
}

unsigned char rbits8(unsigned char w)
{
	w = ((w >> 1) & 0x55) | ((w << 1) & 0xFF);
	w = ((w >> 2) & 0x33) | ((w << 2) & 0xCC);
	w = ((w >> 4) & 0x0F) | ((w << 4) & 0xF0);
	return w;
}

// ----------------------------------------------------------------------------

// Integer base-2 logarithm

unsigned int log2u(unsigned int x)
{
	int y = 0;
	x >>= 1;
	while (x) {
		x >>= 1;
		y++;
	}
	return y;
}

// ----------------------------------------------------------------------------

// Gray encoding and decoding (8 bit)

unsigned char grayencode(unsigned char data)
//unsigned char graydecode(unsigned char data)
{
	unsigned char bits = data;

	bits ^= data >> 1;
	bits ^= data >> 2;
	bits ^= data >> 3;
	bits ^= data >> 4;
	bits ^= data >> 5;
	bits ^= data >> 6;
	bits ^= data >> 7;

	return bits;
}

unsigned char graydecode(unsigned char data)
//unsigned char grayencode(unsigned char data)
{
	return data ^ (data >> 1);
}

// ----------------------------------------------------------------------------

// Rectangular - no pre filtering of data array
void RectWindow(double *array, int n) {
	for (int i = 0; i < n; i++)
		array[i] = 1.0;
}

// Hamming - used by gmfsk
void HammingWindow(double *array, int n) {
	double pwr = 0.0;
	double inv_n = 1.0 / (double)n;
	for (int i = 0; i < n; i++) {
		array[i] = hamming((double)i * inv_n);
		pwr += array[i] * array[i];
	}
	pwr = sqrt((double)n/pwr);
	for (int i = 0; i < n; i++)
		array[i] *= pwr;
}

// Hanning - used by winpsk
void HanningWindow(double *array, int n) {
	double pwr = 0.0;
	double inv_n = 1.0 / (double)n;
	for (int i = 0; i < n; i++) {
		array[i] = hanning((double)i * inv_n);
		pwr += array[i] * array[i];
	}
	pwr = sqrt((double)n/pwr);
	for (int i = 0; i < n; i++)
		array[i] *= pwr;
}

// Best lob suppression - least in band ripple
void BlackmanWindow(double *array, int n) {
	double pwr = 0.0;
	double inv_n = 1.0 / (double)n;
	for (int i = 0; i < n; i++) {
		array[i] = blackman((double)i * inv_n);
		pwr += array[i] * array[i];
	}
	pwr = sqrt((double)n/pwr);
	for (int i = 0; i < n; i++)
		array[i] *= pwr;
}

// Simple about effective as Hamming or Hanning
void TriangularWindow(double *array, int n) {
	double pwr = 0.0;
	for (int i = 0; i < n; i++) array[i] = 1.0;
	double inv_n = 1.0 / (double)n;
	for (int i = 0; i < n / 4; i++) {
			array[i] = 4.0 * (double)i * inv_n ;
			array[n-i] = array[i];
	}
	for (int i = 0; i < n; i++)	pwr += array[i] * array[i];
	pwr = sqrt((double)n/pwr);
	for (int i = 0; i < n; i++)
		array[i] *= pwr;
}
	
