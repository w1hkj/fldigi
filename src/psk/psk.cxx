// ----------------------------------------------------------------------------
// psk.cxx  --  psk modem
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
// Copyright (C) 2009-2010
//		John Douyere, VK2ETA
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
#include "ascii.h"
#include "Viewer.h"

extern waterfall *wf;

// Change the following for DCD low pass filter adjustment
#define SQLCOEFF 0.01
#define SQLDECAY 50

//=====================================================================

#define K		5
#define POLY1	0x17
#define POLY2	0x19

// PSK + FEC + INTERLEAVE
#define PSKR_K		7
#define PSKR_POLY1	0x6d
#define PSKR_POLY2	0x4f

#define SEPARATION	1.4 //separation between carriers expressed as a ratio to sc_bw

#define        GALILEO_K       15
#define        GALILEO_POLY1   046321
#define        GALILEO_POLY2   051271

char pskmsg[80];
viewpsk *pskviewer = (viewpsk *)0;

void psk::tx_init(SoundBase *sc)
{
	scard = sc;
	for (int car = 0; car < numcarriers; car++) {
		phaseacc[car] = 0;
		prevsymbol[car] = cmplx (1.0, 0.0);
	}
	preamble = dcdbits;
	if (_pskr) {
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

//Multiple carriers handling
	accumulated_bits = 0;

}

void psk::rx_init()
{
	for (int car = 0; car < numcarriers; car++) {
		phaseacc[car] = 0;
		prevsymbol[car] = cmplx (1.0, 0.0);
	}
	quality		= cmplx (0.0, 0.0);
	if (_pskr) {
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
	imdValid = false;
	afcmetric = 0.0;
	// interleaver, split incoming bit stream into two, one late by one bit
	rxbitstate = 0;
	fecmet = fecmet2 = 0;

}


void psk::restart()
{
	if (numcarriers == 1)
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
	if (::pskviewer == pskviewer)
		::pskviewer = 0;
	delete pskviewer;
	delete evalpsk;

	// Interleaver
	if (Rxinlv) delete Rxinlv;
	if (Rxinlv2) delete Rxinlv2;
	if (Txinlv) delete Txinlv;

}

psk::psk(trx_mode pskmode) : modem()
{
	cap |= CAP_AFC | CAP_AFC_SR;

	mode = pskmode;
	int  isize = 5;
	int  idepth = 5;  // 5x5x5 interleaver

	numcarriers = 1;

	switch (mode) {
	case MODE_PSK31:
		symbollen = 256;
		_qpsk = false;
		_pskr = false;
		dcdbits = 32;
		numcarriers = 1;
		break;
	case MODE_PSK63:
		symbollen = 128;
		_qpsk = false;
		_pskr = false;
		dcdbits = 64;
		numcarriers = 1;
		break;
	case MODE_PSK125:
		symbollen = 64;
		_qpsk = false;
		_pskr = false;
		dcdbits = 128;
		numcarriers = 1;
		break;
	case MODE_PSK250:
		symbollen = 32;
		_qpsk = false;
		_pskr = false;
		dcdbits = 256;
		numcarriers = 1;
		break;
	case MODE_PSK500:
		symbollen = 16;
		_qpsk = false;
		_pskr = false;
		dcdbits = 512;
		numcarriers = 1;
		break;
	case MODE_PSK1000:
		symbollen = 8;
		_qpsk = false;
		_pskr = false;
		dcdbits = 128;
		numcarriers = 1;
		break;

	case MODE_QPSK31:
		symbollen = 256;
		_qpsk = true;
		_pskr = false;
		dcdbits = 32;
		cap |= CAP_REV;
		numcarriers = 1;
		break;
	case MODE_QPSK63:
		symbollen = 128;
		_qpsk = true;
		_pskr = false;
		dcdbits = 64;
		cap |= CAP_REV;
		numcarriers = 1;
		break;
	case MODE_QPSK125:
		symbollen = 64;
		_qpsk = true;
		_pskr = false;
		dcdbits = 128;
		cap |= CAP_REV;
		numcarriers = 1;
		break;
	case MODE_QPSK250:
		symbollen = 32;
		_qpsk = true;
		_pskr = false;
		dcdbits = 256;
		cap |= CAP_REV;
		numcarriers = 1;
		break;
	case MODE_QPSK500:
		symbollen = 16;
		_qpsk = true;
		_pskr = false;
		dcdbits = 512;
		cap |= CAP_REV;
		numcarriers = 1;
		break;
	case MODE_PSK63F:  // As per Multipsk (BPSK63 + FEC + MFSK Varicode)
		symbollen = 128;
		_qpsk = false;
		_pskr = true;
		dcdbits = 64;
		numcarriers = 1;
		break;

	case MODE_PSK125R:
		symbollen = 64;
		_qpsk = false;
		_pskr = true;
		dcdbits = 128;
		isize = 2;
		idepth = 40;  // 2x2x40 interleaver
		numcarriers = 1;
		break;
	case MODE_PSK250R:
		symbollen = 32;
		_qpsk = false;
		_pskr = true;
		dcdbits = 256;
		isize = 2;
		idepth = 80;  // 2x2x80 interleaver
		numcarriers = 1;
		break;
	case MODE_PSK500R:
		symbollen = 16;
		_qpsk = false;
		_pskr = true;
		dcdbits = 512;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		numcarriers = 1;
		break;
	case MODE_PSK1000R:
		symbollen = 8;
		_qpsk = false;
		_pskr = true;
		dcdbits = 512;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		numcarriers = 1;
		break;

// multi-carrier modems
	case MODE_4X_PSK63R:
		symbollen = 128;//PSK63
		dcdbits = 128;
		_qpsk = false;
		_pskr = true;//PSKR
		numcarriers = 4;
		isize = 2;
		idepth = 80; // 2x2x80 interleaver
		break;
	case MODE_5X_PSK63R:
		symbollen = 128; //PSK63
		dcdbits = 512;
		_qpsk = false;
		_pskr = true; //PSKR
		numcarriers = 5;
		isize = 2;
		idepth = 260; // 2x2x160 interleaver
		break;
	case MODE_10X_PSK63R:
		symbollen = 128; //PSK63
		dcdbits = 512;
		_qpsk = false;
		_pskr = true; //PSKR
		numcarriers = 10;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;
	case MODE_20X_PSK63R:
		symbollen = 128; //PSK63
		dcdbits = 512;
		_qpsk = false;
		_pskr = true; //PSKR
		numcarriers = 20;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;
	case MODE_32X_PSK63R:
		symbollen = 128; //PSK63
		dcdbits = 512;
		_qpsk = false;
		_pskr = true; //PSKR
		numcarriers = 32;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;

	case MODE_4X_PSK125R:
		symbollen = 64;//PSK125
		dcdbits = 512;
		_qpsk = false;
		_pskr = true;//PSKR
		numcarriers = 4;
		isize = 2;
		idepth = 80; // 2x2x80 interleaver
		break;
	case MODE_5X_PSK125R:
		symbollen = 64;//PSK125
		dcdbits = 512;
		_qpsk = false;
		_pskr = true;//PSKR
		numcarriers = 5;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;
	case MODE_10X_PSK125R:
		symbollen = 64;//PSK125
		dcdbits = 512;
		_qpsk = false;
		_pskr = true;//PSKR
		numcarriers = 10;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;

	case MODE_12X_PSK125:
		symbollen = 64;//PSK125
		dcdbits = 128;//512;
		_qpsk = false;
		_pskr = false;
		numcarriers = 12;
		break;
	case MODE_12X_PSK125R:
		symbollen = 64;//PSK125
		dcdbits = 512;
		_qpsk = false;
		_pskr = true;//PSKR
		numcarriers = 12;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;

	case MODE_16X_PSK125R:
		symbollen = 64;//PSK125
		dcdbits = 512;
		_qpsk = false;
		_pskr = true;//PSKR
		numcarriers = 16;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;

	case MODE_2X_PSK250R:
		symbollen = 32;//PSK250
		dcdbits = 512;
		_qpsk = false;
		_pskr = true;//PSKR
		numcarriers = 2;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;
	case MODE_3X_PSK250R:
		symbollen = 32;//PSK250
		dcdbits = 512;
		_qpsk = false;
		_pskr = true;//PSKR
		numcarriers = 3;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;
	case MODE_5X_PSK250R:
		symbollen = 32;//PSK250
		_qpsk = false;
		_pskr = true;//PSKR
		dcdbits = 1024;
		numcarriers = 5;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;
	case MODE_6X_PSK250:
		symbollen = 32;//PSK250
		_qpsk = false;
		_pskr = false;
		dcdbits = 512;
		numcarriers = 6;
		break;
	case MODE_6X_PSK250R:
		symbollen = 32;//PSK250
		_qpsk = false;
		_pskr = true;//PSKR
		dcdbits = 1024;
		numcarriers = 6;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;
	case MODE_7X_PSK250R:
		symbollen = 32;//PSK250
		_qpsk = false;
		_pskr = true;//PSKR
		dcdbits = 1024;
		numcarriers = 7;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;

	case MODE_2X_PSK500:
		symbollen = 16;
		_qpsk = false;
		_pskr = false;
		dcdbits = 512;
		numcarriers = 2;
		break;
	case MODE_4X_PSK500:
		symbollen = 16;
		_qpsk = false;
		_pskr = false;
		dcdbits = 512;
		numcarriers = 4;
		break;

	case MODE_2X_PSK500R:
		symbollen = 16;
		_qpsk = false;
		_pskr = true;
		dcdbits = 1024;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		numcarriers = 2;
		break;
	case MODE_3X_PSK500R:
		symbollen = 16;
		_qpsk = false;
		_pskr = true;
		dcdbits = 1024;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		numcarriers = 3;
		break;
	case MODE_4X_PSK500R:
		symbollen = 16;
		_qpsk = false;
		_pskr = true;
		dcdbits = 1024;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		numcarriers = 4;
		break;

	case MODE_2X_PSK800:
		symbollen = 10;
		_qpsk = false;
		_pskr = false;
		dcdbits = 512;
		numcarriers = 2;
		break;
	case MODE_2X_PSK800R:
		symbollen = 10;
		_qpsk = false;
		_pskr = true;
		dcdbits = 1024;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		numcarriers = 2;
		break;

	case MODE_2X_PSK1000:
		symbollen = 8;//PSK1000
		_qpsk = false;
		_pskr = false;
		dcdbits = 1024;
		numcarriers = 2;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;
	case MODE_2X_PSK1000R:
		symbollen = 8;//PSK1000
		_qpsk = false;
		_pskr = true;//PSKR
		dcdbits = 1024;
		numcarriers = 2;
		isize = 2;
		idepth = 160; // 2x2x160 interleaver
		break;

	default:
		mode = MODE_PSK31;
		symbollen = 256;
		_qpsk = false;
		_pskr = false;
		dcdbits = 32;
		numcarriers = 1;
	}

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

	if (_qpsk) {
		enc = new encoder(K, POLY1, POLY2);
		dec = new viterbi(K, POLY1, POLY2);
	}

	if (_pskr) {
// FEC for BPSK. Use a 2nd Viterbi decoder for comparison.
// Set decode size to 4 since some characters can be as small
// as 3 bits long. This minimises intercharacters decoding
// interactions.
		enc = new encoder(PSKR_K, PSKR_POLY1, PSKR_POLY2);
		dec = new viterbi(PSKR_K, PSKR_POLY1, PSKR_POLY2);
		dec->setchunksize(4);
		dec2 = new viterbi(PSKR_K, PSKR_POLY1, PSKR_POLY2);
		dec2->setchunksize(4);

// Interleaver. To maintain constant time delay between bits,
// we double the number of concatenated square iterleavers for
// each doubling of speed: 2x2x20 for BSK63+FEC, 2x2x40 for
// BPSK125+FEC, etc..
		if (_pskr && (mode != MODE_PSK63F)) {
// 2x2x(20,40,80,160)
			Txinlv = new interleave (isize, idepth, INTERLEAVE_FWD);//numinterleavers, INTERLEAVE_FWD);
// 2x2x(20,40,80,160)
			Rxinlv = new interleave (isize, idepth, INTERLEAVE_REV);//numinterleavers, INTERLEAVE_REV);
// 2x2x(20,40,80,160)
			Rxinlv2 = new interleave (isize, idepth, INTERLEAVE_REV);//numinterleavers, INTERLEAVE_REV);
		}
		bitshreg = 0;
		rxbitstate = 0;
		startpreamble = true;
	}

	tx_shape = new double[symbollen];

	// raised cosine shape for the transmitter
	for ( int i = 0; i < symbollen; i++)
		tx_shape[i] = 0.5 * cos(i * M_PI / symbollen) + 0.5;

	samplerate = PskSampleRate;
	fragmentsize = symbollen;
	sc_bw = samplerate / symbollen;
	//JD added for multiple carriers
	inter_carrier = SEPARATION * sc_bw;
	bandwidth = sc_bw * ( 1 + SEPARATION * (numcarriers - 1));

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
	if (numcarriers == 1) {
		::pskviewer = pskviewer = new viewpsk(evalpsk, mode);
	} else
		::pskviewer = pskviewer = 0;

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

	shreg = (shreg << 1) | !!bit;
	if (_pskr) {
		// MFSK varicode instead of PSK Varicode
		if ((shreg & 7) == 1) {
			c = varidec(shreg >> 1);
			// Voting at the character level
			if (fecmet >= fecmet2) {
				if ((c != -1) && (c != 0) && (dcd == true)) {
					put_rx_char(c);
					if (progdefaults.Pskmails2nreport && (mailserver || mailclient)) {
						s2n_sum += s2n_metric;
						s2n_sum2 += (s2n_metric * s2n_metric);
						s2n_ncount ++;
						if (c == EOT)
							s2nreport();
					}
				}
			}
			shreg = 1;
		}
	} else {
		if ((shreg & 3) == 0) {
			c = psk_varicode_decode(shreg >> 2);
			if ((c != -1) && (dcd == true)) {
				put_rx_char(c);
				if (progdefaults.Pskmails2nreport && (mailserver || mailclient)) {
					s2n_sum += s2n_metric;
					s2n_sum2 += (s2n_metric * s2n_metric);
					s2n_ncount++;
					if (c == EOT)
						s2nreport();
				}
			}
			shreg = 0;
		}
	}
}




void psk::rx_bit2(int bit)
{
	int c;

	shreg2 = (shreg2 << 1) | !!bit;
	// MFSK varicode instead of PSK Varicode
	if ((shreg2 & 7) == 1) {
		c = varidec(shreg2 >> 1);
		// Voting at the character level
		if (fecmet < fecmet2) {
			if ((c != -1) && (c != 0) && (dcd == true)) {
				put_rx_char(c);
				if (progdefaults.Pskmails2nreport && (mailserver || mailclient)) {
					s2n_sum += s2n_metric;
					s2n_sum2 += (s2n_metric * s2n_metric);
					s2n_ncount++;
					if (c == EOT)
						s2nreport();
				}
			}
		}
		shreg2 = 1;
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
		// copy to avoid scrambling symbolpair for the next bit
		twosym[0] = symbolpair[0];
		twosym[1] = symbolpair[1];
		// De-interleave for Robust modes only
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

//JD: disable for multiple carriers as we are running as modem and 
//    therefore use other strategies for frequency alignment like RSID
void psk::phaseafc()
{
	double error;
	if (afcmetric < 0.05 || 
		mode == MODE_PSK500 ||
		mode == MODE_QPSK500 ||
		numcarriers > 1 ) return;

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

	if (phase < 0)
		phase += TWOPI;

	if (_qpsk) {
		bits = ((int) (phase / M_PI_2 + 0.5)) & 3;
		n = 4;
	} else { // bpsk and pskr
		bits = (((int) (phase / M_PI + 0.5)) & 1) << 1;
		// hard decode if needed
		// softbit = (bits & 2) ? 0 : 255;  
		// reversed as we normally pass "!bits" when hard decoding
		// Soft decode section below
		averageamp = decayavg(averageamp, sigamp, SQLDECAY);
		if (sigamp > 0 && averageamp > 0) {
			softamp = clamp( averageamp / sigamp, 1.0, 1e6);
		} else {
			softamp = 1; // arbritary number (50% impact)
		}
		// Compute values between -128 and +127 for phase value only
		if (phase > M_PI) {
			softangle = (127 - (((2 * M_PI - phase) / M_PI) * (double) 255));
		} else {
			softangle = (127 - ((phase / M_PI) * (double) 255));
		}
		// Then apply impact of amplitude. Fanally, re-centre on 127-128
		// as the decoder needs values between 0-255
		softbit = (unsigned char) ((softangle / (1 + softamp)) - 127);
		n = 2;
	}

	// simple low pass filter for quality of signal
//	quality.re = decayavg(quality.re, cos(n*phase), _pskr ? SQLDECAY * 5 : SQLDECAY);
//	quality.im = decayavg(quality.im, sin(n*phase), _pskr ? SQLDECAY * 5 : SQLDECAY);
//	quality.re = decayavg(quality.re, cos(n*phase), _pskr ? SQLDECAY * 10 : SQLDECAY);
//	quality.im = decayavg(quality.im, sin(n*phase), _pskr ? SQLDECAY * 10 : SQLDECAY);
	quality = cmplx(
		decayavg(quality.real(), cos(n*phase), _pskr ? SQLDECAY * 10 : SQLDECAY),
		decayavg(quality.imag(), sin(n*phase), _pskr ? SQLDECAY * 10 : SQLDECAY));
	metric = 100.0 * norm(quality);

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
	if (_pskr) {
		metric = metric * 4;
	}

	if (metric > 100)
		metric = 100;

	afcmetric = decayavg(afcmetric, norm(quality), 50);

	dcdshreg = (dcdshreg << 2) | bits;

	imdValid = false;
	switch (dcdshreg) {

	case 0xAAAAAAAA:	// DCD on by preamble for psk modes
		if (!_pskr) {
			dcd = true;
			acquire = 0;
			quality = cmplx (1.0, 0.0);
			imdValid = true;
			if (progdefaults.Pskmails2nreport && (mailserver || mailclient))
				s2n_sum = s2n_sum2 = s2n_ncount = 0.0;
		}
		break;

	case 0xA0A0A0A0:	// DCD on by preamble for PSKR modes ("11001100" sequence sent as preamble)
		if (_pskr) {
			dcd = true;
			acquire = 0;
			quality = cmplx (1.0, 0.0);
			imdValid = true;
//VK2ETA added logic to prevent resetting
//			noSOHyet = true;
			if (progdefaults.Pskmails2nreport && (mailserver || mailclient))
				s2n_sum = s2n_sum2 = s2n_ncount = 0.0;
		}
		break;

	case 0:			// DCD off by postamble. Not for PSKR modes as this is not unique to postamble. 
		if (!_pskr) {
			dcd = false;
			acquire = 0;
			quality = cmplx (0.0, 0.0);
		}
		break;
	default:
		if (metric > progStatus.sldrSquelchValue || progStatus.sqlonoff == false) {
			dcd = true;
		} else {
			dcd = false;
		}
	}

	if (!_pskr) {
		set_phase(phase, norm(quality), dcd);

		if (dcd == true) {
			if (_qpsk )
				rx_qpsk(bits);
			else
				rx_bit(!bits);
		}
	} else { // pskr processing
		// FEC: moved below the rx_bit to use proper value for dcd
		rx_pskr(softbit);
		set_phase(phase, norm(quality), dcd);
	}

}

void psk::signalquality()
{

	if (m_Energy[1])
		snratio = snfilt->run(m_Energy[0]/m_Energy[1]);
	else
		snratio = snfilt->run(1.0);

	if (m_Energy[0] && imdValid)
		imdratio = imdfilt->run(m_Energy[2]/m_Energy[0]);
	else
		imdratio = imdfilt->run(0.001);

}

void psk::update_syncscope()
{
	static char msg1[15];
	static char msg2[15];

	display_metric(metric);

	s2n = 10.0*log10( snratio );
	snprintf(msg1, sizeof(msg1), "s/n %2d dB", (int)(floor(s2n)));

	imd = 10.0*log10( imdratio );
	snprintf(msg2, sizeof(msg2), "imd %3d dB", (int)(floor(imd)));

	if (imdValid) {
		put_Status1(msg1, progdefaults.StatusTimeout, progdefaults.StatusDim ? STATUS_DIM : STATUS_CLEAR);
		put_Status2(msg2, progdefaults.StatusTimeout, progdefaults.StatusDim ? STATUS_DIM : STATUS_CLEAR);
	}
}

char bitstatus[100];

int psk::rx_process(const double *buf, int len)
{
	double delta[MAX_CARRIERS], frequencies[MAX_CARRIERS];
	cmplx z, z2[MAX_CARRIERS];
	bool can_rx_symbol = false;

	if (numcarriers == 1) {
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

		phaseacc[car] += delta[car];
		if (phaseacc[car] > M_PI)
			phaseacc[car] -= TWOPI;

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

			//Handling of modes faster than PSK500/PSK500R

/*			for (int i = 0; i < 8; i++) {
				sum += (syncbuf[i] - syncbuf[i+8]);
				ampsum += (syncbuf[i] + syncbuf[i+8]);
			}
*/
//			double bitsteps = (symbollen >= 16 ? 16 : 8);
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
* At the end of this cycle, bitclk is pointing at a sample which will
* have the maximum phase difference, if any, from the previous symbol's
* phase.
*
*/                           

//			if (bitclk < 0) bitclk += 16.0;
//			if (bitclk >= 16.0) {
//				bitclk -= 16.0;

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

void psk::tx_symbol(int sym)
{
	double delta[MAX_CARRIERS];
	double	ival, qval, shapeA, shapeB;
	cmplx symbol;
	double	frequencies[MAX_CARRIERS];

	txsymbols[accumulated_bits] = sym;

	if (++accumulated_bits < numcarriers) {
		return;
	} 

	//Process all carrier's symbols, then submit to sound card
	accumulated_bits = 0; //reset
	frequencies[0] = get_txfreq_woffset() + ((-1 * numcarriers) + 1) * inter_carrier / 2;
	delta[0] = TWOPI * frequencies[0] / samplerate;
	for (int car = 1; car < numcarriers; car++) {
			frequencies[car] = frequencies[car - 1] + inter_carrier;
			delta[car] = TWOPI * frequencies[car] / samplerate;
	}

double maxamp = 0;
	for (int car = 0; car < numcarriers; car++) {
		sym = txsymbols[car];

		if (_qpsk && !reverse)
			sym = (4 - sym) & 3;

		// differential QPSK modulation - top bit flipped
		switch (sym) {
		case 0:
			symbol = cmplx (-1.0, 0.0);	// 180 degrees
			break;
		case 1:
			symbol = cmplx (0.0, -1.0);	// 270 degrees
			break;
		case 2:
			symbol = cmplx (1.0, 0.0);		// 0 degrees
			break;
		case 3:
			symbol = cmplx (0.0, 1.0);		// 90 degrees
			break;
		}
		symbol = prevsymbol[car] * symbol;	// cmplx multiplication

		for (int i = 0; i < symbollen; i++) {

			shapeA = tx_shape[i];
			shapeB = (1.0 - shapeA);

			ival = shapeA * prevsymbol[car].real() + shapeB * symbol.real();
			qval = shapeA * prevsymbol[car].imag() + shapeB * symbol.imag();

			if (car != 0) {
				outbuf[i] += (ival * cos(phaseacc[car]) + qval * sin(phaseacc[car]));// / numcarriers; 
			} else {
				outbuf[i] = (ival * cos(phaseacc[car]) + qval * sin(phaseacc[car]));// / numcarriers;
			}
			if (maxamp < fabs(outbuf[i])) {
				maxamp = fabs(outbuf[i]);
			}

			phaseacc[car] += delta[car];
			if (phaseacc[car] > M_PI)
				phaseacc[car] -= 2.0 * M_PI;
		}

		prevsymbol[car] = symbol;
	}
	if (maxamp)
		for (int i = 0; i < symbollen; i++) outbuf[i] /= maxamp;

	ModulateXmtr(outbuf, symbollen);
}

void psk::tx_bit(int bit)
{
	unsigned int sym;

	// qpsk transmission
	if (_qpsk) {
		sym = enc->encode(bit);
		//JD add interleaver
//		Txinlv->bits(&sym); 
		sym = sym & 3;//JD just to make sure
		tx_symbol(sym);
	// else pskr (fec + interleaver) transmission
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
	// else normal bpsk tranmission
	} else {
		sym = bit << 1;
		tx_symbol(sym);
	}
}



void psk::tx_char(unsigned char c)
{
	const char *code;

	if (_pskr) {
		// ARQ varicode instead of MFSK for PSK63FEC
		code = varienc(c);
	} else {
		code = psk_varicode_encode(c);
	}
	while (*code) {
		tx_bit((*code - '0'));
		code++;
	}

	if (! _pskr) {
		// MSFK varicode instead of psk varicode
		tx_bit(0);
		tx_bit(0);
	}
}


void psk::tx_flush()
{
	if (_pskr) {
		//VK2ETA replace with a more effective flushing sequence (avoids cutting the last characters in low s/n)
/*		for (int i = 0; i < dcdbits; i++)
			tx_bit(0);
		}
*/
		for (int i = 0; i < dcdbits / 2; i++) {
			tx_bit(1);
			tx_bit(1);
		}
	}
	// QPSK - flush the encoder
	if (_qpsk) {
		for (int i = 0; i < dcdbits; i++)
		tx_bit(0);
	// FEC : replace unmodulated carrier by an encoded sequence of zeros
	}
	// Standard BPSK postamble
	// DCD off sequence (unmodulated carrier)
	//VK2ETA remove for pskr since it is not used for DCD and only adds delay and creates TX overlaps
	if (!_pskr) {
		for (int i = 0; i < dcdbits; i++)
			tx_symbol(2);
	}
}

// Necessary to clear the interleaver before we start sending
void psk::clearbits()
{
	bitshreg = enc->encode(0);
	for (int k = 0; k < 160; k++) {
		Txinlv->bits(&bitshreg);
	}
}



int psk::tx_process()
{
	int c;

	if (preamble > 0) {
		if (_pskr) {
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
			if (preamble == 0)  tx_bit(0);
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
		return -1;   // we're done
	}

	if (c == GET_TX_CHAR_NODATA) {
		if (_pskr) {
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
	COEF[1] = 2.0 * cos(TWOPI * 18 / GOERTZEL);
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
