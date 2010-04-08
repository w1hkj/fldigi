// ----------------------------------------------------------------------------
// ssb.cxx  --  ssb modem
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

#include <config.h>

#include "ssb.h"
#include "modem.h"
#include "digiscope.h"
#include "fl_digi.h"

#include "debug.h"

#define ssb_bw         4

void ssb::tx_init(SoundBase *sc)
{
}

void ssb::rx_init()
{
	put_MODEstatus(mode);
}

void ssb::init()
{
	modem::init();
	rx_init();
	set_scope_mode(Digiscope::BLANK);
}

ssb::~ssb()
{
}

void ssb::restart()
{
	set_bandwidth(ssb_bw);
}

ssb::ssb()
{
	mode = MODE_SSB;
	samplerate = 8000;
	cap &= ~CAP_TX;
	restart();
}

// dummy process
int ssb::rx_process(const double *buf, int len)
{
	return 0;
}

//=====================================================================
// ssb transmit
// dummy process
//=====================================================================

int ssb::tx_process()
{
	return -1;
}
