// ----------------------------------------------------------------------------
// BLANK.h  --  BASIS FOR ALL MODEMS
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

#ifndef _BLANK_H
#define _BLANK_H

#include "trx.h"
#include "modem.h"
#include "fft.h"
#include "filters.h"
#include "complex.h"

#define	BLANKSampleRate	8000
#define	SYMLEN			512
#define BLANK_BW		100

#define BUFFLEN			4096
#define SCOPE_DATA_LEN	1024

// lp filter
//#define	DEC_1		8		
//#define FIRLEN_1	256
//#define BW_1		10

// NASA coefficients for viterbi encode/decode algorithms
//#define	K	7
//#define	POLY1	0x6d
//#define	POLY2	0x4f


class BLANK : public modem {
protected:
	double			phaseacc;
	double			phaseincr;

	C_FIR_filter	*bandpass;
	C_FIR_filter	*lowpass;
	C_FIR_filter	*hilbert;
	Cmovavg			*moving_avg;
	sfft			*slidingfft;

	int				symlen;
// receive
	double			*scope_data;
	double			*inbuf;
// transmit
	int txstate;

	double			*outbuf;
	unsigned int	buffptr;

public:
	BLANK();
	~BLANK();
	void	init();
	void	rx_init();
	void	tx_init(SoundBase *sc);
	void 	restart();
	int		rx_process(const double *buf, int len);
	int		tx_process();
	void	update_syncscope();

};

#endif
