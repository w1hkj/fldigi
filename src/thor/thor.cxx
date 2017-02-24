// ----------------------------------------------------------------------------
// thor.cxx  --  thor modem
//
// Copyright (C) 2008-2012
//     David Freese <w1hkj@w1hkj.com>
//     John Douyere <vk2eta@gmail.com>
//     John Phelps  <kl4yfd@gmail.com>
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
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <libgen.h>

#include <FL/filename.H>

#include "confdialog.h"
#include "status.h"

#include "thor.h"
#include "trx.h"
#include "fl_digi.h"
#include "filters.h"
#include "misc.h"
#include "sound.h"
#include "thorvaricode.h"

#include "ascii.h"
#include "main.h"
#include "debug.h"

// Enable to enable profiling output for the soft-decision decoder
#define SOFTPROFILE false
//#define SOFTPROFILE true

using namespace std;

char thormsg[80];
char confidence[80];

#include "thor-pic.cxx"

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
		strSecXmtText = "fldigi " PACKAGE_VERSION " ";
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
//	avgsig = 1e-20;
	for (int i = 0; i < THORMAXFFTS; i++)
		phase[i+1] = 0.0;
	put_MODEstatus(mode);
	put_sec_char(0);
	syncfilter->reset();
	datashreg = 1;
	sig = noise = 6;

	fec_confidence = 0;

	s2n_valid = false;
	txstate = TX_STATE_RECEIVE;

	state = TEXT;
	pic_str = "     ";
	img_phase = 0.0;
}

void thor::reset_filters()
{
//LOG_INFO("%s", "Reset filters");
// fft filter at first IF frequency
	if (fft)
	fft->create_filter( (THORFIRSTIF - 0.5 * progdefaults.THOR_BW * bandwidth) / samplerate,
						(THORFIRSTIF + 0.5 * progdefaults.THOR_BW * bandwidth)/ samplerate );

	for (int i = 0; i < THORMAXFFTS; i++)
		if (binsfft[i]) {
			delete binsfft[i];
			binsfft[i] = 0;
		}

	if (slowcpu) {
		extones = 4;
		paths = THORSLOWPATHS;
	} else {
		extones = THORNUMTONES / 2;
		paths = THORFASTPATHS;
	}

	lotone = basetone - extones * doublespaced;
	hitone = basetone + THORNUMTONES * doublespaced + extones * doublespaced;

	numbins = hitone - lotone;

//LOG_INFO("MAX ARRAY SIZE %d, paths %d, numbins %d, array_size %d", MAXPATHS, paths, numbins, numbins * paths);

	for (int i = 0; i < paths; i++) {
		if (binsfft[i]) delete binsfft[i];
		binsfft[i] = new sfft (symlen, lotone, hitone);
	}
//LOG_INFO("binsfft(%d) initialized", paths);

	for (int i = 0; i < THORSCOPESIZE; i++) {
		if (vidfilter[i]) delete vidfilter[i];
		vidfilter[i] = new Cmovavg(16);
	}
//LOG_INFO("vidfilter(%d) initialized", THORSCOPESIZE);

	if (syncfilter) delete syncfilter;
	syncfilter = new Cmovavg(8);
//LOG_INFO("%s", "syncfilter initialized");

	filter_reset = false;
}

void thor::restart()
{
	filter_reset = true;
}

void thor::init()
{
//LOG_INFO("%s", "thor::init");
	modem::init();
//	reset_filters();
	rx_init();
	imageheader.clear();
	avatarheader.clear();

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

	delete picfilter;
	delete pixfilter;
	delete pixsyncfilter;

	activate_thor_image_item(false);
}

thor::thor(trx_mode md) : hilbert(0), fft(0), filter_reset(false)
{
	cap |= CAP_REV;

	mode = md;

	int isize = 4;
	int idepth = 10;
	flushlength = 4;

	switch (mode) {
// 11.025 kHz modes
	case MODE_THOR5:
		symlen = 2048;
		doublespaced = 2;
		samplerate = 11025;
		break;

	case MODE_THOR11:
		cap |= CAP_IMG;
		symlen = 1024;
		doublespaced = 1;
		samplerate = 11025;
		break;

	case MODE_THOR22:
		cap |= CAP_IMG;
		symlen = 512;
		doublespaced = 1;
		samplerate = 11025;
		break;

// 8kHz modes
	case MODE_THORMICRO:
		symlen = 4000; 
		doublespaced = 1;
		samplerate = 8000;
        idepth = 4;
		break;
    
    case MODE_THOR4:
		symlen = 2048;
		doublespaced = 2;
		samplerate = 8000;
		break;

	case MODE_THOR8:
		symlen = 1024;
		doublespaced = 2;
		samplerate = 8000;
		break;

	case MODE_THOR25x4:
		symlen = 320;
		doublespaced = 4;
		samplerate = 8000;
		idepth = 50; // 2 sec interleave
		flushlength = 40;
		break;

	case MODE_THOR50x1:
		symlen = 160;
		doublespaced = 1;
		samplerate = 8000;
		idepth = 50; // 1 sec interleave
		flushlength = 40;
		break;

	case MODE_THOR50x2:
		symlen = 160;
		doublespaced = 2;
		samplerate = 8000;
		idepth = 50; // 1 sec interleave
		flushlength = 40;
		break;

	case MODE_THOR100:
		symlen = 80;
		doublespaced = 1;
		samplerate = 8000;
		idepth = 50; // 0.5 sec interleave
		flushlength = 40;
		break;

	case MODE_THOR16:
	default:
		cap |= CAP_IMG;
		symlen = 512;
		doublespaced = 1;
		samplerate = 8000;
	}

	tonespacing = 1.0 * samplerate * doublespaced / symlen;

	bandwidth = THORNUMTONES * tonespacing;

	hilbert	= new C_FIR_filter();
	hilbert->init_hilbert(37, 1);

// fft filter at first if frequency
	fft = new fftfilt( (THORFIRSTIF - 0.5 * progdefaults.THOR_BW * bandwidth) / samplerate,
	                   (THORFIRSTIF + 0.5 * progdefaults.THOR_BW * bandwidth)/ samplerate,
	                   1024 );

	basetone = (int)floor(THORBASEFREQ * symlen / samplerate + 0.5);

	slowcpu = progdefaults.slowcpu;

	for (int i = 0; i < THORMAXFFTS; i++)
		binsfft[i] = 0;
	for (int i = 0; i < THORSCOPESIZE; i++)
		vidfilter[i] = 0;
	syncfilter = 0;

	reset_filters();

	twosym = 2 * symlen;
	pipe = new THORrxpipe[twosym];

	scopedata.alloc(THORSCOPESIZE);
	videodata.alloc(THORMAXFFTS * numbins );

	pipeptr = 0;

	symcounter = 0;
	metric = 0.0;

	fragmentsize = symlen;

	s2n = 0.0;

	prev1symbol = prev2symbol = 0;

	prev1symbol = prev2symbol = 0;

	if ( mode == MODE_THOR100 || mode == MODE_THOR50x1 || mode == MODE_THOR50x2 || mode == MODE_THOR25x4 ) {
		Enc = new encoder (THOR_K15, K15_POLY1, K15_POLY2);
		Dec = new viterbi (THOR_K15, K15_POLY1, K15_POLY2);
		Dec->settraceback (15 * 12); // Long constraint length codes require longer traceback
	} else {
		Enc = new encoder (THOR_K, THOR_POLY1, THOR_POLY2);
		Dec = new viterbi (THOR_K, THOR_POLY1, THOR_POLY2);
		Dec->settraceback (45); 
	}
	Txinlv = new interleave (isize, idepth, INTERLEAVE_FWD);
	Rxinlv = new interleave (isize, idepth, INTERLEAVE_REV);
	Dec->setchunksize (1);

	bitstate = 0;
	symbolpair[0] = symbolpair[1] = 0;
	datashreg = 1;

	picfilter = new C_FIR_filter();
	picfilter->init_lowpass(257, 1, 1.0 * bandwidth / samplerate);

	IMAGEspp = THOR_IMAGESPP;
	pixfilter = new Cmovavg(IMAGEspp);
	pixsyncfilter = new Cmovavg(3*IMAGEspp);

	init();

	activate_thor_image_item(true);
}

//=====================================================================
// rx modules

cmplx thor::mixer(int n, const cmplx& in)
{
	double f;
// first IF mixer (n == 0) plus
// THORMAXFFTS mixers are supported each separated by 1/THORMAXFFTS bin size
// n == 1, 2, 3, 4 ... THORMAXFFTS
	if (n == 0)
		f = frequency - THORFIRSTIF;
	else
		f = THORFIRSTIF - THORBASEFREQ - bandwidth*0.5 + (samplerate / symlen) * ( (double)n / paths);

	cmplx z( cos(phase[n]), sin(phase[n]) );
	z *= in;
	phase[n] -= TWOPI * f / samplerate;
	if (phase[n] < 0) phase[n] += TWOPI;

	return z;
}

void thor::s2nreport(void)
{
	modem::s2nreport();
	s2n_valid = false;
}

void thor::parse_pic(int ch)
{
	pic_str.erase(0,1);
	pic_str += ch;
	b_ava = false;
	image_mode = 0;
	if (pic_str.find("pic%") == 0) {
		switch (pic_str[4]) {
			case 'A':	picW = 59; picH = 74; b_ava = true; break;
			case 'T':	picW = 59; picH = 74; break;
			case 'S':	picW = 160; picH = 120; break;
			case 'L':	picW = 320; picH = 240; break;
			case 'F':	picW = 640; picH = 480; break;
			case 'V':	picW = 640; picH = 480; break;
			case 'P':	picW = 240; picH = 300; break;
			case 'p':	picW = 240; picH = 300; image_mode = 1; break;
			case 'M':	picW = 120; picH = 150; break;
			case 'm':	picW = 120; picH = 150; image_mode = 1; break;
			default: return;
		}
	} else
		return;

	if (b_ava)
		REQ( thor_clear_avatar );
	else
		REQ( thor_showRxViewer, pic_str[4]);

	image_counter = -symlen / 2;
	col = row = rgb = 0;
	pixsyncfilter->reset();
	pixfilter->reset();
	state = IMAGE_START;
}

void thor::recvchar(int c)
{
	if (c == -1)
		return;
	if (c & 0x100)
		put_sec_char(c & 0xFF);
	else {
		parse_pic(c);
		put_rx_char(c & 0xFF);
		if (progdefaults.Pskmails2nreport && (mailserver || mailclient)) {
			if (((c & 0xFF) == SOH) && !s2n_valid) {
				// starts collecting s2n from first SOH in stream (since start of RX)
				s2n_valid = true;
				s2n_sum = s2n_sum2 = s2n_ncount = 0.0;
			}
			if (s2n_valid) {
				s2n_sum += s2n_metric;
				s2n_sum2 += (s2n_metric * s2n_metric);
				s2n_ncount++;
				if ((c & 0xFF) == EOT)
					s2nreport();
			}
		}
	}
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

	if(met < 255 / 2) fec_confidence -=  2 + fec_confidence / 2;
	else fec_confidence += 2;
	if (fec_confidence < 0) fec_confidence = 0;
	if (fec_confidence > 100) fec_confidence = 100;

	if (progStatus.sqlonoff && metric < progStatus.sldrSquelchValue)
		return;

	datashreg = (datashreg << 1) | !!c;
	if ((datashreg & 7) == 1) {
		ch = thorvaridec(datashreg >> 1);
		recvchar(ch);
//LOG_INFO("thorvaridec %X = %d", datashreg >> 1, ch);
		datashreg = 1;
	}


}

void thor::decodesymbol()
{
	int c;
	double fdiff;//, softmag;
	unsigned char symbols[4];
	bool outofrange = false;

// Decode the IFK+ sequence, which results in a single nibble

	fdiff = currsymbol - prev1symbol;

	if (reverse) fdiff = -fdiff;
	fdiff /= paths;
	fdiff /= doublespaced;

	if (fabs(fdiff) > 17) outofrange = true;

	c = (int)floor(fdiff + .5); {
	 	if (progdefaults.THOR_PREAMBLE) {
			if ( preambledetect(c) ) {
				softflushrx(); // Flush the soft rx pipeline with punctures (interleaver & viterbi decoder)
				return;
			}
		}
	}
	c -= 2;
	if (c < 0) c += THORNUMTONES;

	if (staticburst == true || outofrange == true) // puncture the code
		symbols[3] = symbols[2] = symbols[1] = symbols[0] = 128;
	else {
		symbols[3] = (c & 1) == 1 ? 255 : 0; c /= 2;
		symbols[2] = (c & 1) == 1 ? 255 : 0; c /= 2;
		symbols[1] = (c & 1) == 1 ? 255 : 0; c /= 2;
		symbols[0] = (c & 1) == 1 ? 255 : 0; c /= 2;
	}

	Rxinlv->symbols(symbols);

	for (int i = 0; i < 4; i++) decodePairs(symbols[i]);

}

void thor::softdecodesymbol()
{
	unsigned char one, zero;
	int c, nextmag=127, rawdoppler=0;
	static int lastc=0, lastmag=0, nowmag=0, prev1rawdoppler=0;
	static double lastdoppler=0, nowdoppler=0;
	unsigned char lastsymbols[4];
	bool outofrange=false;

	double fdiff = currsymbol - prev1symbol;
	if (reverse) fdiff = -fdiff;
	fdiff /= paths;
	fdiff /= doublespaced;

	c = (int)floor(fdiff + .5); {
		if (c < -16 || 0 == c || 1 == c || c > 17) outofrange = true; // Out of the range of the function thor::sendsymbol()
		if (progdefaults.THOR_PREAMBLE) {
			if ( preambledetect(c) ) {
				softflushrx(); // Flush the soft rx pipeline with punctures (interleaver & viterbi decoder)
				lastmag = 0;
				return;
			}
		}
#if SOFTPROFILE
		LOG_INFO("Symbol: %3d; DELTAf: +%3d",currsymbol, c);
#endif
	}
	c -= 2;
	if (c < 0) c += THORNUMTONES;


	// Calculate soft-doppler / frequency-error of the symbol
	// For a perfect & undistorted symbol, rawdoppler will == 0 (be a perfect multiple of paths*doublespaced)
	rawdoppler = (currsymbol - prev1symbol) % (paths * doublespaced) ;
#if SOFTPROFILE
	LOG_INFO("Raw Doppler: %3d", rawdoppler);
#endif
	if ( 0 == rawdoppler)
		nowdoppler = 1.0f; // Perfect symbol: assign probability = 100%
	else {
		// Detect modem "de-sync + immediate re-sync" events and reverse the incurred soft-penalty
		// Probability of these events increases as baudrate increases
		if ( -1 * prev1rawdoppler == rawdoppler) {
			rawdoppler = 0;
			lastdoppler = 1.0f;
		}
		// calculate the nowdoppler soft-value as a probability >= 50%
		// centering the "50% confidence point" directly between two symbols.
		if ( abs(rawdoppler) <= paths * doublespaced / 2  )
			nowdoppler = 1.0 - ((1.0 / (paths * doublespaced)) * abs(rawdoppler)) ;
		else
			nowdoppler = 0.0 + ((1.0 / (paths * doublespaced)) * abs(rawdoppler)) ;
	}

	prev1rawdoppler = rawdoppler; // save raw-value for comparison on next run
#if SOFTPROFILE
	LOG_INFO("Doppler Confidence: %3.1f", nowdoppler);
#endif

// Since the THOR modem is differential, when an outofrange error occurs
// there are 2 possibilities:
//   . either previous (reference) symbol or current with a 50%
//     probability of each being the culprit.
// Either way, the current symbol is lost, puncture it. There is also a
// 50% probability the next symbol will have an error
	if (outofrange){
		lastmag /= 2;
		nowmag = 0;
		nextmag /= 2;
	}

	// O is puncture/null-symbol from softdecode
	if (0 == currsymbol) {
	  	nowmag = 0;
		nextmag = 0;
	}

	// puncture while squelched
	if (progStatus.sqlonoff && metric < progStatus.sldrSquelchValue) nowmag = 0;

	// One in 16 chance the correct reference tone chosen in staticburst
	if (staticburst){
		nowmag /= 16;
		nextmag /= 16;
	}

	// Apply the soft-doppler probability to the previous symbol's soft-magnitude
	lastmag *= lastdoppler;

	if (lastmag <= 0) { // puncture
		one = 128;
		zero = 128;
	}
	else if (lastmag > 127) { // limit
		one = 255;
		zero = 0;
	}
	else { // Calculate soft bits
		one = static_cast<unsigned char>( lastmag+128 ); // never > 255
		zero = static_cast<unsigned char>( 127-lastmag ); // never < 0
	}

#if SOFTPROFILE
	if (outofrange) LOG_INFO("%s","outofrange");
	if (staticburst) LOG_INFO("%s","staticburst");
	LOG_INFO("next mag %.3d | now mag %.3d | last mag %.3d \n",nextmag, nowmag, lastmag);
#endif

	lastsymbols[3] = (lastc & 1) == 1 ? one : zero ; lastc /= 2;
	lastsymbols[2] = (lastc & 1) == 1 ? one : zero ; lastc /= 2;
	lastsymbols[1] = (lastc & 1) == 1 ? one : zero ; lastc /= 2;
	lastsymbols[0] = (lastc & 1) == 1 ? one : zero ; lastc /= 2;

	Rxinlv->symbols(lastsymbols);
	for (int i = 0; i < 4; i++) decodePairs(lastsymbols[i]);

// Since modem is differential, wait until next symbol (to detect errors)
// then decode.
	lastc = c;
	lastmag = nowmag;
	nowmag = nextmag;
	lastdoppler = nowdoppler;
}

int thor::harddecode()
{
	double x, max = 0.0;
	int symbol = 0;
	double avg = 0.0;
	static bool cwi[MAXPATHS]; //[paths * numbins];
	double cwmag;

	for (int i = 0; i < MAXPATHS; i++) cwi[i] = false;

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

	for (int i = 0; i <  paths * numbins ; i++) {
		if (cwi[i] == false) {
			x = abs(pipe[pipeptr].vector[i]);
			if (x > max) {
				max = x;
				symbol = i;
			}
		} else
LOG_DEBUG("cwi detected in bin %d", i);
	}

	staticburst = (max / avg < 1.2);

	return symbol;
}

int thor::softdecode()
{
	static bool lastCWI[MAXPATHS] = {false};
	static bool nextCWI[MAXPATHS] = {false};
	static const int SoftBailout=6; // Max number of attempts to get a valid symbol

	double x, max = 0.0, avg = 0.0;
	int symbol = 0;
	int avgcount = 0;
	int soft_symbol_trycount=0;

	int lowest_tone = 0;
	int highest_tone = paths * numbins;

// Clear nextCWI  bool array for this run
	for (int i = 0; i < MAXPATHS; i++)
		nextCWI[i] = false;
//	for (int i = lowest_tone; i < highest_tone; i++) nextCWI[i] = false;

	// Allow for case where symbol == prev2symbol  (before calculating signal average...)
	if (prev2symbol && (prev2symbol < MAXPATHS - 1)) // array bounds checking
		lastCWI[prev2symbol-1] =  lastCWI[prev2symbol] = lastCWI[prev2symbol+1] = false;

	// Constrict the tone set further so CWI detect & set does not go out of intended range
	lowest_tone += 1;
	highest_tone -= 1;

	// Calculate signal average, ignoring CWI signals
	for (int i = lowest_tone; i < highest_tone; i++)
	{
		if ( !lastCWI[i]  ) {
			avg += abs(pipe[pipeptr].vector[i]);
			avgcount++;
		}
	}
	avg /= avgcount;
	if (avg < 1e-10) avg = 1e-10;


	// Run symbol-decoder until a non-CWI && non-Repeated symbol is selected
	do {
		soft_symbol_trycount++;
		max = 0.0;

		for (int i = lowest_tone; i < highest_tone; i++) {
		  	x = abs(pipe[pipeptr].vector[i]);
			if ( x > max && !nextCWI[i-1] && !nextCWI[i] && !nextCWI[i+1] ) {
				max = x;
				symbol = i;
			}
		}

		if (symbol && (symbol < MAXPATHS - 1)) {  // array bounds checking
		// detect repeated symbols (an invalid pattern for IFK+ modems)
			if ( abs(prev1symbol - symbol) < paths ) {
				nextCWI[symbol-1] = nextCWI[symbol] = nextCWI[symbol+1] = true;
			}
		// Update the next CWI mask if the tone from last-run persists
			else if ( lastCWI[symbol-1] || lastCWI[symbol] || lastCWI[symbol+1] ) {
				nextCWI[symbol-1] = nextCWI[symbol] = nextCWI[symbol+1] = true;
			}
		}

	} while ( nextCWI[symbol] && soft_symbol_trycount < SoftBailout ); // Run while the detected symbol has been identified as CWI (alt: bailout after 6 trys)

	// Copy the newly-detected CWI mask to the static lastCWI array for use on next function call
	for (int i = lowest_tone-1; i < highest_tone+1; i++) lastCWI[i] = nextCWI[i];

	staticburst = (max / avg < 1.2);

	// Return a NULL / Puncture symbol if a bailout occured
	if (soft_symbol_trycount >= SoftBailout) return 0;
	else return symbol;
}

bool thor::preambledetect(int c)
{
	static int preamblecheck=0, twocount=0;
	static bool neg16seen=false;

	if (twocount > 14 ) twocount = 0;

	if (-16 == c && twocount > 2 ) neg16seen = true;
// 2 does not reset the neg16seen bool
	else if (2 != c) neg16seen = false;
	else if (2 == c) twocount ++;

// -16 does not reset the twos counter
	if (-16 != c && 2 != c)
		if (twocount > 1) twocount -= 2;
#if SOFTPROFILE
	LOG_INFO("[2count:pcheck] [%d:%d]",twocount, preamblecheck);
#endif
	if ( twocount > 4 && neg16seen ){
		if ( ++preamblecheck > 4 ) return true;
	}
	else preamblecheck = 0;

	return false;
}

// Flush the interleaver and convolutional decoder with punctures
void thor::softflushrx()
{
#if SOFTPROFILE
LOG_INFO("%s", "softflushrx()");
#endif
	unsigned char puncture[2], flushsymbols[4];
	puncture[0]=128;
	puncture[1]=128;

	for (int i = 0; i < 4; i++) flushsymbols[i] = 128;

// flush interleaver with punctured symbols
	for(int i = 0; i < 90; i++) Rxinlv->symbols(flushsymbols);
// flush viterbi with puncture soft-bits
	for (int j = 0; j < 128; j++) Dec->decode (puncture, NULL);
}

void thor::update_syncscope()
{

	double max = 0, min = 1e6, range, mag;

	memset(videodata, 0, paths * numbins * sizeof(double));
//LOG_INFO("%s", "cleared videodata");
	if (!progStatus.sqlonoff || metric >= progStatus.sldrSquelchValue) {
		for (int i = 0; i < paths * numbins; i++ ) {
			mag = abs(pipe[pipeptr].vector[i]);
			if (max < mag) max = mag;
			if (min > mag) min = mag;
		}
		range = max - min;
		for (int i = 0; i < paths * numbins; i++ ) {
			if (range > 2) {
				mag = (abs(pipe[pipeptr].vector[i]) - min) / range + 0.0001;
				mag = 1 + 2 * log10(mag);
				if (mag < 0) mag = 0;
			} else
				mag = 0;
			videodata[(i + paths * numbins / 2)/2] = 255*mag;
		}
	}
	set_video(videodata, paths * numbins, false);
	videodata.next();

	memset(scopedata, 0, THORSCOPESIZE * sizeof(double));
	if (!progStatus.sqlonoff || metric >= progStatus.sldrSquelchValue) {
		for (unsigned int i = 0, j = 0; i < THORSCOPESIZE; i++) {
			j = (pipeptr + i * twosym / THORSCOPESIZE + 1) % (twosym);
			scopedata[i] = vidfilter[i]->run(abs(pipe[j].vector[prev1symbol]));
		}
	}
	set_scope(scopedata, THORSCOPESIZE);
	scopedata.next();
}

void thor::synchronize()
{
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

	synccounter += (int) floor(1.0 * (syn - symlen) / THORNUMTONES + 0.5);

	update_syncscope();

}


void thor::eval_s2n()
{
	double s = abs(pipe[pipeptr].vector[currsymbol]);
	double n = (THORNUMTONES - 1) *
				abs( pipe[(pipeptr + symlen) % twosym].vector[currsymbol]);

	sig = decayavg( sig, s, s - sig > 0 ? 4 : 20);
	noise = decayavg( noise, n, 64);

	if (noise)
		s2n = 20*log10(sig / noise);
	else
		s2n = 0;
	// To partially offset the increase of noise by (THORNUMTONES -1)
	// in the noise calculation above,
	// add 15*log10(THORNUMTONES -1) = 18.4, and multiply by 6
	metric = 6 * (s2n + 18.4);

	if (progdefaults.Pskmails2nreport && (mailserver || mailclient)) {
		// s2n reporting: re-calibrate
		s2n_metric = metric * 2.5 - 70;
		s2n_metric = CLAMP(s2n_metric, 0.0, 100.0);
	}

	metric = metric < 0 ? 0 : metric > 100 ? 100 : metric;

	display_metric(metric);

	snprintf(thormsg, sizeof(thormsg), "s/n %3.0f dB", s2n );
	put_Status1(thormsg);

	// Scale FEC indicatior to reduce erratic / jumpy / unreadable display in GUI
	int scalefec;
	if (fec_confidence++ > 90) scalefec = 100;
	else if (fec_confidence++ > 60) scalefec = 75;
	else if (fec_confidence++ > 40) scalefec = 50;
	else if (fec_confidence++ >= 20) scalefec = 25;
	else if ( fec_confidence > 9) scalefec = 10;
	else scalefec = 0; // Else just round to 0.

	snprintf(confidence, sizeof(confidence), "FEC: %3.1d%%", scalefec);
	put_Status2(confidence);
}

void thor::recvpic(double smpl)
{
	phidiff = 2.0 * M_PI * frequency / samplerate;
	img_phase -= phidiff;
	if (img_phase < 0) img_phase += 2.0 * M_PI;

	cmplx z = smpl * cmplx( cos(img_phase), sin(img_phase ) );
	picfilter->run( z, currz);
	double dphase = arg(conj(prevz) * currz);
	pixel = (samplerate / TWOPI) * pixfilter->run(dphase);
	sync = (samplerate / TWOPI) * pixsyncfilter->run(dphase);
	prevz = currz;

//if (image_counter == - (symlen / 2)) std::cout << "IMAGE START\n";

	image_counter++;
	if (image_counter < 0)
		return;

	if (state == IMAGE_START) {
		if (sync < -0.59 * bandwidth) {
			state = IMAGE_SYNC;
//std::cout << "IMAGE SYNC " << image_counter << "\n";
		}
		return;
	}
	if (state == IMAGE_SYNC) {
		if (sync > -0.51 * bandwidth) {
			state = IMAGE;
//std::cout << "IMAGE RECV " << image_counter << "\n";
		}
		return;
	}

	if ((image_counter % IMAGEspp) == 0) {
		byte = pixel * 256.0 / bandwidth + 128;
		byte = (int)CLAMP( byte, 0.0, 255.0);

		if (image_mode == 1) { // bw transmission
			pixelnbr = 3 * (col + row * picW);
			if (b_ava) {
				REQ(thor_update_avatar, byte, pixelnbr);
				REQ(thor_update_avatar, byte, pixelnbr + 1);
				REQ(thor_update_avatar, byte, pixelnbr + 2);
			} else {
				REQ(thor_updateRxPic, byte, pixelnbr);
				REQ(thor_updateRxPic, byte, pixelnbr + 1);
				REQ(thor_updateRxPic, byte, pixelnbr + 2);
			}
			if (++ col == picW) {
				col = 0;
				row++;
				if (row >= picH) {
					state = TEXT;
					REQ(thor_enableshift);
				}
			}
		} else { // color transmission
			pixelnbr = rgb + 3 * (col + row * picW);
			if (b_ava)
				REQ(thor_update_avatar, byte, pixelnbr);
			else
				REQ(thor_updateRxPic, byte, pixelnbr);
			if (++col == picW) {
				col = 0;
				if (++rgb == 3) {
					rgb = 0;
					++row;
				}
			}
			if (row > picH) {
				state = TEXT;
				REQ(thor_enableshift);
			}
		}
/*
		amplitude *= (samplerate/2)*(.734); // sqrt(3000 / (11025/2))
		s2n = 10 * log10(snfilt->run( amplitude * amplitude / noise));

		metric = 2 * (s2n + 20);
		metric = CLAMP(metric, 0, 100.0);  // -20 to +30 db range
		display_metric(metric);
		amplitude = 0;
*/
	}
}

int thor::rx_process(const double *buf, int len)
{
	cmplx zref, *zp;
	cmplx zarray[1];
	int n;

	if (slowcpu != progdefaults.slowcpu) {
		slowcpu = progdefaults.slowcpu;
		filter_reset = true;
	}

	if (filter_reset) reset_filters();

	while (len) {
		if (state != TEXT) {
			recvpic(*buf);
		} else {

// create analytic signal at first IF
			zref = cmplx( *buf, *buf );
			hilbert->run(zref, zref);
			zref = mixer(0, zref);

			if (progdefaults.THOR_FILTER && fft) {
// filter using fft convolution
				n = fft->run(zref, &zp);
			} else {
				zarray[0] = zref;
				zp = zarray;
				n = 1;
			}

			if (n) {
				for (int i = 0; i < n; i++) {
					cmplx * pipe_pipeptr_vector = pipe[pipeptr].vector ;
					const cmplx zp_i = zp[i];
// process THORMAXFFTS sets of sliding FFTs spaced at 1/THORMAXFFTS bin intervals each of which
// is a matched filter for the current symbol length
					for (int k = 0; k < paths; k++) {
// shift in frequency to base band for the sliding DFTs
						const cmplx z = mixer(k + 1, zp_i );
// copy current vector to the pipe interleaving the FFT vectors
						binsfft[k]->run(z, pipe_pipeptr_vector + k, paths );
					}
					if (--synccounter <= 0) {
						synccounter = symlen;

						if (progdefaults.THOR_SOFTSYMBOLS)
							currsymbol = softdecode();
						else
							currsymbol = harddecode();

						currmag = abs(pipe_pipeptr_vector[currsymbol]);
						eval_s2n();

						if (progdefaults.THOR_SOFTBITS)
							softdecodesymbol();
						else
							decodesymbol();

						synchronize();
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
		}
		buf++;
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
	if (cptr >= strSecXmtText.length()) cptr = 0;
	chr = strSecXmtText[cptr++];
	put_sec_char( chr );
	return chr;
}

void thor::sendtone(int tone, int duration)
{
	double f, phaseincr;
	f = (tone + 0.5) * tonespacing + get_txfreq_woffset() - bandwidth / 2;
	phaseincr = TWOPI * f / samplerate;
	for (int j = 0; j < duration; j++) {
		for (int i = 0; i < symlen; i++) {
			outbuf[i] = cos(txphase);
			txphase -= phaseincr;
			if (txphase < 0) txphase += TWOPI;
		}
		ModulateXmtr(outbuf, symlen);
	}
}

void thor::sendsymbol(int sym)
{
	cmplx z;
    int tone;

	tone = (txprevtone + 2 + sym) % THORNUMTONES;
    txprevtone = tone;
	if (reverse)
		tone = (THORNUMTONES - 1) - tone;
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
	for (int k = 0; k < 1400; k++) {
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
  for (int i = 0; i < flushlength; i++) sendidle();

  bitstate = 0;
}

static bool hide_after_sending = false;

int thor::tx_process()
{
	int i = 0;

	switch (txstate) {
	case TX_STATE_PREAMBLE:
		Clearbits();

		for (int j = 0; j < 16; j++) sendsymbol(0);

		sendidle();
		txstate = TX_STATE_START;
		break;
	case TX_STATE_START:
		sendchar('\r', 0);
        if (mode != MODE_THORMICRO) {
            sendchar(2, 0);		// STX
            sendchar('\r', 0);
        }
			txstate = TX_STATE_DATA;
		break;
	case TX_STATE_DATA:
		if (imageheader.length()) {
			txstate = TX_STATE_IMAGE;
			break;
		}
		if (avatarheader.length()) {
			txstate = TX_STATE_AVATAR;
			break;
		}
		i = get_tx_char();
		if (i == GET_TX_CHAR_NODATA)
			sendsecondary();
		else if (i == GET_TX_CHAR_ETX)
			txstate = TX_STATE_END;
		else
			sendchar(i, 0);
		if (stopflag) {
			txstate = TX_STATE_END;
			stopflag = false;
		}
		break;
	case TX_STATE_END:
		sendchar('\r', 0);
        if (mode != MODE_THORMICRO) {
            sendchar(4, 0);		// EOT
            sendchar('\r', 0);
        }
		txstate = TX_STATE_FLUSH;
		break;
	case TX_STATE_FLUSH:
		flushtx();
		cwid();
		txstate = TX_STATE_RECEIVE;
		return -1;
	case TX_STATE_IMAGE:
		for (size_t n = 0; n < imageheader.length(); n++)
			sendchar(imageheader[n], 0);
		flushtx();
		send_image();
		if (hide_after_sending) thorpicTxWin->hide();
		hide_after_sending = false;
		txstate = TX_STATE_DATA;
		break;
	case TX_STATE_AVATAR:
		for (size_t n = 0; n < avatarheader.length(); n++)
			sendchar(avatarheader[n], 0);
		flushtx();
		send_avatar();
		txstate = TX_STATE_DATA;
		break;
	}
	return 0;
}

// image support

#define PHASE_CORR  20

void thor::send_image() {
	int W = 640, H = 480;  // grey scale transfer (FAX)
	bool color = true;
	float freq, phaseincr;
	float radians = 2.0 * M_PI / samplerate;

	imageheader.clear();

	if (!thorpicTxWin || !thorpicTxWin->visible()) {
		return;
	}

	switch (selthorpicSize->value()) {
		case 0 : W = 59; H = 74; break;
		case 1 : W = 160; H = 120; break;
		case 2 : W = 320; H = 240; break;
		case 3 : W = 640; H = 480; color = false; break;
		case 4 : W = 640; H = 480; break;
		case 5 : W = 240; H = 300; break;
		case 6 : W = 240; H = 300; color = false; break;
		case 7 : W = 120; H = 150; break;
		case 8 : W = 120; H = 150; color = false; break;
	}

	REQ(thor_clear_tximage);

	double black[symlen];

	memset(black, 0, sizeof(*black) * symlen);
	for (int i = 0; i < PHASE_CORR; i++) ModulateXmtr(black, symlen);

	freq = frequency - 0.6 * bandwidth;
	phaseincr = radians * freq;
	for (int i = 0; i < PHASE_CORR; i++) {
		for (int n = 0; n < symlen; n++) {
			black[n] = cos(txphase);
			txphase -= phaseincr;
			if (txphase < 0) txphase += TWOPI;
		}
		ModulateXmtr(black, symlen);
	}

	if (color == false) {  // grey scale image
		for (int row = 0; row < H; row++) {
			memset(outbuf, 0, IMAGEspp * sizeof(*outbuf));
			for (int col = 0; col < W; col++) {
				if (stopflag) return;
				tx_pixelnbr = col + row * W;
				tx_pixel =	0.3 * thorpic_TxGetPixel(tx_pixelnbr, 0) +   // red
							0.6 * thorpic_TxGetPixel(tx_pixelnbr, 1) +   // green
							0.1 * thorpic_TxGetPixel(tx_pixelnbr, 2);    // blue
				REQ(thor_updateTxPic, tx_pixel, tx_pixelnbr*3 + 0);
				REQ(thor_updateTxPic, tx_pixel, tx_pixelnbr*3 + 1);
				REQ(thor_updateTxPic, tx_pixel, tx_pixelnbr*3 + 2);
				freq = frequency + (tx_pixel - 128) * bandwidth / 256.0;
				phaseincr = radians * freq;
				for (int n = 0; n < IMAGEspp; n++) {
					outbuf[n] = cos(txphase);
					txphase -= phaseincr;
					if (txphase < 0) txphase += TWOPI;
				}
				ModulateXmtr(outbuf, IMAGEspp);
				Fl::awake();
			}
		}
	} else {
		for (int row = 0; row < H; row++) {
			for (int color = 0; color < 3; color++) {
				memset(outbuf, 0, IMAGEspp * sizeof(*outbuf));
				for (int col = 0; col < W; col++) {
					if (stopflag) return;
					tx_pixelnbr = col + row * W;
					tx_pixel = thorpic_TxGetPixel(tx_pixelnbr, color);
					REQ(thor_updateTxPic, tx_pixel, tx_pixelnbr*3 + color);
					freq = frequency + (tx_pixel - 128) * bandwidth / 256.0;
					phaseincr = radians * freq;
					for (int n = 0; n < IMAGEspp; n++) {
						outbuf[n] = cos(txphase);
						txphase -= phaseincr;
						if (txphase < 0) txphase += TWOPI;
					}
					ModulateXmtr(outbuf, IMAGEspp);
				}
				Fl::awake();
			}
		}
	}

}

void thor::thor_send_image(std::string image_str) {
	if (!image_str.empty()) {
		hide_after_sending = true;
		imageheader = image_str;
	}
	if (txstate == TX_STATE_RECEIVE)
		start_tx();
}

void thor::send_avatar()
{
	int W = 59, H = 74;
	float freq, phaseincr;
	float radians = 2.0 * M_PI / samplerate;

	avatarheader.clear();

	double black[symlen];

	memset(black, 0, sizeof(*black) * symlen);

	freq = frequency - 0.6 * bandwidth;
	phaseincr = radians * freq;
	for (int i = 0; i < PHASE_CORR; i++) ModulateXmtr(black, symlen);

	for (int i = 0; i < PHASE_CORR; i++) {
		for (int n = 0; n < symlen; n++) {
			black[n] = cos(txphase);
			txphase -= phaseincr;
			if (txphase < 0) txphase += TWOPI;
		}
		ModulateXmtr(black, symlen);
	}

	for (int row = 0; row < H; row++) {
		for (int color = 0; color < 3; color++) {
			memset(outbuf, 0, IMAGEspp * sizeof(*outbuf));
			for (int col = 0; col < W; col++) {
				if (stopflag) return;
				tx_pixelnbr = col + row * W;
				tx_pixel = thor_get_avatar_pixel(tx_pixelnbr, color);
				freq = frequency + (tx_pixel - 128) * bandwidth / 256.0;
				phaseincr = radians * freq;
				for (int n = 0; n < IMAGEspp; n++) {
					outbuf[n] = cos(txphase);
					txphase -= phaseincr;
					if (txphase < 0) txphase += TWOPI;
				}
				ModulateXmtr(outbuf, IMAGEspp);
			}
			Fl::awake();
		}
	}
}

void thor::thor_send_avatar() {
	if (txstate == TX_STATE_RECEIVE) {
		start_tx();
	}
}
