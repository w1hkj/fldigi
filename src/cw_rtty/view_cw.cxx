// ----------------------------------------------------------------------------
// view_cw.cxx
//
// (c) 2014  Mauri Niininen, AG1LE
// (c) 2017  Dave Freese, W1HKJ
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

// viewpsk is a multi channel psk decoder which allows the parallel processing
// of the complete audio spectrum from 400 to 1150 Hz in equal 25 Hz
// channels.  Each channel is separately decoded and the decoded characters
// passed to the user interface routines for presentation.  The number of
// channels can be up to and including 30.

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "cw.h"
#include "view_cw.h"

#include "pskeval.h"
#include "pskcoeff.h"
#include "pskvaricode.h"
#include "misc.h"
#include "configuration.h"
#include "Viewer.h"
#include "qrunner.h"
#include "status.h"
#include "waterfall.h"

extern waterfall *wf;

#define CH_SPACING 50

#define	VCW_DEC_RATIO	4 //4, 8, 16
#define VCW_SAMPLERATE	8000

// VKWPM conversion factor 
// # samples per dot = VKWPM / wpm
// 30 samples @ 20 wpm
// 15 samples @ 40 wpm
#define VKWPM			(12.0 / 10.0) * ( VCW_SAMPLERATE / VCW_DEC_RATIO)

enum {READY, NOT_READY};

cMorse *CW_CHANNEL::morse = 0;

CW_CHANNEL::CW_CHANNEL() {
	bitfilter.setLength(10);
	trackingfilter.setLength(64);

	if (!morse) morse = new cMorse;

	VCW_filter = new C_FIR_filter ();

	smpl_ctr = dec_ctr = 0;
	phi1 = phi2 = 0;

	}

CW_CHANNEL::~CW_CHANNEL() {
	if (morse) {
		delete morse;
		morse = 0;
	}
	delete VCW_filter;
}

void CW_CHANNEL::init(int _ch, double _freq)
{
	VCW_filter->init_bandpass(
					2048, //2048 : 1024 : 512 : 256 : 128 : 64
					VCW_DEC_RATIO,
					(_freq - 0.5 * CH_SPACING) / VCW_SAMPLERATE,
					(_freq + 0.5 * CH_SPACING) / VCW_SAMPLERATE);

	ch_freq = _freq;
	ch = _ch;
	phase = 0.0;
	agc_peak = 0.0;
	sig_avg = 0.5;
	timeout = 0;
	smpl_ctr = 0;
	dec_ctr = 0;
	space_sent = true;
	last_element = 0;
	curr_element = 0;
	two_dots = 2 * VKWPM / progdefaults.CWspeed;

	sync_parameters();
}

void CW_CHANNEL::reset()
{
	space_sent = true;
	last_element = 0;
	curr_element = 0;
	decode_str.clear();
}

void CW_CHANNEL::update_tracking(int dur_1, int dur_2)
{
static int min_dot = (VKWPM / 60) / 2;
static int max_dash = 6 * VKWPM / 10;
	if ((dur_1 > dur_2) && (dur_1 > 4 * dur_2)) return;
	if ((dur_2 > dur_1) && (dur_2 > 4 * dur_1)) return;
	if (dur_1 < min_dot || dur_2 < min_dot) return;
	if (dur_2 > max_dash || dur_2 > max_dash) return;

	two_dots = trackingfilter.run((dur_1 + dur_2) / 2);
}

inline int CW_CHANNEL::sample_count(unsigned int earlier, unsigned int later)
{
	return (earlier >= later) ? 0 : (later - earlier);
}

void CW_CHANNEL::sync_parameters()
{
	trackingfilter.reset();
}

enum {KEYDOWN, KEYUP, POST_TONE, QUERY };

int CW_CHANNEL::decode_state(int cw_state)
{
	switch (cw_state) {
		case KEYDOWN:
		{
			if (cw_receive_state == KEYDOWN)
				return NOT_READY;
// first tone in idle state reset audio sample counter
			if (cw_receive_state == KEYUP) {
				smpl_ctr = 0;
				rx_rep_buf.clear();
			}
// save the timestamp
			cw_rr_start_timestamp = smpl_ctr;
// Set state to indicate we are inside a tone.
			cw_receive_state = KEYDOWN;
			return NOT_READY;
			break;
		}
		case KEYUP:
		{
// The receive state is expected to be inside a tone.
			if (cw_receive_state != KEYDOWN)
				return NOT_READY;
// Save the current timestamp
			curr_element = sample_count( cw_rr_start_timestamp, smpl_ctr );
			cw_rr_end_timestamp = smpl_ctr;

// If the tone length is shorter than any noise cancelling
// threshold that has been set, then ignore this tone.
			if (curr_element < two_dots / 10) {
				cw_receive_state = KEYUP;
				return NOT_READY;
			}

// Set up to track speed on dot-dash or dash-dot pairs for this test to
// work, we need a dot dash pair or a dash dot pair to validate timing
// from and force the speed tracking in the right direction.
			if (last_element > 0) update_tracking( last_element, curr_element );

			last_element = curr_element;
// a dot is anything shorter than 2 dot times
			if (curr_element <= two_dots) {
				rx_rep_buf += '.';
			} else {
				rx_rep_buf += '-';
			}
// We just added a representation to the receive buffer.
// If it's full, then reset everything as it is probably noise
			if (rx_rep_buf.length() > MAX_MORSE_ELEMENTS) {
				cw_receive_state = KEYUP;
				rx_rep_buf.clear();
				smpl_ctr = 0;		// reset audio sample counter
				return NOT_READY;
			}
// All is well.  Move to the more normal after-tone state.
			cw_receive_state = POST_TONE;
			return NOT_READY;
			break;
		}
		case QUERY:
		{
// this should be called quite often (faster than inter-character gap) It looks after timing
// key up intervals and determining when a character, a word space, or an error char '*' should be returned.
// READY is returned when there is a printable character. Nothing to do if we are in a tone
			if (cw_receive_state == KEYDOWN)
				return NOT_READY;
// in this call we expect a pointer to a char to be valid
			if (cw_state == KEYDOWN || cw_state == KEYUP) {
// else we had no place to put character...
				cw_receive_state = KEYUP;
				rx_rep_buf.clear();
// reset decoding pointer
				return NOT_READY;
			}
// compute length of silence so far
//			sync_parameters();
			curr_element = sample_count( cw_rr_end_timestamp, smpl_ctr );
// SHORT time since keyup... nothing to do yet
			if (curr_element < two_dots)
				return NOT_READY;
// MEDIUM time since keyup... check for character space
			if (curr_element < 3 * two_dots) {

				std::string code = morse->rx_lookup(rx_rep_buf);
				if (code.empty()) {
					decode_str.clear();
					cw_receive_state = KEYUP;
					rx_rep_buf.clear();
					space_sent = false;
					return NOT_READY ;
				}

				decode_str = code;
				cw_receive_state = KEYUP;
				rx_rep_buf.clear();
				space_sent = false;
				return READY;
			}
// LONG time since keyup... check for a word space
			if ((curr_element > 2 * two_dots) && !space_sent) {
				decode_str = " ";
				space_sent = true;
				return READY;
			}
			break;
		}
	}
	return NOT_READY;
}

void CW_CHANNEL::detect_tone()
{
	norm_sig = CWupper = CWlower = 0;
	int attack = 0;
	int decay = 0;
	int symbollen = (int)round(VCW_SAMPLERATE * 1.2 / progdefaults.CWspeed);

	switch (progdefaults.cwrx_attack) {
		case 0: attack = symbollen / 2; break;
		case 1: default: attack = symbollen / 4; break;
		case 2: attack = symbollen / 2;
	}
	switch (progdefaults.cwrx_decay) {
		case 0: decay = symbollen * 8; break;
		case 1: default: decay = symbollen * 4; break;
		case 2: decay = symbollen * 2;
	}

	sig_avg = decayavg(sig_avg, value, decay);

	if (value < sig_avg) {
		if (value < noise_floor)
			noise_floor = decayavg(noise_floor, value, attack);
		else 
			noise_floor = decayavg(noise_floor, value, decay);
	}
	if (value > sig_avg)  {
		if (value > agc_peak)
			agc_peak = decayavg(agc_peak, value, attack);
		else
			agc_peak = decayavg(agc_peak, value, decay); 
	}

	norm_sig = sig_avg / agc_peak;

	if (agc_peak)
		value /= agc_peak;
	else
		value = 0;

	metric = 0.8 * metric;
	metric += 0.2 * clamp(20*log10(sig_avg / noise_floor) , 0, 40);

	CWupper = norm_sig + 0.1;
	CWlower = norm_sig - 0.1;

	if (metric > progStatus.VIEWER_cwsquelch ) {

		if ((value >= CWupper) && (cw_receive_state != KEYDOWN))
			decode_state(KEYDOWN);
		if ((value < CWlower) && (cw_receive_state == KEYDOWN))
			decode_state(KEYUP);
		if ((decode_state(QUERY) == READY) ) {
			for (size_t n = 0; n < decode_str.length(); n++)
				REQ(&viewaddchr,
					ch, 
					(int)ch_freq, 
					decode_str[n], (int)MODE_CW);
			timeout = progdefaults.VIEWERtimeout * VPSKSAMPLERATE / WF_BLOCKSIZE;
			decode_str.clear();
			rx_rep_buf.clear();
		}
	}
}

//#include "spectrum_viewer.h"
//#include "fft-monitor.h"
//extern fftmon *fft_modem;
//static double filtered[512];
//static int out_n = 0;

void CW_CHANNEL::rx_process(int who, const double *buf, int len)
{
	double out;
	while (len-- > 0) {
		if (VCW_filter->Irun(*buf, out)) {
			smpl_ctr++;
			value = bitfilter.run(fabs(out));
			detect_tone();

//if (who == 4) {
//filtered[out_n] = out;
//out_n++;
//if (out_n == 512 && fft_modem && spectrum_viewer->visible())
//					fft_modem->rx_process(filtered, 512);
//if (out_n >= 512) out_n = 0;
//}

		}
		buf++;
	}
}

//======================================================================
// view_cw
//======================================================================

view_cw::view_cw()
{
	for (int i = 0; i < VCW_MAXCH; i++) channel[i].reset();

	viewmode = MODE_CW;
}

view_cw::~view_cw()
{
}

void view_cw::init()
{
	for (int i = 0; i < VCW_MAXCH; i++) {
		channel[i].init(i, 400.0 + CH_SPACING * i);
		if (i < progdefaults.VIEWERchannels)
			REQ(&viewclearchannel, i);
	}
}

void view_cw::restart()
{
	for (int i = 0; i < VCW_MAXCH; i++) {
		channel[i].space_sent = true;
		channel[i].last_element = 0;
		channel[i].curr_element = 0;
		channel[i].two_dots = 2 * VKWPM / progdefaults.CWspeed;
		channel[i].init(i, 400.0 + CH_SPACING * i);
		if (i < progdefaults.VIEWERchannels)
			REQ(&viewclearchannel, i);
	}
}

void view_cw::clearch(int n)
{
	if (n >= progdefaults.VIEWERchannels) return;
	channel[n].space_sent = true;
	channel[n].last_element = 0;
	channel[n].curr_element = 0;
	channel[n].two_dots = 2 * VKWPM / progdefaults.CWspeed;
	channel[n].init(n, 400.0 + CH_SPACING * n);
	REQ( &viewclearchannel, n);
	REQ( &viewaddchr, n, (int)NULLFREQ, 0, viewmode);
}

void view_cw::clear()
{
	for (int i = 0; i < VCW_MAXCH; i++) clearch(i);
}

int view_cw::rx_process(const double *buf, int len)
{
	double nf = 1e8;

	for (int n = 0; n < VCW_MAXCH; n++) {
		channel[n].rx_process(n, buf, len);

		if (nf > channel[n].avg_signal() &&
			channel[n].avg_signal() > 1e-3) nf = channel[n].avg_signal();

		if (channel[n].timeout)
			if (! --channel[n].timeout)
				clearch(n);
	}
	if (nf <= 1e-3) nf = 1e-3;
	for (int n = 0; n < VCW_MAXCH; n++) channel[n].set_noise_floor(nf);

	return 0;
}

int view_cw::get_freq(int n)
{
	return (int)channel[n].ch_freq;
}
