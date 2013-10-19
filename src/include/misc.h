// ----------------------------------------------------------------------------
// misc.h  --  Miscellaneous helper functions
//
// Copyright (C) 2006-2008
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

#ifndef _MISC_H
#define _MISC_H

#include <cmath>

extern unsigned long hweight32(unsigned long w);
extern unsigned short int hweight16(unsigned short int w);
extern unsigned char hweight8(unsigned char w);
extern int parity(unsigned long w);
extern unsigned long rbits32(unsigned long w);
extern unsigned short int rbits16(unsigned short int w);
extern unsigned char rbits8(unsigned char w);

extern unsigned int log2u(unsigned int x);

extern unsigned char graydecode(unsigned char data);
extern unsigned char grayencode(unsigned char data);
extern void MilliSleep(long msecs);

inline double sinc(double x)
{
	return (fabs(x) < 1e-10) ? 1.0 : (sin(M_PI * x) / (M_PI * x));
}

inline double cosc(double x)
{
	return (fabs(x) < 1e-10) ? 0.0 : ((1.0 - cos(M_PI * x)) / (M_PI * x));
}

inline double clamp(double x, double min, double max)
{
	return (x < min) ? min : ((x > max) ? max : x);
}

/// This is always called with an int weight
inline double decayavg(double average, double input, int weight)
{
	if (weight <= 1) return input;
	return ( ( input - average ) / (double)weight ) + average ;
}

// following are defined inline to provide best performance
inline double blackman(double x)
{
	return (0.42 - 0.50 * cos(2 * M_PI * x) + 0.08 * cos(4 * M_PI * x));
}

inline double hamming(double x)
{
	return 0.54 - 0.46 * cos(2 * M_PI * x);
}

inline double hanning(double x)
{
	return 0.5 - 0.5 * cos(2 * M_PI * x);
}

inline double rcos( double t, double T, double alpha=1.0 )
{
    if( t == 0 ) return 1.0;
    double taT = T / (2.0 * alpha);
    if( fabs(t) == taT ) return ((alpha/2.0) * sin(M_PI/(2.0*alpha)));
    return (sin(M_PI*t/T)/(M_PI*t/T))*cos(alpha*M_PI*t/T)/(1.0-(t/taT)*(t/taT));
}

// Rectangular - no pre filtering of data array
void RectWindow(double *array, int n);
// Hamming - used by gmfsk
void HammingWindow(double *array, int n);
// Hanning - used by winpsk
void HanningWindow(double *array, int n);
// Best lob suppression - least in band ripple
void BlackmanWindow(double *array, int n);
// Simple about effective as Hamming or Hanning
void TriangularWindow(double *array, int n);

#endif
