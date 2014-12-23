// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
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

#ifndef	_FFTFILT_H
#define	_FFTFILT_H

#include "complex.h"
#include "gfft.h"

//----------------------------------------------------------------------

class fftfilt {
enum {NONE, BLACKMAN, HAMMING, HANNING};

protected:
	int flen;
	int flen2;
	g_fft<double> *fft;
	g_fft<double> *ift;
	cmplx *ht;
	cmplx *filter;
	cmplx *timedata;
	cmplx *freqdata;
	cmplx *ovlbuf;
	cmplx *output;
	int inptr;
	int pass;
	int window;

	inline double fsinc(double fc, int i, int len) {
		return (i == len/2) ? 2.0 * fc: 
				sin(2 * M_PI * fc * (i - len/2)) / (M_PI * (i - len/2));
	}
	inline double _blackman(int i, int len) {
		return (0.42 - 
				 0.50 * cos(2.0 * M_PI * i / len) + 
				 0.08 * cos(4.0 * M_PI * i / len));
	}
	void init_filter();
	void clear_filter();

public:
	fftfilt(double f1, double f2, int len);
	fftfilt(double f, int len);
	~fftfilt();
// f1 < f2 ==> bandpass
// f1 > f2 ==> band reject
	void create_filter(double f1, double f2);
	void create_lpf(double f) {
		create_filter(0, f);
	}
	void create_hpf(double f) {
		create_filter(f, 0);
	}
	void rtty_filter(double);

	int run(const cmplx& in, cmplx **out);
	int flush_size();
};

#endif
