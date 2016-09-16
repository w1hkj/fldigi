// ----------------------------------------------------------------------------
// psk.cxx  --  psk modem
//
// Copyright (C) 2006-2015
//		Dave Freese, W1HKJ
// Copyright (C) 2009-2010
//		John Douyere, VK2ETA
// Copyright (C) 2014
//		John Phelps, KL4YFD
//
// PSK-FEC and PSK-R modes contributed by VK2ETA
//
// This file is part of fldigi.  Adapted from code contained in gmfsk
// source code distribution.
// gmfsk Copyright (C) 2001, 2002, 2003
// Tomi Manninen (oh2bns@sral.fi)
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
#include <stdio.h>
#include <iomanip>
#include <iostream>

#include "psk.h"
#include "main.h"
#include "fl_digi.h"
#include "trx.h"
#include "misc.h"
#include "waterfall.h"
#include "configuration.h"
#include "status.h"
#include "viewpsk.h"
#include "pskeval.h"
#include "modem.h"
#include "Viewer.h"
#include "macros.h"

#include "confdialog.h"

extern waterfall *wf;

// Change the following for DCD low pass filter adjustment
#define SQLCOEFF 0.01
#define SQLDECAY 50

//=====================================================================

#define K		5
#define POLY1	0x17
#define POLY2	0x19

// PSK + FEC + INTERLEAVE
// df=10 : correct up to 4 bits
#define PSKR_K		7
#define PSKR_POLY1	0x6d
#define PSKR_POLY2	0x4f

// df=14 : correct up to 6 bits
#define K11		11
#define K11_POLY1	03073 // 1595
#define K11_POLY2	02365 // 1269

// df=16 : correct up to 7 bits
// Code has good ac(df) and bc(df) parameters for puncturing
#define K13		13
#define K13_POLY1	016461 // 7473
#define K13_POLY2	012767 // 5623

// df=19 : correct up to 9 bits
#define	K16		16
#define	K16_POLY1	0152711 // 54729
#define	K16_POLY2	0126723 // 44499

// For Gray-mapped 8PSK:
// Even when the received phase is distorted by +- 1 phase-position:
//  - One of the bits is still known with 100% certianty.
//  - Only up to 1 bit can be in error
static cmplx graymapped_8psk_pos[] = {
	//			Degrees  Bits In  Mapped Soft-Symbol
	cmplx (1.0, 0.0),         // 0   | 0b000  | 025,000,025
	cmplx (0.7071, 0.7071),   // 45  | 0b001  | 000,025,230
	cmplx (-0.7071, 0.7071),  // 135 | 0b010  | 025,255,025
	cmplx (0.0, 1.0),         // 90  | 0b011  | 000,230,230
	cmplx (0.7071, -0.7071),  // 315 | 0b100  | 230,000,025
	cmplx (0.0, -1.0),        // 270 | 0b101  | 255,025,230
	cmplx (-1.0, 0.0),        // 180 | 0b110  | 230,255,025
	cmplx (-0.7071, -0.7071)  // 225 | 0b111  | 255,230,230
};

// Associated soft-symbols to be used with graymapped_8psk_pos[] constellation
// These softbits have precalculated (a-priori) probabilities applied
// Use of this table automatically Gray-decodes incoming symbols.
static unsigned char graymapped_8psk_softbits[8][3] =  {
	{ 25,   0,  25}, // 0
	{  0,  25, 230}, // 1
	{  0, 230, 230}, // 3
	{ 25, 255,  25}, // 2
	{230, 255,  25}, // 6
	{255, 230, 230}, // 7
	{255,  25, 230}, // 5
	{230,   0,  25}  // 4
};


char pskmsg[80];

void psk::tx_init(SoundBase *sc)
{
	scard = sc;
	for (int car = 0; car < numcarriers; car++) {
		phaseacc[car] = 0;
		prevsymbol[car] = cmplx (1.0, 0.0);
	}
	preamble = dcdbits;
	if (_pskr || _xpsk || _8psk || _16psk) {
		// MFSK based varicode instead of psk
		shreg = 1;
		shreg2 = 1;
	} else {
		shreg = 0;
		shreg2 = 0;
	}
	videoText();

	// interleaver
	bitshreg = 0;
	startpreamble = true;

	symbols = 0;
	acc_symbols = 0;
	ovhd_symbols = 0;
	accumulated_bits = 0;

	if(_8psk && _puncturing) {
		enc->init();
		Txinlv->flush();
	}

	vphase = 0;
	maxamp = 0;

	double bw2 = 6.0 * bandwidth;
	double flo = (get_txfreq_woffset() - bw2);
	if (flo <= 0) flo = 0;
	double fhi = (get_txfreq_woffset() + bw2);
	if (fhi >= 0.48*samplerate) fhi = 0.48*samplerate;
	xmtfilt->init_bandpass (127, 1, flo/samplerate, fhi/samplerate);
}

void psk::rx_init()
{
	for (int car = 0; car < numcarriers; car++) {
		phaseacc[car] = 0;
		prevsymbol[car] = cmplx (1.0, 0.0);
	}
	quality		= cmplx (0.0, 0.0);
	if (_pskr || _xpsk || _8psk || _16psk) {
		// MFSK varicode instead of psk
		shreg = 1;
		shreg2 = 1;
	} else {
		shreg = 0;
		shreg2 = 0;
	}
	dcdshreg = 0;
	dcdshreg2 = 0;
	dcd = 0;
	bitclk = 0;
	freqerr = 0.0;
	if (mailserver && progdefaults.PSKmailSweetSpot) sigsearch = SIGSEARCH;
	else sigsearch = 0;
	put_MODEstatus(mode);
	resetSN_IMD();
	afcmetric = 0.0;
	// interleaver, split incoming bit stream into two, one late by one bit
	rxbitstate = 0;
	fecmet = fecmet2 = 0;
	if (Rxinlv) Rxinlv->flush();
	if (Rxinlv2) Rxinlv2->flush();

}


void psk::restart()
{
	if ((mode >= MODE_PSK31 && mode <= MODE_PSK125) ||
		(mode >= MODE_QPSK31 && mode <= MODE_QPSK125))
		pskviewer->restart(mode);
	evalpsk->setbw(sc_bw);
}

void psk::init()
{
	restart();
	modem::init();
	set_scope_mode(Digiscope::PHASE);
	initSN_IMD();
	snratio = 1.0;
	imdratio = 0.001;
	rx_init();
}

psk::~psk()
{
	if (tx_shape) delete [] tx_shape;
	if (enc) delete enc;
	if (dec) delete dec;
	// FEC 2nd Viterbi decoder
	if (dec2) delete dec2;

	for (int i = 0; i < MAX_CARRIERS; i++) {
		if (fir1[i]) delete fir1[i];
		if (fir2[i]) delete fir2[i];
	}
	if (snfilt) delete snfilt;
	if (imdfilt) delete imdfilt;
	if (e0_filt) delete e0_filt;
	if (e1_filt) delete e1_filt;
	if (e2_filt) delete e2_filt;

	if (pskviewer) delete pskviewer;
	if (evalpsk) delete evalpsk;

	// Interleaver
	if (Rxinlv) delete Rxinlv;
	if (Rxinlv2) delete Rxinlv2;
	if (Txinlv) delete Txinlv;

	if (vestigial_sfft) delete vestigial_sfft;

	if (xmtfilt) delete xmtfilt;
}

psk::psk(trx_mode pskmode) : modem()
{
	cap |= CAP_AFC | CAP_AFC_SR;

	mode = pskmode;

	// Set the defaults that are common to most modes
	samplerate = 8000;
	numcarriers = 1;
	separation = 1.4;
	_16psk = _8psk = _xpsk = _pskr = _qpsk = _disablefec = _puncturing = false;
	symbits = 1;
	flushlength = 0;
	int  isize = 2;
	idepth = 2;
	PSKviterbi = false;
	vestigial = false;

	switch (mode) {
		case MODE_PSK31:
			symbollen = 256;
			dcdbits = 32;
			break;
		case MODE_PSK63:
			symbollen = 128;
			dcdbits = 64;
			break;
		case MODE_PSK125:
			symbollen = 64;
			dcdbits = 128;
			break;
		case MODE_PSK250:
			symbollen = 32;
			dcdbits = 256;
			break;
		case MODE_PSK500:
			symbollen = 16;
			dcdbits = 512;
			break;
		case MODE_PSK1000:
			symbollen = 8;
			dcdbits = 128;
			break;

		case MODE_QPSK31:
			symbollen = 256;
			_qpsk = true;
			dcdbits = 32;
			cap |= CAP_REV;
			break;
		case MODE_QPSK63:
			symbollen = 128;
			_qpsk = true;
			dcdbits = 64;
			cap |= CAP_REV;
			break;
		case MODE_QPSK125:
			symbollen = 64;
			_qpsk = true;
			dcdbits = 128;
			cap |= CAP_REV;
			break;
		case MODE_QPSK250:
			symbollen = 32;
			_qpsk = true;
			dcdbits = 256;
			cap |= CAP_REV;
			break;
		case MODE_QPSK500:
			symbollen = 16;
			_qpsk = true;
			dcdbits = 512;
			cap |= CAP_REV;
			break;
		case MODE_PSK63F:  // As per Multipsk (BPSK63 + FEC + MFSK Varicode)
			symbollen = 128;
			_pskr = true;
			dcdbits = 64;
			break;

	// 8psk modes without FEC
		case MODE_8PSK125:
			symbollen = 128;
			samplerate = 16000;
			_8psk = true;
			_disablefec = true;
			dcdbits = 128;
			vestigial = true;
			cap |= CAP_REV;
			break;
		case MODE_8PSK250: // 250 baud | 375 bits/sec @ 1/2 Rate FEC
			symbollen = 64;
			samplerate = 16000;
			_8psk = true;
			_disablefec = true;
			dcdbits = 256;
			vestigial = true;
			cap |= CAP_REV;
			break;
		case MODE_8PSK500: // 500 baud | 1000 bits/sec @ 2/3 rate FEC
			symbollen = 32;
			samplerate = 16000;
			_8psk = true;
			_disablefec = true;
			dcdbits = 512;
			vestigial = true;
			cap |= CAP_REV;
			break;
		case MODE_8PSK1000: // 1000 baud | 3000 bits/sec  No FEC
			symbollen = 16;
			samplerate = 16000;
			_8psk = true;
			_disablefec = true;
			dcdbits = 1024;
			vestigial = true;
			cap |= CAP_REV;
			break;

	// 8psk modes with FEC
		case MODE_8PSK125FL:
			symbollen = 128;
			idepth = 384; // 1024 milliseconds
			flushlength = 38;
			samplerate = 16000;
			_8psk = true;
			dcdbits = 128;
			vestigial = true;
			cap |= CAP_REV;
			break;
		case MODE_8PSK250FL: // 250 baud | 375 bits/sec @ 1/2 Rate FEC
			symbollen = 64;
			idepth = 512; // 682 milliseconds
			flushlength = 47;
			samplerate = 16000;
			_8psk = true;
			dcdbits = 256;
			vestigial = true;
			cap |= CAP_REV;
			break;
		case MODE_8PSK125F:
			symbollen = 128;
			idepth = 384; // 1024 milliseconds
			flushlength = 38;
			samplerate = 16000;
			_8psk = true;
			dcdbits = 128;
			vestigial = true;
			cap |= CAP_REV;
			break;
		case MODE_8PSK250F: // 250 baud | 375 bits/sec @ 1/2 Rate FEC
			symbollen = 64;
			idepth = 512; // 682 milliseconds
			flushlength = 47;
			samplerate = 16000;
			_8psk = true;
			dcdbits = 256;
			vestigial = true;
			cap |= CAP_REV;
			break;
		case MODE_8PSK500F: // 500 baud | 1000 bits/sec @ 2/3 rate FEC
			symbollen = 32;
			idepth = 640; // 426 milliseconds
			flushlength = 62;
			samplerate = 16000;
			_8psk = true;
			_puncturing = true;
			dcdbits = 512;
			vestigial = true;
			cap |= CAP_REV;
			break;
		case MODE_8PSK1000F: // 1000 baud | 3000 bits/sec
			symbollen = 16;
			idepth = 512;
			flushlength = 56;
			samplerate = 16000;
			_8psk = true;
			dcdbits = 1024;
			cap |= CAP_REV;
			_puncturing = true;
			vestigial = true;
			PSKviterbi = true;
			break;
		case MODE_8PSK1200F: // 1200 baud | 1800 bits/sec
			symbollen = 13;
			idepth = 512;
			flushlength = 56;
			samplerate = 16000;
			_8psk = true;
			_puncturing = true;
			dcdbits = 2048;
			cap |= CAP_REV;
			vestigial = true;
			PSKviterbi = true;
			break;
			// end 8psk modes

		case MODE_PSK125R:
			symbollen = 64;
			_pskr = true;
			dcdbits = 128;
			idepth = 40;  // 2x2x40 interleaver
			break;
		case MODE_PSK250R:
			symbollen = 32;
			_pskr = true;
			dcdbits = 256;
			idepth = 80;  // 2x2x80 interleaver
			break;
		case MODE_PSK500R:
			symbollen = 16;
			_pskr = true;
			dcdbits = 512;
			idepth = 160; // 2x2x160 interleaver
			break;
		case MODE_PSK1000R:
			symbollen = 8;
			_pskr = true;
			dcdbits = 512;
			idepth = 160; // 2x2x160 interleaver
			break;

			// multi-carrier modems
		case MODE_4X_PSK63R:
			symbollen = 128;//PSK63
			dcdbits = 128;
			_pskr = true;//PSKR
			numcarriers = 4;
			idepth = 80; // 2x2x80 interleaver
			break;
		case MODE_5X_PSK63R:
			symbollen = 128; //PSK63
			dcdbits = 512;
			_pskr = true; //PSKR
			numcarriers = 5;
			idepth = 260; // 2x2x160 interleaver
			break;
		case MODE_10X_PSK63R:
			symbollen = 128; //PSK63
			dcdbits = 512;
			_pskr = true; //PSKR
			numcarriers = 10;
			idepth = 160; // 2x2x160 interleaver
			break;
		case MODE_20X_PSK63R:
			symbollen = 128; //PSK63
			dcdbits = 512;
			_pskr = true; //PSKR
			numcarriers = 20;
			idepth = 160; // 2x2x160 interleaver
			break;
		case MODE_32X_PSK63R:
			symbollen = 128; //PSK63
			dcdbits = 512;
			_pskr = true; //PSKR
			numcarriers = 32;
			idepth = 160; // 2x2x160 interleaver
			break;

		case MODE_4X_PSK125R:
			symbollen = 64;//PSK125
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 4;
			idepth = 80; // 2x2x80 interleaver
			break;
		case MODE_5X_PSK125R:
			symbollen = 64;//PSK125
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 5;
			idepth = 160; // 2x2x160 interleaver
			break;
		case MODE_10X_PSK125R:
			symbollen = 64;//PSK125
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 10;
			idepth = 160; // 2x2x160 interleaver
			break;

		case MODE_12X_PSK125:
			symbollen = 64;//PSK125
			dcdbits = 128;//512;
			numcarriers = 12;
			break;
		case MODE_12X_PSK125R:
			symbollen = 64;//PSK125
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 12;
			idepth = 160; // 2x2x160 interleaver
			break;

		case MODE_16X_PSK125R:
			symbollen = 64;//PSK125
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 16;
			idepth = 160; // 2x2x160 interleaver
			break;

		case MODE_2X_PSK250R:
			symbollen = 32;//PSK250
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 2;
			idepth = 160; // 2x2x160 interleaver
			break;
		case MODE_3X_PSK250R:
			symbollen = 32;//PSK250
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 3;
			idepth = 160; // 2x2x160 interleaver
			break;
		case MODE_5X_PSK250R:
			symbollen = 32;//PSK250
			_pskr = true;//PSKR
			dcdbits = 1024;
			numcarriers = 5;
			idepth = 160; // 2x2x160 interleaver
			break;
		case MODE_6X_PSK250:
			symbollen = 32;//PSK250
			dcdbits = 512;
			numcarriers = 6;
			break;
		case MODE_6X_PSK250R:
			symbollen = 32;//PSK250
			_pskr = true;//PSKR
			dcdbits = 1024;
			numcarriers = 6;
			idepth = 160; // 2x2x160 interleaver
			break;
		case MODE_7X_PSK250R:
			symbollen = 32;//PSK250
			_pskr = true;//PSKR
			dcdbits = 1024;
			numcarriers = 7;
			idepth = 160; // 2x2x160 interleaver
			break;

		case MODE_2X_PSK500:
			symbollen = 16;
			dcdbits = 512;
			numcarriers = 2;
			break;
		case MODE_4X_PSK500:
			symbollen = 16;
			dcdbits = 512;
			numcarriers = 4;
			break;

		case MODE_2X_PSK500R:
			symbollen = 16;
			_pskr = true;
			dcdbits = 1024;
			idepth = 160; // 2x2x160 interleaver
			numcarriers = 2;
			break;
		case MODE_3X_PSK500R:
			symbollen = 16;
			_pskr = true;
			dcdbits = 1024;
			idepth = 160; // 2x2x160 interleaver
			numcarriers = 3;
			break;
		case MODE_4X_PSK500R:
			symbollen = 16;
			_pskr = true;
			dcdbits = 1024;
			idepth = 160; // 2x2x160 interleaver
			numcarriers = 4;
			break;

		case MODE_2X_PSK800:
			symbollen = 10;
			_pskr = false;
			dcdbits = 512;
			numcarriers = 2;
			break;
		case MODE_2X_PSK800R:
			symbollen = 10;
			_pskr = true;
			dcdbits = 1024;
			idepth = 160; // 2x2x160 interleaver
			numcarriers = 2;
			break;

		case MODE_2X_PSK1000:
			symbollen = 8;//PSK1000
			dcdbits = 1024;
			numcarriers = 2;
			idepth = 160; // 2x2x160 interleaver
			break;
		case MODE_2X_PSK1000R:
			symbollen = 8;//PSK1000
			_pskr = true;//PSKR
			dcdbits = 1024;
			numcarriers = 2;
			idepth = 160; // 2x2x160 interleaver
			break;

		default:
			mode = MODE_PSK31;
			symbollen = 256;
			dcdbits = 32;
			numcarriers = 1;
	}

	// Set the number of bits-per-symbol based on the chosen constellation
	if (_qpsk || _xpsk) symbits = 2;
	else if (_8psk) symbits = 3;
	else if (_16psk) symbits = 4;
	else symbits = 1; // else BPSK / PSKR


	//printf("%s: symlen %d, dcdbits %d, _qpsk %d, _pskr %d, numc %f\n",
	//mode_info[mode].sname,
	//symbollen, dcdbits, _qpsk, _pskr, numcarriers);


	enc = (encoder *)0;
	dec = (viterbi *)0;
	// BPSK+FEC - 2nd Viterbi decoder and de-interleaver
	dec2 = (viterbi *)0;
	Txinlv = (interleave *)0;
	Rxinlv = (interleave *)0;
	Rxinlv2 = (interleave *)0;
	vestigial_sfft = (sfft *)0;

	// create impulse response for experimental FIR filters
	double fir1c[64];
	double fir2c[64];

	for (int i = 0; i < MAX_CARRIERS; i++) {
		if (i < numcarriers) {
			fir1[i] = new C_FIR_filter();
			fir2[i] = new C_FIR_filter();
		} else {
			fir1[i] = (C_FIR_filter *)0;
			fir2[i] = (C_FIR_filter *)0;
		}
	}

	switch (progdefaults.PSK_filter) {
		case 1:
			// use the original gmfsk matched filters
			for (int i = 0; i < 64; i++) {
				fir1c[i] = gmfir1c[i];
				fir2c[i] = gmfir2c[i];
			}
			for (int i = 0; i < numcarriers; i++) {
				fir1[i]->init(FIRLEN, symbollen > 15 ? symbollen / 16 : 1, fir1c, fir1c);
				fir2[i]->init(FIRLEN, 1, fir2c, fir2c);
			}
			break;
		case 2:
			// creates fir1c matched sin(x)/x filter w hamming
			wsincfilt(fir1c, 1.0 / symbollen, false);
			// creates fir2c matched sin(x)/x filter w hamming
			wsincfilt(fir2c, 1.0 / 16.0, false);
			for (int i = 0; i < numcarriers; i++) {
				fir1[i]->init(FIRLEN, symbollen > 15 ? symbollen / 16 : 1, fir1c, fir1c);
				fir2[i]->init(FIRLEN, 1, fir2c, fir2c);
			}
			break;
		case 3:
			// creates fir1c matched sin(x)/x filter w hamming
			wsincfilt(fir1c, 1.0 / symbollen, false);
			// 1/22 with Hamming window nearly identical to gmfir2c
			wsincfilt(fir2c, 1.0 / 22.0, false);
			for (int i = 0; i < numcarriers; i++) {
				fir1[i]->init(FIRLEN, symbollen > 15 ? symbollen / 16 : 1, fir1c, fir1c);
				fir2[i]->init(FIRLEN, 1, fir2c, fir2c);
			}
			break;
		case 4:
			wsincfilt(fir2c, 1.5 / 16.0, true);
			for (int i = 0; i < numcarriers; i++) {
				fir1[i]->init_lowpass (FIRLEN, 16, 1.5 / symbollen);
				fir2[i]->init(FIRLEN, 1, fir2c, fir2c);
			}
		case 0:
		default :
			// creates fir1c matched sin(x)/x filter w blackman
			wsincfilt(fir1c, 1.0 / symbollen, true);
			// creates fir2c matched sin(x)/x filter w blackman
			wsincfilt(fir2c, 1.0 / 16.0, true);
			for (int i = 0; i < numcarriers; i++) {
				fir1[i]->init(FIRLEN, symbollen > 15 ? symbollen / 16 : 1, fir1c, fir1c);
				fir2[i]->init(FIRLEN, 1, fir2c, fir2c);
			}
	}

	snfilt = new Cmovavg(16);
	imdfilt = new Cmovavg(16);
	e0_filt = new Cmovavg(dcdbits / 2);
	e1_filt = new Cmovavg(dcdbits / 2);
	e2_filt = new Cmovavg(dcdbits / 2);

	if (_disablefec) {
		enc = NULL;
		dec = dec2 = NULL;

	} else if (_qpsk) {
		enc = new encoder(K, POLY1, POLY2);
		dec = new viterbi(K, POLY1, POLY2);

	} else if (_pskr || PSKviterbi) {
		// FEC for BPSK. Use a 2nd Viterbi decoder for comparison.
		// Set decode size to 4 since some characters can be as small
		// as 3 bits long. This minimises intercharacters decoding
		// interactions.
		enc = new encoder(PSKR_K, PSKR_POLY1, PSKR_POLY2);
		dec = new viterbi(PSKR_K, PSKR_POLY1, PSKR_POLY2);
		dec->setchunksize(4);
		dec2 = new viterbi(PSKR_K, PSKR_POLY1, PSKR_POLY2);
		dec2->setchunksize(4);

	} else if (mode == MODE_8PSK125F || mode == MODE_8PSK250F) {
		enc = new encoder(K16, K16_POLY1, K16_POLY2);
		dec = new viterbi(K16, K16_POLY1, K16_POLY2);
		dec->setchunksize(4);
		dec2 = new viterbi(K13, K16_POLY1, K16_POLY2);
		dec2->setchunksize(4);

	} else if (_xpsk || _8psk || _16psk) {
		enc = new encoder(K13, K13_POLY1, K13_POLY2);
		dec = new viterbi(K13, K13_POLY1, K13_POLY2);
		dec->setchunksize(4);
		// Second viterbi decoder is only needed when modem has an odd number of bits/symbol.
		if ( _8psk && !_puncturing ) { // (punctured 8psk has 3-real bits + 1-punctured bit per transmitted symbol)
			dec2 = new viterbi(K13, K13_POLY1, K13_POLY2);
			dec2->setchunksize(4);
		}
		if (_puncturing) { // punctured codes benefit from a longer traceback
			dec->settraceback(K13 * 16);
			if (dec2) dec2->settraceback(K13 * 16);
		}
	}

	// Interleaver. For PSKR to maintain constant time delay between bits,
	// we double the number of concatenated square iterleavers for
	// each doubling of speed: 2x2x20 for BSK63+FEC, 2x2x40 for
	// BPSK125+FEC, etc..

	Txinlv = new interleave (isize, idepth, INTERLEAVE_FWD);
	Rxinlv = new interleave (isize, idepth, INTERLEAVE_REV);
	if (dec2) Rxinlv2 = new interleave (isize, idepth, INTERLEAVE_REV);

	bitshreg = 0;
	rxbitstate = 0;
	startpreamble = true;

	tx_shape = new double[symbollen];

	// raised cosine shape for the transmitter
	for ( int i = 0; i < symbollen; i++)
		tx_shape[i] = 0.5 * cos(i * M_PI / symbollen) + 0.5;

	fragmentsize = symbollen;
	sc_bw = samplerate / symbollen;
	//JD added for multiple carriers
	inter_carrier = separation * sc_bw;
	bandwidth = sc_bw * ( 1 + separation * (numcarriers - 1));

	snratio = s2n = imdratio = imd = 0;

	if (mailserver && progdefaults.PSKmailSweetSpot)
		sigsearch = SIGSEARCH;
	else
		sigsearch = 0;
	for (int i = 0; i < 16; i++)
		syncbuf[i] = 0.0;
	E1 = E2 = E3 = 0.0;
	acquire = 0;

	evalpsk = new pskeval;
	if ((mode >= MODE_PSK31 && mode <= MODE_PSK125) ||
		(mode >= MODE_QPSK31 && mode <= MODE_QPSK125))
		pskviewer = new viewpsk(evalpsk, mode);
	else
		pskviewer = 0;

	if (vestigial) {
		if (samplerate == 16000)
			sfft_size = 16384;
		else
			sfft_size = 8192;

		int bin = sc_bw * sfft_size / samplerate;
		vestigial_sfft = new sfft(sfft_size, bin - 5, bin + 6); // 11 bins
		for (int i = 0; i < 11; i++) sfft_bins[i] = cmplx(0,0);
	}

	xmtfilt = new C_FIR_filter();

}

//=============================================================================
//=========================== psk31 receive routines ==========================
//=============================================================================


void psk::s2nreport(void)
{
	modem::s2nreport();
	s2n_sum = s2n_sum2 = s2n_ncount = 0.0;
}

void psk::rx_bit(int bit)
{
	int c;
	bool do_s2nreport = false;
	shreg = (shreg << 1) | !!bit;
	if (_pskr || _xpsk || _8psk || _16psk) {
		// MFSK varicode instead of PSK Varicode
		if ((shreg & 7) == 1) {
			c = varidec(shreg >> 1);
			// Voting at the character level for only PSKR modes
			if (fecmet >= fecmet2 || _disablefec) {
				if ((c != -1) && (c != 0) && (dcd == true)) {
					put_rx_char(c);
					do_s2nreport = true;
				}
			}
			shreg = 1;
		}
	} else {
		// PSK varicode
		if ((shreg & 3) == 0) {
			c = psk_varicode_decode(shreg >> 2);
			if ((c != -1) && (dcd == true)) {
				put_rx_char(c);
				do_s2nreport = true;
			}
			shreg = 0;
		}
	}

	if (do_s2nreport) {
		if (progdefaults.Pskmails2nreport && (mailserver || mailclient)) {
			s2n_sum += s2n_metric;
			s2n_sum2 += (s2n_metric * s2n_metric);
			s2n_ncount ++;
			if (c == EOT)
				s2nreport();
		}
	}

}




void psk::rx_bit2(int bit)
{
	int c;
	bool do_s2nreport = false;

	shreg2 = (shreg2 << 1) | !!bit;
	// MFSK varicode instead of PSK Varicode
	if ((shreg2 & 7) == 1) {
		c = varidec(shreg2 >> 1);
		// Voting at the character level for only PSKR modes
		if (fecmet < fecmet2 || _disablefec) {
			if ((c != -1) && (c != 0) && (dcd == true)) {
				put_rx_char(c);
				do_s2nreport = true;
			}
		}
		shreg2 = 1;
	}

	if (do_s2nreport) {
		if (progdefaults.Pskmails2nreport && (mailserver || mailclient)) {
			s2n_sum += s2n_metric;
			s2n_sum2 += (s2n_metric * s2n_metric);
			s2n_ncount ++;
			if (c == EOT)
				s2nreport();
		}
	}
}

void psk::rx_qpsk(int bits)
{
	unsigned char sym[2];
	int c;

	if (_qpsk && !reverse)
		bits = (4 - bits) & 3;

	sym[0] = (bits & 1) ? 255 : 0;
	sym[1] = (bits & 2) ? 0 : 255;	// top bit is flipped

	//JD added de-interleaver
	//	Rxinlv->symbols(sym);

	c = dec->decode(sym, NULL);

	if (c != -1) {
		rx_bit(c & 0x80);
		rx_bit(c & 0x40);
		rx_bit(c & 0x20);
		rx_bit(c & 0x10);
		rx_bit(c & 0x08);
		rx_bit(c & 0x04);
		rx_bit(c & 0x02);
		rx_bit(c & 0x01);
	}
}


void psk::rx_pskr(unsigned char symbol)
{
	int met;
	unsigned char twosym[2];
	unsigned char tempc;
	int c;

	//In the case of multiple carriers, if even number of carriers then we
	// know the bit-order and don't need voting otherwise
	// we accumulate the soft bits for the interleaver THEN submit to Viterbi
	// decoder in alternance so that each one is processed one bit later.
	// Only two possibilities for sync: current bit or previous one since
	// we encode with R = 1/2 and send encoded bits one after the other
	// through the interleaver.

	symbolpair[1] = symbolpair[0];
	symbolpair[0] = symbol;


	if (rxbitstate == 0) {
		rxbitstate++;
		//Only use one decoder is using even carriers (we know the bits order)
		//		if (((int)numcarriers) % 2 == 0) {
		//			fecmet2 = -9999.0;
		//			return;
		//		}
		// XPSK and 16PSK have even number of bits/symbol
		// Punctured 8PSK has even number of bits/symbol (3 real + 1 punctured)
		// so bit order known: can use only one decoder to reduce CPU usage
		if ( _xpsk || _16psk || (_8psk && _puncturing) ) {
			fecmet2 = -9999.0;
			return;
		}
		// copy to avoid scrambling symbolpair for the next bit
		twosym[0] = symbolpair[0];
		twosym[1] = symbolpair[1];
		// De-interleave
		if (mode != MODE_PSK63F) Rxinlv2->symbols(twosym);
		// pass de-interleaved bits pair to the decoder, reversed
		tempc = twosym[1];
		twosym[1] = twosym[0];
		twosym[0] = tempc;
		// Then viterbi decoder
		c = dec2->decode(twosym, &met);
		if (c != -1) {
			// FEC only take metric measurement after backtrace
			// Will be used for voting between the two decoded streams
			fecmet2 = decayavg(fecmet2, met, 20);
			rx_bit2(c & 0x08);
			rx_bit2(c & 0x04);
			rx_bit2(c & 0x02);
			rx_bit2(c & 0x01);
		}
	} else {
		// Again for the same stream shifted by one bit
		rxbitstate = 0;
		twosym[0] = symbolpair[0];
		twosym[1] = symbolpair[1];
		// De-interleave
		if (mode != MODE_PSK63F) Rxinlv->symbols(twosym);
		tempc = twosym[1];
		twosym[1] = twosym[0];
		twosym[0] = tempc;
		// Then viterbi decoder
		c = dec->decode(twosym, &met);
		if (c != -1) {
			fecmet = decayavg(fecmet, met, 20);
			rx_bit(c & 0x08);
			rx_bit(c & 0x04);
			rx_bit(c & 0x02);
			rx_bit(c & 0x01);
		}
	}
}

void psk::searchDown()
{
	double srchfreq = frequency - sc_bw * 2;
	double minfreq = sc_bw * 2;
	double spwr, npwr;
	while (srchfreq > minfreq) {
		spwr = wf->powerDensity(srchfreq, sc_bw);
		npwr = wf->powerDensity(srchfreq + sc_bw, sc_bw/2) + 1e-10;
		if (spwr / npwr > pow(10, progdefaults.ServerACQsn / 10)) {
			frequency = srchfreq;
			set_freq(frequency);
			sigsearch = SIGSEARCH;
			break;
		}
		srchfreq -= sc_bw;
	}
}

void psk::searchUp()
{
	double srchfreq = frequency + sc_bw * 2;
	double maxfreq = IMAGE_WIDTH - sc_bw * 2;
	double spwr, npwr;
	while (srchfreq < maxfreq) {
		spwr = wf->powerDensity(srchfreq, sc_bw/2);
		npwr = wf->powerDensity(srchfreq - sc_bw, sc_bw/2) + 1e-10;
		if (spwr / npwr > pow(10, progdefaults.ServerACQsn / 10)) {
			frequency = srchfreq;
			set_freq(frequency);
			sigsearch = SIGSEARCH;
			break;
		}
		srchfreq += sc_bw;
	}
}

int waitcount = 0;

void psk::findsignal()
{
	put_Status1("");
	put_Status2("");
	put_status("");

	int ftest, f1, f2;

	if (sigsearch > 0) {
		sigsearch--;
		if (mailserver) { // mail server search algorithm
			if (progdefaults.PSKmailSweetSpot) {
				f1 = (int)(progdefaults.ServerCarrier - progdefaults.ServerOffset);
				f2 = (int)(progdefaults.ServerCarrier + progdefaults.ServerOffset);
			} else {
				f1 = (int)(frequency - progdefaults.ServerOffset);
				f2 = (int)(frequency + progdefaults.ServerOffset);
			}
			if (evalpsk->sigpeak(ftest, f1, f2) > pow(10, progdefaults.ServerACQsn / 10) ) {
				if (progdefaults.PSKmailSweetSpot) {
					if (fabs(ftest - progdefaults.ServerCarrier) < progdefaults.ServerOffset) {
						frequency = ftest;
						set_freq(frequency);
						freqerr = 0.0;
					} else {
						frequency = progdefaults.ServerCarrier;
						set_freq(frequency);
						freqerr = 0.0;
					}
				} else {
					frequency = ftest;
					set_freq(frequency);
					freqerr = 0.0;
				}
			} else { // less than the detection threshold
				if (progdefaults.PSKmailSweetSpot) {
					frequency = progdefaults.ServerCarrier;
					set_freq(frequency);
					sigsearch = SIGSEARCH;
				}
			}
		} else { // normal signal search algorithm
			f1 = (int)(frequency - progdefaults.SearchRange/2);
			f2 = (int)(frequency + progdefaults.SearchRange/2);
			if (evalpsk->sigpeak(ftest, f1, f2) > pow(10, progdefaults.ACQsn / 10.0) ) {
				frequency = ftest;
				set_freq(frequency);
				freqerr = 0.0;
				sigsearch = 0;
				acquire = dcdbits;
			}
		}
	}
}

//DHF: AFC based on vestigial carrier located at f0 - bandwidth
void psk::vestigial_afc() {

	if (!progdefaults.pskpilot) return;
	if (!vestigial_sfft->is_stable()) return;

	double avg = 0;
	int i = 0;
	for (i = 0; i < 11; i++) avg += abs(sfft_bins[i]);
	avg /= 11.0;
	std::setprecision(2); std::setw(5);
	for (i = 0; i < 11; i++) if (abs(sfft_bins[i]) > 2.0*avg) break;
	if (i < 11) {
//		std::cout	<< "bin: " << i
//					<< ", freq offset: " << (i - 5)*samplerate/16384.0
//					<< ", amp: " << abs(sfft_bins[i])
//					<< ", avg: " << avg << "\n";
		if (i != 5) {
			frequency -= 1.0*(i-5)*samplerate/sfft_size;
			set_freq (frequency);
		}
	}
	vestigial_sfft->reset();
}

//JD: disable for multiple carriers as we are running as modem and
//    therefore use other strategies for frequency alignment like RSID
void psk::phaseafc()
{
	double error;
	// Skip AFC for modes it does not work with

	if (vestigial) return vestigial_afc();

	if (afcmetric < 0.05 ||
		mode == MODE_PSK500 ||
		mode == MODE_QPSK500 || numcarriers > 1) return;

	error = (phase - bits * M_PI / 2.0);
	if (error < -M_PI / 2.0 || error > M_PI / 2.0) return;
	error *= samplerate / (TWOPI * symbollen);
	if (fabs(error) < sc_bw ) {
		freqerr = error / dcdbits;
		frequency -= freqerr;
		if (mailserver) {
			if (frequency < progdefaults.ServerCarrier - progdefaults.ServerAFCrange)
				frequency = progdefaults.ServerCarrier - progdefaults.ServerAFCrange;
			if (frequency > progdefaults.ServerCarrier + progdefaults.ServerAFCrange)
				frequency = progdefaults.ServerCarrier + progdefaults.ServerAFCrange;
		}
		set_freq (frequency);
	}
	if (acquire) acquire--;
}

void psk::afc()
{
	if (!progStatus.afconoff)
		return;
	if (dcd == true || acquire)
		phaseafc();
}

void psk::rx_symbol(cmplx symbol, int car)
{
	int n;
	unsigned char softbit = 0;
	double softangle;
	double softamp;
	double sigamp = norm(symbol);

	static double averageamp;

	phase = arg ( conj(prevsymbol[car]) * symbol );
	prevsymbol[car] = symbol;

	/// align the RX constellation to the TX constellation, for Non-FEC modes
	if (_disablefec && (_16psk || _8psk || _xpsk )) phase -= M_PI;

	if (phase < 0) phase += TWOPI;

	if (_qpsk) {
		n = 4;
		bits = ((int) (phase / M_PI_2 + 0.5)) & (n-1);
	} else if (_xpsk) {
		n = 4;
		bits = ((int) (phase / M_PI_2)) & (n-1);
	} else if (_8psk) {
		n = 8;
		bits = ((int) (phase / (M_PI/4.0) + 0.5)) & (n-1);
	} else if (_16psk) {
		n = 16;
		bits = ((int) (phase / (M_PI/8.0) + 0.5)) & (n-1);
	} else { // bpsk and pskr
		n = 2;
		bits = (((int) (phase / M_PI + 0.5)) & (n-1) ) << 1;
		// hard decode if needed
		// softbit = (bits & 2) ? 0 : 255;
		// reversed as we normally pass "!bits" when hard decoding
		averageamp = decayavg(averageamp, sigamp, SQLDECAY);
		if (sigamp > 0 && averageamp > 0) {
			if (sigamp > averageamp) {
				softamp = clamp( sqrt(sigamp / averageamp), 1.0, 1e6);
			} else {
				softamp = clamp( sqrt(averageamp / sigamp), 1.0, 1e6);
			}
		} else {
			softamp = 2; // arbritary number (50% impact)
		}
		// Compute values between -128 and +127 for phase value only
		double alpha = phase / M_PI;
		if (alpha > 1.0) alpha = 2.0 - alpha;
		softangle = 127.0 - 255.0 * alpha;
		softbit = (unsigned char) ((softangle / ( 1.0 + softamp / 2.0)) + 128);
	}

	// simple low pass filter for quality of signal
	double decay = SQLDECAY;
	double attack = SQLDECAY;
	double cval = cos(n*phase);
	double sval = sin(n*phase);

	if (_8psk) {
		attack *= 2;
		decay *= 4;
	}

	if (_pskr) {
		decay *= 10;
		quality = cmplx(
			decayavg(quality.real(), cval, decay),
			decayavg(quality.imag(), sval, decay));
	} else
		quality = cmplx(
			decayavg(quality.real(), cval, cval > quality.real() ? attack : decay),
			decayavg(quality.imag(), sval, sval > quality.real() ? attack : decay));

	metric = 100.0 * norm(quality);
	if (_pskr && (averageamp < 3e-5)) metric = 0;

	if (progdefaults.Pskmails2nreport && (mailserver || mailclient)) {
		//s2n reporting: rescale depending on mode, clip after scaling
		if (_pskr)
			s2n_metric = metric * 1.5 + 8;
		else
			s2n_metric = metric;
		s2n_metric = CLAMP(s2n_metric, 0.0, 100.0);
	}

	// FEC: adjust squelch for extra sensitivity.
	// Otherwise we miss good characters
	// ***********************************************************
	// **** DHF still needed with attack/decay filtering?
	// ***********************************************************
	//	if (_pskr) {
	//		metric = metric * 4;
	//	}
	//	else if ( (_xpsk || _8psk || _16psk) && !_disablefec) {
	//		metric *= 2 * symbits; /// @TODO scale the metric with the psk constellation density
	//	}

	if (metric > 100)
		metric = 100;

	afcmetric = decayavg(afcmetric, norm(quality), 50);

	dcdshreg = ( dcdshreg << (symbits+1) ) | bits;

	int set_dcdON = -1; // 1 sets DCD on ; 0 sets DCD off ; -1 does neither (no-op)

	switch (dcdshreg) {

			// bpsk DCD on
		case 0xAAAAAAAA:
			if ( _xpsk || _8psk || _16psk) break;
			if (_pskr) break;
			set_dcdON = 1;
			break;

			// pskr DCD on
		case 0x0A0A0A0A:
			if ( _xpsk || _8psk || _16psk) break;
			if (_qpsk) break;
			if (!_pskr) break;
			set_dcdON = 1;
			break;

			// 8psk DCD on (FEC enabled, with Gray-mapped constellation)
		case 0x25252525: // UN-punctured
			if (_pskr || _xpsk || _16psk) break;
			if (!_8psk) break;
			if (_disablefec) break;
			set_dcdON = 1;
			break;
		case 0x22222222: // punctured @ 2/3 rate
			if (_pskr || _xpsk || _16psk) break;
			if (!_8psk) break;
			if (_disablefec) break;
			set_dcdON = 1;
			break;

		case 0x92492492:	// xpsk DCD off (with FEC disabled)
			if (_pskr) break;
			if (_qpsk) break;
			if (!_xpsk) break;
			if (!_disablefec) break;
			set_dcdON = 0;
			break;

		case 0x10842108:	// 16psk DCD off (with FEC disabled)
			if (_pskr) break;
			if (!_16psk) break;
			if (!_disablefec) break;
			set_dcdON = 0;
			break;

		case 0x44444444:	// 8psk DCD off (with FEC disabled)
			if (!_8psk) break;
			if (!_disablefec) break;
			set_dcdON = 0;
			break;

		case 0x10410410:	// xpsk DCD on (with FEC enabled)
			if (_pskr) break;
			if (_qpsk) break;
			if (_8psk) break;
			if (_16psk) break;
			if (!_xpsk) break;
			if (_disablefec) break;
			set_dcdON = 1;
			break;

		case 0x00000000:	// bpsk DCD off.  x,8,16psk DCD on (with FEC disabled).
			if (_pskr) break;
			if (_xpsk || _8psk || _16psk) {
				if (!_disablefec) break;
				set_dcdON = 1;
				break;
			}
			set_dcdON = 0;
			break;

		default:
			if (metric > progStatus.sldrSquelchValue || progStatus.sqlonoff == false) {
				dcd = true;
			} else {
				dcd = false;
			}
	}

	displaysn = false;
	if ( 1 == set_dcdON ) {
		displaysn = true;
		dcd = true;
		acquire = 0;
		quality = cmplx (1.0, 0.0);
		if (progdefaults.Pskmails2nreport && (mailserver || mailclient))
			s2n_sum = s2n_sum2 = s2n_ncount = 0.0;
		//printf("\n DCD ON!!");
	} else if ( 0 == set_dcdON ){
		dcd = false;
		acquire = 0;
		quality = cmplx (0.0, 0.0);
		//printf("\n DCD OFF!!!!!!!!!");
	}

	if (_pskr) {
		rx_pskr(softbit);
		set_phase(phase, norm(quality), dcd);

	} else if (dcd == true) {

		set_phase(phase, norm(quality), dcd);

		if (!_disablefec && (_16psk || _8psk || _xpsk) ) {
			int bitmask = 1;
			unsigned char xsoftsymbols[symbits];

			//printf("\n");
			if ( (_puncturing && _16psk) ) rx_pskr(128); // 16psk: recover punctured low bit

			// Soft-decode of Gray-mapped 8psk
			if (_8psk) {
				bool softpuncture = false;
				static double lastphasequality=0;
				double phasequality = fabs(cos( n/2 * phase));
				phasequality = (phasequality + lastphasequality) / 2; // Differential modem: average probabilities between current and previous symbols
				lastphasequality = phasequality;
				int soft_qualityerror = static_cast<int>(128 - (128 * phasequality)) ;

				if (soft_qualityerror > 255-25) // Prevent soft-bit wrap-around (crossing of value 128)
					softpuncture = true;
				else if (soft_qualityerror < 128/3) // First 1/3 of phase delta is considered a perfect signal
					soft_qualityerror = 0;
				else if (soft_qualityerror > 128 - (128/8) ) // Last 1/8 of phase delta triggers a puncture
					softpuncture = true;
				else
					soft_qualityerror /= 2; // Scale the FEC error to prevent premature cutoff


				if (softpuncture) {
					for(int i=0; i<symbits; i++) rx_pskr(128);
				} else {
					int bitindex = static_cast<int>(bits);
					for(int i=symbits-1; i>=0; i--) { // Use predefined Gray-mapped softbits for soft-decoding
						if (graymapped_8psk_softbits[bitindex][i] > 128) // Soft-One
							rx_pskr( (graymapped_8psk_softbits[bitindex][i]) - soft_qualityerror );
						else // Soft-Zero
							rx_pskr( (graymapped_8psk_softbits[bitindex][i]) + soft_qualityerror  );
					}
				}

			} else {
				//Hard Decode Section
				for(int i=0; i<symbits; i++) { // Hard decode symbits into soft-symbols
					xsoftsymbols[i] = (bits & bitmask) ? 255 : 0 ;
					//printf(" %.3u ", xsoftsymbols[i]);
					rx_pskr(xsoftsymbols[i]); // Feed to the PSKR FEC decoder, one bit at a time.
					bitmask = bitmask << 1;
				}
			}
			if (_puncturing) rx_pskr(128); // x/8/16psk: Recover punctured high bit

		} else if (_16psk || _8psk || _xpsk) {
			// Decode symbol one bit at a time
			int bitmask = 1;
			for (int i = 0; i < symbits; i++) {
				rx_bit( ((bits) & bitmask) );
				bitmask = bitmask << 1;
			}

		} else if (_qpsk)
			rx_qpsk(bits);

		else
			rx_bit(!bits); //default to BPSK
	}
}

static double e0, e1, e2;

void psk::signalquality()
{
//	double e0, e1, e2;
	e0 = e0_filt->run(m_Energy[0]);
	e1 = e1_filt->run(m_Energy[1]);
	e2 = e2_filt->run(m_Energy[2]);

	if (((e0 - e1) > 0) && (e1 > 0))
		snratio = (e0 - e1) / e1;
	else
		snratio = 1000.0;

	if (((e0 - e1) > 0) && ((e2 - e1) > 0) )
		imdratio = (e2 - e1) / (e0 - e1);
	else
		imdratio = 0.001;

}

void psk::update_syncscope()
{
	static char msg1[16];
	static char msg2[16];

	display_metric(metric);

	if (displaysn) {
		memset(msg1, 0, sizeof(msg1));
		memset(msg2, 0, sizeof(msg2));

		s2n = 10.0*log10( snratio );
		snprintf(msg1, sizeof(msg1), "s/n %2.0f dB", s2n);

		imd = 10.0*log10( imdratio );
		snprintf(msg2, sizeof(msg2), "imd %2.0f dB", imd);

		put_Status1(	msg1,
						progdefaults.StatusTimeout,
						progdefaults.StatusDim ? STATUS_DIM : STATUS_CLEAR);
		put_Status2(	msg2,
						progdefaults.StatusTimeout,
						progdefaults.StatusDim ? STATUS_DIM : STATUS_CLEAR);
	}

//static char msg3[50];
//memset(msg3, 0, sizeof(msg3));
//snprintf(msg3, sizeof(msg3), "%10.3f, %10.3f, %10.3f",
//e0, e1, e2);
//put_status(msg3);

}

char bitstatus[100];

int psk::rx_process(const double *buf, int len)
{
	double delta[MAX_CARRIERS], frequencies[MAX_CARRIERS];
	cmplx z, z2[MAX_CARRIERS];
	bool can_rx_symbol = false;

	if (mode >= MODE_PSK31 && mode <= MODE_PSK125) {
		if (!progdefaults.report_when_visible ||
			dlgViewer->visible() || progStatus.show_channels )
			if (pskviewer && !bHistory) pskviewer->rx_process(buf, len);
		if (evalpsk)
			evalpsk->sigdensity();
	}

	frequencies[0] = frequency + ((-1 * numcarriers) + 1) * inter_carrier / 2;
	delta[0] = TWOPI * frequencies[0] / samplerate;
	for (int car = 1; car < numcarriers; car++) {
		frequencies[car] = frequencies[car - 1] + inter_carrier;
		delta[car] = TWOPI * frequencies[car] / samplerate;
	}

	while (len-- > 0) {

		for (int car = 0; car < numcarriers; car++) {

			// Mix with the internal NCO
			z = cmplx ( *buf * cos(phaseacc[car]), *buf * sin(phaseacc[car]) );

// if we re-enable multi-carrier vestigial carrier
//			if (vestigial && car == 0) vestigial_sfft->run(z, sfft_bins, 1);
			if (vestigial && progdefaults.pskpilot) vestigial_sfft->run(z, sfft_bins, 1);

			phaseacc[car] += delta[car];
			if (phaseacc[car] > TWOPI) phaseacc[car] -= TWOPI;

			// Filter and downsample
			// by 16 (psk31, qpsk31)
			// by  8 (psk63, qpsk63)
			// by  4 (psk125, qpsk125)
			// by  2 (psk250, qpsk250)
			// by  1 (psk500, qpsk500) = no down sampling
			// first filter
			if (fir1[car]->run( z, z )) { // fir1 returns true every Nth sample
										  // final filter
				fir2[car]->run( z, z2[car] ); // fir2 returns value on every sample

				//On last carrier processing
				if (car == numcarriers - 1) {

					calcSN_IMD(z); //JD OR all carriers together check logic???

	/**
	* This is the symbol timing recovery mechanism.  After the demodulated
	* signal is processed by the matched filters, the signal lobes are
	* expected to have been modified to a fairly symmetric shape.  The
	* magnitude of the samples are taken, thus rectifying the signal to
	* positive values. "bitclk" is a counter that is very close in rate to
	* (samples / symbol).  Its purpose is to repeatedly "draw" one symbol
	* waveform in the syncbuf array, according to its amplitude (not phase).
	*/

					int idx = (int) bitclk;
					double sum = 0.0;
					double ampsum = 0.0;
					for (int ii = 0; ii < numcarriers; ii++) {
						sum += abs(z2[ii])/numcarriers;
					}
					//			syncbuf[idx] = 0.8 * syncbuf[idx] + 0.2 * z2[car].mag();
					syncbuf[idx] = 0.8 * syncbuf[idx] + 0.2 * sum;
					sum = 0.0;

					double bitsteps = (symbollen >= 16 ? 16 : symbollen);
					int symsteps = (int) (bitsteps / 2);

	/**
	* Here we sum up the difference between each sample's magnitude in the
	* lower half of the array with its counterpart on the upper half of the
	* array, or the other side of the waveform.  Each pair's difference is
	* divided by their sum, scaling it so that the signal amplitude does not
	* affect the result.  When the differences are summed, it gives an
	* indication of which side is larger than the other.
	*/

					for (int i = 0; i < symsteps; i++) {
						sum += (syncbuf[i] - syncbuf[i+symsteps]);
						ampsum += (syncbuf[i] + syncbuf[i+symsteps]);
					}
					// added correction as per PocketDigi
					sum = (ampsum == 0 ? 0 : sum / ampsum);

	/**
	* If the lower side is larger (meaning that the waveform is shifted in that
	* direction), then the sum is negative, and bitclk needs to be adjusted to
	* be a little faster, so that the next drawing of the waveform in syncbuf
	* will be shifted right. Conversely, if the sum is positive, then it needs
	* to slow down bitclk so that the waveform is shifted left.  Thus the
	* error is subtracted from bitclk, rather than added.  The goal is to
	* get the error as close to zero as possible, so that the receiver is
	* exactly synced with the transmitter and the waveform is exactly in
	* the middle of syncbuf.
	*/

					//			bitclk -= sum / 5.0;
					bitclk -= sum / (5.0 * 16 / bitsteps);
					bitclk += 1;

	/**
	* When bitclock reaches the end of the buffer, then a complete waveform
	* has been received.  It is time to output the current sample and wrap
	* around to the next cycle.
	*
	* There is a complete symbol waveform in syncbuf, so that each
	* sample[0..N/2-1] is very close in amplitude with the corresponding
	* sample in [N/2..N-1].
	*
	*     |            ********                       ********            |
	*     |        ****        ****               ****        ****        |
	*     |     ***                ***         ***                ***     |
	*     |   **                      **     **                      **   |
	*     |  *                          *   *                          *  |
	*     | *                            * *                            * |
	*     |*                              *                              *|
	*     |_______________________________________________________________|
	*     0                              N/2                             N-1
	*
	*     === or some variation of it .... ===
	*
	*     |****                       ********                       *****|
	*     |    ****               ****        ****               ****     |
	*     |        ***         ***                ***         ***         |
	*     |           **     **                      **     **            |
	*     |             *   *                          *   *              |
	*     |              * *                            * *               |
	*     |               *                              *                |
	*     |_______________________________________________________________|
	*     0                              N/2                             N-1
	*
	* A	t the end of this cycle, bitclk is pointing at a sample which will
	* have the maximum phase difference, if any, from the previous symbol's
	* phase.
	*
	*/

					if (bitclk < 0) bitclk += bitsteps;
					if (bitclk >= bitsteps) {
						bitclk -= bitsteps;
						can_rx_symbol = true;
						update_syncscope();
						afc();
					}
				}

			}
		}
		if (can_rx_symbol) {
			for (int car = 0; car < numcarriers; car++) {
				rx_symbol(z2[car], car);
			}
			can_rx_symbol = false;
		}
		buf++;
	}

	if (sigsearch)
		findsignal();
	else if (mailserver) {
		if (waitcount > 0) {
			--waitcount;
			if (waitcount == 0) {
				if (progdefaults.PSKmailSweetSpot) {
					frequency = progdefaults.PSKsweetspot;
					set_freq(frequency);
				}
				sigsearch = SIGSEARCH;
			}
		}
		else if ( E1/ E2 <= 1.0) {
			waitcount = 8;
			sigsearch = 0;
		}
	}
	return 0;
}

//=====================================================================
// transmit processes
//=====================================================================

void psk::transmit(double *buf, int len)
{
	if (btn_imd_on->value())
		for (int i = 0; i < len; i++) xmtfilt->Irun(buf[i], buf[i]);

	ModulateXmtr(buf, len);
}

#define SVP_MASK 0xF
#define SVP_COUNT (SVP_MASK + 1)

static cmplx sym_vec_pos[SVP_COUNT] = {
	cmplx (-1.0, 0.0),        // 180 degrees
	cmplx (-0.9238, -0.3826), // 202.5 degrees
	cmplx (-0.7071, -0.7071), // 225 degrees
	cmplx (-0.3826, -0.9238), // 247.5 degrees
	cmplx (0.0, -1.0),        // 270 degrees
	cmplx (0.3826, -0.9238),  // 292.5 degrees
	cmplx (0.7071, -0.7071),  // 315 degrees
	cmplx (0.9238, -0.3826),  // 337.5 degrees
	cmplx (1.0, 0.0),         // 0 degrees
	cmplx (0.9238, 0.3826),   // 22.5 degrees
	cmplx (0.7071, 0.7071),   // 45 degrees
	cmplx (0.3826, 0.9238),   // 67.5 degrees
	cmplx (0.0, 1.0),         // 90 degrees
	cmplx (-0.3826, 0.9238),  // 112.5 degrees
	cmplx (-0.7071, 0.7071),  // 135 degrees
	cmplx (-0.9238, 0.3826)   // 157.5 degrees
};

void psk::tx_carriers()
{
	double delta[MAX_CARRIERS];
	double	ival, qval, shapeA, shapeB;
	cmplx symbol;
	double	frequencies[MAX_CARRIERS];

	//Process all carrier's symbols, then submit to sound card
	accumulated_bits = 0; //reset
	frequencies[0] = get_txfreq_woffset() + ((-1 * numcarriers) + 1) * inter_carrier / 2;
	delta[0] = TWOPI * frequencies[0] / samplerate;
	for (int car = 1; car < symbols; car++) {
		frequencies[car] = frequencies[car - 1] + inter_carrier;
		delta[car] = TWOPI * frequencies[car] / samplerate;
	}

	int sym;
	for (int car = 0; car < symbols; car++) {
		sym = txsymbols[car];

		if (_qpsk && !reverse)
			sym = (4 - sym) & 3;

		if (_8psk && !_disablefec) // Use Gray-mapped 8psk constellation
			symbol = prevsymbol[car] * graymapped_8psk_pos[(sym & 7)];	// complex multiplication

		else { // Map the incoming symbols to the underlying 16psk constellation.
			if (_xpsk) sym = sym * 4 + 2; // Give it the "X" constellation shape
			else if (_8psk) sym *= 2; // Map 8psk to 16psk
			else sym *= 4; // For BPSK and QPSK
			symbol = prevsymbol[car] * sym_vec_pos[(sym & SVP_MASK)];	// complex multiplication
		}


		for (int i = 0; i < symbollen; i++) {

			shapeA = tx_shape[i];
			shapeB = (1.0 - shapeA);

			ival = shapeA * prevsymbol[car].real() + shapeB * symbol.real();
			qval = shapeA * prevsymbol[car].imag() + shapeB * symbol.imag();

			if (car != 0) {
				outbuf[i] += (ival * cos(phaseacc[car]) + qval * sin(phaseacc[car])) / numcarriers;
			} else {
				outbuf[i] = (ival * cos(phaseacc[car]) + qval * sin(phaseacc[car])) / numcarriers;
			}
// create an imd value
			double maxmag = xmtimd->value();
			if (btn_imd_on->value())
				if (fabs(outbuf[i]) > maxmag)
					outbuf[i] = maxmag * (outbuf[i] < 0 ? -1 : 1);

			phaseacc[car] += delta[car];
			if (phaseacc[car] > TWOPI) phaseacc[car] -= TWOPI;
		}

		prevsymbol[car] = symbol;
	}

	if (vestigial && progdefaults.pskpilot) {
		double dvp = TWOPI * (frequencies[0] - sc_bw) / samplerate;
		double amp = pow(10, progdefaults.pilot_power / 20.0) * maxamp;
		for (int i = 0; i < symbollen; i++) {
			outbuf[i] += amp * cos(vphase);
			outbuf[i] /= (1 + amp);
			vphase += dvp;
			if (vphase > TWOPI) vphase -= TWOPI;
		}
	}

	maxamp = 0;
	for (int i = 0; i < symbollen; i++)
		if (maxamp < fabs(outbuf[i])) maxamp = fabs(outbuf[i]);
	maxamp *= 1.02;
	if (maxamp) {
		for (int i = 0; i < symbollen; i++)
			outbuf[i] /= maxamp;
	}

	transmit(outbuf, symbollen);
}

void psk::tx_symbol(int sym)
{
	acc_symbols++;
	txsymbols[symbols] = sym;
	if (++symbols < numcarriers) {
		return;
	}
	tx_carriers();
	symbols = 0; //reset
}

void psk::tx_bit(int bit)
{
	unsigned int sym;
	static int bitcount=0;
	static int xpsk_sym=0;

	// qpsk transmission
	if (_qpsk) {
		sym = enc->encode(bit);
		sym = sym & 3;//JD just to make sure
		tx_symbol(sym);
		// pskr (fec + interleaver) transmission
	} else if (_pskr) {
		// Encode into two bits
		bitshreg = enc->encode(bit);
		// pass through interleaver
		if (mode != MODE_PSK63F) Txinlv->bits(&bitshreg);
		// Send low bit first. tx_symbol expects 0 or 2 for BPSK
		sym = (bitshreg & 1) << 1;
		tx_symbol(sym);
		sym = bitshreg & 2;
		tx_symbol(sym);
	} else if (_16psk || _8psk || _xpsk) {
		if (_disablefec) {
			//Accumulate tx bits until the correct number for symbol-size is reached
			xpsk_sym |= bit << bitcount++ ;
			if (bitcount == symbits) {
				tx_symbol(xpsk_sym);
				xpsk_sym = bitcount = 0;
			}
		} else
			tx_xpsk(bit);
		// else normal bpsk transmission
	} else {
		sym = bit << 1;
		tx_symbol(sym);
	}
}

void psk::tx_xpsk(int bit)
{
	static int bitcount = 0;
	static unsigned int xpsk_sym = 0;
	int fecbits = 0;

	// If invalid value of bitcount, reset to 0
	if (_puncturing || _xpsk || _16psk)
		if ( (bitcount & 0x1) )
			bitcount = 0;

	//printf("\n\n bit: %d", bit);

	// Pass one bit and return two bits
	bitshreg = enc->encode(bit);
	// Interleave
	Txinlv->bits(&bitshreg); // Bit-interleave
	fecbits = bitshreg;


	/// DEBUG
	/*
	 * 	static bool flip;
	 if (flip) {
	 flip = false;
	 fecbits = 0;
	 } else {
	 flip = true;
	 fecbits = 3;
	 }
	 */

	//printf("\nfecbits: %u", fecbits);
	//printf("\nbitcount: %d", bitcount);

	if (_xpsk) { // 2 bits-per-symbol. Transmit every call
		xpsk_sym = static_cast<unsigned int>(fecbits);
		tx_symbol(xpsk_sym);
		return;
	}

	// DEBUG
	/*
	 if (_8psk) {
	 tx_symbol(0);
	 tx_symbol(7);
	 return;
	 }
	 */

	// DEBUG, send a known pattern of symbols / bits
	/*
	 * 	static int counter = 0;
	 counter++;
	 if ( counter > 7 ) counter = 0;
	 tx_symbol(1);
	 return;
	 */

	/*
	 else if (mode == MODE_8PSK ???) { // Puncture @ 5/6 rate | tx 3bits/symbol (8psk)
	 // Collect up to 8 bits
	 if ( x_bitcount < 8 ) {
	 x_xpsk_sym |= (static_cast<unsigned int>(fecbits) & 1) << x_bitcount ;
	 x_xpsk_sym |= (static_cast<unsigned int>(fecbits) & 2) << x_bitcount ;
	 x_bitcount += 2;
	 return;

	 // When 10 bits are buffered,
	 //	drop 4 bits then transmit 6 bits (2-symbols)
	 } else {
	 x_xpsk_sym |= (static_cast<unsigned int>(fecbits) & 1) << x_bitcount ;
	 x_xpsk_sym |= (static_cast<unsigned int>(fecbits) & 2) << x_bitcount ;
	 tx_symbol( (x_xpsk_sym & 14) >> 1);
	 tx_symbol( (x_xpsk_sym & 448) >> 6);
	 x_xpsk_sym = x_bitcount = 0;
	 return;
	 }
	 }
	 */

	else if (_8psk && _puncturing) { // @ 2/3 rate

		if ( 0 == bitcount) {
			xpsk_sym = static_cast<unsigned int>(fecbits);
			bitcount = 2;
			return;

		} else if ( 2 == bitcount ) {
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 1) << 2 ;
			/// Puncture -> //xpsk_sym |= (static_cast<unsigned int>(fecbits) & 2) << 2 ;
			tx_symbol(xpsk_sym & 7);
			xpsk_sym = bitcount = 0;
			return;
		}
	}


	else if (_8psk) { // 3 bits-per-symbol. Accumulate then tx.

		if ( 0 == bitcount ) { // Empty xpsk_sym buffer: add 2 bits and return
							   //printf("\nxpsk_sym|preadd: %u", xpsk_sym);
			xpsk_sym = static_cast<unsigned int>(fecbits);
			//printf("\nxpsk_sym|postadd: %u", xpsk_sym);
			bitcount = 2;
			return ;

		} else if ( 1 == bitcount ) { // xpsk_sym buffer with one bit: add 2 bits then tx and clear
									  //xpsk_sym <<= 2; // shift left 2 bits

			//printf("\nxpsk_sym|preadd: %u", xpsk_sym);
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 1) << 1 ;
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 2) << 1 ;
			//printf("\nxpsk_sym|postadd: %u", xpsk_sym);

			//printf("\nxpsk_sym|postinlv: %u", xpsk_sym);
			tx_symbol(xpsk_sym);
			xpsk_sym = bitcount = 0;
			return;

		} else if ( 2 == bitcount ) { // xpsk_sym buffer with 2 bits: add 1 then tx and save next bit
									  //printf("\nxpsk_sym|preadd: %u", xpsk_sym);
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 1) << 2 ;
			//printf("\nxpsk_sym|postadd: %u", xpsk_sym);

			//printf("\nxpsk_sym|postinlv: %u", xpsk_sym);

			tx_symbol(xpsk_sym);
			xpsk_sym = bitcount = 0;

			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 2) >> 1 ;
			bitcount = 1;
			return;
		}
	}

	else if (_puncturing && _16psk) { // @ 3/4 Rate

		if ( 0 == bitcount) {
			xpsk_sym = static_cast<unsigned int>(fecbits);
			bitcount = 2;
			return;

		} else if ( 2 == bitcount ) {
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 1) << 2 ;
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 2) << 2 ;
			bitcount = 4;
			return;
		} else if ( 4 == bitcount ) {
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 1) << 4 ;
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 2) << 4 ;
			xpsk_sym >>= 1; // Shift right to drop the lowest bit
			xpsk_sym &= 15; // Drop the highest bit
			tx_symbol(xpsk_sym);
			xpsk_sym = bitcount = 0;
			return;
		}
	}

	else if (_16psk) { // 4 bits-per-symbol. Transmit every-other run.

		if ( 0 == bitcount) {
			xpsk_sym = static_cast<unsigned int>(fecbits);
			bitcount = 2;
			return;

		} else if ( 2 == bitcount ) {
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 1) << 2 ;
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 2) << 2 ;
			//Txinlv->bits(&xpsk_sym);
			tx_symbol(xpsk_sym & 7);
			xpsk_sym = bitcount = 0;
			return;
		}
	}
}


unsigned char ch;
void psk::tx_char(unsigned char c)
{
	ch = c;
	const char *code;
	char_symbols = acc_symbols;
	if (_pskr || _xpsk || _8psk || _16psk) {
		//		acc_symbols = 0;
		// ARQ varicode instead of MFSK for PSK63FEC
		code = varienc(c);
	} else {
		code = psk_varicode_encode(c);
	}
	while (*code) {
		tx_bit((*code - '0'));
		code++;
	}

	// Insert PSK varicode character-delimiting bits
	if (! _pskr && !_xpsk && !_8psk && !_16psk) {
		tx_bit(0);
		tx_bit(0);
	}
	char_symbols = acc_symbols - char_symbols;
}


void psk::tx_flush()
{
	if (_pskr) {
		ovhd_symbols = ((numcarriers - symbols) % numcarriers);
		//VK2ETA replace with a more effective flushing sequence (avoids cutting the last characters in low s/n)
		for (int i = 0; i < ovhd_symbols/2; i++) tx_bit(0);

		for (int i = 0; i < dcdbits / 2; i++) {
			tx_bit(1);
			tx_bit(1);
		}
		// QPSK - flush the encoder
	} else if (_qpsk) {
		for (int i = 0; i < dcdbits; i++)
			tx_bit(0);
		// FEC : replace unmodulated carrier by an encoded sequence of zeros
	} else if (!_disablefec && (_xpsk || _8psk || _16psk) ) {
		for (int i = 0; i < flushlength; i++) {
			tx_char(0);   // <NUL>
		}
		// FEC disabled: use unmodulated carrier
	} else if (_disablefec && ( _xpsk || _8psk || _16psk) ) {
		for (int i=0; i<symbits; i++) {
			tx_char(0); // Send <NUL> to clear bit accumulators on both Tx and Rx ends.
		}

		int symbol;
		if (_16psk) symbol = 8;
		else if (_8psk) symbol = 4;
		else symbol = 2;

		int _dcdbits = dcdbits - 1;
		if(progStatus.psk8DCDShortFlag)
			_dcdbits  = 32/(symbits - 1);

		for (int i = 0; i <= _dcdbits; i++) // DCD window is only 32-bits wide
			tx_symbol(symbol); // 0 degrees
							   // Standard BPSK postamble
							   // DCD off sequence (unmodulated carrier)
	} else {
		for (int i = 0; i < dcdbits; i++)
			tx_symbol(2); // 0 degrees
	}
}

// Necessary to clear the interleaver before we start sending
void psk::clearbits()
{
	bitshreg = 0;
	enc->init();
	Txinlv->flush();
}

int psk::tx_process()
{
	int c;

	// DCD window is only 32 bits, send a maximum of 3-times.
	if(progStatus.psk8DCDShortFlag) {
		if ( (_8psk || _xpsk || _16psk) && preamble > 96)
			preamble = 96;
	}

	if (preamble > 0) {
		if (_pskr || ((_xpsk || _8psk || _16psk) && !_disablefec) ) {
			if (startpreamble == true) {
				if (mode != MODE_PSK63F) clearbits();
				startpreamble = false;
			}
			// FEC prep the encoder with one/zero sequences of bits
			preamble--;
			preamble--;
			tx_bit(1);
			tx_bit(0);
			// FEC: Mark start of first character with a double zero
			// to ensure sync at end of preamble
			if (preamble == 0) {
				while (acc_symbols % numcarriers) tx_bit(0);
				tx_char(0); // <NUL>
			}
			return 0;
		} else {
			//JD for QPSK500R
			//			if (mode == MODE_QPSK500) clearbits();
			// Standard BPSK/QPSK preamble
			preamble--;
			tx_symbol(0);   // send phase reversals
			return 0;
		}
	}

	c = get_tx_char();

	if (c == GET_TX_CHAR_ETX || stopflag) {
		tx_flush();
		stopflag = false;
		cwid();

		char_samples = char_symbols * symbollen / numcarriers;
		xmt_samples  = acc_symbols * symbollen / numcarriers;
		ovhd_samples = (acc_symbols - char_symbols - ovhd_symbols) * symbollen / numcarriers;

		return -1;   // we're done
	}

	if (c == GET_TX_CHAR_NODATA) {
		if (_pskr || _xpsk || _8psk || _16psk) {
			// MFSK varicode instead of psk
			tx_char(0);   // <NUL>
			tx_bit(1);
			// extended zero bit stream
			for (int i = 0; i < 32; i++)
				tx_bit(0);
		} else {
			tx_bit(0);
		}
	} else {
		tx_char(c);
		put_echo_char(c);
	}
	return 0;
}

//============================================================================
// psk signal evaluation
// using Goertzel IIR filter
// derived from pskcore by Moe Wheatley, AE4JY
//============================================================================

void psk::initSN_IMD()
{
	for(int i = 0; i < NUM_FILTERS; i++)
	{
		I1[i] = I2[i] = Q1[i] = Q2[i] = 0.0;
		m_Energy[i] = 0.0;
	}
	m_NCount = 0;

	COEF[0] = 2.0 * cos(TWOPI * 9 / GOERTZEL);
	COEF[1] = 2.0 * cos(TWOPI * 36 / GOERTZEL);
	COEF[2] = 2.0 * cos(TWOPI  * 27 / GOERTZEL);
}

void psk::resetSN_IMD()
{
	for(int i = 0; i < NUM_FILTERS; i++) {
		I1[i] = I2[i] = Q1[i] = Q2[i] = 0.0;
	}
	m_NCount = 0;
}

//============================================================================
//  This routine calculates the energy in the frequency bands of
//   carrier=F0(15.625), noise=F1(31.25), and
//   3rd order product=F2(46.875)
//  It is called with cmplx data samples at 500 Hz.
//============================================================================

void psk::calcSN_IMD(cmplx z)
{
	int i;
	double tempI = 0, tempQ = 0;

	for(i = 0; i < NUM_FILTERS; i++) {
		tempI = I1[i];
		tempQ = Q1[i];
		I1[i] = I1[i] * COEF[i]- I2[i] + z.real();
		Q1[i] = Q1[i] * COEF[i]- Q2[i] + z.imag();
		I2[i] = tempI;
		Q2[i] = tempQ;
	}

	if( ++m_NCount >= GOERTZEL ) {
		m_NCount = 0;
		for(i = 0; i < NUM_FILTERS; i++) {
			m_Energy[i] =   I1[i]*I1[i] + Q1[i]*Q1[i]
			+ I2[i]*I2[i] + Q2[i]*Q2[i]
			- I1[i]*I2[i]*COEF[i]
			- Q1[i]*Q2[i]*COEF[i];
			I1[i] = I2[i] = Q1[i] = Q2[i] = 0.0;
		}
		signalquality();
	}
}
