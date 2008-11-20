// ----------------------------------------------------------------------------
// viewpsk.h  --  psk modem
//
// Copyright (C) 2008
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
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

#ifndef _VIEWPSK_H
#define _VIEWPSK_H

#include "complex.h"
#include "trx.h"
#include "filters.h"
#include "fldigi-config.h"

//=====================================================================
#define	VPSKSAMPLERATE	(8000)
#define VSNTHRESHOLD 2.0 // 3 db s/n
#define VAFCDECAY 8
#define MAXCHANNELS 30
#define VSEARCHWIDTH 50
#define VSIGSEARCH 5
#define VWAITCOUNT 4
//=====================================================================

class viewpsk {
private:
	trx_mode viewmode;

	int				symbollen;
	double			phaseacc[MAXCHANNELS];
	complex			prevsymbol[MAXCHANNELS];
	complex			quality[MAXCHANNELS];
	unsigned int	shreg[MAXCHANNELS];
	double			metric[MAXCHANNELS];
	
	double			frequency[MAXCHANNELS];
	int				nomfreq[MAXCHANNELS];
	double			freqerr[MAXCHANNELS];
	double			phase[MAXCHANNELS];
	double			bandwidth;

	C_FIR_filter	*fir1[MAXCHANNELS];
	C_FIR_filter	*fir2[MAXCHANNELS];
	
	double			sigpwr[4000];
	double			sigavg;
	double			sigmin;
	Cmovavg			*power[MAXCHANNELS];

	int				bits[MAXCHANNELS];
	double 			bitclk[MAXCHANNELS];
	double 			syncbuf[MAXCHANNELS * 16];
	unsigned int	dcdshreg[MAXCHANNELS];
	int 			dcd[MAXCHANNELS];
	int				dcdbits;
	int				sigsearch[MAXCHANNELS];
	int				waitcount[MAXCHANNELS];
	time_t			now;
	time_t			timeout[MAXCHANNELS];

	void			rx_symbol(int ch, complex symbol);
	void 			rx_bit(int ch, int bit);

	void			findsignal(int);
	void			afc(int);
	
public:
	viewpsk(trx_mode mode);
	~viewpsk();
	void init();
	void restart(trx_mode mode);
	void rx_init(){};
	void tx_init(SoundBase *sc){};
	void restart() {};
	int rx_process(const double *buf, int len);
	int get_freq(int n) { return (int)frequency[n];}
	void set_freq(int n, double f) { frequency[n] = f; nomfreq[n] = (int)f; }
};

extern viewpsk *pskviewer;

#endif
