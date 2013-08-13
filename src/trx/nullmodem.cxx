// ----------------------------------------------------------------------------
// NULLMODEM.cxx  --  NULLMODEM modem
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gMFSK source code
// distribution.
//  gMFSK Copyright (C) 2001, 2002, 2003
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

#include <config.h>

#include <stdlib.h>
#include <iostream>

#include "nullmodem.h"
#include "fl_digi.h"
#include "ascii.h"

#define null_bw 1

// a NULLMODEM and will be instantiated before the dynamic member wf, 
// digiscope, and fl_digi_main, the main dialog

NULLMODEM:: NULLMODEM() : modem() 
{
	mode = MODE_NULL;
	samplerate = 8000;
	restart();
}

NULLMODEM::~NULLMODEM() {};

void  NULLMODEM::tx_init(SoundBase *sc)
{
	scard = sc;
}

void  NULLMODEM::rx_init()
{
	if (fl_digi_main)
		put_MODEstatus(mode);
}

void NULLMODEM::init()
{
	modem::init();
	rx_init();
	if (digiscope)
		digiscope->mode(Digiscope::SCOPE);
}

void NULLMODEM::restart()
{
	if (wf) set_bandwidth(null_bw);
}


//=====================================================================
// receive processing
//=====================================================================

int NULLMODEM::rx_process(const double *buf, int len)
{
	return 0;
}

//=====================================================================
// transmit processing
//=====================================================================


int NULLMODEM::tx_process()
{
	MilliSleep(10);
	if (!fl_digi_main) return 0;

	if ( get_tx_char() == GET_TX_CHAR_ETX || stopflag) {
		stopflag = false;
		return -1;
	}
	return 0;
}

