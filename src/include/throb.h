// ----------------------------------------------------------------------------
// throb.h  --  BASIS FOR ALL MODEMS
//
// Copyright (C) 2006-2007
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

#ifndef _THROB_H
#define _THROB_H

#include "modem.h"
#include "globals.h"
#include "fftfilt.h"
#include "filters.h"
#include "complex.h"

#include "mbuffer.h"

#define	THROB_SAMPLE_RATE	8000
#define	SYMLEN			512

#define BUFFLEN			4096
#define SCOPE_DATA_LEN	1024

#define	DOWN_SAMPLE	32

#define	SYMLEN_1	8192
#define	SYMLEN_2	4096
#define	SYMLEN_4	2048

#define	MAX_RX_SYMLEN	(SYMLEN_1 / DOWN_SAMPLE)

#define	FilterFFTLen	8192

class throb : public modem {
	
static double ThrobToneFreqsNar[];
static double ThrobToneFreqsWid[];
static double ThrobXToneFreqsNar[];
static double ThrobXToneFreqsWid[];
static unsigned char ThrobCharSet[];
static unsigned char ThrobXCharSet[];
static int  ThrobTonePairs[][2];
static int  ThrobXTonePairs[][2];

protected:
	
	int			num_tones;
	int			num_chars;
	int			idlesym;
	int			spacesym;
	char			lastchar;

	double			phaseacc;
	double			phaseincr;

	fftfilt			*fftfilter;
	C_FIR_filter	*syncfilt;
	C_FIR_filter	*hilbert;
	Cmovavg			*snfilter;

	int				symlen;
	double 			freqs[55];

// receive
	double			*scope_data;
	cmplx 		*rxtone[55];
	cmplx 		symbol[MAX_RX_SYMLEN];

	double			syncbuf[MAX_RX_SYMLEN];
	mbuffer<double, MAX_RX_SYMLEN, 2> dispbuf;

	double			rxcntr;
	double			signal;
	double			noise;
	
	double			s2n;

	int rxsymlen;
	int symptr;
	int deccntr;
	int shift;
	int waitsync;
	
	cmplx			mixer(cmplx in);
	void			sync(cmplx in);
	void			rx(cmplx in);
	void			decodechar(int tone1, int tone2);
	int				findtones(cmplx *word, int &tone1, int &tone2);
	cmplx			*mk_rxtone(double freq, double *pulse, int len);
	void			show_char(int);
	void			flip_syms();
	void			reset_syms();


// transmit
	int txstate;

	int				preamble;
	double			*txpulse;

	double			*outbuf;
	unsigned int	buffptr;
	
	double			*mk_semi_pulse(int len);
	double			*mk_full_pulse(int len);
	void			send(int);

public:
	throb(trx_mode);
	~throb();
	void	init();
	void	rx_init();
	void	tx_init(SoundBase *sc);
	void 	restart() {};
	int		rx_process(const double *buf, int len);
	int		tx_process();
	void	update_syncscope();

};

#endif
