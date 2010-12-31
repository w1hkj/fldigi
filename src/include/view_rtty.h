// ----------------------------------------------------------------------------
// rtty.h  --  RTTY modem
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code
// distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
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

#ifndef VIEW_RTTY_H
#define VIEW_RTTY_H

#include "rtty.h"
#include "complex.h"
#include "modem.h"
#include "globals.h"
#include "filters.h"
#include "fftfilt.h"
#include "digiscope.h"

#define	VIEW_RTTY_SampleRate	8000

#define	VIEW_RTTYMaxSymLen	(VIEW_RTTY_SampleRate / 23)

#define MAX_CHANNELS 30

enum CHANNEL_STATE {IDLE, SRCHG, RCVNG, WAITING};

struct RTTY_CHANNEL {

	int				state;

	double			phaseacc;

	C_FIR_filter	*lpfilt;
	Cmovavg *bitfilt;
	fftfilt *bpfilt;

	double bbfilter[MAXPIPE];
	unsigned int filterptr;

	double			metric;

	int				rxmode;
	RTTY_RX_STATE	rxstate;

	double			frequency;
	double			freqerr;
	double			phase;
	double			posfreq;
	double			negfreq;
	double			freqerrhi;
	double			freqerrlo;
	double			poserr;
	double			negerr;
	int				poscnt;
	int				negcnt;
	int				timeout;

	double			sigpwr;
	double			noisepwr;
	double			avgsig;

	double			prevsymbol;
	complex			prevsmpl;
	int				counter;
	int				bitcntr;
	int				rxdata;

	int				sigsearch;
};


class view_rtty : public modem {
public:
	static const double SHIFT[];
	static const double BAUD[];
	static const int    BITS[];

private:

	double shift;
	int symbollen;
	int nbits;
	int stoplen;
	int msb;
	bool useFSK;

	RTTY_CHANNEL		channel[MAX_CHANNELS];
	C_FIR_filter	*hilbert;

	double		rtty_squelch;
	double		rtty_shift;
	double      rtty_BW;
	double		rtty_baud;
	int 		rtty_bits;
	RTTY_PARITY	rtty_parity;
	int			rtty_stop;
	bool 		rtty_reverse;
	bool		rtty_msbfirst;

	int bflen;
	double bp_filt_lo;
	double bp_filt_hi;

	int txmode;
	int preamble;

	void clear_syncscope();
	void update_syncscope();
	inline complex mixer(int ch, complex in);

	unsigned char bitreverse(unsigned char in, int n);
	int decode_char(int ch);
	int rttyparity(unsigned int);
	bool rx(int ch, bool bit);

	int rttyxprocess();
	char baudot_dec(int ch, unsigned char data);
	void Metric(int ch);
public:
	view_rtty(trx_mode mode);
	~view_rtty();
	void init();
	void rx_init();
	void tx_init(SoundBase *sc){}
	void restart();
	int rx_process(const double *buf, int len);
	int tx_process();

	void find_signals();
	void clearch(int ch);
	void clear();
	int get_freq(int n) { return (int)channel[n].frequency;}

};

extern view_rtty *rttyviewer;

#endif
