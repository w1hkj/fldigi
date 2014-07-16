// ----------------------------------------------------------------------------
// anal.h  --  frequency analysis modem
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

#ifndef _anal_H
#define _anal_H

#include <stdio.h>

#include <string>
#include <ctime>
#include <complex>

#include "filters.h"
#include "fftfilt.h"
#include "modem.h"
#include "mbuffer.h"

class anal : public modem {
private:

	std::string	analysisFilename;
	bool write_to_csv;

	std::complex<double> *fftbuff;
	std::complex<double> *dftbuff;

	int restart_count;

	double max;
	double test;
	double maxnom;
	double maxlower;
	double maxupper;
	double noise;
	double sig_level;
	double snr;
	double	freq_corr;
	double	wf_freq;
	double	anal_squelch;
	long  ticks;
	int   slen;
	double tracking_freq;

	struct timespec start_time;

	C_FIR_filter	*hilbert;
	fftfilt *bpfilt;

	int rx(bool bit);
	void writeFile();
	std::complex<double> dft (std::complex<double> *buff, double fm, double Ts, double offset);
	inline double blackman(double x) {
		return (0.42 - 0.50 * cos(2 * M_PI * x) + 0.08 * cos(4 * M_PI * x));
	}

public:
	anal();
	~anal();
	void init();
	void rx_init();
	void tx_init(SoundBase *sc);
	void restart();
	void start_csv();
	void stop_csv();
	int  is_csv() { return write_to_csv;}
	int  rx_process(const double *buf, int len);
	int tx_process();
	double track_freq() { return tracking_freq; }

};

#endif
