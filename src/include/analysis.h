// ----------------------------------------------------------------------------
// anal.h  --  anal modem
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  
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

#ifndef _anal_H
#define _anal_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/time.h>
#include <string>

#include "complex.h"
#include "modem.h"
#include "trx.h"
#include "misc.h"
#include "filters.h"
#include "fftfilt.h"
#include "digiscope.h"
#include "main.h"

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
	
	double pipe[analMaxSymLen];
	int pipeptr;

	double prevsymbol;
	complex prevsmpl;
	int	symbollen;
	
	double		fout_1;
	double		fout_2;

	double sum;
	
	void clear_syncscope();
	inline complex mixer(complex in);
	int rx(bool bit);

	double nco(double freq);
	void writeFile();
	
	ofstream	analysisFile;
	string		analysisFilename;
	
public:
	anal();
	~anal();
	void init();
	void rx_init();
	void tx_init(cSound *sc);
	void restart();
	int rx_process(double *buf, int len);
	int tx_process();

};

#endif
