// ----------------------------------------------------------------------------
// rtty.cxx  --  RTTY modem
//
// Copyright (C) 2012
//		Dave Freese, W1HKJ
//		Stefan Fendt, DL1SMF
// 
// CW 2.0 Copyright (C) 2015
//		John Phelps, KL4YFD
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

//#include "rtty.h"
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
// Code has good ac(df) and bc(df) parameters for puncturing
#define K13		13
#define K13_POLY1	016461 // 7473
#define K13_POLY2	012767 // 5623

int dspcnt = 0;

static char msg1[20];

const double	rtty::SHIFT[] = {23, 85, 160, 170, 182, 200, 240, 350, 425, 850};
// FILTLEN must be same size as BAUD
const double	rtty::BAUD[]  = {45, 45.45, 50, 56, 75, 100, 110, 150, 20, 40};
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

// Customizes output of Synop decoded data.
struct rtty_callback : public synop_callback {
	// Callback for writing decoded synop messages.
	void print(const char * str, size_t nb, bool bold ) const {
		// Could choose: FTextBase::CTRL,XMIT,RECV
		int style = bold ? FTextBase::XMIT : FTextBase::RECV;
		for( size_t i = 0; i < nb; ++i ) {
			unsigned char c = str[i];
			put_rx_char(progdefaults.rx_lowercase ? tolower(c) : c, style );
		}
	}
	// Should we log new Synop messages to the current Adif log file ?
	bool log_adif(void) const { return progdefaults.SynopAdifDecoding ;}
	// Should we log new Synop messages to KML file ?
	bool log_kml(void) const { return progdefaults.SynopKmlDecoding ;}

	bool interleaved(void) const { return progdefaults.SynopInterleaved ;}
};

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

	// Synop file is reloaded each time we enter this modem. Ideally do that when the file is changed.
	static bool wmo_loaded = false ;
	if( wmo_loaded == false ) {
		wmo_loaded = true ;
		SynopDB::Init(PKGDATADIR);
	}
	/// Used by weather reports decoding.
	synop::setup<rtty_callback>();
	synop::instance()->init();
}

void rtty::init()
{
	bool wfrev = wf->Reverse();
	bool wfsb = wf->USB();
	// Probably not necessary because similar to modem::set_reverse
	reverse = wfrev ^ !wfsb;
	stopflag = false;

	if (progdefaults.StartAtSweetSpot)
		set_freq(progdefaults.RTTYsweetspot);
	else if (progStatus.carrier != 0) {
		set_freq(progStatus.carrier);
#if !BENCHMARK_MODE
		progStatus.carrier = 0;
#endif
	} else
		set_freq(wf->Carrier());

	rx_init();

	if (rtty_baud == 20) put_MODEstatus(" CW 2.0 ");
	else if (rtty_baud == 40) put_MODEstatus("CW 2.0 FEC");
	
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
	double stl;

	rtty_shift = shift = (progdefaults.rtty_shift < numshifts ?
				  SHIFT[progdefaults.rtty_shift] : progdefaults.rtty_custom_shift);
	if (progdefaults.rtty_baud > numbauds - 1) progdefaults.rtty_baud = numbauds - 1;
	rtty_baud = BAUD[progdefaults.rtty_baud];
	filter_length = FILTLEN[progdefaults.rtty_baud];

	nbits = rtty_bits = BITS[progdefaults.rtty_bits];
	rtty_parity = RTTY_PARITY_NONE;

	// (exists below already)  rtty_stop = progdefaults.rtty_stop;

	txmode = LETTERS;
	rxmode = LETTERS;
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

// stop length = 1, 1.5 or 2 bits
	rtty_stop = progdefaults.rtty_stop;
	if (rtty_stop == 0) stl = 1.0;
	else if (rtty_stop == 1) stl = 1.5;
	else stl = 2.0;
	stoplen = (int) (stl * samplerate / rtty_baud + 0.5);
	freqerr = 0.0;
	pipeptr = 0;

	for (int i = 0; i < MAXBITS; i++ ) bit_buf[i] = 0.0;

	metric = 0.0;

	// KL4YFD
	if (rtty_baud == 20) put_MODEstatus(" CW 2.0 ");
	else if (rtty_baud == 40) put_MODEstatus("CW 2.0 FEC");
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
	
	enc = new encoder (K13, K13_POLY1, K13_POLY2);
	dec1 = new viterbi (K13, K13_POLY1, K13_POLY2);
	dec2 = new viterbi (K13, K13_POLY1, K13_POLY2);
	dec1->setchunksize (1);
	dec2->setchunksize (1);
	
	/// KL4YFD  temp/testing values for inlv
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


bool rtty::is_mark_space( int &correction)
{
	correction = 0;
// test for rough bit position
	if (bit_buf[0] && !bit_buf[symbollen-1]) {
// test for mark/space straddle point
		for (int i = 0; i < symbollen; i++)
			correction += bit_buf[i];
		if (abs(symbollen/2 - correction) < 6) // too small & bad signals are not decoded
			return true;
	}
	return false;
}

bool rtty::is_mark()
{
	return bit_buf[symbollen / 2];
}



int rtty::decodesymbol(unsigned char symbol)
{
	int c, met;
	static unsigned char symbolpair[2];
	static int symcounter, met1, met2;
	
	symbolpair[0] = symbolpair[1];
	symbolpair[1] = symbol;

	symcounter = symcounter ? 0 : 1;

	if (symcounter) {
		if ((c = dec1->decode(symbolpair, &met)) == -1)
			return -1;
		met1 = decayavg(met1, met, 50);
		if (met1 < met2)
			return -1;
		metric = met1;
	
	} else {
		if ((c = dec2->decode(symbolpair, &met)) == -1)
			return -1;
		met2 = decayavg(met2, met, 50);
		if (met2 < met1)
			return -1;
		 metric = met2;
	}

	// Re-scale the metric and update main window
	metric -= 60.0;
	metric *= 0.5;

	metric = CLAMP(metric, 0.0, 100.0);

	return(c &1);

}


bool rtty::rx(bool bit) // original modified for probability test
{
  /*
	bool flag = false;
	unsigned char c = 0;
	int correction;
  */	
	// kl4yfd
	// temp hard-decode solution
	static int onescount = 0;
	static int zeroscount = 0;
	static int bitcounter = 0;
	
	static unsigned int datashreg = 1;
	
	for (int i = 1; i < symbollen; i++) bit_buf[i-1] = bit_buf[i];
	bit_buf[symbollen - 1] = bit;
	
	// count the passed bits for a vote
	if (bit) onescount++;
	else zeroscount++;
	
	
	// kl4yfd
	// BUG: need a working alignment/correction algorithm
	/*
	if (onescount + zeroscount > 2*symbollen/3) {
		if (onescount == zeroscount)
		      bitcounter += symbollen/3;
	}
	*/
	
	if (++bitcounter == symbollen) {
		int hardbit = -1;
		int softbit = 128;
		
		if (onescount > zeroscount) {
			hardbit = 1;
			softbit = 255;
		} else {
			hardbit = 0;
			softbit = 0;
		}
		bitcounter = onescount = zeroscount = 0;
		
		
		if (rtty_baud == 40) {
			///rx_pskr(softbit);
			///return false;
			rxdata = softbit;
			/// rxinlv->bits(&rxdata); /// BUG: Interleaver not implemented for CW 2.0 FEC
			hardbit = decodesymbol(rxdata);
			if (hardbit == -1)
				return false;
		}
		
		int c;
		datashreg = (datashreg << 1) | !!hardbit;
		if ((datashreg & 7) == 1) {
			c = varidec(datashreg >> 1);
			put_rx_char(c);
			datashreg = 1;
		}
		return true;
	}
	
	return false;
	
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
		// Voting at the character level for only PSKR modes
		if (fecmet >= fecmet2) {
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
		// Voting at the character level for only PSKR modes
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

void rtty::searchDown()
{
	double srchfreq = frequency - shift -100;
	double minfreq = shift * 2 + 100;
	double spwrlo, spwrhi, npwr;
	while (srchfreq > minfreq) {
		spwrlo = wf->powerDensity(srchfreq - shift/2, 2*rtty_baud);
		spwrhi = wf->powerDensity(srchfreq + shift/2, 2*rtty_baud);
		npwr = wf->powerDensity(srchfreq + shift, 2*rtty_baud) + 1e-10;
		if ((spwrlo / npwr > 10.0) && (spwrhi / npwr > 10.0)) {
			frequency = srchfreq;
			set_freq(frequency);
			sigsearch = SIGSEARCH;
			break;
		}
		srchfreq -= 5.0;
	}
}

void rtty::searchUp()
{
	double srchfreq = frequency + shift +100;
	double maxfreq = IMAGE_WIDTH - shift * 2 - 100;
	double spwrhi, spwrlo, npwr;
	while (srchfreq < maxfreq) {
		spwrlo = wf->powerDensity(srchfreq - shift/2, 2*rtty_baud);
		spwrhi = wf->powerDensity(srchfreq + shift/2, 2*rtty_baud);
		npwr = wf->powerDensity(srchfreq - shift, 2*rtty_baud) + 1e-10;
		if ((spwrlo / npwr > 10.0) && (spwrhi / npwr > 10.0)) {
			frequency = srchfreq;
			set_freq(frequency);
			sigsearch = SIGSEARCH;
			break;
		}
		srchfreq += 5.0;
	}
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
			double v3;

// no ATC
//			v0 = mark_mag - space_mag;
// Linear ATC
//			v1 = mark_mag - space_mag - 0.5 * (mark_env - space_env);
// Clipped ATC
//			v2  = (mclipped - noise_floor) - (sclipped - noise_floor) - 0.5 * (
//					(mark_env - noise_floor) - (space_env - noise_floor));
// Optimal ATC
			v3  = (mclipped - noise_floor) * (mark_env - noise_floor) -
					(sclipped - noise_floor) * (space_env - noise_floor) - 0.25 * (
					(mark_env - noise_floor) * (mark_env - noise_floor) -
					(space_env - noise_floor) * (space_env - noise_floor));
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
//				bit = v1 > 0;
//				break;
//			case 1: // clipped ATC
//				bit = v2 > 0;
//				break;
//			case 2: // optimal ATC
				bit = v3 > 0;
//				break;
//			case 3: // Kahn linear ATC
//				bit = v4 > 0;
//				break;
//			case 4: // Kahn clipped
//				bit = v5 > 0;
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
			if ( rx( reverse ? !bit : bit ) ) {
				dspcnt = symbollen * (nbits + 2);
				if (!bHighSpeed) Update_syncscope();
				clear_zdata = true;
				bitcount = 5 * nbits * symbollen;
				if (sigsearch) sigsearch--;
					int mp0 = inp_ptr - 2;
				int mp1 = mp0 + 1;
				if (mp0 < 0) mp0 += MAXPIPE;
				if (mp1 < 0) mp1 += MAXPIPE;
				double ferr = (TWOPI * samplerate / rtty_baud) *
						(!reverse ?
							arg(conj(mark_history[mp1]) * mark_history[mp0]) :
							arg(conj(space_history[mp1]) * space_history[mp0]));
				if (fabs(ferr) > rtty_baud / 2) ferr = 0;
				freqerr = decayavg ( freqerr, ferr / 8,
					progdefaults.rtty_afcspeed == 0 ? 8 :
					progdefaults.rtty_afcspeed == 1 ? 4 : 1 );
				if (progStatus.afconoff &&
					(metric > progStatus.sldrSquelchValue || !progStatus.sqlonoff))
					set_freq(frequency - freqerr);
			} else
				if (bitcount) --bitcount;
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

double rtty::nco(double freq)
{
	phaseacc += TWOPI * freq / samplerate;

	if (phaseacc > TWOPI) phaseacc -= TWOPI;

	return cos(phaseacc);
}

double rtty::FSKnco()
{
	FSKphaseacc += TWOPI * 1000 / samplerate;

	if (FSKphaseacc > TWOPI) FSKphaseacc -= TWOPI;

	return sin(FSKphaseacc);

}


void rtty::send_symbol(unsigned int symbol, int len)
{
	acc_symbols += len;

	double freq;

	// transmit mark-only
	// TODO : Tx audio is only for testing:
	// Final mode will hard-key the transmitters CW key.
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

	if (progdefaults.PseudoFSK)
		ModulateStereo(outbuf, FSKbuf, symbollen);
	else
		ModulateXmtr(outbuf, symbollen);
}

void rtty::send_char(int c)
{
	if (restartchar) { // Send a NULL char to re-synchronize the receiver
		const char *code = varienc(0);
		while (*code)
			send_symbol( (*code++ - '0'), symbollen);
		if (rtty_baud == 40) { // send twice for fec mode
			code = varienc(0);	
			while (*code)
				send_symbol( (*code++ - '0'), symbollen);
		}
		  
		restartchar = false;
	}
  
	if (rtty_baud == 20) { // Non FEC mode
	  	const char *code = varienc(c);
		while (*code)
			send_symbol( (*code++ - '0'), symbollen);
		
	} else if (rtty_baud == 40) { // FEC MODE
	  	const char *code = varienc(c);
		while (*code) {
		  	txdata = enc->encode( (*code++ - '0') );
			//txinlv->bits(&txdata); /// BUG: Interleaver unimplemented
			send_symbol(txdata &1 , symbollen);
			send_symbol(txdata &2 , symbollen);
		}	
	}

	put_echo_char(c);
	return;
}

/// kl4yfd
/// send idle in a way that both keeps FEC synch and decodes to nothing in MFSK varicode
// After a few 0's as input, the FEC will output a string of 0's (key-ups)
void rtty::send_idle()
{
	///send_char(0);
  	txdata = enc->encode( 0 ); // Keep synchronization with string of 0 bits
	//txinlv->bits(&txdata); /// BUG: Interleaver unimplemented
	send_symbol(txdata &1 , symbollen);
	send_symbol(txdata &2 , symbollen);
}

static int line_char_count = 0;

int rtty::tx_process()
{
	int c;

	/// CW 2.0 feature: The Non-FEC preamble is heard as the letter "N" twice in Morse code
	if (preamble) {
		send_char('\n'); // CR : enter
		send_char('\n');
		preamble = false;
	}
	c = get_tx_char();

// TX buffer empty
	if (c == GET_TX_CHAR_ETX || stopflag) {
		stopflag = false;
		line_char_count = 0;
		/// CW 2.0 feature: The Non-FEC postamble is heard as the letter "N" twice in Morse code
		send_char('\n'); // CR : enter
		send_char('\n');
		cwid(); /// send callsign in Morse code 
		return -1;
	}

	/// CW 2.0 Feature: 
	/// When there is no character to send, mode becomes RF silent.
	// (sends a constant stream of "key-up" or 0 values)
	if (c == GET_TX_CHAR_NODATA) {
		if (!restartchar) {
			send_char(0); // when NODATA idle starts: send a NULL to flush the last-sent character through Rx
			if (rtty_baud == 40)
				send_char(0); // send twice for FEC mode
		}
	  	restartchar = true; // Request need for a buffer/restart NULL character (to re-synchronize)
		
		if (rtty_baud != 20)
			send_idle(); /// FEC requires synchronization to be kept...
		else
			send_symbol(0, symbollen); // nothing to send: don't waste the RF power with an idle signal
		return 0;
	}


	acc_symbols = 0;
	send_char(c);
	xmt_samples = char_samples = acc_symbols;

	return 0;
}
  