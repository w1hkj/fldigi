//
// mt63.h
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code
// distribution.
//	Copyright (C) 2005
//	Tomi Manninen (oh2bns@sral.fi)
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
//
//

#ifndef MT63_MODEM_H
#define MT63_MODEM_H

#include "dsp.h"
#include "mt63base.h"
#include "modem.h"

class mt63 : public modem {
private:
	int     Interleave;
	int     flush;
	int     escape;
    bool    long_integral;

	MT63tx  *Tx;
	MT63rx  *Rx;

	dspLevelMonitor *InpLevel;
	double_buff     *InpBuff;
	double_buff     *emptyBuff;
	bool            flushbuffer;
	double          FEC_offset;
	double          FEC_snr;

public:
	mt63(trx_mode mode);
	~mt63();
	void    init();
	void    rx_init();
	void    tx_init(SoundBase*);
	void    restart();
	int     rx_process(const double *buf, int len);
	int     tx_process();

	void    rx_flush();
	void	set_freq(double);
};


#endif
