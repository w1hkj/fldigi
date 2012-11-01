// ----------------------------------------------------------------------------
// blank.cxx  --  BLANK modem
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

#include "BLANK.h"
#include "ascii.h"

char BLANKmsg[80];

void  BLANK::tx_init(SoundBase *sc)
{
	scard = sc;
}

void  BLANK::rx_init()
{
	put_MODEstatus(mode);
}

void BLANK::init()
{
	modem::init();
	rx_init();
	digiscope->mode(Digiscope::SCOPE);
}

BLANK::~BLANK()
{
	delete bandpass;
	delete hilbert;
	delete lowpass;
	delete sliding;
	delete [] scope_data;
	delete [] out_buf;
	delete [] in_buf;
}

BLANK::BLANK(trx_mode BLANK_mode) : modem()
{
	double cf, flo, fhi;

	mode = BLANK_mode;
	symlen = SYMLEN;
	bandwidth = BLANK_BW;
	samplerate = BLANKSampleRate;
	
	flo = LP_F1 / corrRxSampleRate();	
	lowpass		= new C_FIR_filter();
	lowpass->init_lowpass (LP_FIRLEN, LP_DEC, flo );
	
	flo = BP_F1 / corrRxSampleRate();	
	fhi = BP_F2 / corrRxSampleRate();	
	bandpass	= new C_FIR_filter();
	bandpass->init_bandpass (BP_FIRLEN, BP_DEC, flo, fhi );

	hilbert		= new C_FIR_filter();
	hilbert->init_hilbert(37, 1);

	sliding		= new sfft (SL_LEN, SL_F1, SL_F2); // all integer values

	scope_data	= new double [SCOPE_DATA_LEN];
	out_buf		= new double [SYMLEN];
	in_buf		= new double [BUFLEN];

//	init();
}

//=====================================================================
// receive processing
//=====================================================================

void BLANK::recvchar(int c)
{
	if (c == -1)
		return;
	put_rx_char(c);
}

void BLANK::decodesymbol(unsigned char symbol)
{
}

complex BLANK::mixer(complex in, double f)
{
	complex z;

// f may have to be modified
	z = in * complex( cos(phaseacc), sin(phaseacc) );

	phaseacc -= TWOPI * f / corrRxSampleRate();
	if (phaseacc > TWOPI) phaseacc -= TWOPI;
	if (phaseacc < -TWOPI) phaseacc += TWOPI;
	
	return z;
}

void BLANK::update_syncscope()
{
	int j;
	memset(scopedata, 0, 2 * SCOPE_DATA_LEN);
	if (!squelchon || metric >= squelch)
		for (int i = 0; i < 2 * symlen; i++) {
//			j = (i + pipeptr) % (2 * symlen);
//			scopedata[i] = (pipe[j].vector[prev1symbol]).mag();
		}
	set_scope(scope_data, SCOPE_DATA_LEN);
}

void BLANK::afc()
{
	complex z;
	double x;

	if (metric < squelch)
		return;
// adjust "frequency" iaw with afc processing
}

int BLANK::rx_process(const double *buf, int len)
{
	complex z;
	int i;

	while (len-- > 0) {
// create analytic signal...
		z.re = z.im = *buf++;
		hbfilt->run ( z, z );
// shift in frequency to the base freq of 1000 hz
		z = mixer(z, frequency);
// bandpass filter around the shifted center frequency
// with required bandwidth 
		bandpass->run ( z, z );
		
// binsfft->run(z) copies frequencies of interest
		complex dummy ;
		sliding->run (z, &dummy, 0 );
// etc
		decodesymbol();
		update_syncscope();
		afc();
	}

	return 0;
}


//=====================================================================
// transmit processing
//=====================================================================


void BLANK::sendchar(unsigned char c)
{
// need to generate the outbuf
	ModulateXmtr(outbuf, symlen);
	put_echo_char(c);
}

void sendidle() {
}

int BLANK::tx_process()
{
int xmtbyte;
	switch (txstate) {
	case TX_STATE_PREAMBLE:
		for (int i = 0; i < 32; i++)
			sendbit(0);
		txstate = TX_STATE_START;
		break;

	case TX_STATE_START:
		sendchar('\r');
		sendchar(2);		// STX
		sendchar('\r');
		txstate = TX_STATE_DATA;
		break;

	case TX_STATE_DATA:
		xmtbyte = get_tx_char();
		if (xmtbyte == GET_TX_CHAR_NODATA)
			sendidle();
		else if ( xmtbyte == GET_TX_CHAR_ETX || stopflag)
			txstate = TX_STATE_FLUSH;
		else
			sendchar(xmtbyte);
		break;

	case TX_STATE_FLUSH:
		sendchar('\r');
		sendchar(4);		// EOT
		sendchar('\r');
		flushtx();
		stopflag = false;
// tell trx process that xmt is done
		return -1;

	default:
		break;
	}
	return 0;
}


