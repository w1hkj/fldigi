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


#include <stdlib.h>
#include <stdio.h>

#include "psk.h"
#include "waterfall.h"
#include "configuration.h"

extern waterfall *wf;

//=====================================================================
// Change the following constant to adjust the # of DCD symbols 
// sent at the end of every transmission.  The DCD symbol is 0x02.
// 32 is the # used in gMFSK and earlier versions of fldigi

#define DCDOFF  32

//=====================================================================


#define	K		5
#define	POLY1	0x17
#define	POLY2	0x19

char pskmsg[80];

void psk::tx_init(cSound *sc)
{
	scard = sc;
	phaseacc = 0;
	prevsymbol = complex (1.0, 0.0);
	preamble = dcdbits;
	shreg = 0;
	if (trx_state != STATE_TUNE && progdefaults.sendid == true)
		wfid->transmit(mode);
	else if (trx_state != STATE_TUNE && progdefaults.macroid == true) {
		wfid->transmit(mode);
		progdefaults.macroid = false;
	}
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
	if (mailserver) sigsearch = 3;
	digiscope->mode(Digiscope::PHASE);
	put_MODEstatus(mode);
}

void psk::restart()
{
//	reverse = false;
}

void psk::init()
{
	modem::init();
	restart();
	rx_init();
}

psk::~psk()
{
	if (tx_shape) delete [] tx_shape;
	if (enc) delete enc;
	if (dec) delete dec;
	if (fir1) delete fir1;
	if (fir2) delete fir2;
	if (wfid) delete wfid;
}

psk::psk(trx_mode pskmode) : modem()
{
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
		break;
//	case MODE_PSK256:
//		symbollen = 32;
//		_qpsk = false;
//		dcdbits = 256;
//		break;
//	case MODE_QPSK256:
//		symbollen = 32;
//		_qpsk = true;
//		dcdbits = 256;
//		break;
	default:
		mode = MODE_BPSK31;
		symbollen = 256;
		_qpsk = false;
		dcdbits = 32;
	}
	fir1 = fir2 = (C_FIR_filter *)0;
	enc = (encoder *)0;
	dec = (viterbi *)0;
	
// create impulse response for experimental FIR filters
	fir1c = new double[64];
	fir2c = new double[64];

//	raisedcosfilt(fir1c);	// creates fir1c

	wsincfilt(fir1c, 1.0 / symbollen);		// creates fir1c matched sin(x)/x filter

	wsincfilt(fir2c, 1.0 / 16.0);				// creates fir2c matched sin(x)/x filter

	fir1 = new C_FIR_filter();
	fir1->init(FIRLEN, symbollen / 16, fir1c, fir1c);

	fir2 = new C_FIR_filter();
	fir2->init(FIRLEN,1, fir2c, fir2c);

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
	wfid = new id(this);
	
	pipeptr = 0;
	if (mailserver)
		sigsearch = 3;
	else
		sigsearch = 0;
	for (int i = 0; i < 16; i++)
		syncbuf[i] = 0.0;
	E1 = E2 = E3 = 0.0;
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
	double minfreq = bandwidth * 4;
	double spwr, npwr;
	while (srchfreq > minfreq) {
		spwr = wf->powerDensity(srchfreq, bandwidth/2);
		npwr = wf->powerDensity(srchfreq + bandwidth, bandwidth/2) + 1e-10;
		if (spwr / npwr > 4.0) {
			frequency = srchfreq;
			set_freq(frequency);
			sigsearch = 5;
			break;
		}
		srchfreq -= bandwidth/4;
	}
}

void psk::searchUp()
{
	double srchfreq = frequency + bandwidth * 2;
	double maxfreq = IMAGE_WIDTH - bandwidth * 4;
	double spwr, npwr;
	while (srchfreq < maxfreq) {
		spwr = wf->powerDensity(srchfreq, bandwidth/2);
		npwr = wf->powerDensity(srchfreq - bandwidth, bandwidth/2) + 1e-10;
		if (spwr / npwr > 4.0) {
			frequency = srchfreq;
			set_freq(frequency);
			sigsearch = 5;
			break;
		}
		srchfreq += bandwidth/4;
	}
}

//static char phasemsg[50];

void psk::afc()
{
	double error, ftest, sigpwr, noise;
	if (mailserver)
		ftest = wf->peakFreq((int)progdefaults.PSKsweetspot, (int) bandwidth);
	else
		ftest = wf->peakFreq((int)frequency, (int)(bandwidth) );
	sigpwr = wf->powerDensity(ftest,  bandwidth);
	noise = wf->powerDensity(ftest + 3 * bandwidth / 2, bandwidth);
// fast search for peak signal frequency		
	if (sigsearch) {
		freqerr = 0.0;
		if (sigpwr/noise > 2.0) {//afcthreshold) {
			if (!mailserver || (mailserver && (fabs(progdefaults.PSKsweetspot - ftest) < 15))) {
				frequency = ftest;
				set_freq(frequency);
				sigsearch--;
			}
		}
		else
			if (!mailserver)
				sigsearch = 0;
	} 
// continuous AFC based on phase error		
	else if (dcd == true) {
		error = (phase - bits * M_PI / 2);
		if (error < M_PI / 2)
			error += 2 * M_PI;
		if (error > M_PI / 2)
			error -= 2 * M_PI;
		error *= ((samplerate / (symbollen * 2 * M_PI)/16));
		freqerr = decayavg( freqerr, error, 8);//32);
		frequency -= freqerr;
		set_freq (frequency);
//sprintf(phasemsg,"%5.4f  %8.2f", freqerr, frequency);
//put_status(phasemsg);
	} else if (mailserver)
		sigsearch = 3;
}

void psk::rx_symbol(complex symbol)
{
//	double phase, error;
//	int bits, n;
	int n;
	phase = (prevsymbol % symbol).arg();
	prevsymbol = symbol;

	if (phase < 0) 
		phase += 2 * M_PI;
	if (_qpsk) {
		bits = ((int) (phase / M_PI_2 + 0.5)) & 3;
		n = 4;
	} else {
		bits = (((int) (phase / M_PI + 0.5)) & 1) << 1;
		n = 2;
	}
// simple low pass filter for quality of signal
	quality.re = 0.02 * cos(n * phase) + 0.98 * quality.re;
	quality.im = 0.02 * sin(n * phase) + 0.98 * quality.im;

	metric = 100.0 * quality.norm();
	
	dcdshreg = (dcdshreg << 2) | bits;

	switch (dcdshreg) {
	case 0xAAAAAAAA:	/* DCD on by preamble */
		dcd = true;
		quality = complex (1.0, 0.0);
		break;

	case 0:			/* DCD off by postamble */
		dcd = false;
		quality = complex (0.0, 0.0);
		break;

	default:
		if (metric > squelch)// && snratio > 0.0)
			dcd = true;
		else dcd = false;
	}

	if (squelchon == false)
		set_phase(phase, true);
	else if (metric > squelch)
		set_phase(phase, true);
	else
		set_phase(M_PI, false);

	if (dcd == true || squelchon == false) {
		if (_qpsk)
			rx_qpsk(bits);
		else
			rx_bit(!bits);
	}
	
	if (afcon == true)
		afc();
}

void psk::update_syncscope()
{
	static char msg1[15];
	static char msg2[15];
	double bw = bandwidth / 2;
	double sp = bandwidth / 4;
	
	E1 = decayavg(	E1, 
					wf->powerDensity(frequency - bw,  sp) +
					wf->powerDensity(frequency + bw, sp), 
					64);
	E2 = decayavg(	E2, 
					wf->powerDensity(frequency - 2 * bw, sp) +
					wf->powerDensity(frequency + 2 * bw, sp), 
					64);
	E3 = decayavg(	E3, 
					wf->powerDensity(frequency - 3 * bw, sp) +
					wf->powerDensity(frequency + 3 * bw, sp), 
					64);
	snratio = (E1/(E2 + 1e-10));
	imdratio = (E3/(E1 + 1e-10));
	
	s2n = 10.0*log10( snratio );
	display_metric(metric);

	sprintf(msg1, "s/n %2d dB", (int)(floor(s2n))); put_Status1(msg1);

	if (imdratio < 1)
		imd = 10.0*log10( imdratio );
	else
		imd = 0.0;
	sprintf(msg2, "imd %3d dB", (int)(floor(imd))); put_Status2(msg2);

}



int psk::rx_process(double *buf, int len)
{
	double delta;
	complex z;

	delta = 2.0 * M_PI * frequency / samplerate;

	while (len-- > 0) {
// Mix with the internal NCO
		z = complex ( *buf * cos(phaseacc), *buf * sin(phaseacc) );
		buf++;
		phaseacc += delta;
		if (phaseacc > M_PI)
			phaseacc -= 2.0 * M_PI;
// Filter and downsample 
// by 16 (psk31, qpsk31) 
// by  8 (psk63, qpsk63)
// by  4 (psk125, qpsk125)
// first filter
		if (fir1->run( z, z )) { // fir1 returns true every Nth sample
// final filter
			fir2->run( z, z ); // fir3 returns value on every sample
			
// Now the sync correction routine...
// save amplitude value for the sync scope
			int idx = (int) bitclk;
			double sum = 0.0;

//			scope_pipe[pipeptr % PipeLen] = syncbuf[idx] = z.mag();
			scope_pipe[pipeptr] = syncbuf[idx] = z.mag();
			
			for (int i = 0; i < 8; i++)
				sum += (syncbuf[i] - syncbuf[i+8]);

			bitclk -= sum / 5.0;
			bitclk += 1;
			if (bitclk < 0) bitclk += 16;
			if (bitclk >= 16) {
				bitclk -= 16;
				rx_symbol(z);
				update_syncscope();
			}
			pipeptr = (pipeptr + 1) % PipeLen;
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

	delta = 2.0 * M_PI * tx_frequency / samplerate;

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
	char *code;

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
	for (int i = 0; i < DCDOFF; i++)
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

// end of transmission == 0x03 <ETX>
	if (c == 0x03 || stopflag) {
		tx_flush();
		stopflag = false;
		return -1;	/* we're done */
	}

	if (!c)
		tx_bit(0);
	else {
		tx_char(c);
		put_echo_char(c);
	}
	return 0;
}


