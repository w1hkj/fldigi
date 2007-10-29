// ----------------------------------------------------------------------------
// psk.h  --  psk modem
//
// Copyright (C) 2006
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

#ifndef _PSK_H
#define _PSK_H

#include "complex.h"
#include "trx.h"
#include "viterbi.h"
#include "filters.h"
#include "pskcoeff.h"
#include "pskvaricode.h"

//=====================================================================
#define	PskSampleRate	(8000)
#define PipeLen			(64)

#define SNTHRESHOLD 2.0
#define AFCDECAY 8
//=====================================================================

class psk : public modem {
private:
// tx & rx
	int				symbollen;
	bool			_qpsk;
	double			phaseacc;
	complex			prevsymbol;
	unsigned int	shreg;

// rx variables & functions

	C_FIR_filter	*fir1;
	C_FIR_filter	*fir2;
	double		*fir1c;
	double		*fir2c;

	encoder 		*enc;
	viterbi 		*dec;
	double			phase;
	double			freqerr;
	int				bits;
	double 			bitclk;
	double 			syncbuf[16];
	double 			scope_pipe[2*PipeLen];//[PipeLen];
	unsigned int 	pipeptr;
	unsigned int	dcdshreg;
	int 			dcd;
	int				dcdbits;
	complex			quality;
	void			rx_symbol(complex symbol);
	void 			rx_bit(int bit);
	void			rx_qpsk(int bits);
	double 			scopedata[16];
// IMD & s/n variables
	double			k0, k1, k2;
	double			I11, I12, I21, I22, I31, I32;
	double			snratio, s2n, imdratio, imd;
	double			E1, E2, E3;
//	complex thirdorder;
// tx variables & functions
	double			*tx_shape;
	int 			preamble;
	void 			tx_symbol(int sym);
	void			tx_bit(int bit);
	void			tx_char(unsigned char c);
	void			tx_flush();
	void			update_syncscope();
	void			signalquality();
	void			findsignal();
	void			phaseafc();
	void			afc();
	
public:
	psk(trx_mode mode);
	~psk();
	void init();
	void rx_init();
	void tx_init(cSound *sc);
	void restart();
	int rx_process(const double *buf, int len);
	int tx_process();
	void searchDown();
	void searchUp();
};

#endif
