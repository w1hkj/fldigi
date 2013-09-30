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

#include <string>
#include <ctime>

#include "complex.h"
#include "filters.h"
#include "fftfilt.h"
#include "modem.h"
#include "mbuffer.h"

#define	anal_SampleRate	8000
#define	analMaxSymLen	512

#define dispwidth 100

class anal : public modem {
private:

	double		phaseacc;
	double		anal_squelch;

	C_FIR_filter	*hilbert;
	fftfilt *bpfilt;
	Cmovavg *ffilt;
	Cmovavg *favg;

	mbuffer<double, analMaxSymLen, 2> pipe;
	int pipeptr;

	double prevsymbol;
	cmplx prevsmpl;
	int	symbollen;

	int restart_count;

	double		fout_1;
	double		fout_2;
	long		wf_freq;

	struct timespec start_time;

	double sum;

	void clear_syncscope();
	inline cmplx mixer(cmplx in);
	int rx(bool bit);

	double nco(double freq);
	void writeFile();

	std::string	analysisFilename;

public:
	anal();
	~anal();
	void init();
	void rx_init();
	void tx_init(SoundBase *sc);
	void restart();
	int rx_process(const double *buf, int len);
	int tx_process();

};

#endif
