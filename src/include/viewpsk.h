// ----------------------------------------------------------------------------
// viewpsk.h  --  psk modem
//
// Copyright (C) 2008
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

#ifndef _VIEWPSK_H
#define _VIEWPSK_H

#include "complex.h"
#include "modem.h"
#include "globals.h"
#include "filters.h"
#include "pskeval.h"
#include "viterbi.h"
#include "mfskvaricode.h"
#include "interleave.h"

//=====================================================================
#define	VPSKSAMPLERATE	(8000)
#define VAFCDECAY 8
#define MAXCHANNELS 30
#define VSEARCHWIDTH 70
#define VSIGSEARCH 5
#define VWAITCOUNT 4
#define NULLFREQ 1e6
//=====================================================================

struct CHANNEL {
	double			phaseacc;
	cmplx			prevsymbol;
	cmplx			quality;
	unsigned int	shreg;
	unsigned int	shreg2;
	double			metric;

	double			frequency;
	double			freqerr;
	double			phase;
	double			syncbuf[16];
	double			averageamp;

	C_FIR_filter	*fir1;
	C_FIR_filter	*fir2;
	viterbi 		*dec;
	viterbi			*dec2;

	unsigned int 	bitshreg;
	int 			rxbitstate;

	unsigned char	symbolpair[2];
	int				fecmet;
	int				fecmet2;

	interleave		*Rxinlv;
	interleave		*Rxinlv2;

	int				bits;
	double 			bitclk;
	unsigned int	dcdshreg;
	unsigned int	dcdshreg2;
	int 			dcd;
	int				waitcount;
	int				timeout;
	bool			reset;
	int				acquire;

};

class viewpsk {

private:
	trx_mode	viewmode;

	int			symbollen;
	int			symbits;
	double		bandwidth;
	int			dcdbits;

	int			fa;
	int			fb;
	int			ftest;
	double		test_peak_amp;
	time_t		now;
	bool		reset_all;
	bool		tracked;
	bool		browser_changed;

	CHANNEL		channel[MAXCHANNELS];
	int			nchannels;
	int			lowfreq;

	bool		_pskr;
	bool		_qpsk;

	pskeval*	evalpsk;

	void		rx_symbol(int ch, cmplx symbol);
	void 		rx_bit(int ch, int bit);
	void		findsignal(int);
	void		afc(int);

	void 		rx_bit2(int ch, int bit);
	void		rx_pskr(int ch, unsigned char symbol);
	void		rx_qpsk(int ch, int bits);

	bool		is_valid_char(int &c);

	inline void		timeout_check();
	inline void		insert();

public:
	viewpsk(pskeval* eval, trx_mode mode);
	~viewpsk();
	void init();
	void restart(trx_mode mode);
	void rx_init() {};
	void tx_init() {};
	void restart() {};
	int rx_process(const double *buf, int len);
	int get_freq(int n);
	void set_freq(int n, double f) { channel[n].frequency = f; }
	void findsignals();
	void clearch(int n);
	void clear();

};

extern viewpsk *pskviewer;

#endif
