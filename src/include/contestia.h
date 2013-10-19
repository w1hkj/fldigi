// ----------------------------------------------------------------------------
// contestia.h
//
// Copyright (C) 2006-2010
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

#ifndef _CONTESTIA_H
#define _CONTESTIA_H

#include "modem.h"
#include "jalocha/pj_mfsk.h"
#include "sound.h"

#define TONE_DURATION (SCBLOCKSIZE * 16)
#define SR4 ((TONE_DURATION) / 4)

class contestia : public modem {
private:

	MFSK_Transmitter < double >*Tx;
	MFSK_Receiver < double >*Rx;

	double		*txfbuffer;
	int 		txbufferlen;

	double		phaseacc;
	cmplx		prevsymbol;
	int			preamble;
	unsigned int	shreg;

	double		np;
	double		sp;
	double		sigpwr;
	double		noisepwr;
	
	int			escape;
	int			smargin;
	int			sinteg;
	int			tones;
	int			bw;
	double		tone_bw;

	int			preamblesent;
	int			postamblesent;
	double		preamblephase;

	double		txbasefreq;
	double		tone_midfreq;
	double		lastfreq;

	double		ampshape[SR4];
	double		tonebuff[TONE_DURATION];

	double		nco(double freq);
	void		send_tones();
	
public:
	contestia();
	~contestia();
	void init();
	void rx_init();
	void rx_flush();
	void tx_init(SoundBase *sc);
	void restart();
	int rx_process(const double *buf, int len);
	int tx_process();
	int unescape(int c);
};


#endif
