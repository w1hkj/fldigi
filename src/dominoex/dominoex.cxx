// ----------------------------------------------------------------------------
//
// dominoex.cxx  --  DominoEX modem
//
// Copyright (C) 2008-20012
//     David Freese   <w1hkj@w1hkj.com>
//     Hamish Moffatt <hamish@debian.org>
//     John Phelps    <kl4yfd@gmail.com>
//
// based on code in gmfsk
//
// This file is part of fldigi.
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

#include <map>

#include "confdialog.h"
#include "status.h"

#include "dominoex.h"
#include "trx.h"
#include "fl_digi.h"
#include "filters.h"
#include "misc.h"
#include "sound.h"
#include "mfskvaricode.h"
#include "debug.h"

LOG_FILE_SOURCE(debug::LOG_MODEM);

using namespace std;

char dommsg[80];
static map<int, unsigned char> mupsksec2pri;

bool usingFEC = false;

void dominoex::tx_init(SoundBase *sc)
{
	scard = sc;
	txstate = TX_STATE_PREAMBLE;
	txprevtone = 0;
	Mu_bitstate = 0;
	counter = 0;
	txphase = 0;

	strSecXmtText = progdefaults.secText;
	if (strSecXmtText.length() == 0)
		strSecXmtText = "fldigi "PACKAGE_VERSION" ";

	videoText();
}

void dominoex::rx_init()
{
	synccounter = 0;
	symcounter = 0;
	Mu_symcounter = 0;
	met1 = 0.0;
	met2 = 0.0;
	counter = 0;
	phase[0] = 0.0;
	for (int i = 0; i < MAXFFTS; i++)
		phase[i+1] = 0.0;
	put_MODEstatus(mode);
	put_sec_char(0);
	syncfilter->reset();

	Mu_datashreg = 1;

	staticburst = false;

	sig = noise = 6;
}

void dominoex::reset_filters()
{
// fft filter at first IF frequency
	fft->create_filter( (FIRSTIF - 0.5 * progdefaults.DOMINOEX_BW * bandwidth) / samplerate,
						(FIRSTIF + 0.5 * progdefaults.DOMINOEX_BW * bandwidth)/ samplerate );

	for (int i = 0; i < MAXFFTS; i++) {
		if (binsfft[i]) delete binsfft[i];
		binsfft[i] = 0;
	}

	if (slowcpu) {
		extones = 4;
		paths = 3;
	} else {
		extones = NUMTONES / 2;
		paths = 5;
	}

	lotone = basetone - extones * doublespaced;
	hitone = basetone + NUMTONES * doublespaced + extones * doublespaced;

	numbins = hitone - lotone;

	for (int i = 0; i < paths; i++)//MAXFFTS; i++)
		binsfft[i] = new sfft (symlen, lotone, hitone);

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
//	reset_filters();
	rx_init();

	set_scope_mode(Digiscope::DOMDATA);
}

void dominoex::MuPsk_sec2pri_init(void)
{
	int chars[] = { 'A', 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, // À, Á, Â, Ã, Ä, Å
			0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, -1,  // à, á, â, ã, ä, å
			'B', 0xdf, -1,						   // ß
			'C', 0xc7, 0xe7, 0xa9, -1,			   // Ç, ç, ©,
			'D', 0xd0, 0xb0, -1,					 // Ð, °
			'E', 0xc6, 0xe6, 0xc8, 0xc9, 0xca, 0xcb, // Æ, æ, È, É, Ê, Ë
			0xe8, 0xe9, 0xea, 0xeb, -1,			  // è, é, ê, ë
			'F', 0x192, -1,						  // ƒ
			'I', 0xcc, 0xcd, 0xce, 0xcf, 0xec, 0xed, // Ì, Í, Î, Ï, ì, í
			0xee, 0xef, 0xa1, -1,					// î, ï, ¡
			'L', 0xa3, -1,						   // £
			'N', 0xd1, 0xf1, -1,					 // Ñ, ñ
			'O', 0xf4, 0xf6, 0xf2, 0xd6, 0xf3, 0xd3, // ô, ö, ò, Ö, ó, Ó
			0xd4, 0xd2, 0xf5, 0xd5, -1,			  // Ô, Ò, õ, Õ
			'R', 0xae, -1,						   // ®
			'U', 0xd9, 0xda, 0xdb, 0xdc, 0xf9, 0xfa, // Ù, Ú, Û, Ü, ù, ú
			0xfb, 0xfc, -1,						  // û, ü
			'X', 0xd7, -1,						   // ×
			'Y', 0xff, 0xfd, 0xdd, -1,			   // ÿ, ý, Ý
			'0', 0xd8, -1,						   // Ø
			'1', 0xb9, -1,						   // ¹
			'2', 0xb2, -1,						   // ²
			'3', 0xb3, -1,						   // ³
			'?', 0xbf, -1,						   // ¿
			'!', 0xa1, -1,						   // ¡
			'<', 0xab, -1,						   // «
			'>', 0xbb, -1,						   // »
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
		binsfft[i] = 0;
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
	cap |= CAP_REV;

	mode = md;

	switch (mode) {
// 11.025 kHz modes
	case MODE_DOMINOEX5:
		symlen = 2048;
		doublespaced = 2;
		samplerate = 11025;
		break;
	case MODE_DOMINOEX11:
		symlen = 1024;
		doublespaced = 1;
		samplerate = 11025;
		break;
	case MODE_DOMINOEX22:
		symlen = 512;
		doublespaced = 1;
		samplerate = 11025;
		break;
// 8kHz modes
	case MODE_DOMINOEX4:
		symlen = 2048;
		doublespaced = 2;
		samplerate = 8000;
		break;
	case MODE_DOMINOEX8:
		symlen = 1024;
		doublespaced = 2;
		samplerate = 8000;
		break;
	case MODE_DOMINOEX16:
		symlen = 512;
		doublespaced = 1;
		samplerate = 8000;
		break;
// experimental 
	case MODE_DOMINOEX44:
		symlen = 256;
		doublespaced = 2;
		samplerate = 11025;
		break;
	case MODE_DOMINOEX88:
		symlen = 128;
 		doublespaced = 1;
		samplerate = 11025;
 		break;


	default: // EX8
		symlen = 1024;
		doublespaced = 2;
		samplerate = 8000;
	}

	tonespacing = 1.0 * samplerate * doublespaced / symlen;

	bandwidth = NUMTONES * tonespacing;

	hilbert	= new C_FIR_filter();
	hilbert->init_hilbert(37, 1);

// fft filter at first if frequency
	fft = new fftfilt( (FIRSTIF - 0.5 * progdefaults.DOMINOEX_BW * bandwidth) / samplerate,
					   (FIRSTIF + 0.5 * progdefaults.DOMINOEX_BW * bandwidth)/ samplerate,
					   1024 );

	basetone = (int)floor(BASEFREQ * symlen / samplerate + 0.5);

	slowcpu = progdefaults.slowcpu;

	for (int i = 0; i < MAXFFTS; i++)
		binsfft[i] = 0;

	reset_filters();

	for (int i = 0; i < SCOPESIZE; i++)
		vidfilter[i] = new Cmovavg(16);

	syncfilter = new Cmovavg(16);

	twosym = 2 * symlen;
	pipe = new domrxpipe[twosym];

	scopedata.alloc(SCOPESIZE);
	videodata.alloc(MAXFFTS * numbins);

	pipeptr = 0;

	symcounter = 0;
	Mu_symcounter = 0;
	metric = 0.0;

	fragmentsize = symlen;

	s2n = 0.0;

	prev1symbol = prev2symbol = 0;

	MuPskEnc	= new encoder (K, POLY1, POLY2);
	MuPskDec	= new viterbi (K, POLY1, POLY2);
	MuPskDec->settraceback (45);
	MuPskDec->setchunksize (1);
	MuPskTxinlv = new interleave (4, 4, INTERLEAVE_FWD);
	MuPskRxinlv = new interleave (4, 4, INTERLEAVE_REV);
	Mu_bitstate = 0;
	Mu_symbolpair[0] = Mu_symbolpair[1] = 0;
	Mu_datashreg = 1;
//	init();
}

//=====================================================================
// rx modules
cmplx dominoex::mixer(int n, cmplx in)
{
	cmplx z;
	double f;

// first IF mixer (n == 0) plus
// MAXFFTS mixers are supported each separated by tonespacing/paths
// n == 1, 2, 3, 4 ... MAXFFTS
	if (n == 0)
		f = frequency - FIRSTIF;
	else
		f = FIRSTIF - BASEFREQ - bandwidth / 2.0 + tonespacing * (1.0 * (n - 1) / paths );
	z = cmplx( cos(phase[n]), sin(phase[n]));
	z = z * in;
	phase[n] -= TWOPI * f / samplerate;
	if (phase[n] > M_PI)
		phase[n] -= TWOPI;
	else if (phase[n] < M_PI)
		phase[n] += TWOPI;
	return z;
}

void dominoex::recvchar(int c)
{
	if (!progStatus.sqlonoff || metric > progStatus.sldrSquelchValue) {

		if (c == -1)
			return;
		if (c & 0x100)
			put_sec_char(c & 0xFF);
		else
			put_rx_char(c & 0xFF);
		}
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

				if (!progdefaults.DOMINOEX_FEC)
					if (!staticburst && !outofrange)
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
	fdiff /= doublespaced;
	fdiff /= paths;

//	if (fabs(fdiff) > 17)
//		outofrange = true;
//	else
		outofrange = false;

	c = (int)floor(fdiff + .5) - 2;
	if (c < 0) c += NUMTONES;

	decodeDomino(c);
	decodeMuPskEX(c);
}

int dominoex::harddecode()
{
	double x, max = 0.0;
	int symbol = 0;
	double avg = 0.0;
	bool cwi[paths * numbins];
	double cwmag;

	for (int i = 0; i < paths * numbins; i++)
		avg += abs(pipe[pipeptr].vector[i]);
	avg /= (paths * numbins);

	if (avg < 1e-10) avg = 1e-10;

	int numtests = 10;
	int count = 0;
	for (int i = 0; i < paths * numbins; i++) {
		cwmag = 0.0;
		count = 0;
		for (int j = 1; j <= numtests; j++) {
			int p = pipeptr - j;
			if (p < 0) p += twosym;
			cwmag = abs(pipe[j].vector[i])/numtests;
			if (cwmag >= 50.0 * (1.0 - progdefaults.ThorCWI) * avg) count++;
		}
		cwi[i] = (count == numtests);
	}

	for (int i = 0; i <  (paths * numbins); i++) {
		if (cwi[i] == false) {
			x = abs(pipe[pipeptr].vector[i]);
			avg += x;
			if (x > max) {
				max = x;
				symbol = i;
			}
		}
	}
	avg /= (paths * numbins);
	staticburst = (max / avg < 1.2);

	return symbol;
}

void dominoex::update_syncscope()
{

	double max = 0, min = 1e6, range, mag;

// dom waterfall
	memset(videodata, 0, (paths * numbins) * sizeof(double));

	if (!progStatus.sqlonoff || metric >= progStatus.sldrSquelchValue) {
		for (int i = 0; i < (paths * numbins); i++ ) {
			mag = abs(pipe[pipeptr].vector[i]);
			if (max < mag) max = mag;
			if (min > mag) min = mag;
		}
		range = max - min;
		for (int i = 0; i < (paths * numbins); i++ ) {
			if (range > 2) {
				mag = (abs(pipe[pipeptr].vector[i]) - min) / range + 0.0001;
				mag = 1 + 2 * log10(mag);
				if (mag < 0) mag = 0;
			} else
				mag = 0;
			videodata[(i + paths * numbins / 2)/2] = 255*mag;
		}
	}
	set_video(videodata, (paths * numbins), false);
	videodata.next();

//	set_scope(scopedata, twosym);
// 64 data points is sufficient to show the signal progression through the
// convolution filter.
	memset(scopedata, 0, SCOPESIZE * sizeof(double));
	if (!progStatus.sqlonoff || metric >= progStatus.sldrSquelchValue) {
		for (unsigned int i = 0, j = 0; i < SCOPESIZE; i++) {
			j = (pipeptr + i * twosym / SCOPESIZE + 1) % (twosym);
			scopedata[i] = vidfilter[i]->run(abs(pipe[j].vector[prev1symbol]));
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

	if (staticburst == true) return;

	if (currsymbol == prev1symbol)
		return;
	if (prev1symbol == prev2symbol)
		return;

	for (unsigned int i = 0, j = pipeptr; i < twosym; i++) {
		val = abs(pipe[j].vector[prev1symbol]);
		if (val > max) {
			max = val;
			syn = i;
		}
		j = (j + 1) % twosym;
	}

	syn = syncfilter->run(syn);

	synccounter += (int) floor(1.0 * (syn - symlen) / NUMTONES + 0.5);

	update_syncscope();
}

void dominoex::eval_s2n()
{
	double s = abs(pipe[pipeptr].vector[currsymbol]);
	double n = (NUMTONES - 1 ) * abs(pipe[(pipeptr + symlen) % twosym].vector[currsymbol]);

	sig = decayavg( sig, s, abs( s - sig) > 4 ? 4 : 32);
	noise = decayavg( noise, n, 64);

	if (noise)
		s2n = 20*log10(sig / noise) - 6;
	else
		s2n = 0;

//	metric = 4 * s2n;
	// To partially offset the increase of noise by (THORNUMTONES -1)
	// in the noise calculation above, 
	// add 15*log10(THORNUMTONES -1) = 18.4, and multiply by 6
	metric = 6 * (s2n + 18.4);

	metric = metric < 0 ? 0 : metric > 100 ? 100 : metric;

	display_metric(metric);

	snprintf(dommsg, sizeof(dommsg), "s/n %3.0f dB", s2n );
	put_Status1(dommsg);
}

int dominoex::rx_process(const double *buf, int len)
{
	cmplx zref,  z, *zp;
	cmplx zarray[1];
	int n;

	if (filter_reset) reset_filters();

	if (slowcpu != progdefaults.slowcpu) {
		slowcpu = progdefaults.slowcpu;
		reset_filters();
	}

	while (len) {
// create analytic signal at first IF
		zref = cmplx( *buf, *buf );
		buf++;
		hilbert->run(zref, zref);
		zref = mixer(0, zref);

		if (progdefaults.DOMINOEX_FILTER) {
// filter using fft convolution
			n = fft->run(zref, &zp);
		} else {
			zarray[0] = zref;
			zp = zarray;
			n = 1;
		}

		if (n) {
			for (int i = 0; i < n; i++) {
// process MAXFFTS sets of sliding FFTs spaced at 1/MAXFFTS bin intervals each of which
// is a matched filter for the current symbol length
				for (int j = 0; j < paths; j++) {
// shift in frequency to base band for the sliding DFTs
					z = mixer(j + 1, zp[i]);
// copy current vector to the pipe interleaving the FFT vectors
					binsfft[j]->run(z, pipe[pipeptr].vector + j, paths );
				}
				if (--synccounter <= 0) {
					synccounter = symlen;
					currsymbol = harddecode();
					decodesymbol();
					synchronize();
//					update_syncscope();
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
	if (cptr >= strSecXmtText.length()) cptr = 0;
	chr = strSecXmtText[cptr++];
	put_sec_char( chr );
	return chr;
}

void dominoex::sendtone(int tone, int duration)
{
	double f, phaseincr;
	f = (tone + 0.5) * tonespacing + get_txfreq_woffset() - bandwidth / 2.0;
	phaseincr = TWOPI * f / samplerate;
	for (int j = 0; j < duration; j++) {
		for (int i = 0; i < symlen; i++) {
			outbuf[i] = cos(txphase);
			txphase -= phaseincr;
			if (txphase > M_PI)
				txphase -= TWOPI;
			else if (txphase < M_PI)
				txphase += TWOPI;
		}
		ModulateXmtr(outbuf, symlen);
	}
}

void dominoex::sendsymbol(int sym)
{
//static int first = 0;
	cmplx z;
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
		if (i == GET_TX_CHAR_NODATA)
			sendsecondary();
		else if (i == GET_TX_CHAR_ETX)
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

	Mu_symbolpair[0] = Mu_symbolpair[1];
	Mu_symbolpair[1] = symbol;

	Mu_symcounter = Mu_symcounter ? 0 : 1;

	if (Mu_symcounter) return;

	c = MuPskDec->decode (Mu_symbolpair, &met);

	if (c == -1)
		return;

	if (progStatus.sqlonoff && metric < progStatus.sldrSquelchValue)
		return;

	Mu_datashreg = (Mu_datashreg << 1) | !!c;
	if ((Mu_datashreg & 7) == 1) {
		ch = varidec(Mu_datashreg >> 1);
		if (progdefaults.DOMINOEX_FEC)
			recvchar(MuPskPriSecChar(ch));
		Mu_datashreg = 1;
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
	if (staticburst == true || outofrange == true)
		symbols[3] = symbols[2] = symbols[1] = symbols[0] = 0;

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
	Mu_bitstate = 0;
}

void dominoex::MuPskClearbits()
{
	int data = MuPskEnc->encode(0);
	for (int k = 0; k < 100; k++) {
		for (int i = 0; i < 2; i++) {
			bitshreg = (bitshreg << 1) | ((data >> i) & 1);
			Mu_bitstate++;

			if (Mu_bitstate == 4) {
				MuPskTxinlv->bits(&bitshreg);
				Mu_bitstate = 0;
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
	// if (secondary == 0)
	// 	LOG_DEBUG("char=%hhu, code=\"%s\"", c, code);
	while (*code) {
		int data = MuPskEnc->encode(*code++ - '0');
		// LOG_DEBUG("data=%d", data;
		for (int i = 0; i < 2; i++) {
			bitshreg = (bitshreg << 1) | ((data >> i) & 1);
			Mu_bitstate++;
			if (Mu_bitstate == 4) {

				MuPskTxinlv->bits(&bitshreg);

				// LOG_DEBUG("bitshreg=%d", bitshreg);

				sendsymbol(bitshreg);

				// decodeMuPskEX(bitshreg);

				Mu_bitstate = 0;
				bitshreg = 0;
			}
		}
	}
}
