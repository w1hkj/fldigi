// ----------------------------------------------------------------------------
// mt63.h
//
// Copyright (C) 2006-2009
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code
// distribution.
//	Copyright (C) 2005
//	Tomi Manninen (oh2bns@sral.fi)
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
