// ----------------------------------------------------------------------------
// olivia.h
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
//	Copyright (C) 2005
//	Tomi Manninen (oh2bns@sral.fi)
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

#ifndef _OLIVIA_H
#define _OLIVIA_H

#include "modem.h"
#include "jalocha/pj_mfsk.h"

#define TONE_DURATION 8192
#define SR4 ((TONE_DURATION) / 4)

class olivia : public modem {
private:

	MFSK_Transmitter < double >*Tx;
	MFSK_Receiver < double >*Rx;

	double		*txfbuffer;
	int 		txbufferlen;

	double		phaseacc;
	complex		prevsymbol;
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

	int			preamblesent;
	int			postamblesent;
	double		preamblephase;

	double		txbasefreq;
	double		last_txbasefreq;
	double		lastfreq;

	double		ampshape[TONE_DURATION / 4];
	double		tonebuff[TONE_DURATION];

	double		nco(double freq);
	void		send_tones();
	void		create_tones();
	
public:
	olivia();
	~olivia();
	void init();
	void rx_init();
	void tx_init(SoundBase *sc);
	void restart();
	int rx_process(const double *buf, int len);
	int tx_process();
	int unescape(int c);
};


#endif
