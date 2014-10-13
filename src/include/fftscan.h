// ----------------------------------------------------------------------------
// fftscan.h  --  frequency fftscan modem
//
// Copyright (C) 2006
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

#ifndef _fftscan_H
#define _fftscan_H

#include <stdio.h>

#include <string>
#include <ctime>
#include <complex>

#include "filters.h"
#include "fftfilt.h"
#include "modem.h"
#include "mbuffer.h"
#include "gfft.h"

class fftscan : public modem {
public:
#define fftscan_BW			4
#define	fftscan_SampleRate	8000
#define fftscanFFT_LEN		8192 // approximately 1 sec of bpf'd audio stream
#define NYQUIST				4000 // 1/2 fftscanFFT_LEN
private:

	std::string	fftscanFilename;
	bool write_to_csv;
	bool _refresh;

	double *fftbuff;
	std::complex<double> *dftbuff;
	double *buffer;

	g_fft<double>	*scanfft;

	// int restart_count;

	//double sig_level;
	double	wf_freq;
	long  ticks;
	double  scans;

	void writeFile();
	std::complex<double> dft (std::complex<double> *buff, double fm, double Ts, double offset);
	inline double blackman(double x) {
		return (0.42 - 0.50 * cos(2 * M_PI * x) + 0.08 * cos(4 * M_PI * x));
	}

public:
	fftscan();
	~fftscan();
	void init();
	void rx_init();
	void tx_init(SoundBase *sc);
	void restart();
	void start_csv();
	void stop_csv();
	int  is_csv() { return write_to_csv;}
	void update_syncscope();
	int  rx_process(const double *buf, int len);
	void refresh_scope() { _refresh = true; }

	int tx_process();

};

#endif
