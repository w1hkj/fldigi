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
#include <map>

#include "confdialog.h"
#include "status.h"

#include "dominoex.h"
#include "trx.h"
#include "fft.h"
#include "filters.h"
#include "misc.h"
#include "sound.h"
#include "mfskvaricode.h"

using namespace std;

char dommsg[80];
static map<int, unsigned char> mupsksec2pri;

void dominoex::tx_init(SoundBase *sc)
{
	scard = sc;
	txstate = TX_STATE_PREAMBLE;
	txprevtone = 0;
	bitstate = 0;
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
	for (int i = 0; i < MAXFFTS; i++)
		phase[i+1] = 0.0;
	put_MODEstatus(mode);
	put_sec_char(0);
	syncfilter->reset();
	datashreg = 1;
	for (int i = 0; i < VBINS; i++) vbins[i] = 0;
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
	if (mupsksec2pri.empty())
		MuPsk_sec2pri_init();

	modem::init();
	reset_filters();
	rx_init();

	strSecXmtText = txtSecondary->value();
	if (strSecXmtText.length() == 0)
		strSecXmtText = "fldigi "PACKAGE_VERSION" ";

	set_scope_mode(Digiscope::DOMDATA);
}

void dominoex::MuPsk_sec2pri_init(void)
{
	int chars[] = { 'A', 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, // À, Á, Â, Ã, Ä, Å
			0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, -1,  // à, á, â, ã, ä, å
			'B', 0xdf, -1,                           // ß
			'C', 0xc7, 0xe7, 0xa9, -1,               // Ç, ç, ©,
			'D', 0xd0, 0xb0, -1,                     // Ð, °
			'E', 0xc6, 0xe6, 0xc8, 0xc9, 0xca, 0xcb, // Æ, æ, È, É, Ê, Ë
			0xe8, 0xe9, 0xea, 0xeb, -1,              // è, é, ê, ë
			'F', 0x192, -1,                          // ƒ
			'I', 0xcc, 0xcd, 0xce, 0xcf, 0xec, 0xed, // Ì, Í, Î, Ï, ì, í
			0xee, 0xef, 0xa1, -1,                    // î, ï, ¡
			'L', 0xa3, -1,                           // £
			'N', 0xd1, 0xf1, -1,                     // Ñ, ñ
			'O', 0xf4, 0xf6, 0xf2, 0xd6, 0xf3, 0xd3, // ô, ö, ò, Ö, ó, Ó
			0xd4, 0xd2, 0xf5, 0xd5, -1,              // Ô, Ò, õ, Õ
			'R', 0xae, -1,                           // ®
			'U', 0xd9, 0xda, 0xdb, 0xdc, 0xf9, 0xfa, // Ù, Ú, Û, Ü, ù, ú
			0xfb, 0xfc, -1,                          // û, ü
			'X', 0xd7, -1,                           // ×
			'Y', 0xff, 0xfd, 0xdd, -1,               // ÿ, ý, Ý
			'0', 0xd8, -1,                           // Ø
			'1', 0xb9, -1,                           // ¹
			'2', 0xb2, -1,                           // ²
			'3', 0xb3, -1,                           // ³
			'?', 0xbf, -1,                           // ¿
			'!', 0xa1, -1,                           // ¡
			'<', 0xab, -1,                           // «
			'>', 0xbb, -1,                           // »
			'{', '(', -1,
			'}', ')', -1,
			'|', '\\'
	};

	int c = chars[0];
	for (size_t i = 1; i < sizeof(chars)/sizeof(*chars); i++) {
		if (chars[i] != -1)
			mupsksec2pri[chars[i]] = c;
		else
			c = chars[++i];
	}
}

dominoex::~dominoex()
{
	if (hilbert) delete hilbert;
	
	for (int i = 0; i < MAXFFTS; i++) {
		if (binsfft[i]) delete binsfft[i];
	}

	for (int i = 0; i < SCOPESIZE; i++) {
		if (vidfilter[i]) delete vidfilter[i];
	}
	if (syncfilter) delete syncfilter;
	
	if (pipe) delete [] pipe;
	if (fft) delete fft;
	
	if (MuPskRxinlv) delete MuPskRxinlv;
	if (MuPskTxinlv) delete MuPskTxinlv;
	if (MuPskDec) delete MuPskDec;
	if (MuPskEnc) delete MuPskEnc;
	
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

	paths = progdefaults.DOMINOEX_PATHS;

	for (int i = 0; i < MAXFFTS; i++)
		binsfft[i] = new sfft (symlen, lotone, hitone);

// fft filter at first if frequency
	fft = new fftfilt( (FIRSTIF - 0.5 * progdefaults.DOMINOEX_BW * bandwidth) / samplerate,
	                   (FIRSTIF + 0.5 * progdefaults.DOMINOEX_BW * bandwidth)/ samplerate,
	                   1024 );
	
	for (int i = 0; i < SCOPESIZE; i++)
		vidfilter[i] = new Cmovavg(16);
		
	syncfilter = new Cmovavg(8);
	
	twosym = 2 * symlen;
	pipe = new domrxpipe[twosym];
	
	scopedata.alloc(SCOPESIZE);
	videodata.alloc((MAXFFTS * NUMTONES * 2  * (doublespaced?2:1) ));

	pipeptr = 0;
	
	symcounter = 0;
	metric = 0.0;

	fragmentsize = symlen;

	s2n = 0.0;

	prev1symbol = prev2symbol = 0;

	MuPskEnc	= new encoder (K, POLY1, POLY2);
	MuPskDec	= new viterbi (K, POLY1, POLY2);
	MuPskDec->settraceback (45);
	MuPskDec->setchunksize (1);
	MuPskTxinlv = new interleave (-1, INTERLEAVE_FWD);
	MuPskRxinlv = new interleave (-1, INTERLEAVE_REV);
	bitstate = 0;
	symbolpair[0] = symbolpair[1] = 0;
	datashreg = 1;

	init();
}

//=====================================================================
// rx modules

complex dominoex::mixer(int n, complex in)
{
	complex z;
	double f;
// first IF mixer (n == 0) plus
// MAXFFTS mixers are supported each separated by 1/MAXFFTS bin size
// n == 1, 2, 3, 4 ... MAXFFTS
	if (n == 0)
		f = frequency - FIRSTIF;
	else
		f = FIRSTIF - BASEFREQ - bandwidth/2 + (samplerate / symlen) * (1.0 * n / paths );
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

void dominoex::decodeDomino(int c)
{
	int sym, ch;
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

void dominoex::decodesymbol()
{
	int c;
	double fdiff;

// Decode the IFK+ sequence, which results in a single nibble

	fdiff = currsymbol - prev1symbol;
	if (reverse) fdiff = -fdiff;
	if (doublespaced) fdiff /= 2 * paths;
	else              fdiff /= paths;
	c = (int)floor(fdiff + .5) - 2;
	if (c < 0) c += NUMTONES;

	if (progdefaults.DOMINOEX_FEC)
		decodeMuPskEX(c);
	else
		decodeDomino(c);
}

int dominoex::harddecode()
{
	double x, max = 0.0;
	int symbol = 0;
	for (int i = 0; i <  (paths * NUMTONES * 2  * (doublespaced ? 2 : 1) ); i++) {
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
	memset(videodata, 0, (paths * NUMTONES * 2  * (doublespaced?2:1) ) * sizeof(double));

	if (!progStatus.sqlonoff || metric >= progStatus.sldrSquelchValue) {
		for (int i = 0; i < (paths * NUMTONES * 2  * (doublespaced?2:1) ); i++ ) {
			mag = pipe[pipeptr].vector[i].mag();
			if (max < mag) max = mag;
			if (min > mag) min = mag;
		}
		range = max - min;
		for (int i = 0; i < (paths * NUMTONES * 2  * (doublespaced?2:1) ); i++ ) {
			if (range > 2) {
				mag = (pipe[pipeptr].vector[i].mag() - min) / range + 0.0001;
				mag = 1 + 2 * log10(mag);
				if (mag < 0) mag = 0;
			} else
				mag = 0;
			videodata[i] = 255*mag;
		}
	}
	set_video(videodata, (paths * NUMTONES * 2  * (doublespaced?2:1) ), false);
	videodata.next();

//	set_scope(scopedata, twosym);
// 64 data points is sufficient to show the signal progression through the
// convolution filter.
	memset(scopedata, 0, SCOPESIZE * sizeof(double));
	if (!progStatus.sqlonoff || metric >= progStatus.sldrSquelchValue) {
		for (unsigned int i = 0, j = 0; i < SCOPESIZE; i++) {
			j = (pipeptr + i * twosym / SCOPESIZE + 1) % (twosym);
			scopedata[i] = vidfilter[i]->run(pipe[j].vector[prev1symbol].mag());
		}
	}
	set_scope(scopedata, SCOPESIZE);
	scopedata.next();
}

void dominoex::synchronize()
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
	
	synccounter += (int) floor(1.0 * (syn - symlen) / NUMTONES + 0.5);
}


void dominoex::eval_s2n()
{
	if (currsymbol != prev1symbol && prev1symbol != prev2symbol) {
		sig = pipe[pipeptr].vector[currsymbol].mag();
		noise = 0.0;
		for (int i = 0; i < paths * NUMTONES * 2  * (doublespaced?2:1); i++) {
			if (i != currsymbol)
				noise += pipe[pipeptr].vector[i].mag();
		}	
		noise /= (paths * NUMTONES * 2  * (doublespaced?2:1) - 1);
	
		s2n = decayavg( s2n, sig / noise, 8);

		metric = 3*(20*log10(s2n) - 9.0);

		display_metric(metric);

		snprintf(dommsg, sizeof(dommsg), "s/n %3.0f dB", metric / 3.0 - 2.0);
		put_Status1(dommsg);
	}
}

int dominoex::rx_process(const double *buf, int len)
{
	complex zref,  z, *zp, *bins = 0;
	int n;

	if (filter_reset) reset_filters();

	if (paths != progdefaults.DOMINOEX_PATHS) {
		paths = progdefaults.DOMINOEX_PATHS;
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
// process MAXFFTS sets of sliding FFTs spaced at 1/MAXFFTS bin intervals each of which
// is a matched filter for the current symbol length
				for (int n = 0; n < paths; n++) {
// shift in frequency to base band for the sliding DFTs
					z = mixer(n + 1, zp[i]);
					bins = binsfft[n]->run(z);
// copy current vector to the pipe interleaving the FFT vectors
					for (int i = 0; i < NUMTONES * 2 * (doublespaced ? 2 : 1); i++) {
						pipe[pipeptr].vector[n + paths * i] = bins[i];
					}
				}
				if (--synccounter <= 0) {
					synccounter = symlen;
					currsymbol = harddecode();
        		    decodesymbol();
//        		    MuPskSoftdecode(bins);
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

void dominoex::sendtone(int tone, int duration)
{
	double f, phaseincr;
	f = (tone + 0.5) * tonespacing + get_txfreq_woffset() - bandwidth / 2.0;
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

void dominoex::sendsymbol(int sym)
{
//static int first = 0;
	complex z;
    int tone;
	
	tone = (txprevtone + 2 + sym) % NUMTONES;
    txprevtone = tone;
	if (reverse)
		tone = (NUMTONES - 1) - tone;
	sendtone(tone, 1);
}

void dominoex::sendchar(unsigned char c, int secondary)
{
	if (progdefaults.DOMINOEX_FEC) 
		sendMuPskEX(c, secondary);
	else {
		unsigned char *code = dominoex_varienc(c, secondary);
    	sendsymbol(code[0]);
// Continuation nibbles all have the MSB set
	    for (int sym = 1; sym < 3; sym++) {
    	    if (code[sym] & 0x8) 
        	    sendsymbol(code[sym]);
        	else
            	break;
    	}
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
//	if (progdefaults.DOMINOEX_FEC)
//		MuPskFlushTx();
//	else {
// flush the varicode decoder at the receiver end
	    for (int i = 0; i < 4; i++)
    	    sendidle();
//	}
}

int dominoex::tx_process()
{
	int i;

	switch (txstate) {
	case TX_STATE_PREAMBLE:
		if (progdefaults.DOMINOEX_FEC)
			MuPskClearbits();
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

//=============================================================================
// MultiPsk compatible FEC methods
//=============================================================================

//=============================================================================
// Varicode support methods
// MultiPsk varicode is based on a modified MFSK varicode table in which
// Character substition is used for secondary text.  The resulting table does
// NOT contain the full ASCII character set as the primary.  Many of the
// control codes and characters above 0x80 are lost.
//=============================================================================

// Convert from Secondary to Primary character

unsigned char dominoex::MuPskSec2Pri(int c)
{
	if (c >= 'a' && c <= 'z') c -= 32;

	c = mupsksec2pri.find(c) != mupsksec2pri.end() ? mupsksec2pri[c] : c;

	if (c >= 'A' && c <= 'Z') c = c - 'A' + 127;
	else if (c >= '0' && c <= '9') c = c - '0' + 14;
	else if (c >= ' ' && c <= '"') c = c - ' ' + 1;
	else if (c == '_') c = 4;
	else if (c >= '$' && c <= '&') c = c - '$' + 5;
	else if (c >= '\'' && c <= '*') c = c - '\'' + 9;
	else if (c >= '+' && c <= '/') c = c - '+' + 24;
	else if (c >= ':' && c <= '<') c = c - ':' + 29;
	else if (c >= '=' && c <= '@') c = c - '=' + 153;
	else if (c >= '[' && c <= ']') c = c - '[' + 157;
	else c = '_';
	
	return c;
}

// Convert Primary to Split Primary / Secondary character

unsigned int dominoex::MuPskPriSecChar(unsigned int c)
{
	if (c >= 127 && c < 153) c += ('A' - 127) + 0x100;
	else if (c >=14 && c < 24) c += ('0' - 14) + 0x100;
	else if (c >= 1 && c < 4) c += (' ' - 1) + 0x100;
	else if (c == 4) c = '_' + 0x100;
	else if (c >= 5 && c < 8) c += ('$' - 5) + 0x100;
	else if (c >= 9 && c < 13) c += ('\'' - 9) + 0x100;
	else if (c >= 24 && c < 29) c += ('+' - 24) + 0x100;
	else if (c >= 29 && c < 32) c += (':' - 29) + 0x100;
	else if (c >= 153 && c < 157) c += ('=' - 153) + 0x100;
	else if (c >= 157 && c < 160) c += ('[' - 157) + 0x100;
	return c;
}

//=============================================================================
// Receive
//=============================================================================

void dominoex::decodeMuPskSymbol(unsigned char symbol)
{
	int c, ch, met;

	symbolpair[0] = symbolpair[1];
	symbolpair[1] = symbol;

	symcounter = symcounter ? 0 : 1;
	
	if (symcounter) return;

	c = MuPskDec->decode (symbolpair, &met);

	if (c == -1)
		return;

	if (progStatus.sqlonoff && metric < progStatus.sldrSquelchValue)
		return;

	datashreg = (datashreg << 1) | !!c;
	if ((datashreg & 7) == 1) {
		ch = varidec(datashreg >> 1);
		recvchar(MuPskPriSecChar(ch));
		datashreg = 1;
	}
}

void dominoex::decodeMuPskEX(int ch)
{
	unsigned char symbols[4];
	int c = ch;
	
	for (int i = 0; i < 4; i++) {
		if ((c & 1) == 1) symbols[3-i] = 255;
		else symbols[3-i] = 1;//-255;
		c = c / 2;
	}

	MuPskRxinlv->symbols(symbols);

	for (int i = 0; i < 4; i++) decodeMuPskSymbol(symbols[i]);

}

//=============================================================================
// Transmit
//=============================================================================

void dominoex::MuPskFlushTx()
{
// flush the varicode decoder at the other end
// flush the convolutional encoder and interleaver
	sendsymbol(1);
	for (int i = 0; i < 107; i++)
		sendsymbol(0);
	bitstate = 0;
}

void dominoex::MuPskClearbits()
{
	int data = MuPskEnc->encode(0);
	for (int k = 0; k < 100; k++) {
		for (int i = 0; i < 2; i++) {
			bitshreg = (bitshreg << 1) | ((data >> i) & 1);
			bitstate++;

			if (bitstate == 4) {
				MuPskTxinlv->bits(&bitshreg);
				bitstate = 0;
				bitshreg = 0;
			}
		}
	}
}

// Send MultiPsk FEC varicode with minimalist interleaver

void dominoex::sendMuPskEX(unsigned char c, int secondary)
{
	const char *code;
	if (secondary == 1) 
		c = MuPskSec2Pri(c);
	else {
		if (c == 10) 
			return;
		if ( (c >= 1 && c <= 7) || (c >= 9 && c <= 12) || (c >= 14 && c <= 31) ||
		     (c >= 127 && c <= 159))
		   c = '_';
	}
	code = varienc(c);
//if (secondary == 0)
//std::cout << (int)c <<  " ==> " << code << " ==> ";
	while (*code) {
		int data = MuPskEnc->encode(*code++ - '0');
//std::cout << (int)data << " ";
		for (int i = 0; i < 2; i++) {
			bitshreg = (bitshreg << 1) | ((data >> i) & 1);
			bitstate++;
			if (bitstate == 4) {

				MuPskTxinlv->bits(&bitshreg);
				
//std::cout << "(" << bitshreg << ") ";
				
				sendsymbol(bitshreg);

//decodeMuPskEX(bitshreg);

				bitstate = 0;
				bitshreg = 0;
			}
		}
	}
//std::cout << std::endl;
}
