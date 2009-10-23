// ----------------------------------------------------------------------------
// psk.cxx  --  psk modem
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk 
// source code distribution.
// gmfsk Copyright (C) 2001, 2002, 2003
// Tomi Manninen (oh2bns@sral.fi)
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
// Foundation, Inc., 59 Temple Place, Suite 330, 
// Boston, MA  02111-1307  USA
// --------------------------------------------------------------------


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

extern waterfall *wf;

// Change the following for DCD low pass filter adjustment
#define SQLCOEFF 0.01
#define SQLDECAY 50

//=====================================================================

#define	K		5
#define	POLY1	0x17
#define	POLY2	0x19

char pskmsg[80];
viewpsk *pskviewer = (viewpsk *)0;

void psk::tx_init(SoundBase *sc)
{
	scard = sc;
	phaseacc = 0;
	prevsymbol = complex (1.0, 0.0);
	preamble = dcdbits;
	shreg = 0;
	videoText();
}

void psk::rx_init()
{
	phaseacc = 0;
	prevsymbol	= complex (1.0, 0.0);
	quality		= complex (0.0, 0.0);
	shreg = 0;
	dcdshreg = 0;
	dcd = 0;
	bitclk = 0;
	freqerr = 0.0;
	if (mailserver && progdefaults.PSKmailSweetSpot) sigsearch = SIGSEARCH;
	put_MODEstatus(mode);
	resetSN_IMD();
	imdValid = false;
	afcmetric = 0.0;
}

void psk::restart()
{
	pskviewer->restart(mode);
	evalpsk->setbw(bandwidth);
}

void psk::init()
{
	modem::init();
	restart();
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
	if (fir1) delete fir1;
	if (fir2) delete fir2;
	if (snfilt) delete snfilt;
	if (imdfilt) delete imdfilt;
	if (::pskviewer == pskviewer)
		::pskviewer = 0;
	delete pskviewer;
	delete evalpsk;
}

psk::psk(trx_mode pskmode) : modem()
{
	cap = CAP_AFC | CAP_AFC_SR;

	mode = pskmode;

	switch (mode) {
	case MODE_BPSK31:
		symbollen = 256;
		_qpsk = false;
		dcdbits = 32;
		break;
	case MODE_QPSK31:
		symbollen = 256;
		_qpsk = true;
		dcdbits = 32;
		cap |= CAP_REV;
		break;
	case MODE_PSK63:
		symbollen = 128;
		_qpsk = false;
		dcdbits = 64;
		break;
	case MODE_QPSK63:
		symbollen = 128;
		_qpsk = true;
		dcdbits = 64;
		cap |= CAP_REV;
		break;
	case MODE_PSK125:
		symbollen = 64;
		_qpsk = false;
		dcdbits = 128;
		break;
	case MODE_QPSK125:
		symbollen = 64;
		_qpsk = true;
		dcdbits = 128;
		cap |= CAP_REV;
		break;
	case MODE_PSK250:
		symbollen = 32;
		_qpsk = false;
		dcdbits = 256;
		break;
	case MODE_QPSK250:
		symbollen = 32;
		_qpsk = true;
		dcdbits = 256;
		cap |= CAP_REV;
		break;
	default:
		mode = MODE_BPSK31;
		symbollen = 256;
		_qpsk = false;
		dcdbits = 32;
	}

	enc = (encoder *)0;
	dec = (viterbi *)0;
	
// create impulse response for experimental FIR filters
	double fir1c[64];
	double fir2c[64];

	fir1 = new C_FIR_filter();
	fir2 = new C_FIR_filter();

    switch (progdefaults.PSK_filter) {
        case 1:
// use the original gmfsk matched filters
        	for (int i = 0; i < 64; i++) {
		        fir1c[i] = gmfir1c[i];
		        fir2c[i] = gmfir2c[i];
	        }
        	fir1->init(FIRLEN, symbollen / 16, fir1c, fir1c);
	        fir2->init(FIRLEN, 1, fir2c, fir2c);
            break;
        case 2:
// creates fir1c matched sin(x)/x filter w hamming
	        wsincfilt(fir1c, 1.0 / symbollen, false);
        	fir1->init(FIRLEN, symbollen / 16, fir1c, fir1c);
// creates fir2c matched sin(x)/x filter w hamming
	        wsincfilt(fir2c, 1.0 / 16.0, false);
	        fir2->init(FIRLEN, 1, fir2c, fir2c);
            break;
        case 3:
// creates fir1c matched sin(x)/x filter w hamming
	        wsincfilt(fir1c, 1.0 / symbollen, false);
        	fir1->init(FIRLEN, symbollen / 16, fir1c, fir1c);
// 1/22 with Hamming window nearly identical to gmfir2c
	        wsincfilt(fir2c, 1.0 / 22.0, false);
	        fir2->init(FIRLEN, 1, fir2c, fir2c);
            break;
        case 4:
            fir1->init_lowpass (FIRLEN, 16, 1.5 / symbollen);
        	wsincfilt(fir2c, 1.5 / 16.0, true);
            fir2->init(FIRLEN, 1, fir2c, fir2c);
        case 0:
        default :
// creates fir1c matched sin(x)/x filter w blackman
        	wsincfilt(fir1c, 1.0 / symbollen, true);
        	fir1->init(FIRLEN, symbollen / 16, fir1c, fir1c);
// creates fir2c matched sin(x)/x filter w blackman
        	wsincfilt(fir2c, 1.0 / 16.0, true);
	        fir2->init(FIRLEN, 1, fir2c, fir2c);
    }
    
	snfilt = new Cmovavg(16);
	imdfilt = new Cmovavg(16);

	if (_qpsk) {
		enc = new encoder(K, POLY1, POLY2);
		dec = new viterbi(K, POLY1, POLY2);
	}

	tx_shape = new double[symbollen];

	/* raised cosine shape for the transmitter */
	for ( int i = 0; i < symbollen; i++)
		tx_shape[i] = 0.5 * cos(i * M_PI / symbollen) + 0.5;

	samplerate = PskSampleRate;
	fragmentsize = symbollen;
	bandwidth = samplerate / symbollen;
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
	::pskviewer = pskviewer = new viewpsk(evalpsk, mode);

	init();
}

//=============================================================================
//=========================== psk31 receive routines ==========================
//=============================================================================

void psk::rx_bit(int bit)
{
	int c;
	shreg = (shreg << 1) | !!bit;
	if ((shreg & 3) == 0) {
		c = psk_varicode_decode(shreg >> 2);
		if (c != -1)
			put_rx_char(c);
		shreg = 0;
	}
}

void psk::rx_qpsk(int bits)
{
	unsigned char sym[2];
	int c;

	if (_qpsk && !reverse)
		bits = (4 - bits) & 3;

	sym[0] = (bits & 1) ? 255 : 0;
	sym[1] = (bits & 2) ? 0 : 255;		/* top bit is flipped */

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

void psk::searchDown()
{
	double srchfreq = frequency - bandwidth * 2;
	double minfreq = bandwidth * 2;
	double spwr, npwr;
	while (srchfreq > minfreq) {
		spwr = wf->powerDensity(srchfreq, bandwidth);
		npwr = wf->powerDensity(srchfreq + bandwidth, bandwidth/2) + 1e-10;
		if (spwr / npwr > pow(10, progdefaults.ServerACQsn / 10)) {
			frequency = srchfreq;
			set_freq(frequency);
			sigsearch = SIGSEARCH;
			break;
		}
		srchfreq -= bandwidth;
	}
}

void psk::searchUp()
{
	double srchfreq = frequency + bandwidth * 2;
	double maxfreq = IMAGE_WIDTH - bandwidth * 2;
	double spwr, npwr;
	while (srchfreq < maxfreq) {
		spwr = wf->powerDensity(srchfreq, bandwidth/2);
		npwr = wf->powerDensity(srchfreq - bandwidth, bandwidth/2) + 1e-10;
		if (spwr / npwr > pow(10, progdefaults.ServerACQsn / 10)) {
			frequency = srchfreq;
			set_freq(frequency);
			sigsearch = SIGSEARCH;
			break;
		}
		srchfreq += bandwidth;
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

void psk::phaseafc()
{
	double error;
	if (afcmetric < 0.05) return;
	
	error = (phase - bits * M_PI / 2.0);
	if (error < -M_PI / 2.0 || error > M_PI / 2.0) return;
	error *= samplerate / (TWOPI * symbollen);
	if (fabs(error) < bandwidth ) {
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


void psk::rx_symbol(complex symbol)
{
	int n;
	phase = (prevsymbol % symbol).arg();
	prevsymbol = symbol;

	if (phase < 0) 
		phase += TWOPI;
	if (_qpsk) {
		bits = ((int) (phase / M_PI_2 + 0.5)) & 3;
		n = 4;
	} else {
		bits = (((int) (phase / M_PI + 0.5)) & 1) << 1;
		n = 2;
	}
// simple low pass filter for quality of signal
	quality.re = decayavg(quality.re, cos(n*phase), SQLDECAY);
	quality.im = decayavg(quality.im, sin(n*phase), SQLDECAY);
	
	metric = 100.0 * quality.norm();
	afcmetric = decayavg(afcmetric, quality.norm(), 50);
	
	dcdshreg = (dcdshreg << 2) | bits;

	switch (dcdshreg) {
	case 0xAAAAAAAA:	/* DCD on by preamble */
		dcd = true;
		acquire = 0;
		quality = complex (1.0, 0.0);
		imdValid = true;
		break;

	case 0:			/* DCD off by postamble */
		dcd = false;
		acquire = 0;
		quality = complex (0.0, 0.0);
		imdValid = false;
		break;

	default:
		if (metric > progStatus.sldrSquelchValue || progStatus.sqlonoff == false)
			dcd = true;
		else
			dcd = false;
		imdValid = false;
	}

	set_phase(phase, quality.norm(), dcd);

	if (dcd == true) {
		if (_qpsk)
			rx_qpsk(bits);
		else
			rx_bit(!bits);
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
	double delta;
	complex z, z2;

	if (pskviewer && !bHistory) pskviewer->rx_process(buf, len);
	if (evalpsk) evalpsk->sigdensity();
		
	delta = TWOPI * frequency / samplerate;
	
	while (len-- > 0) {
// Mix with the internal NCO
		z = complex ( *buf * cos(phaseacc), *buf * sin(phaseacc) );

		buf++;
		phaseacc += delta;
		if (phaseacc > M_PI)
			phaseacc -= TWOPI;

// Filter and downsample 
// by 16 (psk31, qpsk31) 
// by  8 (psk63, qpsk63)
// by  4 (psk125, qpsk125)
// by  2 (psk250, qpsk250)
// first filter
		if (fir1->run( z, z )) { // fir1 returns true every Nth sample
// final filter
			fir2->run( z, z2 ); // fir2 returns value on every sample
			calcSN_IMD(z);
			
//			fir3->run( z, z3);
//			coreafc(z3);
						
			int idx = (int) bitclk;
			double sum = 0.0;
			double ampsum = 0.0;
			syncbuf[idx] = 0.8 * syncbuf[idx] + 0.2 * z2.mag();
			
			for (int i = 0; i < 8; i++) {
				sum += (syncbuf[i] - syncbuf[i+8]);
				ampsum += (syncbuf[i] + syncbuf[i+8]);
			}
// added correction as per PocketDigi
			sum = (ampsum == 0 ? 0 : sum / ampsum);
			
			bitclk -= sum / 5.0;
			bitclk += 1;
			
			if (bitclk < 0) bitclk += 16.0;
			if (bitclk >= 16.0) {
				bitclk -= 16.0;
				rx_symbol(z2);
				update_syncscope();
				afc();
			}
		}
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
		else if ( E1/ E2 <= 1.0) { //(snratio <= 1.0) {
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
	double delta;
	double	ival, qval, shapeA, shapeB;
	complex symbol;

	if (_qpsk && !reverse)
		sym = (4 - sym) & 3;

	/* differential QPSK modulation - top bit flipped */
	switch (sym) {
	case 0:
		symbol = complex (-1.0, 0.0);	// 180 degrees
		break;
	case 1:
		symbol = complex (0.0, -1.0);	// 270 degrees
		break;
	case 2:
		symbol = complex (1.0, 0.0);		//   0 degrees
		break;
	case 3:
		symbol = complex (0.0, 1.0);		//  90 degrees
		break;
	}
	symbol = prevsymbol * symbol;	// complex multiplication

	delta = 2.0 * M_PI * get_txfreq_woffset() / samplerate;

	for (int i = 0; i < symbollen; i++) {
		
		shapeA = tx_shape[i];
		shapeB = (1.0 - shapeA);
		
		ival = shapeA * prevsymbol.real() + shapeB * symbol.real();
		qval = shapeA * prevsymbol.imag() + shapeB * symbol.imag();
		
		outbuf[i] = ival * cos(phaseacc) + qval * sin(phaseacc);
		
		phaseacc += delta;
		if (phaseacc > M_PI)
			phaseacc -= 2.0 * M_PI;
	}

	ModulateXmtr(outbuf, symbollen);

	prevsymbol = symbol;
}

void psk::tx_bit(int bit)
{
	unsigned int sym;

	if (_qpsk)
		sym = enc->encode(bit);
	else
		sym = bit << 1;

	tx_symbol(sym);
}

void psk::tx_char(unsigned char c)
{
	const char *code;

	code = psk_varicode_encode(c);

	while (*code) {
		tx_bit((*code - '0'));
		code++;
	}
	tx_bit(0);
	tx_bit(0);
}

void psk::tx_flush()
{
// flush the encoder (QPSK only)
	if (_qpsk) {
		for (int i = 0; i < dcdbits; i++)
		tx_bit(0);
	}

// DCD off sequence (unmodulated carrier)
	for (int i = 0; i < dcdbits; i++)
		tx_symbol(2);
}

int psk::tx_process()
{
	int c;

	if (preamble > 0) {
		preamble--;
		tx_symbol(0);	/* send phase reversals */
		return 0;
	}

	c = get_tx_char();

	if (c == 0x03 || stopflag) {
		tx_flush();
		stopflag = false;
		cwid();
		return -1;	/* we're done */
	}

	if (c == -1)
		tx_bit(0);
	else {
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
//  It is called with complex data samples at 500 Hz.
//============================================================================

void psk::calcSN_IMD(complex z)
{
	int i;
	complex temp;

	for(i = 0; i < NUM_FILTERS; i++) {
		temp.re = I1[i]; temp.im = Q1[i];
		I1[i] = I1[i] * COEF[i]- I2[i] + z.re;
		Q1[i] = Q1[i] * COEF[i]- Q2[i] + z.im;
		I2[i] = temp.re; Q2[i] = temp.im;
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
