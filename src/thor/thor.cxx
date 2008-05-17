//
// thor.cxx  --  thor modem
//
// Copyright (C) 2008
//		David Freese (w1hkj@w1hkj.com)
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#include <config.h>

#include <stdlib.h>

#include <iostream>

#include "confdialog.h"
#include "status.h"

#include "thor.h"
#include "trx.h"
#include "fft.h"
#include "filters.h"
#include "misc.h"
#include "sound.h"
#include "thorvaricode.h"

using namespace std;

char thormsg[80];

void thor::tx_init(SoundBase *sc)
{
	scard = sc;
	txstate = TX_STATE_PREAMBLE;
	txprevtone = 0;
	bitstate = 0;
	counter = 0;
	txphase = 0;
	videoText();
	strSecXmtText = progdefaults.THORsecText;
	if (strSecXmtText.length() == 0)
		strSecXmtText = "fldigi "PACKAGE_VERSION" ";
	cptr = 0;
}

void thor::rx_init()
{
	synccounter = 0;
	symcounter = 0;
	met1 = 0.0;
	met2 = 0.0;
	counter = 0;
	phase[0] = 0.0;
	currmag = prev1mag = prev2mag = 0.0;
	for (int i = 0; i < THORMAXFFTS; i++)
		phase[i+1] = 0.0;
	put_MODEstatus(mode);
	put_sec_char(0);
	syncfilter->reset();
	datashreg = 1;
}

void thor::reset_filters()
{
// fft filter at first IF frequency
	if (progdefaults.THOR_FILTER == false) {
		fft->create_filter( (THORFIRSTIF - 2.0 * bandwidth) / samplerate,
 		                    (THORFIRSTIF + 2.0 * bandwidth)/ samplerate );
	} else {
		fft->create_filter( (THORFIRSTIF - 0.5 * progdefaults.THOR_BW * bandwidth) / samplerate,
 		                    (THORFIRSTIF + 0.5 * progdefaults.THOR_BW * bandwidth)/ samplerate );
	}
	filter_reset = false;               
}

void thor::restart()
{
	filter_reset = true;
}

void thor::init()
{
	modem::init();
	reset_filters();
	rx_init();

	set_scope_mode(Digiscope::DOMDATA);
}

thor::~thor()
{
	if (hilbert) delete hilbert;
	
	for (int i = 0; i < THORMAXFFTS; i++) {
		if (binsfft[i]) delete binsfft[i];
	}

	for (int i = 0; i < THORSCOPESIZE; i++) {
		if (vidfilter[i]) delete vidfilter[i];
	}
	if (syncfilter) delete syncfilter;
	
	if (pipe) delete [] pipe;
	if (fft) delete fft;
	
	if (Rxinlv) delete Rxinlv;
	if (Txinlv) delete Txinlv;
	if (Dec) delete Dec;
	if (Enc) delete Enc;
	
}

thor::thor(trx_mode md)
{
	int basetone, lotone, hitone;
	
	mode = md;

	switch (mode) {
// 11.025 kHz modes
	case MODE_THOR5:
		symlen = 2048;
		doublespaced = 1;
		samplerate = 11025;
		break;

	case MODE_THOR11:
		symlen = 1024;
		doublespaced = 0;
		samplerate = 11025;
		break;

	case MODE_TSOR11:
		symlen = 1024;
		doublespaced = 1;
		samplerate = 11025;
		break;

	case MODE_THOR22:
		symlen = 512;
		doublespaced = 0;
		samplerate = 11025;
		break;
// 8kHz modes
	case MODE_THOR4:
		symlen = 2048;
		doublespaced = 1;
		samplerate = 8000;
		break;

	case MODE_THOR8:
		symlen = 1024;
		doublespaced = 1;
		samplerate = 8000;
		break;
	case MODE_THOR16:
	default:
		symlen = 512;
		doublespaced = 0;
		samplerate = 8000;
	}


	basetone = (int)floor(THORBASEFREQ * symlen / samplerate + 0.5);
	lotone = basetone - (THORNUMMTONES/2) * (doublespaced ? 2 : 1);
	hitone = basetone + 3 * (THORNUMMTONES/2) * (doublespaced ? 2 : 1);

	tonespacing = (double) (samplerate * ((doublespaced) ? 2 : 1)) / symlen;

	bandwidth = THORNUMMTONES * tonespacing;

	hilbert	= new C_FIR_filter();
	hilbert->init_hilbert(37, 1);

	paths = progdefaults.THOR_PATHS;

	for (int i = 0; i < THORMAXFFTS; i++)
		binsfft[i] = new sfft (symlen, lotone, hitone);

// fft filter at first if frequency
	fft = new fftfilt( (THORFIRSTIF - 0.5 * progdefaults.THOR_BW * bandwidth) / samplerate,
	                   (THORFIRSTIF + 0.5 * progdefaults.THOR_BW * bandwidth)/ samplerate,
	                   1024 );
	
	for (int i = 0; i < THORSCOPESIZE; i++)
		vidfilter[i] = new Cmovavg(16);
		
	syncfilter = new Cmovavg(8);
	
	twosym = 2 * symlen;
	pipe = new THORrxpipe[twosym];
	
	scopedata.alloc(THORSCOPESIZE);
	videodata.alloc((THORMAXFFTS * THORNUMMTONES * 2  * (doublespaced?2:1) ));

	pipeptr = 0;
	
	symcounter = 0;
	metric = 0.0;

	fragmentsize = symlen;

	s2n = 0.0;

	prev1symbol = prev2symbol = 0;

	Enc	= new encoder (THOR_K, THOR_POLY1, THOR_POLY2);
	Dec	= new viterbi (THOR_K, THOR_POLY1, THOR_POLY2);
	Dec->settraceback (45);
	Dec->setchunksize (1);
	Txinlv = new interleave (4, INTERLEAVE_FWD); // 4x4x10
	Rxinlv = new interleave (4, INTERLEAVE_REV); // 4x4x10
	bitstate = 0;
	symbolpair[0] = symbolpair[1] = 0;
	datashreg = 1;

	init();
}

//=====================================================================
// rx modules

complex thor::mixer(int n, complex in)
{
	complex z;
	double f;
// first IF mixer (n == 0) plus
// THORMAXFFTS mixers are supported each separated by 1/THORMAXFFTS bin size
// n == 1, 2, 3, 4 ... THORMAXFFTS
	if (n == 0)
		f = frequency - THORFIRSTIF;
	else
		f = THORFIRSTIF - THORBASEFREQ - bandwidth/2 + (samplerate / symlen) * (1.0 * n / paths );
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

void thor::recvchar(int c)
{
	if (c == -1)
		return;
	if (c & 0x100)
		put_sec_char(c & 0xFF);
	else
		put_rx_char(c & 0xFF);
}

//=============================================================================
// Receive
//=============================================================================

void thor::decodePairs(unsigned char symbol)
{
	int c, ch, met;

	symbolpair[0] = symbolpair[1];
	symbolpair[1] = symbol;

	symcounter = symcounter ? 0 : 1;
	
	if (symcounter) return;

	c = Dec->decode (symbolpair, &met);

	if (c == -1)
		return;

	if (progStatus.sqlonoff && metric < progStatus.sldrSquelchValue)
		return;

	datashreg = (datashreg << 1) | !!c;
	if ((datashreg & 7) == 1) {
		ch = thorvaridec(datashreg >> 1);
		recvchar(ch);
		datashreg = 1;
	}
}

void thor::decodeEX(int ch)
{
	unsigned char symbols[4];
	int c = ch;
	
	for (int i = 0; i < 4; i++) {
		if ((c & 1) == 1) symbols[3-i] = 255;
		else symbols[3-i] = 1;
		c = c / 2;
	}

	Rxinlv->symbols(symbols);

	for (int i = 0; i < 4; i++) decodePairs(symbols[i]);

}

void thor::decodesymbol()
{
	int c;
	double fdiff;

// Decode the IFK+ sequence, which results in a single nibble

	fdiff = currsymbol - prev1symbol;
	if (reverse) fdiff = -fdiff;
	if (doublespaced) fdiff /= 2 * paths;
	else              fdiff /= paths;
	c = (int)floor(fdiff + .5) - 2;
	if (c < 0) c += THORNUMMTONES;

//	decodeEX(c);

	unsigned char symbols[4];
	double avg = (currmag + prev1mag + prev2mag) / 3.0;
	if (avg == 0.0) avg = 1e-20;
	double softmag = currmag / avg;
		
	for (int i = 0; i < 4; i++) {
// hard symbol decode
		if (progdefaults.THOR_SOFT == false) {
			if ((c & 1) == 1) symbols[3-i] = 255;
			else symbols[3-i] = 1;
// soft symbol decode
		} else
			symbols[3-i] = (unsigned char)clamp(256.0 * (c & 1) * softmag, 1, 255);
		
		c = c / 2;
	}

	Rxinlv->symbols(symbols);

	for (int i = 0; i < 4; i++) decodePairs(symbols[i]);
	
}

int thor::harddecode()
{
	double x, max = 0.0;
	int symbol = 0;
	for (int i = 0; i <  (paths * THORNUMMTONES * 2  * (doublespaced ? 2 : 1) ); i++) {
		x = pipe[pipeptr].vector[i].mag();
		if (x > max) {
			max = x;
			symbol = i;
		}
	}
	return symbol;
}

void thor::update_syncscope()
{

	double max = 0, min = 1e6, range, mag;

// dom waterfall
	memset(videodata, 0, (paths * THORNUMMTONES * 2  * (doublespaced?2:1) ) * sizeof(double));

	if (!progStatus.sqlonoff || metric >= progStatus.sldrSquelchValue) {
		for (int i = 0; i < (paths * THORNUMMTONES * 2  * (doublespaced?2:1) ); i++ ) {
			mag = pipe[pipeptr].vector[i].mag();
			if (max < mag) max = mag;
			if (min > mag) min = mag;
		}
		range = max - min;
		for (int i = 0; i < (paths * THORNUMMTONES * 2  * (doublespaced?2:1) ); i++ ) {
			if (range > 2) {
				mag = (pipe[pipeptr].vector[i].mag() - min) / range + 0.0001;
				mag = 1 + 2 * log10(mag);
				if (mag < 0) mag = 0;
			} else
				mag = 0;
			videodata[i] = 255*mag;
		}
	}
	set_video(videodata, (paths * THORNUMMTONES * 2  * (doublespaced?2:1) ), false);
	videodata.next();

//	set_scope(scopedata, twosym);
// 64 data points is sufficient to show the signal progression through the
// convolution filter.
	memset(scopedata, 0, THORSCOPESIZE * sizeof(double));
	if (!progStatus.sqlonoff || metric >= progStatus.sldrSquelchValue) {
		for (unsigned int i = 0, j = 0; i < THORSCOPESIZE; i++) {
			j = (pipeptr + i * twosym / THORSCOPESIZE + 1) % (twosym);
			scopedata[i] = vidfilter[i]->run(pipe[j].vector[prev1symbol].mag());
		}
	}
	set_scope(scopedata, THORSCOPESIZE);
	scopedata.next();
}

void thor::synchronize()
{
//	int syn = -1;
	double syn = -1;
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
	syn = syncfilter->run(syn);
	
	synccounter += (int) floor(1.0 * (syn - symlen) / THORNUMMTONES + 0.5);
}


void thor::eval_s2n()
{
	if (currsymbol != prev1symbol && prev1symbol != prev2symbol) {
		sig = pipe[pipeptr].vector[currsymbol].mag();
		noise = 0.0;
		for (int i = 0; i < paths * THORNUMMTONES * 2  * (doublespaced?2:1); i++) {
			if (i != currsymbol)
				noise += pipe[pipeptr].vector[i].mag();
		}	
		noise /= (paths * THORNUMMTONES * 2  * (doublespaced?2:1) - 1);
	
		if (noise)
			s2n = decayavg( s2n, sig / noise, 8);

		metric = 3*(20*log10(s2n) - 9.0);

		display_metric(metric);

		snprintf(thormsg, sizeof(thormsg), "s/n %3.0f dB", metric / 3.0 - 2.0);
		put_Status1(thormsg);
	}
}

int thor::rx_process(const double *buf, int len)
{
	complex zref,  z, *zp, *bins = 0;
	int n;

	if (filter_reset) reset_filters();

	if (paths != progdefaults.THOR_PATHS) {
		paths = progdefaults.THOR_PATHS;
		reset_filters();
	}
	
	while (len) {
// create analytic signal at first IF
		zref.re = zref.im = *buf++;
		hilbert->run(zref, zref);
		zref = mixer(0, zref);
// filter using fft convolution
		n = fft->run(zref, &zp);
		
		if (n) {
			for (int i = 0; i < n; i++) {
// process THORMAXFFTS sets of sliding FFTs spaced at 1/THORMAXFFTS bin intervals each of which
// is a matched filter for the current symbol length
				for (int n = 0; n < paths; n++) {
// shift in frequency to base band for the sliding DFTs
					z = mixer(n + 1, zp[i]);
					bins = binsfft[n]->run(z);
// copy current vector to the pipe interleaving the FFT vectors
					for (int i = 0; i < THORNUMMTONES * 2 * (doublespaced ? 2 : 1); i++) {
						pipe[pipeptr].vector[n + paths * i] = bins[i];
					}
				}
				if (--synccounter <= 0) {
					synccounter = symlen;
					currsymbol = harddecode();
					currmag = pipe[pipeptr].vector[currsymbol].mag();
					eval_s2n();
        		    decodesymbol();
					synchronize();
					update_syncscope();
					prev2symbol = prev1symbol;
					prev1symbol = currsymbol;
					prev2mag = prev1mag;
					prev1mag = currmag;
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

//=============================================================================
// Transmit methods
//=============================================================================

int thor::get_secondary_char()
{
	char chr;
	if (cptr > strSecXmtText.length()) cptr = 0;
	chr = strSecXmtText[cptr++];
	put_sec_char( chr );
	return chr;
}

void thor::sendtone(int tone, int duration)
{
	double f, phaseincr;
	f = (tone + 0.5) * tonespacing + get_txfreq_woffset() - bandwidth / 2;
	phaseincr = twopi * f / samplerate;
	for (int j = 0; j < duration; j++) {
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
}

void thor::sendsymbol(int sym)
{
	complex z;
    int tone;
	
	tone = (txprevtone + 2 + sym) % THORNUMMTONES;
    txprevtone = tone;
	if (reverse)
		tone = (THORNUMMTONES - 1) - tone;
	sendtone(tone, 1);
}

// Send THOR FEC varicode

void thor::sendchar(unsigned char c, int secondary)
{
	const char *code;

	code = thorvarienc(c, secondary);
	
	while (*code) {
		int data = Enc->encode(*code++ - '0');
		for (int i = 0; i < 2; i++) {
			bitshreg = (bitshreg << 1) | ((data >> i) & 1);
			bitstate++;
			if (bitstate == 4) {
				Txinlv->bits(&bitshreg);
				sendsymbol(bitshreg);
				bitstate = 0;
				bitshreg = 0;
			}
		}
	}
	if (!secondary)
		put_echo_char(c);
}

void thor::sendidle()
{
	sendchar(0, 0);	// <NUL>
}

void thor::sendsecondary()
{
	int c = get_secondary_char();
	sendchar(c & 0xFF, 1);
}

void thor::Clearbits()
{
	int data = Enc->encode(0);
	for (int k = 0; k < 100; k++) {
		for (int i = 0; i < 2; i++) {
			bitshreg = (bitshreg << 1) | ((data >> i) & 1);
			bitstate++;
			if (bitstate == 4) {
				Txinlv->bits(&bitshreg);
				bitstate = 0;
				bitshreg = 0;
			}
		}
	}
}

void thor::flushtx()
{
// flush the varicode decoder at the other end
// flush the convolutional encoder and interleaver
    for (int i = 0; i < 4; i++)
  	    sendidle();
	bitstate = 0;
}

int thor::tx_process()
{
	int i;

	switch (txstate) {
	case TX_STATE_PREAMBLE:
		Clearbits();
		for (int j = 0; j < 16; j++) sendsymbol(0);
//		sendtone(THORNUMMTONES/2, 4);
//		for (int k = 0; k < 3; k++) {
//			sendtone(THORNUMMTONES, 3);
//			sendtone(0, 3);
//		}
//		sendtone(THORNUMMTONES/2, 4);

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

