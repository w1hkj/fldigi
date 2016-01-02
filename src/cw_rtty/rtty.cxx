// ----------------------------------------------------------------------------
// cw20.cxx  --  CW 2.0 modem
//
// CW 2.0 Copyright (C) 2015
//		John Phelps, KL4YFD
//
// Most functions for this modem were borrowed from
// the following Fldigi files: psk.cxx. mfsk.cxx, & rtty.cxx
// Many thanks to the following authors for the re-use of this code.
//		Dave Freese,  W1HKJ
//		John Douyere, VK2ETA
//		Stefan Fendt, DL1SMF
// 

///
/// Major Bugs:
/// Rx loses symbol alignment and stops decoding
/// 

//
// This file is part of fldigi.
//
// This code bears some resemblance to code contained in gmfsk from which
// it originated.  Much has been changed, but credit should still be
// given to Tomi Manninen (oh2bns@sral.fi), who so graciously distributed
// his gmfsk modem under the GPL.
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
#include <iostream>
#include <fstream>

using namespace std;

#include "view_rtty.h"
#include "fl_digi.h"
#include "digiscope.h"
#include "misc.h"
#include "waterfall.h"
#include "confdialog.h"
#include "configuration.h"
#include "status.h"
#include "digiscope.h"
#include "trx.h"
#include "debug.h"
#include "synop.h"
#include "main.h"
#include "modem.h"
#include "rtty.h"

#define FILTER_DEBUG 0

#define SHAPER_BAUD 150

// df=16 : correct up to 7 bits
#define K13		13
#define K13_POLY1	016461 // 7473
#define K13_POLY2	012767 // 5623

extern bool withnoise;


int dspcnt = 0;

//static char msg1[20];

const double	rtty::SHIFT[] = {23, 85, 160, 170, 182, 200, 240, 350, 425, 850};
// FILTLEN must be same size as BAUD
const double	rtty::BAUD[]  = {45, 45.45, 50, 56, 75, 100, 110, 80, 20, 40};
const int		rtty::FILTLEN[] = { 512, 512, 512, 512, 512, 512, 512, 256, 512, 512};
const int		rtty::BITS[]  = {5, 7, 8};
const int		rtty::numshifts = (int)(sizeof(SHIFT) / sizeof(*SHIFT));
const int		rtty::numbauds = (int)(sizeof(BAUD) / sizeof(*BAUD));

void rtty::tx_init(SoundBase *sc)
{
	scard = sc;
	phaseacc = 0;
	preamble = true;
	videoText();

	symbols = 0;
	acc_symbols = 0;
	ovhd_symbols = 0;
}


void rtty::rx_init()
{
	rxstate = RTTY_RX_STATE_IDLE;
	rxmode = LETTERS;
	phaseacc = 0;
	FSKphaseacc = 0;

	for (int i = 0; i < MAXBITS; i++ ) bit_buf[i] = 0.0;

	mark_phase = 0;
	space_phase = 0;
	xy_phase = 0.0;

	mark_mag = 0;
	space_mag = 0;
	mark_env = 0;
	space_env = 0;

	inp_ptr = 0;

	lastchar = 0;
}

void rtty::init()
{

	stopflag = false;

	rx_init();

	if (rtty_baud == 20) put_MODEstatus(" CW 2.0 ");
	else if (rtty_baud == 40) put_MODEstatus("CW 2.0 FEC");
	else if (rtty_baud == 80) put_MODEstatus("CW 2.0 FAST");
	
	if (progdefaults.PreferXhairScope)
		set_scope_mode(Digiscope::XHAIRS);
	else
		set_scope_mode(Digiscope::RTTY);
	for (int i = 0; i < MAXPIPE; i++) mark_history[i] = space_history[i] = cmplx(0,0);

	lastchar = 0;
}

rtty::~rtty()
{
	if (rttyviewer) delete rttyviewer;

	if (mark_filt) delete mark_filt;
	if (space_filt) delete space_filt;
	if (pipe) delete [] pipe;
	if (dsppipe) delete [] dsppipe;
	if (bits) delete bits;
	if (rxinlv1) delete rxinlv1;
	if (rxinlv2) delete rxinlv2;
	if (txinlv) delete txinlv;
	if (dec2) delete dec2;
	if (dec1) delete dec1;
	if (enc) delete enc;
}

void rtty::reset_filters()
{
	delete mark_filt;
	mark_filt = new fftfilt(rtty_baud/samplerate, filter_length);
	mark_filt->rtty_filter(rtty_baud/samplerate);
	delete space_filt;
	space_filt = new fftfilt(rtty_baud/samplerate, filter_length);
	space_filt->rtty_filter(rtty_baud/samplerate);
}

void rtty::restart()
{
	rtty_shift = shift = (progdefaults.rtty_shift < numshifts ?
				  SHIFT[progdefaults.rtty_shift] : progdefaults.rtty_custom_shift);
	if (progdefaults.rtty_baud > numbauds - 1) progdefaults.rtty_baud = numbauds - 1;
	rtty_baud = BAUD[progdefaults.rtty_baud];
	filter_length = FILTLEN[progdefaults.rtty_baud];

	nbits = rtty_bits = BITS[progdefaults.rtty_bits];


	symbollen = (int) (samplerate / rtty_baud + 0.5);
	set_bandwidth(shift);

	rtty_BW = progdefaults.RTTY_BW = rtty_baud * 2;

	wf->redraw_marker();

	reset_filters();

	if (bits)
		bits->setLength(symbollen / 8);//2);
	else
		bits = new Cmovavg(symbollen / 8);//2);
	mark_noise = space_noise = 0;
	bit = nubit = true;


	freqerr = 0.0;
	pipeptr = 0;

	for (int i = 0; i < MAXBITS; i++ ) bit_buf[i] = 0.0;

	metric = 0.0;

	if (rtty_baud == 20) put_MODEstatus(" CW 2.0 ");
	else if (rtty_baud == 40) put_MODEstatus("CW 2.0 FEC");
	else if (rtty_baud == 80) put_MODEstatus("CW 2.0 FAST");
	//put_Status1(msg1);
	
	
	for (int i = 0; i < MAXPIPE; i++)
		QI[i] = cmplx(0.0, 0.0);
	sigpwr = 0.0;
	noisepwr = 0.0;
	sigsearch = 0;
	dspcnt = 2*(nbits + 2);

	clear_zdata = true;

	mark_phase = 0;
	space_phase = 0;
	xy_phase = 0.0;

	mark_mag = 0;
	space_mag = 0;
	mark_env = 0;
	space_env = 0;

	inp_ptr = 0;
	
	txinlv->flush();
	rxinlv1->flush();
	rxinlv2->flush();


	for (int i = 0; i < MAXPIPE; i++) mark_history[i] = space_history[i] = cmplx(0,0);

	if (rttyviewer) rttyviewer->restart();

	progStatus.rtty_filter_changed = false;
	

}

rtty::rtty(trx_mode tty_mode)
{
	cap |= CAP_AFC | CAP_REV;

	mode = tty_mode;

	samplerate = RTTY_SampleRate;

	mark_filt = (fftfilt *)0;
	space_filt = (fftfilt *)0;

	bits = (Cmovavg *)0;

	pipe = new double[MAXPIPE];
	dsppipe = new double [MAXPIPE];

	rttyviewer = new view_rtty(mode);
	
	/// CW 2.0 Feature: 9+ db gain FEC
	enc = new encoder (K13, K13_POLY1, K13_POLY2);
	dec1 = new viterbi (K13, K13_POLY1, K13_POLY2);
	dec2 = new viterbi (K13, K13_POLY1, K13_POLY2);
	dec1->setchunksize (4);
	dec2->setchunksize (4);
	
	///CW 2.0 Feature: 500 ms interleave
	txinlv = new interleave (2, 20, INTERLEAVE_FWD);
	rxinlv1 = new interleave (2, 20, INTERLEAVE_REV);
	rxinlv2 = new interleave (2, 20, INTERLEAVE_REV);
	
	restart();

}

void rtty::Update_syncscope()
{
	int j;
	for (int i = 0; i < symbollen; i++) {
		j = pipeptr - i;
		if (j < 0) j += symbollen;
		dsppipe[i] = pipe[j];
	}
	set_scope(dsppipe, symbollen, false);
}

void rtty::Clear_syncscope()
{
	set_scope(0, 0, false);
}

cmplx rtty::mixer(double &phase, double f, cmplx in)
{
	cmplx z = cmplx( cos(phase), sin(phase)) * in;

	phase -= TWOPI * f / samplerate;
	if (phase < -TWOPI) phase += TWOPI;

	return z;
}



bool rtty::rx(bool bit)
{
	static int bitcounter = 0;
	static unsigned int hardshreg = 1;
	
	// Shift contents of the the bit buffer and add the passed bit
	for (int i = 1; i < symbollen*SYMBLOCK; i++) bit_buf[i-1] = bit_buf[i];
	bit_buf[symbollen*SYMBLOCK - 1] = bit;

	if (bitcounter++ >= symbollen-1) {
		bitcounter = 0;

		// debugging:
		//for (int i = 1; i < symbollen; i++) printf( "%d", bit_buf[i] );

		int softzeros[3] = {0};
		int  softones[3] = {0};
		
		// Count bits in the bit_buf to produce a soft symbol from first 1/3 of symbol
		for (int i = 0; i < symbollen/3; i++) {
			if (bit_buf[i]) softones[0]++;
			else softzeros[0]++;
		}
		// Count bits in the bit_buf to produce a soft symbol from middle 1/3 of symbol
		for (int i = symbollen/3; i < 2 * symbollen/3; i++) {
			if (bit_buf[i]) softones[1]++;
			else softzeros[1]++;
		}
		// Count bits in the bit_buf to produce a soft symbol from last 1/3 of symbol
		for (int i = 2 * symbollen/3; i < symbollen; i++) {
			if (bit_buf[i]) softones[2]++;
			else softzeros[2]++;
		}
		
		int votescore[3] = {-1}; // Preload with invalid value 
		int vote = -1; // Preload with invalid value 
		// Calculate the vote on which 1/3 of the symbol has the greatest difference of ones to zeros
		for (int i=0; i<3; i++) {
			votescore[i] = abs(softones[i] - softzeros[i]);
			if (votescore[i] > vote)
				vote = i;
		}
		
		int ones = softones[vote];
		int zeros = softzeros[vote];
			  
		int hardbit = -1;
		int softbit = 128;
		// Both hard and soft decode here
		if (ones > zeros) {
			hardbit = 1;
			softbit = 255 - zeros * 1.4; // 1.4 is a magic number specific to 40 baud
		} else {
			hardbit = 0;
			softbit = 0 + ones * 1.4; // 1.4 is a magic number specific to 40 baud
		}
		
		// If FEC mode, soft-decode and return
		if (rtty_baud == 40) {
			rx_pskr(softbit);
			return false;
		}
		
		// Implied else Non FEC mode: decode hard-bits to character 
		int c;
		hardshreg = (hardshreg << 1) | !!hardbit;
		if ((hardshreg & 7) == 1) {
			c = varidec(hardshreg >> 1);
			put_rx_char(c);
			hardshreg = 1;
		}
		return true;
	}
	return true;	
}

void rtty::rx_pskr(unsigned char symbol)
{
	int met;
	unsigned char twosym[2];
	unsigned char tempc;
	int c;
	
	static int rxbitstate=0;

	// we accumulate the soft bits for the interleaver THEN submit to Viterbi
	// decoder in alternance so that each one is processed one bit later.
	// Only two possibilities for sync: current bit or previous one since
	// we encode with R = 1/2 and send encoded bits one after the other
	// through the interleaver.

	symbolpair[1] = symbolpair[0];
	symbolpair[0] = symbol;


	if (rxbitstate == 0) {
		rxbitstate++;
		// copy to avoid scrambling symbolpair for the next bit
		twosym[0] = symbolpair[0];
		twosym[1] = symbolpair[1];
		// De-interleave
		rxinlv2->symbols(twosym);
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
		rxinlv1->symbols(twosym);
		tempc = twosym[1];
		twosym[1] = twosym[0];
		twosym[0] = tempc;
		// Then viterbi decoder
		c = dec1->decode(twosym, &met);
		if (c != -1) {
			fecmet = decayavg(fecmet, met, 20);
			rx_bit(c & 0x08);
			rx_bit(c & 0x04);
			rx_bit(c & 0x02);
			rx_bit(c & 0x01);
		}
	}
}




void rtty::rx_bit(int bit)
{
	int c;
	static unsigned int shreg = 1;

	shreg = (shreg << 1) | !!bit;
	if ((shreg & 7) == 1) {
		c = varidec(shreg >> 1);
		// Voting at the character level
		if (fecmet > fecmet2) {
			if ((c != -1) && (c != 0))
				put_rx_char(c);
		}
		shreg = 1;
	}
}


void rtty::rx_bit2(int bit)
{
	int c;
	static unsigned int shreg2 = 1;
	
	shreg2 = (shreg2 << 1) | !!bit;
	// MFSK varicode instead of PSK Varicode
	if ((shreg2 & 7) == 1) {
		c = varidec(shreg2 >> 1);
		// Voting at the character level
		if (fecmet < fecmet2) {
			if ((c != -1) && (c != 0))
				put_rx_char(c);
		}
		shreg2 = 1;
	}
}


char snrmsg[80];
void rtty::Metric()
{
	double delta = rtty_baud/8.0;
	double np = wf->powerDensity(frequency, delta) * 3000 / delta;
	double sp =
		wf->powerDensity(frequency - shift/2, delta) +
		wf->powerDensity(frequency + shift/2, delta) + 1e-10;
	double snr = 0;

	sigpwr = decayavg( sigpwr, sp, sp > sigpwr ? 2 : 8);
	noisepwr = decayavg( noisepwr, np, 16 );
	snr = 10*log10(sigpwr / noisepwr);

	snprintf(snrmsg, sizeof(snrmsg), "s/n %-3.0f dB", snr);
	put_Status2(snrmsg);
	metric = CLAMP((3000 / delta) * (sigpwr/noisepwr), 0.0, 100.0);
	display_metric(metric);
}

int rtty::rx_process(const double *buf, int len)
{
	const double *buffer = buf;
	int length = len;
	static int showxy = symbollen;

	cmplx z, zmark, zspace, *zp_mark, *zp_space;

	int n_out = 0;
	static int bitcount = 5 * nbits * symbollen;

	if ( !progdefaults.report_when_visible ||
		 dlgViewer->visible() || progStatus.show_channels )
		if (!bHistory && rttyviewer) rttyviewer->rx_process(buf, len);

	if (progStatus.rtty_filter_changed) {
		progStatus.rtty_filter_changed = false;
		reset_filters();
	}

	Metric();

	while (length-- > 0) {

// Create analytic signal from sound card input samples


	z = cmplx(*buffer, *buffer);
	buffer++;

// Mix it with the audio carrier frequency to create two baseband signals
// mark and space are separated and processed independently
// lowpass Windowed Sinc - Overlap-Add convolution filters.
// The two fftfilt's are the same size and processed in sync
// therefore the mark and space filters will concurrently have the
// same size outputs available for further processing

		zmark = mixer(mark_phase, frequency + shift/2.0, z);
		mark_filt->run(zmark, &zp_mark);

		zspace = mixer(space_phase, frequency - shift/2.0, z);
		n_out = space_filt->run(zspace, &zp_space);

		for (int i = 0; i < n_out; i++) {

			mark_mag = abs(zp_mark[i]);
			mark_env = decayavg (mark_env, mark_mag,
						(mark_mag > mark_env) ? symbollen / 4 : symbollen * 16);
			mark_noise = decayavg (mark_noise, mark_mag,
						(mark_mag < mark_noise) ? symbollen / 4 : symbollen * 48);
			space_mag = abs(zp_space[i]);
			space_env = decayavg (space_env, space_mag,
						(space_mag > space_env) ? symbollen / 4 : symbollen * 16);
			space_noise = decayavg (space_noise, space_mag,
						(space_mag < space_noise) ? symbollen / 4 : symbollen * 48);

			noise_floor = min(space_noise, mark_noise);

// clipped if clipped decoder selected
			double mclipped = 0, sclipped = 0;
			mclipped = mark_mag > mark_env ? mark_env : mark_mag;
			sclipped = space_mag > space_env ? space_env : space_mag;
			if (mclipped < noise_floor) mclipped = noise_floor;
			if (sclipped < noise_floor) sclipped = noise_floor;

			/// KL4YFD  CW 2.0 mark only decode
			space_env = sclipped = noise_floor;


//			double v0, v1, v2, v3, v4, v5;

// no ATC
//			v0 = mark_mag - space_mag;
// Linear ATC
			double v1 = mark_mag - space_mag - 0.5 * (mark_env - space_env);
// Clipped ATC
//			v2  = (mclipped - noise_floor) - (sclipped - noise_floor) - 0.5 * (
//					(mark_env - noise_floor) - (space_env - noise_floor));
// Optimal ATC
//			v3  = (mclipped - noise_floor) * (mark_env - noise_floor) -
//					(sclipped - noise_floor) * (space_env - noise_floor) - 0.25 * (
//					(mark_env - noise_floor) * (mark_env - noise_floor) -
//					(space_env - noise_floor) * (space_env - noise_floor));
// Kahn Squarer with Linear ATC
//			v4 =  (mark_mag - noise_floor) * (mark_mag - noise_floor) -
//					(space_mag - noise_floor) * (space_mag - noise_floor) - 0.25 * (
//					(mark_env - noise_floor) * (mark_env - noise_floor) -
//					(space_env - noise_floor) * (space_env - noise_floor));
// Kahn Squarer with Clipped ATC
//			v5 =  (mclipped - noise_floor) * (mclipped - noise_floor) -
//					(sclipped - noise_floor) * (sclipped - noise_floor) - 0.25 * (
//					(mark_env - noise_floor) * (mark_env - noise_floor) -
//					(space_env - noise_floor) * (space_env - noise_floor));
//				switch (progdefaults.rtty_demodulator) {
//			switch (2) { // Optimal ATC
//			case 0: // linear ATC
				bit = v1 > 0;
//				break;
//			case 1: // clipped ATC
//				bit = v2 > 0;
//				break;
//			case 2: // optimal ATC
//				bit = v3 > 0;
//				break;
//			case 3: // Kahn linear ATC
//				bit = v4 > 0;
//				break;
//			case 4: // Kahn clipped
				//bit = v5 > 0;
//				break;
//			case 5: // No ATC
//			default :
//				bit = v0 > 0;
//			}

// XY scope signal generation

			if (progdefaults.true_scope) {
//----------------------------------------------------------------------
// "true" scope implementation------------------------------------------
//----------------------------------------------------------------------

// get the baseband-signal and...
				xy = cmplx(
						zp_mark[i].real() * cos(xy_phase) + zp_mark[i].imag() * sin(xy_phase),
						zp_space[i].real() * cos(xy_phase) + zp_space[i].imag() * sin(xy_phase) );

// if mark-tone has a higher magnitude than the space-tone,
// further reduce the scope's space-amplitude and vice versa
// this makes the scope looking a little bit nicer, too...
// aka: less noisy...
				if( abs(zp_mark[i]) > abs(zp_space[i]) ) {
// note ox x complex lib does not support xy.real(double) or xy.imag(double)
					xy = cmplx( xy.real(),
								xy.imag() * abs(zp_space[i])/abs(zp_mark[i]) );
//					xy.imag() *= abs(zp_space[i])/abs(zp_mark[i]);
				} else {
					xy = cmplx( xy.real() / ( abs(zp_space[i])/abs(zp_mark[i]) ),
								xy.imag() );
//					xy.real() /= abs(zp_space[i])/abs(zp_mark[i]);
				}

// now normalize the scope
				double const norm = 1.3*(abs(zp_mark [i]) + abs(zp_space[i]));
				xy /= norm;

			} else {
//----------------------------------------------------------------------
// "ortho" scope implementation-----------------------------------------
//----------------------------------------------------------------------
// get magnitude of the baseband-signal
				if (bit)
					xy = cmplx( mark_mag * cos(xy_phase), space_noise * sin(xy_phase) / 2.0);
				else
					xy = cmplx( mark_noise * cos(xy_phase) / 2.0, space_mag * sin(xy_phase));
// now normalize the scope
				double const norm = (mark_env + space_env);
				xy /= norm;
			}

// Rotate the scope x-y iaw frequency error.  Old scopes were not capable
// of this, but it should be very handy, so... who cares of realism anyways?
			double const rotate = 8 * TWOPI * freqerr / rtty_shift;
			xy = xy * cmplx(cos(rotate), sin(rotate));

			QI[inp_ptr] = xy;

// shift it to 128Hz(!) and not to it's original position.
// this makes it more pretty and does not remove it's other
// qualities. Reason is that this is a fraction of the used
// block-size.
			xy_phase += (TWOPI * (128.0 / samplerate));
// end XY signal generation

			mark_history[inp_ptr] = zp_mark[i];
			space_history[inp_ptr] = zp_space[i];

			inp_ptr = (inp_ptr + 1) % MAXPIPE;

			if (dspcnt && (--dspcnt % (nbits + 2) == 0)) {
				pipe[pipeptr] = bit - 0.5; //testbit - 0.5;
				pipeptr = (pipeptr + 1) % symbollen;
			}

// detect TTY signal transitions
// rx(...) returns true if valid TTY bit stream detected
// either character or idle signal
			
			rx(bit);
/*
			if ( rx( bit ) ) {
				dspcnt = symbollen * (nbits + 2);
				if (!bHighSpeed) Update_syncscope();
				clear_zdata = true;
				bitcount = 5 * nbits * symbollen;
				if (sigsearch) sigsearch--;
					int mp0 = inp_ptr - 2;
				int mp1 = mp0 + 1;
				if (mp0 < 0) mp0 += MAXPIPE;
				if (mp1 < 0) mp1 += MAXPIPE;
				double ferr = (TWOPI * samplerate / rtty_baud) * arg(conj(mark_history[mp1]) * mark_history[mp0]);
				if (fabs(ferr) > rtty_baud / 2) ferr = 0;
				freqerr = decayavg ( freqerr, ferr / 8,
					progdefaults.rtty_afcspeed == 0 ? 8 :
					progdefaults.rtty_afcspeed == 1 ? 4 : 1 );
				if (progStatus.afconoff &&
					(metric > progStatus.sldrSquelchValue || !progStatus.sqlonoff))
					set_freq(frequency - freqerr);
			} else
				if (bitcount) --bitcount;
				
*/
		}
		if (!bHighSpeed) {
			if (!bitcount) {
				if (clear_zdata) {
					clear_zdata = false;
					Clear_syncscope();
					for (int i = 0; i < MAXPIPE; i++)
						QI[i] = cmplx(0.0, 0.0);
				}
			}
			if (!--showxy) {
				set_zdata(QI, MAXPIPE);
				showxy = symbollen;
			}
		}
	}
	return 0;
}

//=====================================================================
// RTTY transmit
//=====================================================================
//double freq1;
double maxamp = 0;

// Left channel audio signal
// Is only generated for testing when starting Fldigi with parameter:  --noise
double rtty::nco(double freq)
{
	if (!withnoise) return 0.0f; // No Left-channel / Mic-input Audio unless testing: CW 2.0 Transmits by keying the radio's CW key

	// audio signal for testing only. DO NOT USE ON-AIR!
	phaseacc += TWOPI * freq / samplerate;
	if (phaseacc > TWOPI) phaseacc -= TWOPI;
	return cos(phaseacc);
}

// Right channel PTT signal at 3.2Khz
// 3.2Khz is outside of normal SSB audio passband in case of incorrect connections
// but well within the 4Khz Nyquist limit
double rtty::FSKnco()
{
	FSKphaseacc += TWOPI * 3200 / samplerate;

	if (FSKphaseacc > TWOPI) FSKphaseacc -= TWOPI;

	return sin(FSKphaseacc);

}


void rtty::send_symbol(unsigned int symbol, int len)
{
	acc_symbols += len;

	double freq;

	// transmit mark-only
	// BUG: Rx and Tx center freq is offset by + shift/2
	freq = get_txfreq_woffset() + shift / 2.0;

	for (int i = 0; i < len; i++) {
		if (symbol) {
		  	outbuf[i] = nco(freq);
			FSKbuf[i] = FSKnco();
		} else {
			outbuf[i] = 0.0 * nco(freq); // phase-coherent:keep 0 deg phase difference between symbols
			FSKbuf[i] = 0.0 * FSKnco();
		}
	}

	/// With --noise commandline parameter: only generate the Audio / Left channel
	/// Without --noise: only generate the CW-KEY-PTT / Right channel
	if (!withnoise)
		ModulateStereo(outbuf, FSKbuf, symbollen);
	else
		ModulateXmtr(outbuf, symbollen);
}

void rtty::send_char(unsigned char  c)
{
	if (rtty_baud != 40) { // Non FEC mode
	  	if (restartchar) { // Send a NULL character to re-synchronize the receiver
			const char *code = varienc(0);
			while (*code)
				send_symbol( (*code++ - '0'), symbollen);
			restartchar = false;
		}
		
		const char *code = varienc(c);
		while (*code)
			send_symbol( (*code++ - '0'), symbollen);
		
	} else { // FEC MODE
	  	if (restartchar) { // Send a NULL character to re-synchronize the receiver
			const char *code = varienc(0);
			while (*code) {
				txdata = enc->encode( (*code++ - '0') );
				txinlv->bits(&txdata);
				send_symbol(txdata &1 , symbollen);
				send_symbol(txdata &2 , symbollen);
			}
			restartchar = false;
		}
		
	  	const char *code = varienc(c);
		while (*code) {
		  	txdata = enc->encode( (*code++ - '0') );
			txinlv->bits(&txdata);
			send_symbol(txdata &1 , symbollen);
			send_symbol(txdata &2 , symbollen);
		}	
	}

	put_echo_char(c);
	return;
}

// send idle in a way that both keeps FEC synchronized and decodes to nothing in MFSK varicode
// After a few 0's as input, the FEC will output a constant string of 0's (key-ups)
void rtty::send_idle()
{
	txdata = enc->encode( 0 ); // Keep synchronization with string of 0 bits
	txinlv->bits(&txdata);
	send_symbol(txdata &1 , symbollen);
	send_symbol(txdata &2 , symbollen);
}

static int line_char_count = 0;

int rtty::tx_process()
{
	int c;

	/// CW 2.0 feature: The Non-FEC preamble is heard as the letter "N" twice in Morse code
	if (preamble) {
	  	// Clear the bits in Viterbi encoder and TX interleaver
		enc->init();
		txinlv->flush();
		
		if (rtty_baud != 40) { // Non FEC
			send_char('\n'); // CR : enter
			send_char('\n');
		} else { // FEC
			send_char(0); // NULL
		}
		preamble = false;
	}
	c = get_tx_char();

// Stop or end of transmission
	if (c == GET_TX_CHAR_ETX || stopflag) {
		stopflag = false;
		line_char_count = 0;
		if (rtty_baud == 40) { // FEC
			for (int i=0; i<4; i++) send_char(0); // flush the FEC TR & Rx pipelines with NULLs
		} else { // CW 2.0 feature: The Non-FEC postamble is heard as the letter "N" twice in Morse code
			send_char('\n'); // CR : enter
			send_char('\n');
		}
		cwid(); /// send callsign in Morse code 
		return -1;
	}

// TX buffer empty
	/// CW 2.0 Feature: 
	/// When there is no character to send, mode becomes RF silent.
	if (c == GET_TX_CHAR_NODATA) {
		if (!restartchar) {
			send_char(0); // send a NULL to flush last characters through Rx pipeline
			if (rtty_baud == 40) send_char(0); // for FEC: send a second NULL to flush last characters through FEC pipeline
		}
	  	restartchar = true; // Request later need for a buffer/restart character (to re-synchronize)
		
		if (rtty_baud == 40)
			send_idle(); // FEC requires synchronization to be kept...
		else
			send_symbol(0, symbollen); // nothing to send: don't waste the RF power with an idle signal
		return 0;
	}


	acc_symbols = 0;
	send_char(c);
	xmt_samples = char_samples = acc_symbols;

	return 0;
}
  