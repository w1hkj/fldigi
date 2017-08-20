// ----------------------------------------------------------------------------
// fftmon.h  --  frequency fftmon modem
//
// Copyright (C) 2017
//		Dave Freese, W1HKJ
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

#ifndef _fftmon_H
#define _fftmon_H

#include <stdio.h>

#include <string>
#include <ctime>
#include <complex>

#include "filters.h"
#include "fftfilt.h"
#include "modem.h"
#include "mbuffer.h"
#include "gfft.h"

extern void writeFile();

class fftmon : public modem {
friend void toggle_scans(void *);

public:
#define fftmonFFT_LEN		8192 // approximately 1 sec of bpf'd audio stream
#define LENdiv2				4096

private:

	double *fftbuff;
	double *dftbuff;
	double *buffer;
	Cmovavg *fftfilt[LENdiv2];

	double bshape[fftmonFFT_LEN];

	int  fftmon_sr;

	g_fft<double>	*scanfft;

	bool  scans_stable;
	int   numscans;

	inline double blackman(double x) {
		return (0.42 - 0.50 * cos(2 * M_PI * x) + 0.08 * cos(4 * M_PI * x));
	}
	void update_fftscope();

public:
	fftmon();
	~fftmon();
	void init();
	void rx_init() {}
	void tx_init() {}
	void restart();

	int  rx_process(const double *buf, int len);

	int tx_process() { return -1; }

};

#endif
