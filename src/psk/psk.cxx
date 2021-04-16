// ----------------------------------------------------------------------------
// psk.cxx  --  psk modem
//
// Copyright (C) 2006-2021
//		Dave Freese, W1HKJ
// Copyright (C) 2009-2010
//		John Douyere, VK2ETA
// Copyright (C) 2014-2021
//		John Phelps, KL4YFD
//      Modified by Joe Counsil, K0OG - Flushlengths on 8PSKxF Modes
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
#include "test_signal.h"

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

#define	THOR_K15	15
#define	K15_POLY1	044735
#define	K15_POLY2	063057

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

// For Gray-mapped xPSK:
// Even when the received phase is distorted by +- 1 phase-position:
//  - Only up to 1 bit can be in error
static cmplx graymapped_xpsk_pos[] = {
	//				 Degrees  Bits In
	cmplx (0.7071, 0.7071),   // 45  | 0b00
	cmplx (-0.7071, 0.7071),  // 135 | 0b01
	cmplx (0.7071, -0.7071),  // 315 | 0b10
	cmplx (-0.7071, -0.7071)  // 225 | 0b11
};

// Associated soft-symbols to be used with graymapped_xpsk_pos[] constellation
// Gray-coding of xPSK simply makes all probabilites equal.
// Use of this table automatically Gray-decodes incoming symbols.
static unsigned char graymapped_xpsk_softbits[4][2] = {
	{0,0},		// 0
	{0,255},	// 1
	{255,255},	// 3
	{255,0},	// 2
};

// For Gray-mapped 16PSK:
// Even when the received phase is distorted by +- 1 phase-position:
//  - Two of the bits are still known with 100% certianty.
//  - Only up to 1 bit can be in error
static cmplx graymapped_16psk_pos[] = {
	cmplx (1.0, 0.0),         // 0 degrees		0b0000	| 025,000,000,025
	cmplx (0.9238, 0.3826),   // 22.5 degrees	0b0001	| 000,000,025,230
	cmplx (0.3826, 0.9238),   // 67.5 degrees	0b0010	| 000,025,255,025
	cmplx (0.7071, 0.7071),   // 45 degrees		0b0011	| 000,000,230,230
	cmplx (-0.9238, 0.3826),  // 157.5 degrees	0b0100	| 025,255,000,025
	cmplx (-0.7071, 0.7071),  // 135 degrees	0b0101	| 000,255,025,230
	cmplx (0.0, 1.0),         // 90 degrees		0b0110	| 000,230,255,025
	cmplx (-0.3826, 0.9238),  // 112.5 degrees	0b0111	| 000,255,230,230
	cmplx (0.9238, -0.3826),  // 337.5 degrees	0b1000	| 230,000,000,025
	cmplx (0.7071, -0.7071),  // 315 degrees	0b1001	| 255,000,025,230
	cmplx (0.0, -1.0),        // 270 degrees	0b1010	| 255,025,255,025
	cmplx (0.3826, -0.9238),  // 292.5 degrees	0b1011	| 255,000,230,230
	cmplx (-1.0, 0.0),        // 180 degrees	0b1100	| 230,255,000,025
	cmplx (-0.9238, -0.3826), // 202.5 degrees	0b1101	| 255,255,025,230
	cmplx (-0.3826, -0.9238), // 247.5 degrees	0b1110	| 255,230,255,000
	cmplx (-0.7071, -0.7071)  // 225 degrees	0b1111	| 255,255,025,025
};

// Associated soft-symbols to be used with graymapped_16psk_pos[] constellation
// These softbits have precalculated (a-priori) probabilities applied
// Use of this table automatically Gray-decodes incoming symbols.
static unsigned char graymapped_16psk_softbits[16][4] = {
	{025,000,000,025}, // 0
	{000,000,025,230}, // 1
	{000,000,230,230}, // 3
	{000,025,255,025}, // 2
	{000,230,255,025}, // 6
	{000,255,230,230}, // 7
	{000,255,025,230}, // 5
	{025,255,000,025}, // 4
	{230,255,000,025}, // 12
	{255,255,025,230}, // 13
	{255,255,025,025}, // 15
	{255,230,255,000}, // 14
	{255,025,255,025}, // 10
	{255,000,230,230}, // 11
	{255,000,025,230}, // 9
	{230,000,000,025}  // 8
};



char pskmsg[80];

void psk::tx_init()
{
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

	if (mode == MODE_OFDM_500F || mode == MODE_OFDM_750F || mode == MODE_OFDM_2000F) {
		enc->init();
		Txinlv->flush();
	}

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

	for (int i = 0; i < NUM_FILTERS; i++) {
		re_Gbin[i]->reset();
		im_Gbin[i]->reset();
	}
}

bool psk::viewer_mode()
{
	if (mode == MODE_PSK31 || mode == MODE_PSK63 || mode == MODE_PSK63F ||
		mode == MODE_PSK125 || mode == MODE_PSK250 || mode == MODE_PSK500 ||
		mode == MODE_PSK125R || mode == MODE_PSK250R || mode == MODE_PSK500R ||
		mode == MODE_QPSK31 || mode == MODE_QPSK63 || mode == MODE_QPSK125 ||
		mode == MODE_QPSK250 || mode == MODE_QPSK500 )
		return true;
	return false;
}

void psk::restart()
{
	if (viewer_mode())
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

	set_freqlock(false); // re-lock modems after setting center-frequency.

	if (mode == MODE_OFDM_2000F || mode == MODE_OFDM_2000)
		set_freq(1325);
	else if (mode == MODE_OFDM_3500)
		set_freq(2250);
	else if (mode == MODE_OFDM_500F || mode == MODE_OFDM_750F)
		set_freq(1500);
	else if (progdefaults.StartAtSweetSpot)
		set_freq(progdefaults.PSKsweetspot);
	else if (progStatus.carrier != 0) {
		set_freq(progStatus.carrier);
#if !BENCHMARK_MODE
		progStatus.carrier = 0;
#endif
	} else
		set_freq(wf->Carrier());

	if (mode == MODE_OFDM_2000 || mode == MODE_OFDM_2000F || mode == MODE_OFDM_3500)
		set_freqlock(true);

}

psk::~psk()
{
	if (tx_shape) delete [] tx_shape;
	if (imd_shape) delete [] imd_shape;
	if (enc) delete enc;
	if (dec) delete dec;
	// FEC 2nd Viterbi decoder
	if (dec2) delete dec2;

	for (int i = 0; i < MAX_CARRIERS; i++) {
		if (fir1[i]) delete fir1[i];
		if (fir2[i]) delete fir2[i];
	}

	if (e0_filt) delete e0_filt;
	if (e1_filt) delete e1_filt;
	if (e2_filt) delete e2_filt;
	if (e3_filt) delete e3_filt;

	for (int i = 0; i < NUM_FILTERS; i++) {
		delete re_Gbin[i];
		delete im_Gbin[i];
	}

	if (pskviewer) delete pskviewer;
	if (evalpsk) delete evalpsk;

	// Interleaver
	if (Rxinlv) delete Rxinlv;
	if (Rxinlv2) delete Rxinlv2;
	if (Txinlv) delete Txinlv;

	if (vestigial_sfft) delete vestigial_sfft;
	set_freqlock(false);
}

psk::psk(trx_mode pskmode) : modem()
{
enum FIR_TYPE {PSK_CORE, GMFSK, SINC};
FIR_TYPE fir_type = PSK_CORE;

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
			fir_type = PSK_CORE;
			break;
		case MODE_PSK63:
			symbollen = 128;
			dcdbits = 64;
			fir_type = PSK_CORE;
			break;
		case MODE_PSK125:
			symbollen = 64;
			dcdbits = 128;
			fir_type = SINC;
			break;
		case MODE_PSK250:
			symbollen = 32;
			dcdbits = 256;
			fir_type = SINC;
			break;
		case MODE_PSK500:
			symbollen = 16;
			dcdbits = 512;
			fir_type = SINC;
			break;
		case MODE_PSK1000:
			symbollen = 8;
			dcdbits = 128;
			fir_type = SINC;
			break;

		case MODE_QPSK31:
			symbollen = 256;
			_qpsk = true;
			dcdbits = 32;
			cap |= CAP_REV;
			fir_type = PSK_CORE;
			break;
		case MODE_QPSK63:
			symbollen = 128;
			_qpsk = true;
			dcdbits = 64;
			cap |= CAP_REV;
			fir_type = PSK_CORE;
			break;
		case MODE_QPSK125:
			symbollen = 64;
			_qpsk = true;
			dcdbits = 128;
			cap |= CAP_REV;
			fir_type = SINC;
			break;
		case MODE_QPSK250:
			symbollen = 32;
			_qpsk = true;
			dcdbits = 256;
			cap |= CAP_REV;
			fir_type = SINC;
			break;
		case MODE_QPSK500:
			symbollen = 16;
			_qpsk = true;
			dcdbits = 512;
			cap |= CAP_REV;
			fir_type = SINC;
			break;
		case MODE_PSK63F:  // As per Multipsk (BPSK63 + FEC + MFSK Varicode)
			symbollen = 128;
			_pskr = true;
			dcdbits = 64;
			fir_type = PSK_CORE;
			break;

	// OFDM modes
		case MODE_OFDM_500F: // 62.5 baud xPSK | 4 carriers | 250 bits/sec @ 1/2 FEC
			symbollen = 256;
			samplerate = 16000;
			_xpsk = true;
			_disablefec = false;
			_puncturing = false;
			numcarriers = 4;
			separation = 2.0f;
			idepth = 2000; // 4000 milliseconds
			flushlength = 200;
			dcdbits = 192;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;

		case MODE_OFDM_750F: // 125 baud 8PSK | 3 carriers | 562 bits/sec @ 1/2 FEC
			symbollen = 128;
			samplerate = 16000;
			_8psk = true;
			_disablefec = false;
			_puncturing = false;
			numcarriers = 3;
			separation = 2.0f;
			idepth = 3600; // 3200 milliseconds
			flushlength = 360;
			dcdbits = 384;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;

		case MODE_OFDM_2000F: // 125 baud 8PSK | 8 carriers | 2000 bits/sec @ 2/3 FEC
			symbollen = 128;
			samplerate = 16000;
			_8psk = true;
			_disablefec = false;
			_puncturing = true;
			numcarriers = 8;
			separation = 2.0f;
			idepth = 4800; // 1600 milliseconds
			flushlength = 480;
			dcdbits = 1536;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;

		case MODE_OFDM_2000: // 250 baud 8PSK | 4 carriers | 3000 bits/sec NO FEC
			symbollen = 64;
			samplerate = 16000;
			_8psk = true;
			_disablefec = true;
			numcarriers = 4;
			separation = 2.0f;
			dcdbits = 1536;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;

		case MODE_OFDM_3500: // 250 baud 8PSK | 7 carriers | 5250 bits/sec NO FEC
			symbollen = 64;
			samplerate = 16000;
			_8psk = true;
			_disablefec = true;
			numcarriers = 7;
			separation = 2.0f;
			dcdbits = 1536;
			vestigial = false;
			cap |= CAP_REV;
			fir_type = SINC;
			break;
	// End OFDM modes

	// 8psk modes without FEC
		case MODE_8PSK125: // 125 baud | 375 bits/sec No FEC
			symbollen = 128;
			samplerate = 16000;
			_8psk = true;
			_disablefec = true;
			dcdbits = 128;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;
		case MODE_8PSK250: // 250 baud | 750 bits/sec No FEC
			symbollen = 64;
			samplerate = 16000;
			_8psk = true;
			_disablefec = true;
			dcdbits = 256;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;
		case MODE_8PSK500: // 500 baud | 1500 bits/sec No FEC
			symbollen = 32;
			samplerate = 16000;
			_8psk = true;
			_disablefec = true;
			dcdbits = 512;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;
		case MODE_8PSK1000: // 1000 baud | 3000 bits/sec  No FEC
			symbollen = 16;
			samplerate = 16000;
			_8psk = true;
			_disablefec = true;
			dcdbits = 1024;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;

	// 8psk modes with FEC
		case MODE_8PSK125FL: // 125 baud | 187 bits/sec @ 1/2 Rate K=13 FEC
			symbollen = 128;
			idepth = 384; // 2048 milliseconds
			flushlength = 55;
			samplerate = 16000;
			_8psk = true;
			dcdbits = 128;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;
		case MODE_8PSK250FL: // 250 baud | 375 bits/sec @ 1/2 Rate K=13 FEC
			symbollen = 64;
			idepth = 512; // 1365 milliseconds
			flushlength = 65;
			samplerate = 16000;
			_8psk = true;
			dcdbits = 256;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;
		case MODE_8PSK125F: // 125 baud | 187 bits/sec @ 1/2 Rate K=16 FEC
			symbollen = 128;
			idepth = 384; // 2048 milliseconds
			flushlength = 55;
			samplerate = 16000;
			_8psk = true;
			dcdbits = 128;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;
		case MODE_8PSK250F: // 250 baud | 375 bits/sec @ 1/2 Rate K=16 FEC
			symbollen = 64;
			idepth = 512; // 1365 milliseconds
			flushlength = 65;
			samplerate = 16000;
			_8psk = true;
			dcdbits = 256;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;
		case MODE_8PSK500F: // 500 baud | 1000 bits/sec @ 2/3 rate K=13 FEC
			symbollen = 32;
			idepth = 640; // 426 milliseconds
			flushlength = 80;
			samplerate = 16000;
			_8psk = true;
			_puncturing = true;
			dcdbits = 512;
			vestigial = true;
			cap |= CAP_REV;
			fir_type = SINC;
			break;
		case MODE_8PSK1000F: // 1000 baud | 2000 bits/sec @ 2/3 rate K=7 FEC
			symbollen = 16;
			idepth = 512; // 170 milliseconds
			flushlength = 120;
			samplerate = 16000;
			_8psk = true;
			dcdbits = 1024;
			cap |= CAP_REV;
			_puncturing = true;
			vestigial = true;
			PSKviterbi = true;
			fir_type = SINC;
			break;
		case MODE_8PSK1200F: // 1200 baud | 2400 bits/sec @ 2/3 rate K=7 FEC
			symbollen = 13;
			idepth = 512; // 142 milliseconds
			flushlength = 175;
			samplerate = 16000;
			_8psk = true;
			_puncturing = true;
			dcdbits = 2048;
			cap |= CAP_REV;
			vestigial = true;
			PSKviterbi = true;
			fir_type = SINC;
			break;
			// end 8psk modes

		case MODE_PSK125R:
			symbollen = 64;
			_pskr = true;
			dcdbits = 128;
			idepth = 40;  // 2x2x40 interleaver
			fir_type = SINC;
			break;
		case MODE_PSK250R:
			symbollen = 32;
			_pskr = true;
			dcdbits = 256;
			idepth = 80;  // 2x2x80 interleaver
			fir_type = SINC;
			break;
		case MODE_PSK500R:
			symbollen = 16;
			_pskr = true;
			dcdbits = 512;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;
		case MODE_PSK1000R:
			symbollen = 8;
			_pskr = true;
			dcdbits = 512;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;

			// multi-carrier modems
		case MODE_4X_PSK63R:
			symbollen = 128;//PSK63
			dcdbits = 128;
			_pskr = true;//PSKR
			numcarriers = 4;
			idepth = 80; // 2x2x80 interleaver
			fir_type = SINC;
			break;
		case MODE_5X_PSK63R:
			symbollen = 128; //PSK63
			dcdbits = 512;
			_pskr = true; //PSKR
			numcarriers = 5;
			idepth = 260; // 2x2x160 interleaver
			fir_type = SINC;
			break;
		case MODE_10X_PSK63R:
			symbollen = 128; //PSK63
			dcdbits = 512;
			_pskr = true; //PSKR
			numcarriers = 10;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;
		case MODE_20X_PSK63R:
			symbollen = 128; //PSK63
			dcdbits = 512;
			_pskr = true; //PSKR
			numcarriers = 20;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;
		case MODE_32X_PSK63R:
			symbollen = 128; //PSK63
			dcdbits = 512;
			_pskr = true; //PSKR
			numcarriers = 32;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;

		case MODE_4X_PSK125R:
			symbollen = 64;//PSK125
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 4;
			idepth = 80; // 2x2x80 interleaver
			fir_type = SINC;
			break;
		case MODE_5X_PSK125R:
			symbollen = 64;//PSK125
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 5;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;
		case MODE_10X_PSK125R:
			symbollen = 64;//PSK125
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 10;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;

		case MODE_12X_PSK125:
			symbollen = 64;//PSK125
			dcdbits = 128;//512;
			numcarriers = 12;
			fir_type = SINC;
			break;
		case MODE_12X_PSK125R:
			symbollen = 64;//PSK125
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 12;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;

		case MODE_16X_PSK125R:
			symbollen = 64;//PSK125
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 16;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;

		case MODE_2X_PSK250R:
			symbollen = 32;//PSK250
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 2;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;
		case MODE_3X_PSK250R:
			symbollen = 32;//PSK250
			dcdbits = 512;
			_pskr = true;//PSKR
			numcarriers = 3;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;
		case MODE_5X_PSK250R:
			symbollen = 32;//PSK250
			_pskr = true;//PSKR
			dcdbits = 1024;
			numcarriers = 5;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;
		case MODE_6X_PSK250:
			symbollen = 32;//PSK250
			dcdbits = 512;
			numcarriers = 6;
			fir_type = SINC;
			break;
		case MODE_6X_PSK250R:
			symbollen = 32;//PSK250
			_pskr = true;//PSKR
			dcdbits = 1024;
			numcarriers = 6;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;
		case MODE_7X_PSK250R:
			symbollen = 32;//PSK250
			_pskr = true;//PSKR
			dcdbits = 1024;
			numcarriers = 7;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;

		case MODE_2X_PSK500:
			symbollen = 16;
			dcdbits = 512;
			numcarriers = 2;
			fir_type = SINC;
			break;
		case MODE_4X_PSK500:
			symbollen = 16;
			dcdbits = 512;
			numcarriers = 4;
			fir_type = SINC;
			break;

		case MODE_2X_PSK500R:
			symbollen = 16;
			_pskr = true;
			dcdbits = 1024;
			idepth = 160; // 2x2x160 interleaver
			numcarriers = 2;
			fir_type = SINC;
			break;
		case MODE_3X_PSK500R:
			symbollen = 16;
			_pskr = true;
			dcdbits = 1024;
			idepth = 160; // 2x2x160 interleaver
			numcarriers = 3;
			fir_type = SINC;
			break;
		case MODE_4X_PSK500R:
			symbollen = 16;
			_pskr = true;
			dcdbits = 1024;
			idepth = 160; // 2x2x160 interleaver
			numcarriers = 4;
			fir_type = SINC;
			break;

		case MODE_2X_PSK800:
			symbollen = 10;
			_pskr = false;
			dcdbits = 512;
			numcarriers = 2;
			fir_type = SINC;
			break;
		case MODE_2X_PSK800R:
			symbollen = 10;
			_pskr = true;
			dcdbits = 1024;
			idepth = 160; // 2x2x160 interleaver
			numcarriers = 2;
			fir_type = SINC;
			break;

		case MODE_2X_PSK1000:
			symbollen = 8;//PSK1000
			dcdbits = 1024;
			numcarriers = 2;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;
		case MODE_2X_PSK1000R:
			symbollen = 8;//PSK1000
			_pskr = true;//PSKR
			dcdbits = 1024;
			numcarriers = 2;
			idepth = 160; // 2x2x160 interleaver
			fir_type = SINC;
			break;

		default:
			mode = MODE_PSK31;
			symbollen = 256;
			dcdbits = 32;
			numcarriers = 1;
			fir_type = PSK_CORE;
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
	double fir1c[FIRLEN+1];
	double fir2c[FIRLEN+1];

	for (int i = 0; i < MAX_CARRIERS; i++) {
		if (i < numcarriers) {
			fir1[i] = new C_FIR_filter();
			fir2[i] = new C_FIR_filter();
		} else {
			fir1[i] = (C_FIR_filter *)0;
			fir2[i] = (C_FIR_filter *)0;
		}
	}

	switch (fir_type) {
		case PSK_CORE: // PSKcore filter
			raisedcosfilt(fir1c, FIRLEN);
			for (int i = 0; i <= FIRLEN; i++)
				fir2c[i] = pskcore_filter[i];
			for (int i = 0; i < numcarriers; i++) {
				fir1[i]->init(FIRLEN+1, symbollen > 15 ? symbollen / 16 : 1, fir1c, fir1c);
				fir2[i]->init(FIRLEN+1, 1, fir2c, fir2c);
			}
			break;
		default:
		case SINC: // fir1c & fir2c matched sin(x)/x filter w blackman
			wsincfilt(fir1c, 1.0 / symbollen, FIRLEN);
			wsincfilt(fir2c, 1.0 / 16.0, FIRLEN);
			for (int i = 0; i < numcarriers; i++) {
				fir1[i]->init(FIRLEN, symbollen > 15 ? symbollen / 16 : 1, fir1c, fir1c);
				fir2[i]->init(FIRLEN, 1, fir2c, fir2c);
			}
			break;
	}

	e0_filt = new Cmovavg(dcdbits / 2);
	e1_filt = new Cmovavg(dcdbits / 2);
	e2_filt = new Cmovavg(dcdbits / 2);
	e3_filt = new Cmovavg(dcdbits / 2);

	re_Gbin[0] = new goertzel(160, 0,      500.0);	// base
	re_Gbin[1] = new goertzel(160, 15.625, 500.0);	// fundamental
	re_Gbin[2] = new goertzel(160, 62.5,   500.0);	// 4th harmonic (noise)
	re_Gbin[3] = new goertzel(160, 46.875, 500.0);	// 3rd harmonic (imd)

	im_Gbin[0] = new goertzel(160, 0,      500.0);	// base
	im_Gbin[1] = new goertzel(160, 15.625, 500.0);	// fundamental
	im_Gbin[2] = new goertzel(160, 62.5,   500.0);	// 4th harmonic (noise)
	im_Gbin[3] = new goertzel(160, 46.875, 500.0);	// 3rd harmonic (imd)


	// If no FEC used, these just stay NULL
	enc = NULL;
	dec = dec2 = NULL;

	if (_qpsk) {
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
		dec2 = new viterbi(K16, K16_POLY1, K16_POLY2);
		dec2->setchunksize(4);

	} else if (mode == MODE_OFDM_500F) {
		enc = new encoder(THOR_K15, K15_POLY1, K15_POLY2);
		dec = new viterbi(THOR_K15, K15_POLY1, K15_POLY2);
		dec->setchunksize(4);
		dec->settraceback(THOR_K15 * 10);
		
	} else if (mode == MODE_OFDM_2000F) {
		enc = new encoder(K11, K11_POLY1, K11_POLY2);
		dec = new viterbi(K11, K11_POLY1, K11_POLY2);
		dec->setchunksize(4);
		dec->settraceback(K11 * 14); // OFDM-2000F is a punctured code

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
		} else {
			dec->settraceback(K13 * 10);
			if (dec2) dec2->settraceback(K13 * 10);
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

	tx_shape = new double[symbollen];
	imd_shape = new double[symbollen];

	// raised cosine shape for the transmitter
	double sym_ph = 0;
	for ( int i = 0; i < symbollen; i++) {
		sym_ph = i * M_PI / symbollen;
		tx_shape[i] = 0.5 * cos(sym_ph) + 0.5;
		imd_shape[i] = 0.5 * ( cos(3.0 * sym_ph) +
							   (3.0/5.0) * cos(5.0 * sym_ph) +
							   (3.0/7.0) * cos(7.0 * sym_ph) +
							   (3.0/9.0) * cos(9.0 * sym_ph) );
	}

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
//	E1 = E2 = E3 = 0.0;
	acquire = 0;

	evalpsk = new pskeval;
	if (viewer_mode())
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

	if (mode == MODE_OFDM_2000 || mode == MODE_OFDM_2000F) {
		set_freqlock(false);
		set_freq(1325);
		set_freqlock(true);
	} else if (mode == MODE_OFDM_3500) {
		set_freqlock(false);
		set_freq(2250);
		set_freqlock(true);
	}
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
			// Voting at the character level
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
					if (abs(ftest - progdefaults.ServerCarrier) < progdefaults.ServerOffset) {
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
			resetSN_IMD();
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

	if (!vestigial) return;
	if (!vestigial_sfft->is_stable()) return;

	double avg = 0;
	double max = 0;
	int i = -1;
	static int previous1 = -1;
	static int previous2 = -2;
    
	for (i = 0; i < 11; i++) avg += abs(sfft_bins[i]);
	avg /= 11.0;
	
	// No real signal present: ignore AFC, reset, and return
	if (avg == 0.0f) {
		vestigial_sfft->reset();
		return;
	}
	
	std::setprecision(2); std::setw(5);
	for (int k = 0; k < 11; k++) {
		if (abs(sfft_bins[k]) > max) {
			max = abs(sfft_bins[k]);
			i = k;
		}
	}
	
	// Validity-check the AFC: must see same tone twice in a row,
	// and previous tones must be within 1Hz of each other.
	// Operates with 1hz/sec drift-rates
	if ( i != previous1 || abs(previous1-previous2) > 1 ) {
		previous2 = previous1;
		previous1 = i;
		vestigial_sfft->reset();
		return;
	}
	
    if (i < 11 && i > -1) {
		if (i != 5) {
			frequency -= 1.0*(i-5)*samplerate/sfft_size;
			set_freq (frequency);
		}
	}
	
	previous2 = previous1;
	previous1 = i;
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
	if (mode >= MODE_OFDM_500F && mode <= MODE_OFDM_2000)
		return vestigial_afc();
	else if (!progStatus.afconoff)
		return;
	else if (dcd == true || acquire)
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


	// Only update phase_quality once every dcdbits/4 function-calls
	static int counter=0;
	if (counter++ > dcdbits/4) {
		counter = 0;

		// Calculate how far the phase/constellation is off from perfect, on a scale of 0-100
		double PhaseOffset = (phase * 180.0 / M_PI) / (360.0/n);
		PhaseOffset -= (int)PhaseOffset; 	// cutoff integer, leave just the decimal part

		if (_xpsk) PhaseOffset += 0.5; // xPSK constellation is offset by 45degrees. adjust calculations to compensate

		if (PhaseOffset > 0.5) 				// fix the wraparound issue
			PhaseOffset = 1.0 - PhaseOffset;

		int phase_quality= (int)(100 - PhaseOffset * 200); // Save the phase_quality to a global for later display

		// Adjust the phase-error for non 8psk modes
		if (n == 2) 		// bpsk, pskr
			phase_quality *= 1.25;

		if (phase_quality > 100) phase_quality = 100;
		else if (phase_quality < 0) phase_quality = 0;

		update_quality(phase_quality);

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

	int set_dcd = -1; // 1 sets DCD on ; 0 sets DCD off ; -1 does neither (no-op)
	static int dcdOFFcounter=0; // to prevent a data loss bug... only set DCD-off when correct shreg bitpattern seen multiple times.

	switch (dcdshreg) {

		// bpsk DCD ON
		case 0xAAAAAAAA:
			if (_xpsk || _8psk || _16psk) break;
			if (_pskr) break;
			set_dcd = 1;
			break;

			// pskr DCD ON
			// the pskr FEC pipeline is flushed with an alternating 1/0 pattern, giving 4 possible DCD-ON sequences.
		case 0x0A0A0A0A:
		case 0x28282828:
		case 0xA0A0A0A0:
		case 0x82828282:
			if (!_pskr) break;
			set_dcd = 1;
			break;

			// xpsk DCD OFF
		case 0x92492492:
			if (_qpsk) break; // the QPSK preamble and postamble are identical... Since cant differentiate, QPSK modes do not use DCD
			if (!_xpsk) break;
			if (!_disablefec) break; // xPSK with FEC-enabled does not use DCD-OFF
			set_dcd = 0;
			break;

			// 8psk DCD OFF
		case 0x44444444:
			if (!_8psk) break;
			if (!_disablefec) break; // 8psk with FEC-enabled does not use DCD-OFF
			set_dcd = 0;
			break;

			// 16psk DCD OFF
		case 0x10842108:
			if (!_16psk) break;
			if (!_disablefec) break; // 16psk with FEC-enabled does not use DCD-OFF
			set_dcd = 0;
			break;

			// bpsk DCD OFF
			// 8psk & xpsk DCD ON
		case 0x00000000:
			if (_pskr) break; // pskr does not use DCD-OFF
			if (_16psk) break; // TODO: 16psk dcd on/off unimplemented
			if (_8psk || _xpsk) {
				set_dcd = 1;
				break;
			}
			set_dcd = 0;
			break;

		default:
			if (metric > progStatus.sldrSquelchValue || progStatus.sqlonoff == false) {
				dcd = true;
			// FIXME BUG OFDM FEC modes have NO DCD OFF yet. TODO KL4YFD MAR2021
			} else if (mode != MODE_OFDM_500F && mode != MODE_OFDM_750F && mode != MODE_OFDM_2000F) {
				dcd = false;
			}
			dcdOFFcounter -= 1; // If no DCD-off sequence seen in bitshreg, then subtract 1 from counter (to prevent a accumulative-triggering bug)
			if (dcdOFFcounter < 0) dcdOFFcounter = 0; // prevent wraparound to negative
			break;
	}
	//printf("\n%08x", dcdshreg);

	// Set DCD to ON
	if ( 1 == set_dcd ) {
		dcdOFFcounter = 0;
		dcd = true;
		acquire = 0;
		quality = cmplx (1.0, 0.0);
		if (progdefaults.Pskmails2nreport && (mailserver || mailclient))
			s2n_sum = s2n_sum2 = s2n_ncount = 0.0;
		//printf("\t DCD ON!!");

	// Set DCD to OFF only if seen 6 bit-shifts in a row. (prevent false-triggers and data loss mid-stream)
	} else if ( 0 == set_dcd ) {
		if (++dcdOFFcounter > 5) {
			dcdOFFcounter = 0;
			dcd = false;
			acquire = 0;
			quality = cmplx (0.0, 0.0);
			//printf("\t DCD OFF!!!!!!!!!");
		}
	}

	if (_pskr) {
		rx_pskr(softbit);
		set_phase(phase, norm(quality), dcd);

	} else if (dcd == true) {

		set_phase(phase, norm(quality), dcd);

		if (!_disablefec && (_16psk || _8psk || _xpsk) ) {
			int bitmask = 1;
			unsigned char xsoftsymbols[symbits];

			if (_puncturing && _16psk) // 16psk: recover 3/4-rate punctured low-bit
				rx_pskr(128);

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

			} else if (_xpsk) {
				bool softpuncture = false;
				static double lastphasequality=0;
				double phasequality = fabs(cos( n/2 * phase));
				phasequality = (phasequality + lastphasequality) / 2; // Differential modem: average probabilities between current and previous symbols
				lastphasequality = phasequality;
				int soft_qualityerror = static_cast<int>(128 * phasequality);

				if (soft_qualityerror > 255-12) // Prevent soft-bit wrap-around (crossing of value 128)
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
						if (graymapped_xpsk_softbits[bitindex][i] > 128) // soft-one
							rx_pskr(graymapped_xpsk_softbits[bitindex][i] - soft_qualityerror); // Feed to the PSKR FEC decoder, one bit at a time.
						else // soft-zero
							rx_pskr(graymapped_xpsk_softbits[bitindex][i] + soft_qualityerror); // Feed to the PSKR FEC decoder, one bit at a time.
					}
				}

			} else if (_16psk) {
				bool softpuncture = false;
				static double lastphasequality=0;
				double phasequality = fabs(cos( n/2 * phase));
				phasequality = (phasequality + lastphasequality) / 2; // Differential modem: average probabilities between current and previous symbols
				lastphasequality = phasequality;
				int soft_qualityerror = static_cast<int>(128 * phasequality);

				if (soft_qualityerror > 255-25) // Prevent soft-bit wrap-around (crossing of value 128)
					softpuncture = true;
				else if (soft_qualityerror < 128/3) // First 1/3 of phase delta is considered a perfect signal
					soft_qualityerror = 0;
				else if (soft_qualityerror > 128 - (128/14) ) // Last 1/14 of phase delta triggers a puncture
					softpuncture = true;
				else
					soft_qualityerror /= 2; // Scale the FEC error to prevent premature cutoff

				if (softpuncture) {
					for(int i=0; i<symbits; i++) rx_pskr(128);
				} else {
					int bitindex = static_cast<int>(bits);
					for(int i=symbits-1; i>=0; i--) { // Use predefined Gray-mapped softbits for soft-decoding
						if (graymapped_16psk_softbits[bitindex][i] > 128) // soft-one
							rx_pskr(graymapped_16psk_softbits[bitindex][i] - soft_qualityerror); // Feed to the PSKR FEC decoder, one bit at a time.
						else // soft-zero
							rx_pskr(graymapped_16psk_softbits[bitindex][i] + soft_qualityerror); // Feed to the PSKR FEC decoder, one bit at a time.
						}
					}

			} else {
				//Hard Decode Section
				for(int i=0; i<symbits; i++) { // Hard decode symbits into soft-symbols
					xsoftsymbols[i] = (bits & bitmask) ? 255 : 0 ;
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

static double e0, e1, e2, e3;

void psk::signalquality()
{
	double r0 = re_Gbin[0]->mag();
	double r1 = re_Gbin[1]->mag();
	double r2 = re_Gbin[2]->mag();
	double r3 = re_Gbin[3]->mag();

	double i0 = im_Gbin[0]->mag();
	double i1 = im_Gbin[1]->mag();
	double i2 = im_Gbin[2]->mag();
	double i3 = im_Gbin[3]->mag();

	r0 = sqrtf(r0*r0 + i0*i0);
	r1 = sqrtf(r1*r1 + i1*i1);
	r2 = sqrtf(r2*r2 + i2*i2);
	r3 = sqrtf(r3*r3 + i3*i3);

	e0 = e0_filt->run(r0);
	e1 = e1_filt->run(r1);
	e2 = e2_filt->run(r2);
	e3 = e3_filt->run(r3);

	if (e1 > e0) {
		if ((e1 > 2 * e2) && (e2 > 0)) {
			snratio = e1 / e2;
			if (snratio < 1.0)
				snratio = 1.0;
		} else
			snratio = 1.0;
	} else {
		if ((e0 > 2 * e2) && (e2 > 0)) {
			snratio = e0 / e2;
			if (snratio < 1.0) snratio = 1.0;
		} else
			snratio = 1.0;
	}

	if ( (e1 > 2 * e3) && (e3 > 2 * e2) ) {
		imdratio = e3 / e1;
		if (imdratio < (1.0 /snratio)) imdratio = 1.0 / snratio;
	} else
		imdratio = 1.0 / snratio ;

	displaysn = false;
	if (snratio > 4)
		displaysn = true;

	if (r0 > r1) {
		if ((r0 / r2 < 0.1 * snratio ) || (r0 / r2 < 2.0)) {
			displaysn = false;
		}
	} else {
		if ((r1 / r2 < 0.1 * snratio ) || (r1 / r2 < 2.0)) {
			displaysn = false;
		}
	}

}

void psk::update_syncscope()
{
	static char msg1[16];
	static char msg2[16];

	display_metric(metric);

	if (displaysn && mode == MODE_PSK31) {
		memset(msg1, 0, sizeof(msg1));
		memset(msg2, 0, sizeof(msg2));

		s2n = 10.0*log10( snratio );
		if (s2n < 6)
			strcpy(msg1, "S/N ---");
		else
			snprintf(msg1, sizeof(msg1), "S/N %2.0f dB", s2n);

		put_Status1(
			msg1,
			progdefaults.StatusTimeout,
			progdefaults.StatusDim ? STATUS_DIM : STATUS_CLEAR);

		imd = 10.0*log10( imdratio );

		if (imd > -10)
			strcpy(msg2, "IMD ---");
		else
			snprintf(msg2, sizeof(msg2), "IMD %2.0f dB", imd);

		put_Status2(
			msg2,
			progdefaults.StatusTimeout,
			progdefaults.StatusDim ? STATUS_DIM : STATUS_CLEAR);



	} else if (_xpsk || _8psk || _16psk) {
		// Only update the GUI once every dcdbits/4 function calls
		// to prevent high CPU load from high-baudrate modes
		static int counter=0;
		if (counter++ < dcdbits/4)
			return;
		else counter = 0;

		memset(msg1, 0, sizeof(msg1));
		memset(msg2, 0, sizeof(msg2));

		s2n = 10.0*log10( snratio );
		if (s2n < 0)
			strcpy(msg1, "S/N ---");
		else
			snprintf(msg1, sizeof(msg1), "S/N %2.0f dB", s2n);

		put_Status1(
			msg1,
			progdefaults.StatusTimeout,
			progdefaults.StatusDim ? STATUS_DIM : STATUS_CLEAR);

		// Display the phase-error of the
		snprintf(msg2, sizeof(msg2), "Phase: %d%%", get_quality() );

		put_Status2(
			msg2,
			progdefaults.StatusTimeout,
			progdefaults.StatusDim ? STATUS_DIM : STATUS_CLEAR);

	} else if (displaysn) {

		// Only update the GUI once every dcdbits/4 function calls
		// to prevent high CPU load from high-baudrate modes
		static int counter=0;
		if (counter++ < dcdbits/4)
			return;
		else counter = 0;

		memset(msg1, 0, sizeof(msg1));
		memset(msg2, 0, sizeof(msg2));

		s2n = 10.0*log10( snratio );
		if (s2n < 0)
			strcpy(msg1, "S/N ---");
		else
			snprintf(msg1, sizeof(msg1), "S/N %2.0f dB", s2n);

		put_Status1(
			msg1,
			progdefaults.StatusTimeout,
			progdefaults.StatusDim ? STATUS_DIM : STATUS_CLEAR);

		// Display the phase-error of the
		snprintf(msg2, sizeof(msg2), "Phase: %d%%", get_quality() );

		put_Status2(
			msg2,
			progdefaults.StatusTimeout,
			progdefaults.StatusDim ? STATUS_DIM : STATUS_CLEAR);
	}

}

char bitstatus[100];

int psk::rx_process(const double *buf, int len)
{
	double delta[MAX_CARRIERS], frequencies[MAX_CARRIERS];
	cmplx z, z2[MAX_CARRIERS];
	bool can_rx_symbol = false;

	if (viewer_mode()) {
		if (!progdefaults.report_when_visible ||
			dlgViewer->visible() || progStatus.show_channels )
			if (pskviewer && !bHistory)
				pskviewer->rx_process(buf, len);
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

			if (numcarriers == 1 && vestigial && progdefaults.pskpilot) {
				vestigial_sfft->run(z, sfft_bins, 1);
			} 
			else if (numcarriers > 1 && vestigial && car == 0) {
				vestigial_sfft->run(z, sfft_bins, 1);
			}

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
		else if ( snratio <= 1.0) {
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
	ModulateXmtr(buf, len);
}

#define SVP_MASK 0xF
#define SVP_COUNT (SVP_MASK + 1)

static cmplx sym_vec_pos[SVP_COUNT] = {
	cmplx (-1.0, 0.0),				// 180 degrees
	cmplx (-0.9238, -0.3826),		// 202.5 degrees
	cmplx (-0.7071, -0.7071),		// 225 degrees
	cmplx (-0.3826, -0.9238),		// 247.5 degrees
	cmplx (0.0, -1.0),				// 270 degrees
	cmplx (0.3826, -0.9238),		// 292.5 degrees
	cmplx (0.7071, -0.7071),		// 315 degrees
	cmplx (0.9238, -0.3826),		// 337.5 degrees
	cmplx (1.0, 0.0),				// 0 degrees
	cmplx (0.9238, 0.3826),			// 22.5 degrees
	cmplx (0.7071, 0.7071),			// 45 degrees
	cmplx (0.3826, 0.9238),			// 67.5 degrees
	cmplx (0.0, 1.0),				// 90 degrees
	cmplx (-0.3826, 0.9238),		// 112.5 degrees
	cmplx (-0.7071, 0.7071),		// 135 degrees
	cmplx (-0.9238, 0.3826) 		// 157.5 degrees
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

		if (_xpsk && !_disablefec) { // Use Gray-mapped xpsk constellation
			symbol = prevsymbol[car] * graymapped_xpsk_pos[(sym & 3)];	// complex multiplication

		} else if (_8psk && !_disablefec) { // Use Gray-mapped 8psk constellation
			symbol = prevsymbol[car] * graymapped_8psk_pos[(sym & 7)];	// complex multiplication

		} else if (_16psk && !_disablefec) { // Use Gray-mapped 16psk constellation
				symbol = prevsymbol[car] * graymapped_16psk_pos[(sym & 15)];	// complex multiplication

		} else { // Map the incoming symbols to the underlying 16psk constellation.
			if (_xpsk) {
				sym = sym * 4 + 2; // Give it the "X" constellation shape
			} else if (_8psk) {
				sym *= 2; // Map 8psk to 16psk
			} else { // BPSK and QPSK
				if (_qpsk && !reverse) {
					sym = (4 - sym) & 3;
				}
				sym *= 4; // For BPSK and QPSK
			}
			symbol = prevsymbol[car] * sym_vec_pos[(sym & SVP_MASK)];	// complex multiplication
		}


		for (int i = 0; i < symbollen; i++) {

			shapeA = tx_shape[i];

			if (test_signal_window && test_signal_window->visible() && btn_imd_on->value()) {
				double imd = pow(10, xmtimd->value()/20.0);
				shapeA -= (imd * imd_shape[i]);
			}

			shapeB = (1.0 - shapeA);

			ival = shapeA * prevsymbol[car].real() + shapeB * symbol.real();
			qval = shapeA * prevsymbol[car].imag() + shapeB * symbol.imag();

			if (car != 0) {
				outbuf[i] += (ival * cos(phaseacc[car]) + qval * sin(phaseacc[car])) / numcarriers;
			} else {
				outbuf[i] = (ival * cos(phaseacc[car]) + qval * sin(phaseacc[car])) / numcarriers;
			}

			phaseacc[car] += delta[car];
			if (phaseacc[car] > TWOPI) phaseacc[car] -= TWOPI;
		}

		prevsymbol[car] = symbol;
	}

	double amp=0;
	bool tx_vestigial = false;
	if (vestigial && progdefaults.pskpilot) {
		tx_vestigial = true;
		amp = pow(10, progdefaults.pilot_power / 20.0) * maxamp;
	}
	if (tx_vestigial) {
		double dvp = TWOPI * (frequencies[0] - sc_bw) / samplerate;
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
	if ( (_8psk && _puncturing) || _xpsk || _16psk)
		if ( (bitcount & 0x1) )
			bitcount = 0;

	// Pass one bit and return two bits
	bitshreg = enc->encode(bit);
	// Interleave
	Txinlv->bits(&bitshreg); // Bit-interleave
	fecbits = bitshreg;

	if (_xpsk) { // 2 bits-per-symbol. Transmit every call
		xpsk_sym = static_cast<unsigned int>(fecbits);
		tx_symbol(xpsk_sym);
		return;
	}

	else if (_8psk && _puncturing) { // @ 2/3 rate

		if ( 0 == bitcount) {
			xpsk_sym = static_cast<unsigned int>(fecbits);
			bitcount = 2;
			return;

		} else if ( 2 == bitcount ) {
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 1) << 2 ;
			/// Punctured anyways, so skip -> //xpsk_sym |= (static_cast<unsigned int>(fecbits) & 2) << 2 ;
			tx_symbol(xpsk_sym & 7); /// Drop/puncture the high-bit on Tx
			xpsk_sym = bitcount = 0;
			return;
		}
	}


	else if (_8psk) { // 3 bits-per-symbol. Accumulate then tx.

		if ( 0 == bitcount ) { // Empty xpsk_sym buffer: add 2 bits and return
			xpsk_sym = static_cast<unsigned int>(fecbits);
			bitcount = 2;
			return ;

		} else if ( 1 == bitcount ) { // xpsk_sym buffer with one bit: add 2 bits then tx and clear
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 1) << 1 ;
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 2) << 1 ;
			tx_symbol(xpsk_sym);
			xpsk_sym = bitcount = 0;
			return;

		} else if ( 2 == bitcount ) { // xpsk_sym buffer with 2 bits: add 1 then tx and save next bit
			xpsk_sym |= (static_cast<unsigned int>(fecbits) & 1) << 2 ;
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
	sig_start = false;
	if (_pskr) {
		ovhd_symbols = ((numcarriers - symbols) % numcarriers);
		//VK2ETA replace with a more effective flushing sequence (avoids cutting the last characters in low s/n)
		for (int i = 0; i < ovhd_symbols/2; i++) tx_bit(0);

		for (int i = 0; i < dcdbits / 2; i++) {
			tx_bit(1);
			if (i == dcdbits/2 - 1) sig_stop = true;
			tx_bit(1);
		}

	// QPSK - flush the encoder
	} else if (_qpsk) {
		for (int i = 0; i < dcdbits; i++) {
			if (i == dcdbits - 1) sig_stop = true;
			tx_bit(0);
		}

	// FEC enabled: Sens the NULL character in MFSk varicode as the flush / post-amble sequence
	} else if (!_disablefec && (_xpsk || _8psk || _16psk) ) {
		for (int i=0; i<flushlength; i++) {
			if (i == flushlength - 1) sig_stop = true;
			tx_char(0); // Send <NUL> to clear bit accumulators on both Tx and Rx ends.
		}

	// FEC disabled: use unmodulated carrier (bpsk-like single tone)
	} else if (_disablefec && ( _xpsk || _8psk || _16psk) ) {
		for (int i=0; i<symbits; i++) {
			if (i == symbits - 1) sig_stop = true;
			tx_char(0); // Send <NUL> to clear bit accumulators on both Tx and Rx ends.
		}

		int symbol;
		if (_16psk) symbol = 8;
		else if (_8psk) symbol = 4;
		else symbol = 2; // xpsk

		for (int i = 0; i <= 96; i++) { // DCD window is only 32-bits wide. Send 3-times
			if (i == 95) sig_stop = true;
			tx_symbol(symbol);
		}

	// Standard BPSK postamble
	} else {
		for (int i = 0; i < dcdbits; i++) {
			if (i == dcdbits - 1) sig_stop = true;
			tx_symbol(2); // 0 degrees
		}
	}
	for (int i = 0; i < 2048; i++) outbuf[i] = 0;
	transmit(outbuf, 2048);
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
	modem::tx_process();

	if (preamble) {
		if (_pskr) {
			if (mode != MODE_PSK63F)
				clearbits();
			// FEC prep the encoder with one/zero sequences of bits
			sig_start = true;
			sig_stop = false;
			for (int i = 0; i < preamble/2; i++) {
				tx_bit(1);
				tx_bit(0);
			}
			// FEC: Mark start of first character with a double zero
			// to ensure sync at end of preamble
			while (acc_symbols % numcarriers) tx_bit(0);
			tx_char(0); // <NUL>
			preamble = 0;
			return 0;

		} else if (mode == MODE_OFDM_500F || mode == MODE_OFDM_750F || mode == MODE_OFDM_2000F) {
			// RSID is used for frequency-setting and AFC: no preamble needed or used. Just send a header.
			clearbits();
			sig_start = true;
			sig_stop = false;
			for (int i=0; i<5; i++)
				tx_char(0); // Send 4 <NUL> characters as both sacrificial-header and pad-to-ramp-up-the-FEC.
			tx_char(10); // <LF> newline as first character
			preamble = 0;
			return 0;

		} else if (_8psk) {
			if (!_disablefec) clearbits();
			if(progStatus.psk8DCDShortFlag)
				if (preamble > 96) preamble = 96;
			if (_disablefec) {
				// Send continuous symbol 0: Usual PSK 2-tone preamble
				sig_start = true;
				sig_stop = false;
				for (int i = 0; i < preamble; i++ )
					tx_symbol(0);
				tx_char(0);
				preamble = 0;
				return 0;
			} else {
				_disablefec = true;
				for (int i = 0; i < preamble/2; i++) tx_symbol(0);
				_disablefec = false;
				// FEC prep the encoder with encoded sequence of double-zeros
				// sends a single centered preamble tone for FEC modes
				sig_start = true;
				sig_stop = false;
				for (int i = 0; i < preamble; i += 2) {
					tx_bit(0);
					tx_bit(0);
				}
				tx_char(0);
				preamble = 0;
				return 0;
			}

		} else {
			// Standard BPSK/QPSK/xPSK preamble
			sig_start = true;
			sig_stop = false;
			for (int i = 0; i < preamble; i++) {
				tx_symbol(0);   // send phase reversals
			}
			preamble = 0;
			return 0;
		}
	}

	int c = get_tx_char();

	if (c == GET_TX_CHAR_ETX || stopflag) {
		tx_flush();
		stopflag = false;

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

static bool reset_filters;

void psk::initSN_IMD()
{
	reset_filters = true;
}

void psk::resetSN_IMD()
{
	reset_filters = true;
}

//============================================================================
//  This routine calculates the energy in the frequency bands of
//   carrier = base (0), fundamental = F1(15.625), noise = F2(31.25), and
//   3rd order product = F3(46.875)
//  It is called with cmplx data samples at 500 Hz.
//============================================================================

void psk::calcSN_IMD(cmplx z)
{
	if (!re_Gbin[0]) return;

	if (reset_filters) {
		e0_filt->reset();
		e1_filt->reset();
		e2_filt->reset();
		e3_filt->reset();

		for(int i = 0; i < NUM_FILTERS; i++) {
			re_Gbin[i]->reset();
			im_Gbin[i]->reset();
		}
		reset_filters = false;
	}

	bool isvalid = true;

	for (int i = 0; i < NUM_FILTERS; i++) {
		isvalid &= re_Gbin[i]->run(real(z));
		isvalid &= im_Gbin[i]->run(imag(z));
	}
	if (isvalid) {
		signalquality();
		for (int i = 0; i < NUM_FILTERS; i++) {
			re_Gbin[i]->reset();
			im_Gbin[i]->reset();
		}
	}
	return;
}
