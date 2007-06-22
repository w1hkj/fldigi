//
//    dominoex.cxx  --  DominoEX modem
//
//	Copyright (C) 2001, 2002, 2003
//	Tomi Manninen (oh2bns@sral.fi)
//	Copyright (C) 2006
//	Hamish Moffatt (hamish@debian.org)
//	Copyright (C) 2006
//		David Freese (w1hkj@w1hkj.com)
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

#include <stdlib.h>

#include <iostream>

#include "version.h"
#include "Config.h"

#include "dominoex.h"
#include "trx.h"
#include "fft.h"
#include "filters.h"
#include "misc.h"
#include "sound.h"

using namespace std;

#define AFC_COUNT	32

char dommsg[80];

void dominoex::tx_init(cSound *sc)
{
	scard = sc;
	txstate = TX_STATE_PREAMBLE;
	txprevtone = 0;
	counter = 0;
	phaseacc = 0.0;
	if (trx_state != STATE_TUNE && progdefaults.sendid == true)
		wfid->transmit(mode);
	else if (trx_state != STATE_TUNE && progdefaults.macroid == true) {
		wfid->transmit(mode);
		progdefaults.macroid = false;
	}
}

void dominoex::rx_init()
{
	synccounter = 0;
	symcounter = 0;
	met1 = 0.0;
	met2 = 0.0;
	counter = 0;
	phaseacc = 0.0;
	freqerr = 0.0;
	clear_StatusMessages();
	put_MODEstatus(mode);
	put_sec_char(0);
}

void dominoex::restart()
{
	double flo, fhi, bw, cf;
	


	bw = bandwidth * progdefaults.DOMINOEX_BW;
	cf = 1000.0 + bandwidth / 2.0;	// basetone is always 1000 Hz
					

	// mid frequency is always 1000 Hz + bandwidth / 2
	flo = (cf - bw/2) / samplerate;
	fhi = (cf + bw/2) / samplerate;
	if (filt)
		filt->init_bandpass (127, 1, flo, fhi);

	strSecXmtText = txtSecondary->value();
	if (strSecXmtText.length() == 0)
		strSecXmtText = "fldigi "FLDIGI_VERSION" ";
}

void dominoex::init()
{
	modem::init();
	restart();
	rx_init();
//	digiscope->mode(Digiscope::SCOPE);
	digiscope->mode(Digiscope::DOMDATA);
}

dominoex::~dominoex()
{
	if (binsfft) delete binsfft;
	if (hilbert) delete hilbert;
	if (pipe) delete [] pipe;
	if (scopedata) delete [] scopedata;
	if (filt) delete filt;
	if (wfid) delete wfid;
}

dominoex::dominoex(trx_mode md)
{
	double cf, bw, flo, fhi;

	numtones = DOMNUMTONES;
	mode = md;

	switch (mode) {
// 8kHz modes
	case MODE_DOMINOEX4:
		symlen = 2048;
		basetone = 256;		// 1000 Hz
		doublespaced = 1;
		samplerate = 8000;
		break;

	case MODE_DOMINOEX8:
		symlen = 1024;
		basetone = 128;		// 1000 Hz
		doublespaced = 1;
		samplerate = 8000;
		break;

	case MODE_DOMINOEX16:
		symlen = 512;
		basetone = 64;		// 1000 Hz
		doublespaced = 0;
		samplerate = 8000;
		break;

// 11.025 kHz modes
	case MODE_DOMINOEX5:
		symlen = 2048;
		basetone = 186;		// 1001.3 Hz
		doublespaced = 1;
		samplerate = 11025;
		break;

	case MODE_DOMINOEX11:
		symlen = 1024;
		basetone = 93;		// 1001.3 Hz
		doublespaced = 0;
		samplerate = 11025;
		break;

	case MODE_DOMINOEX22:
		symlen = 512;
		basetone = 46;		// 990 Hz
		doublespaced = 0;
		samplerate = 11025;
		break;
	default:
	//	case MODE_DOMINOEX8:
		symlen = 1024;
		basetone = 128;		// 1000 Hz
		doublespaced = 1;
		samplerate = 8000;
	}

	tonespacing = (double) (samplerate * ((doublespaced) ? 2 : 1)) / symlen;

	binsfft = new sfft(	symlen, 
						basetone - numtones*(doublespaced?2:1), 
						basetone + 2*numtones*(doublespaced ? 2 : 1) );


	hilbert	= new C_FIR_filter();
	hilbert->init_hilbert(37, 1);
	afcfilt		= new Cmovavg(AFC_COUNT);

	pipe = new domrxpipe[2 * symlen];
	scopedata = new double[2 * symlen];
	pipeptr = 0;
	symcounter = 0;
	metric = 0.0;

	bandwidth = (numtones - 1) * tonespacing;
	bw = bandwidth * progdefaults.DOMINOEX_BW;
	
	cf = 1000.0 + bandwidth / 2.0; // basetone is always 1000 Hz

	flo = (cf - bw/2) / samplerate;
	fhi = (cf + bw/2) / samplerate;

	filt = new C_FIR_filter();
	filt->init_bandpass (127, 1, flo, fhi);

	fragmentsize = symlen;

	s2n = 0.0;
	wfid = new id(this);

	prev1symbol = prev2symbol = 0;
	prev1vector = prev2vector = complex(0.0, 0.0);

	init();
}

//=====================================================================
// rx modules

complex dominoex::mixer(complex in, double f)
{
	complex z;
// Basetone is always 1000 Hz
	f -= (1000.0 + bandwidth/2);
	z.re = cos(phaseacc);
	z.im = sin(phaseacc);
	z = z * in;
	phaseacc -= twopi * f / samplerate;
	if (phaseacc > M_PI)
		phaseacc -= twopi;
	else if (phaseacc < M_PI)
		phaseacc += twopi;
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

void dominoex::decodesymbol(unsigned char curtone, unsigned char prevtone)
{
	int c, sym, ch;

// Decode the IFK+ sequence, which results in a single nibble
	c = curtone - prevtone;
	if (reverse) c = -c;
	if (doublespaced) c /= 2;
	c -= 2;
	if (c < 0) c += numtones;

//	If the new symbol is the start of a new character (MSB is low), complete the previous character
	if (!(c & 0x8)) {
		if (symcounter <= MAX_VARICODE_LEN) {
			sym = 0;
			for (int i = 0; i < symcounter; i++)
				sym |= symbolbuf[i] << (4 * i);
			ch = dominoex_varidec(sym);
			if (!squelchon || metric > squelch)		
				recvchar(ch);
		}
		symcounter = 0;
	}

// Add to the symbol buffer. Position 0 is the newest symbol.
	for (int i = MAX_VARICODE_LEN-1; i >= 0; i--)
		symbolbuf[i] = symbolbuf[i-1];
	symbolbuf[0] = c;

// Increment the counter, but clamp at max+1 to avoid overflow
	symcounter++;
	if (symcounter > MAX_VARICODE_LEN + 1)
		symcounter = MAX_VARICODE_LEN + 1;

}

int dominoex::harddecode(complex *in)
{
	double x, max = 0.0;
	int symbol = 0;
	
	for (int i = 0; i < numtones * 3*(doublespaced?2:1); i++) {
		x = in[i].mag();
		if (x > max) {
			max = x;
			symbol = i;
		}
	}
	return symbol;
}

void dominoex::update_syncscope(complex *bins)
{
	double max = 0, min = 1e6, range, mag;
	double buffer[numtones * 6];
	int numbins = numtones * 3 * (doublespaced ? 2 : 1);
// dom waterfall
	for (int i = 0; i < numbins; i++ ) {
		mag = bins[i].mag();
		if (max < mag) max = mag;
		if (min > mag) min = mag;
	}
	range = max - min;
	for (int i = 0; i < numbins; i++ ) {
		if (range > 2) {
			mag = (bins[i].mag() - min) / range;
			mag = 1 + log10(mag);
			if (mag < 0) mag = 0;
		} else
			mag = 0;
		buffer[i] = 255*mag;
	}
	if (!squelchon || metric >= squelch)
		set_video(buffer, numbins);

// dom symbol synch data	
	memset(scopedata, 0, 2 * symlen * sizeof(double));
	if (!squelchon || metric >= squelch)
		for (int i = 0, j = 0; i < 2 * symlen; i++) {
			j = (i + pipeptr) % (2 * symlen);
			scopedata[i] = (pipe[j].vector[prev1symbol]).mag();
		}
	set_scope(scopedata, 2 * symlen);

}

void dominoex::synchronize()
{
	int syn = -1;
	double val, max = 0.0;

	if (currsymbol == prev1symbol)
		return;
	if (prev1symbol == prev2symbol)
		return;

	for (int i = 0, j = pipeptr; i < 2 * symlen; i++) {
		val = (pipe[j].vector[prev1symbol]).mag();
		if (val > max) {
			max = val;
			syn = i;
		}
		j = (j + 1) % (2 * symlen);
	}
	synccounter += (int) floor((syn - symlen) / numtones + 0.5);
}

void dominoex::reset_afc() {
	freqerr = 0.0;
	for (int i = 0; i < AFC_COUNT; i++) afcfilt->run(0.0);
	return;
}

void dominoex::afc()
{
	complex z;
	complex prevvector;
	double f, fsym, err;
	double ds = doublespaced ? 2 : 1;

	if (sigsearch) {
		reset_afc();
		sigsearch = 0;
	}
	
	if (pipeptr == 0)
		prevvector = pipe[2*symlen - 1].vector[currsymbol];
	else
		prevvector = pipe[pipeptr - 1].vector[currsymbol];
	
	z = prevvector % currvector;

	f = z.arg() * samplerate / twopi;
	fsym = (currsymbol / ds - numtones) * samplerate * ds / symlen;
	fsym += 1000;
	err = f - fsym;

//	freqerr = afcfilt->run(freqerr/numtones);
	freqerr = decayavg(freqerr, err, 64);
	
//	std::cout << currsymbol << ", " << freqerr << std::endl; 
	fflush(stdout);
	
	if (afcon && (metric > squelch || squelchon == false)) {
		set_freq(frequency + freqerr);
	}
}

void dominoex::eval_s2n(complex curr, complex n)
{
	sig = curr.mag(); // signal + noise energy
	noise = n.mag() + 1e-10; // noise energy

	s2n = decayavg( s2n, fabs((sig - noise) / noise), 8);

	metric = 20*log10(s2n);

	display_metric(metric);

	sprintf(dommsg, "s/n %3.0f dB", metric);
	put_Status1(dommsg);

}

int dominoex::rx_process(double *buf, int len)
{
	complex z, *bins, noise;

	while (len-- > 0) {
// create analytic signal...shift in frequency to base band & bandpass filter
		z.re = z.im = *buf++;
		hilbert->run(z, z);
		z = mixer(z, frequency);
		filt->run(z, z);
		
// feed it to the sliding FFT
		bins = binsfft->run(z);

// copy current vector to the pipe
		for (int i = 0; i < numtones*3*(doublespaced?2:1); i++)
			pipe[pipeptr].vector[i] = bins[i];

		if (--synccounter <= 0) {
			synccounter = symlen;
			currsymbol = harddecode(bins);
			currvector = bins[currsymbol];
// decode symbol
            decodesymbol(currsymbol, prev1symbol);
// update the scope
			update_syncscope(bins);
// symbol sync
			synchronize();
// frequency tracking
			afc();
			eval_s2n(currvector, bins[(numtones + 2) * (doublespaced ? 2 : 1)]);

			prev2symbol = prev1symbol;
			prev2vector = prev1vector;
			prev1symbol = currsymbol;
			prev1vector = currvector;
		}
		pipeptr = (pipeptr + 1) % (2 * symlen);
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
	
	tone = (txprevtone + 2 + sym) % numtones;
    txprevtone = tone;
	if (reverse)
		tone = (numtones - 1) - tone;

	f = tone * tonespacing + get_txfreq_woffset() - bandwidth / 2;
	
	phaseincr = twopi * f / samplerate;
	
	for (int i = 0; i < symlen; i++) {
		outbuf[i] = cos(phaseacc);
		phaseacc -= phaseincr;
		if (phaseacc > M_PI)
			phaseacc -= twopi;
		else if (phaseacc < M_PI)
			phaseacc += twopi;
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
		if (i == 0)
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
		return -1;
	}
	return 0;
}
