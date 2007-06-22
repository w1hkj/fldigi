// ----------------------------------------------------------------------------
// misc.h  --  Miscellaneous helper functions
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  These filters were adapted from code contained
// in the gmfsk source code distribution.
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

#ifndef _MISC_H
#define _MISC_H

#include <math.h>
#include <sys/time.h>

extern unsigned long hweight32(unsigned long w);
extern unsigned short int hweight16(unsigned short int w);
extern unsigned char hweight8(unsigned char w);
extern int parity(unsigned long w);
extern unsigned long rbits32(unsigned long w);
extern unsigned short int rbits16(unsigned short int w);
extern unsigned char rbits8(unsigned char w);
extern unsigned int log2(unsigned int x);
extern unsigned char graydecode(unsigned char data);
extern unsigned char grayencode(unsigned char data);
extern void MilliSleep(long msecs);

// following are defined inline to provide best performance

inline double rect(double x)
{
	return 1.0;
}

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

inline double decayavg(double average, double input, double weight)
{
	return input * (1.0 / weight) + average * (1.0 - (1.0 / weight));
}

#endif
