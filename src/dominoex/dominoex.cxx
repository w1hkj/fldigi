//
// dominoex.cxx  --  DominoEX modem
//
// Copyright (C) 2008
//		David Freese (w1hkj@w1hkj.com)
// based on code in gmfsk
//	Copyright (C) 2006
//	Hamish Moffatt (hamish@debian.org)
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#include <config.h>

#include <stdlib.h>

#include <iostream>

#include "confdialog.h"
#include "status.h"

#include "dominoex.h"
#include "trx.h"
#include "fft.h"
#include "filters.h"
#include "misc.h"
#include "sound.h"

using namespace std;

char dommsg[80];

void dominoex::tx_init(SoundBase *sc)
{
	scard = sc;
	txstate = TX_STATE_PREAMBLE;
	txprevtone = 0;
	counter = 0;
	txphase = 0;
	videoText();
}

void dominoex::rx_init()
{
	synccounter = 0;
	symcounter = 0;
	met1 = 0.0;
	met2 = 0.0;
	counter = 0;
	phase[0] = 0.0;
	for (int i = 0; i < NUMFFTS; i++)
		phase[i+1] = 0.0;
	put_MODEstatus(mode);
	put_sec_char(0);
}

void dominoex::reset_filters()
{
// fft filter at first IF frequency
	if (progdefaults.DOMINOEX_FILTER == false) {
		fft->create_filter( (FIRSTIF - 2.0 * bandwidth) / samplerate,
 		                    (FIRSTIF + 2.0 * bandwidth)/ samplerate );
	} else {
		fft->create_filter( (FIRSTIF - 0.5 * progdefaults.DOMINOEX_BW * bandwidth) / samplerate,
 		                    (FIRSTIF + 0.5 * progdefaults.DOMINOEX_BW * bandwidth)/ samplerate );
	}
	filter_reset = false;               
}

void dominoex::restart()
{
	filter_reset = true;
}

void dominoex::init()
{
	modem::init();
	reset_filters();
	rx_init();

	strSecXmtText = txtSecondary->value();
	if (strSecXmtText.length() == 0)
		strSecXmtText = "fldigi "PACKAGE_VERSION" ";

	set_scope_mode(Digiscope::DOMDATA);
}

dominoex::~dominoex()
{
	if (hilbert) delete hilbert;
	
	for (int i = 0; i < NUMFFTS; i++) {
		if (binsfft[i]) delete binsfft[i];
	}

	for (int i = 0; i < SCOPESIZE; i++) {
		if (vidfilter[i]) delete vidfilter[i];
	}
	
	if (pipe) delete [] pipe;
	if (fft) delete fft;
}

dominoex::dominoex(trx_mode md)
{
	int basetone, lotone, hitone;
	
	mode = md;

	switch (mode) {
// 11.025 kHz modes
	case MODE_DOMINOEX5:
		symlen = 2048;
		doublespaced = 1;
		samplerate = 11025;
		break;

	case MODE_DOMINOEX11:
		symlen = 1024;
		doublespaced = 0;
		samplerate = 11025;
		break;

	case MODE_DOMINOEX22:
		symlen = 512;
		doublespaced = 0;
		samplerate = 11025;
		break;
// 8kHz modes
	case MODE_DOMINOEX4:
		symlen = 2048;
		doublespaced = 1;
		samplerate = 8000;
		break;

	case MODE_DOMINOEX16:
		symlen = 512;
		doublespaced = 0;
		samplerate = 8000;
		break;

	case MODE_DOMINOEX8:
	default:
		symlen = 1024;
		doublespaced = 1;
		samplerate = 8000;
	}

	basetone = (int)floor(BASEFREQ * symlen / samplerate + 0.5);
	lotone = basetone - (NUMTONES/2) * (doublespaced ? 2 : 1);
	hitone = basetone + 3 * (NUMTONES/2) * (doublespaced ? 2 : 1);

	tonespacing = (double) (samplerate * ((doublespaced) ? 2 : 1)) / symlen;

	bandwidth = NUMTONES * tonespacing;

	hilbert	= new C_FIR_filter();
	hilbert->init_hilbert(37, 1);

	for (int i = 0; i < NUMFFTS; i++) {
		binsfft[i] = new sfft (symlen, lotone, hitone);
	}

// fft filter at first if frequency
	fft = new fftfilt( (FIRSTIF - 0.5 * progdefaults.DOMINOEX_BW * bandwidth) / samplerate,
	                   (FIRSTIF + 0.5 * progdefaults.DOMINOEX_BW * bandwidth)/ samplerate,
	                   1024 );

	for (int i = 0; i < SCOPESIZE; i++)
		vidfilter[i] = new Cmovavg(16);
		
	twosym = 2 * symlen;
	pipe = new domrxpipe[twosym];
	
	scopedata.alloc(SCOPESIZE);
	videodata.alloc((NUMFFTS * NUMTONES * 2  * (doublespaced?2:1) ));

	pipeptr = 0;
	
	symcounter = 0;
	metric = 0.0;

	fragmentsize = symlen;

	s2n = 0.0;

	prev1symbol = prev2symbol = 0;

	init();
}

//=====================================================================
// rx modules

complex dominoex::mixer(int n, complex in)
{
	complex z;
	double f;
// first IF mixer (n == 0) plus
// NUMFFTS mixers are supported each separated by 1/NUMFFTS bin size
// n == 1, 2, 3, 4 ... NUMFFTS
	if (n == 0)
		f = frequency - FIRSTIF;
	else
		f = FIRSTIF - BASEFREQ - bandwidth/2 + (samplerate / symlen) * (1.0 * n / NUMFFTS);
	z.re = cos(phase[n]);
	z.im = sin(phase[n]);
	z = z * in;
	phase[n] -= twopi * f / samplerate;
	if (phase[n] > M_PI)
		phase[n] -= twopi;
	else if (phase[n] < M_PI)
		phase[n] += twopi;
	return z;
}

void dominoex::recvchar(int c)
{
	if (c == -1)
		return;
	if (c & 0x100)
		put_sec_char(c & 0xFF);
	else
		put_rx_char(c & 0xFF);
}

void dominoex::decodesymbol()
{
	int c, sym, ch;
	double fdiff;

// Decode the IFK+ sequence, which results in a single nibble

	fdiff = currsymbol - prev1symbol;
	if (reverse) fdiff = -fdiff;
	if (doublespaced) fdiff /= 2 * NUMFFTS;
	else              fdiff /= NUMFFTS;
	c = (int)floor(fdiff + .5) - 2;
	if (c < 0) c += NUMTONES;
	
//	If the new symbol is the start of a new character (MSB is low), complete the previous character
	if (!(c & 0x8)) {
		if (symcounter <= MAX_VARICODE_LEN) {
			sym = 0;
			for (int i = 0; i < symcounter; i++)
				sym |= symbolbuf[i] << (4 * i);
			ch = dominoex_varidec(sym);
			if (!progStatus.sqlonoff || metric > progStatus.sldrSquelchValue)		
				recvchar(ch);
		}
		symcounter = 0;
	}

// Add to the symbol buffer. Position 0 is the newest symbol.
	for (int i = MAX_VARICODE_LEN-1; i > 0; i--)
		symbolbuf[i] = symbolbuf[i-1];
	symbolbuf[0] = c;

// Increment the counter, but clamp at max+1 to avoid overflow
	symcounter++;
	if (symcounter > MAX_VARICODE_LEN + 1)
		symcounter = MAX_VARICODE_LEN + 1;

}

int dominoex::harddecode()
{
	double x, max = 0.0;
	int symbol = 0;
	for (int i = 0; i <  (NUMFFTS * NUMTONES * 2  * (doublespaced?2:1) ); i++) {
		x = pipe[pipeptr].vector[i].mag();
		if (x > max) {
			max = x;
			symbol = i;
		}
	}
	return symbol;
}

void dominoex::update_syncscope()
{

	double max = 0, min = 1e6, range, mag;

// dom waterfall
	memset(videodata, 0, (NUMFFTS * NUMTONES * 2  * (doublespaced?2:1) ) * sizeof(double));

	if (!progStatus.sqlonoff || metric >= progStatus.sldrSquelchValue) {
		for (int i = 0; i < (NUMFFTS * NUMTONES * 2  * (doublespaced?2:1) ); i++ ) {
			mag = pipe[pipeptr].vector[i].mag();
			if (max < mag) max = mag;
			if (min > mag) min = mag;
		}
		range = max - min;
		for (int i = 0; i < (NUMFFTS * NUMTONES * 2  * (doublespaced?2:1) ); i++ ) {
			if (range > 2) {
				mag = (pipe[pipeptr].vector[i].mag() - min) / range + 0.0001;
				mag = 1 + 2 * log10(mag);
				if (mag < 0) mag = 0;
			} else
				mag = 0;
			videodata[i] = 255*mag;
		}
	}
	set_video(videodata, (NUMFFTS * NUMTONES * 2  * (doublespaced?2:1) ), false);
	videodata.next();

//	set_scope(scopedata, twosym);
// 64 data points is sufficient to show the signal progression through the
// convolution filter.
	memset(scopedata, 0, SCOPESIZE * sizeof(double));
	if (!progStatus.sqlonoff || metric >= progStatus.sldrSquelchValue) {
		for (unsigned int i = 0, j = 0; i < SCOPESIZE; i++) {
			j = (pipeptr + i * twosym / SCOPESIZE + 1) % (twosym);
			scopedata[i] = vidfilter[i]->run(pipe[j].vector[prev1symbol].mag());
//			scopedata[i] = pipe[j].vector[prev1symbol].mag();
		}
	}
	set_scope(scopedata, SCOPESIZE);
	scopedata.next();
}

void dominoex::synchronize()
{
	int syn = -1;
	double val, max = 0.0;

	if (currsymbol == prev1symbol)
		return;
	if (prev1symbol == prev2symbol)
		return;

	for (unsigned int i = 0, j = pipeptr; i < twosym; i++) {
		val = (pipe[j].vector[prev1symbol]).mag();
		if (val > max) {
			max = val;
			syn = i;
		}
		j = (j + 1) % twosym;
	}
	synccounter += (int) floor(1.0 * (syn - symlen) / NUMTONES + 0.5);
}


void dominoex::eval_s2n()
{
	if (currsymbol != prev1symbol && prev1symbol != prev2symbol) {
		sig = pipe[pipeptr].vector[currsymbol].mag();
		noise = 0.0;
		for (int i = 0; i < NUMFFTS * NUMTONES * 2  * (doublespaced?2:1); i++) {
			if (i != currsymbol)
				noise += pipe[pipeptr].vector[i].mag();
		}	
		noise /= (NUMFFTS * NUMTONES * 2  * (doublespaced?2:1) - 1);
	
		s2n = decayavg( s2n, sig / noise, 8);

		metric = 3*(20*log10(s2n) - 9.0);

		display_metric(metric);

		snprintf(dommsg, sizeof(dommsg), "s/n %3.0f dB", metric / 3.0);
		put_Status1(dommsg);
	}
}

int dominoex::rx_process(const double *buf, int len)
{
	complex zref,  z, *zp, *bins;
	int n;

	if (filter_reset) reset_filters();
	
	while (len) {
// create analytic signal at first IF
		zref.re = zref.im = *buf++;
		hilbert->run(zref, zref);
		zref = mixer(0, zref);
// filter using fft convolution
		n = fft->run(zref, &zp);
		
		if (n) {
			for (int i = 0; i < n; i++) {
// process NUMFFTS sets of sliding FFTs spaced at 1/NUMFFTS bin intervals each of which
// is a matched filter for the current symbol length
				for (int n = 0; n < NUMFFTS; n++) {
// shift in frequency to base band for the sliding FFTs
					z = mixer(n + 1, zp[i]);
					bins = binsfft[n]->run(z);
// copy current vector to the pipe interleaving the FFT vectors
					for (int i = 0; i < NUMTONES * 2 * (doublespaced ? 2 : 1); i++) {
						pipe[pipeptr].vector[n + NUMFFTS * i] = bins[i];
					}
				}
				if (--synccounter <= 0) {
					synccounter = symlen;
					currsymbol = harddecode();
        		    decodesymbol();
					synchronize();
					update_syncscope();
					eval_s2n();
					prev2symbol = prev1symbol;
					prev1symbol = currsymbol;
				}
				pipeptr++;
				if (pipeptr >= twosym)
					pipeptr = 0;
			}
		}
		--len;
	}
			
	return 0;
}

//=====================================================================
// dominoex tx modules

int dominoex::get_secondary_char()
{
	static unsigned int cptr = 0;
	char chr;
	if (cptr > strSecXmtText.length()) cptr = 0;
	chr = strSecXmtText[cptr++];
	put_sec_char( chr );
	return chr;
}

void dominoex::sendsymbol(int sym)
{
//static int first = 0;
	complex z;
    int tone;
	double f, phaseincr;
	
	tone = (txprevtone + 2 + sym) % NUMTONES;
    txprevtone = tone;
	if (reverse)
		tone = (NUMTONES - 1) - tone;

	f = tone * tonespacing + get_txfreq_woffset() - bandwidth / 2;
	
	phaseincr = twopi * f / samplerate;
	
	for (int i = 0; i < symlen; i++) {
		outbuf[i] = cos(txphase);
		txphase -= phaseincr;
		if (txphase > M_PI)
			txphase -= twopi;
		else if (txphase < M_PI)
			txphase += twopi;
	}
	ModulateXmtr(outbuf, symlen);

}


void dominoex::sendchar(unsigned char c, int secondary)
{
	unsigned char *code = dominoex_varienc(c, secondary);

    sendsymbol(code[0]);
// Continuation nibbles all have the MSB set
    for (int sym = 1; sym < 3; sym++) {
        if (code[sym] & 0x8) 
            sendsymbol(code[sym]);
        else
            break;
    }
	if (!secondary)
		put_echo_char(c);
}

void dominoex::sendidle()
{
	sendchar(0, 1);	// <NUL>
}

void dominoex::sendsecondary()
{
	int c = get_secondary_char();
	sendchar(c & 0xFF, 1);
}

void dominoex::flushtx()
{
// flush the varicode decoder at the receiver end
    for (int i = 0; i < 4; i++)
        sendidle();
}

int dominoex::tx_process()
{
	int i;

	switch (txstate) {
	case TX_STATE_PREAMBLE:
        sendidle();
		txstate = TX_STATE_START;
		break;
	case TX_STATE_START:
		sendchar('\r', 0);
		sendchar(2, 0);		// STX
		sendchar('\r', 0);
		txstate = TX_STATE_DATA;
		break;
	case TX_STATE_DATA:
		i = get_tx_char();
		if (i == -1)
			sendsecondary();
		else if (i == 3)
			txstate = TX_STATE_END;
		else
			sendchar(i, 0);
		if (stopflag)
			txstate = TX_STATE_END;
		break;
	case TX_STATE_END:
		sendchar('\r', 0);
		sendchar(4, 0);		// EOT
		sendchar('\r', 0);
		txstate = TX_STATE_FLUSH;
		break;
	case TX_STATE_FLUSH:
		flushtx();
		cwid();
		return -1;
	}
	return 0;
}
