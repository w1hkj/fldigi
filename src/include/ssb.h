// ----------------------------------------------------------------------------
// ssb.h  --  ssb modem
//
// Copyright (C) 2010
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

#ifndef _SSB_H
#define _SSB_H

#include "modem.h"

#define	ssb_SampleRate	8000

class ssb : public modem
{
public:
	ssb();
	~ssb();
	void init();
	void rx_init();
	void tx_init(SoundBase *sc);
	void restart();
	int rx_process(const double *buf, int len);
	int tx_process();
};

#endif
