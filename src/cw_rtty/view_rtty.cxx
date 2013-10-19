// ----------------------------------------------------------------------------
// rtty.cxx  --  RTTY modem
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code
// distribution.
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
using namespace std;

#include "rtty.h"
#include "view_rtty.h"
#include "fl_digi.h"
#include "digiscope.h"
#include "misc.h"
#include "waterfall.h"
#include "confdialog.h"
#include "configuration.h"
#include "status.h"
#include "digiscope.h"
#include "Viewer.h"
#include "qrunner.h"

//=====================================================================
// Baudot support
//=====================================================================

static char letters[32] = {
	'\0',	'E',	'\n',	'A',	' ',	'S',	'I',	'U',
	'\r',	'D',	'R',	'J',	'N',	'F',	'C',	'K',
	'T',	'Z',	'L',	'W',	'H',	'Y',	'P',	'Q',
	'O',	'B',	'G',	' ',	'M',	'X',	'V',	' '
};

// U.S. version of the figures case.
static char figures[32] = {
	'\0',	'3',	'\n',	'-',	' ',	'\a',	'8',	'7',
	'\r',	'$',	'4',	'\'',	',',	'!',	':',	'(',
	'5',	'"',	')',	'2',	'#',	'6',	'0',	'1',
	'9',	'?',	'&',	' ',	'.',	'/',	';',	' '
};

const double view_rtty::SHIFT[] = {23, 85, 160, 170, 182, 200, 240, 350, 425, 850};
const double view_rtty::BAUD[]  = {45, 45.45, 50, 56, 75, 100, 110, 150, 200, 300};
const int    view_rtty::BITS[]  = {5, 7, 8};

void view_rtty::rx_init()
{
	for (int ch = 0; ch < progdefaults.VIEWERchannels; ch++) {
		channel[ch].state = IDLE;
		channel[ch].rxstate = RTTY_RX_STATE_IDLE;
		channel[ch].rxmode = LETTERS;
		channel[ch].phaseacc = 0;
		channel[ch].timeout = 0;
		channel[ch].frequency = NULLFREQ;
		channel[ch].poserr = channel[ch].negerr = 0.0;

		channel[ch].mark_phase = 0;
		channel[ch].space_phase = 0;
		channel[ch].mark_mag = 0;
		channel[ch].space_mag = 0;
		channel[ch].mark_env = 0;
		channel[ch].space_env = 0;

		channel[ch].inp_ptr = 0;

		for (int i = 0; i < MAXPIPE; i++)
			channel[ch].mark_history[i] = 
			channel[ch].space_history[i] = cmplx(0,0);
	}
}

void view_rtty::init()
{
	bool wfrev = wf->Reverse();
	bool wfsb = wf->USB();
	reverse = wfrev ^ !wfsb;
	rx_init();
}

view_rtty::~view_rtty()
{
	for (int ch = 0; ch < MAX_CHANNELS; ch ++) {
		if (channel[ch].mark_filt) delete channel[ch].mark_filt;
		if (channel[ch].space_filt) delete channel[ch].space_filt;
	}
}

void view_rtty::reset_filters(int ch)
{
	int filter_length = 1024;
	if (channel[ch].mark_filt) {
		channel[ch].mark_filt->rtty_filter(rtty_baud/samplerate);
	} else {
		channel[ch].mark_filt = new fftfilt(rtty_baud/samplerate, filter_length);
		channel[ch].mark_filt->rtty_filter(rtty_baud/samplerate);
	}

	if (channel[ch].space_filt) {
		channel[ch].space_filt->rtty_filter(rtty_baud/samplerate);
	} else {
		channel[ch].space_filt = new fftfilt(rtty_baud/samplerate, filter_length);
		channel[ch].space_filt->rtty_filter(rtty_baud/samplerate);
	}
}

void view_rtty::restart()
{
	double stl;

	rtty_shift = shift = (progdefaults.rtty_shift >= 0 ?
			      SHIFT[progdefaults.rtty_shift] : progdefaults.rtty_custom_shift);
	rtty_baud = BAUD[progdefaults.rtty_baud];
	nbits = rtty_bits = BITS[progdefaults.rtty_bits];
	if (rtty_bits == 5)
		rtty_parity = RTTY_PARITY_NONE;
	else
		switch (progdefaults.rtty_parity) {
			case 0 : rtty_parity = RTTY_PARITY_NONE; break;
			case 1 : rtty_parity = RTTY_PARITY_EVEN; break;
			case 2 : rtty_parity = RTTY_PARITY_ODD; break;
			case 3 : rtty_parity = RTTY_PARITY_ZERO; break;
			case 4 : rtty_parity = RTTY_PARITY_ONE; break;
			default : rtty_parity = RTTY_PARITY_NONE; break;
		}
	rtty_stop = progdefaults.rtty_stop;


	symbollen = (int) (samplerate / rtty_baud + 0.5);
	bflen = symbollen/3;

	set_bandwidth(shift);

	rtty_BW = progdefaults.RTTY_BW;

	bp_filt_lo = (shift/2.0 - rtty_BW/2.0) / samplerate;
	if (bp_filt_lo < 0) bp_filt_lo = 0;
	bp_filt_hi = (shift/2.0 + rtty_BW/2.0) / samplerate;

	for (int ch = 0; ch < MAX_CHANNELS; ch ++) {

		reset_filters(ch);

		channel[ch].state = IDLE;
		channel[ch].timeout = 0;
		channel[ch].freqerr = 0.0;
		channel[ch].metric = 0.0;
		channel[ch].sigpwr = 0.0;
		channel[ch].noisepwr = 0.0;
		channel[ch].sigsearch = 0;
		channel[ch].frequency = NULLFREQ;
		channel[ch].counter = symbollen / 2;
		channel[ch].mark_phase = 0;
		channel[ch].space_phase = 0;
		channel[ch].mark_mag = 0;
		channel[ch].space_mag = 0;
		channel[ch].mark_env = 0;
		channel[ch].space_env = 0;
		channel[ch].inp_ptr = 0;

		if (channel[ch].bits)
			channel[ch].bits->setLength(symbollen / 8);
		else
			channel[ch].bits = new Cmovavg(symbollen / 8);

		channel[ch].mark_noise = channel[ch].space_noise = 0;
		channel[ch].bit = channel[ch].nubit = true;

		for (int i = 0; i < VIEW_RTTY_MAXBITS; i++) channel[ch].bit_buf[i] = 0.0;

		for (int i = 0; i < MAXPIPE; i++) 
			channel[ch].mark_history[i] = channel[ch].space_history[i] = cmplx(0,0);
	}

// stop length = 1, 1.5 or 2 bits
	rtty_stop = progdefaults.rtty_stop;
	if (rtty_stop == 0) stl = 1.0;
	else if (rtty_stop == 1) stl = 1.5;
	else stl = 2.0;
	stoplen = (int) (stl * samplerate / rtty_baud + 0.5);

	rx_init();
}

view_rtty::view_rtty(trx_mode tty_mode)
{
	cap |= CAP_AFC | CAP_REV;

	mode = tty_mode;

	samplerate = RTTY_SampleRate;

	for (int ch = 0; ch < MAX_CHANNELS; ch ++) {
		channel[ch].mark_filt = (fftfilt *)0;
		channel[ch].space_filt = (fftfilt *)0;
		channel[ch].bits = (Cmovavg *)0;
	}

	restart();
}

cmplx view_rtty::mixer(double &phase, double f, cmplx in)
{
	cmplx z = cmplx( cos(phase), sin(phase)) * in;;

	phase -= TWOPI * f / samplerate;
	if (phase < - TWOPI) phase += TWOPI;

	return z;
}


unsigned char view_rtty::bitreverse(unsigned char in, int n)
{
	unsigned char out = 0;

	for (int i = 0; i < n; i++)
		out = (out << 1) | ((in >> i) & 1);

	return out;
}

static int rparity(int c)
{
	int w = c;
	int p = 0;
	while (w) {
		p += (w & 1);
		w >>= 1;
	}
	return p & 1;
}

int view_rtty::rttyparity(unsigned int c)
{
	c &= (1 << nbits) - 1;

	switch (rtty_parity) {
	default:
	case RTTY_PARITY_NONE:
		return 0;

	case RTTY_PARITY_ODD:
		return rparity(c);

	case RTTY_PARITY_EVEN:
		return !rparity(c);

	case RTTY_PARITY_ZERO:
		return 0;

	case RTTY_PARITY_ONE:
		return 1;
	}
}

int view_rtty::decode_char(int ch)
{
	unsigned int parbit, par, data;

	parbit = (channel[ch].rxdata >> nbits) & 1;
	par = rttyparity(channel[ch].rxdata);

	if (rtty_parity != RTTY_PARITY_NONE && parbit != par)
		return 0;

	data = channel[ch].rxdata & ((1 << nbits) - 1);

	if (nbits == 5)
		return baudot_dec(ch & 0x7F, data);

	return data;
}

bool view_rtty::is_mark_space( int ch, int &correction)
{
	correction = 0;
// test for rough bit position
	if (channel[ch].bit_buf[0] && !channel[ch].bit_buf[symbollen-1]) {
// test for mark/space straddle point
		for (int i = 0; i < symbollen; i++)
			correction += channel[ch].bit_buf[i];
		if (abs(symbollen/2 - correction) < 6) // too small & bad signals are not decoded
			return true;
	}
	return false;
}

bool view_rtty::is_mark(int ch)
{
	return channel[ch].bit_buf[symbollen / 2];
}

bool view_rtty::rx(int ch, bool bit)
{
	bool flag = false;
	unsigned char c = 0;

	int correction = 0;

	for (int i = 1; i < symbollen; i++)
		channel[ch].bit_buf[i-1] = channel[ch].bit_buf[i];
	channel[ch].bit_buf[symbollen - 1] = bit;

	switch (channel[ch].rxstate) {
	case RTTY_RX_STATE_IDLE:
		if ( is_mark_space(ch, correction)) {
			channel[ch].rxstate = RTTY_RX_STATE_START;
			channel[ch].counter = correction;
		}
		break;
	case RTTY_RX_STATE_START:
		if (--channel[ch].counter == 0) {
			if (!is_mark(ch)) {
				channel[ch].rxstate = RTTY_RX_STATE_DATA;
				channel[ch].counter = symbollen;
				channel[ch].bitcntr = 0;
				channel[ch].rxdata = 0;
			} else {
				channel[ch].rxstate = RTTY_RX_STATE_IDLE;
			}
		}
		break;
	case RTTY_RX_STATE_DATA:
		if (--channel[ch].counter == 0) {
			channel[ch].rxdata |= is_mark(ch) << channel[ch].bitcntr++;
			channel[ch].counter = symbollen;
		}
		if (channel[ch].bitcntr == nbits + (rtty_parity != RTTY_PARITY_NONE ? 1 : 0))
			channel[ch].rxstate = RTTY_RX_STATE_STOP;
		break;
	case RTTY_RX_STATE_STOP:
		if (--channel[ch].counter == 0) {
			if (is_mark(ch)) {
				if (channel[ch].metric > rtty_squelch) {
					c = decode_char(ch);
// print this RTTY_CHANNEL
					if ( c != 0 )
						REQ(&viewaddchr, ch, (int)channel[ch].frequency, c, mode);
				}
				flag = true;
			}
			channel[ch].rxstate = RTTY_RX_STATE_IDLE;
		}
		break;
	default : break;
	}

	return flag;
}

void view_rtty::Metric(int ch)
{
	double delta = rtty_baud/2.0;
	double np = wf->powerDensity(channel[ch].frequency, delta) * 3000 / delta;
	double sp =
		wf->powerDensity(channel[ch].frequency - shift/2, delta) +
		wf->powerDensity(channel[ch].frequency + shift/2, delta) + 1e-10;

	channel[ch].sigpwr = decayavg( channel[ch].sigpwr, sp, sp - channel[ch].sigpwr > 0 ? 2 : 16);

	channel[ch].noisepwr = decayavg( channel[ch].noisepwr, np, 16 );

	channel[ch].metric = CLAMP(channel[ch].sigpwr/channel[ch].noisepwr, 0.0, 100.0);

	if (channel[ch].state == RCVNG)
		if (channel[ch].metric < rtty_squelch) {
			channel[ch].timeout = progdefaults.VIEWERtimeout * samplerate / WFBLOCKSIZE;
			channel[ch].state = WAITING;
		}

	if (channel[ch].timeout) {
		channel[ch].timeout--;
		if (!channel[ch].timeout) {
			channel[ch].frequency = NULLFREQ;
			channel[ch].metric = 0;
			channel[ch].freqerr = 0;
			channel[ch].state = IDLE;
			REQ(&viewclearchannel, ch);
		}
	}
}

void view_rtty::find_signals()
{
	double spwrhi = 0.0, spwrlo = 0.0, npwr = 0.0;
	double rtty_squelch = pow(10, progStatus.VIEWER_rttysquelch / 10.0);
	for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
		if (channel[i].state != IDLE) continue;
		int cf = progdefaults.LowFreqCutoff + 100 * i;
		if (cf < shift) cf = shift;
		double delta = rtty_baud / 8;
		for (int chf = cf; chf < cf + 100 - rtty_baud / 4; chf += 5) {
			spwrlo = wf->powerDensity(chf - shift/2, delta);
			spwrhi = wf->powerDensity(chf + shift/2, delta);
			npwr = (wf->powerDensity(chf, delta) * 3000 / rtty_baud) + 1e-10;
			if ((spwrlo / npwr > rtty_squelch) && (spwrhi / npwr > rtty_squelch)) {
				if (!i && (channel[i+1].state == SRCHG || channel[i+1].state == RCVNG)) break;
				if ((i == (progdefaults.VIEWERchannels -2)) && 
					(channel[i+1].state == SRCHG || channel[i+1].state == RCVNG)) break;
				if (i && (channel[i-1].state == SRCHG || channel[i-1].state == RCVNG)) break;
				if (i > 3 && (channel[i-2].state == SRCHG || channel[i-2].state == RCVNG)) break;
				channel[i].frequency = chf;
				channel[i].sigsearch = SIGSEARCH;
				channel[i].state = SRCHG;
				REQ(&viewaddchr, i, (int)channel[i].frequency, 0, mode);
				break;
			}
		}
	}
	for (int i = 1; i < progdefaults.VIEWERchannels; i++ )
		if (fabs(channel[i].frequency - channel[i-1].frequency) < rtty_baud/2)
			clearch(i);
}

void view_rtty::clearch(int ch)
{
	channel[ch].state = IDLE;
	channel[ch].rxstate = RTTY_RX_STATE_IDLE;
	channel[ch].rxmode = LETTERS;
	channel[ch].phaseacc = 0;
	channel[ch].frequency = NULLFREQ;
	channel[ch].poserr = channel[ch].negerr = 0.0;
	REQ( &viewclearchannel, ch);
}

void view_rtty::clear()
{
	for (int ch = 0; ch < progdefaults.VIEWERchannels; ch++) {
		channel[ch].state = IDLE;
		channel[ch].rxstate = RTTY_RX_STATE_IDLE;
		channel[ch].rxmode = LETTERS;
		channel[ch].phaseacc = 0;
		channel[ch].frequency = NULLFREQ;
		channel[ch].poserr = channel[ch].negerr = 0.0;
	}
}

int view_rtty::rx_process(const double *buf, int buflen)
{
	cmplx z, zmark, zspace, *zp_mark, *zp_space;
	static bool bit = true;
	int n = 0;

	rtty_squelch = pow(10, progStatus.VIEWER_rttysquelch / 10.0);

	for (int ch = 0; ch < progdefaults.VIEWERchannels; ch++) {
		if (channel[ch].state == IDLE)
			continue;
		if (channel[ch].sigsearch) {
			channel[ch].sigsearch--;
			if (!channel[ch].sigsearch)
				channel[ch].state = RCVNG;
		}

		for (int len = 0; len < buflen; len++) {
			z = cmplx(buf[len], buf[len]);

			zmark = mixer(channel[ch].mark_phase, channel[ch].frequency + shift/2.0, z);
			channel[ch].mark_filt->run(zmark, &zp_mark);

			zspace = mixer(channel[ch].space_phase, channel[ch].frequency - shift/2.0, z);
			n = channel[ch].space_filt->run(zspace, &zp_space);

// n loop
			if (n) Metric(ch);

			for (int i = 0; i < n; i++) {

				channel[ch].mark_mag = abs(zp_mark[i]);
				channel[ch].mark_env = decayavg (channel[ch].mark_env, channel[ch].mark_mag,
					(channel[ch].mark_mag > channel[ch].mark_env) ? symbollen / 4 : symbollen * 16);
				channel[ch].mark_noise = decayavg (channel[ch].mark_noise, channel[ch].mark_mag,
					(channel[ch].mark_mag < channel[ch].mark_noise) ? symbollen / 4 : symbollen * 48);
				channel[ch].space_mag = abs(zp_space[i]);
				channel[ch].space_env = decayavg (channel[ch].space_env, channel[ch].space_mag,
					(channel[ch].space_mag > channel[ch].space_env) ? symbollen / 4 : symbollen * 16);
				channel[ch].space_noise = decayavg (channel[ch].space_noise, channel[ch].space_mag,
					(channel[ch].space_mag < channel[ch].space_noise) ? symbollen / 4 : symbollen * 48);

				channel[ch].noise_floor = min(channel[ch].space_noise, channel[ch].mark_noise);

// clipped if clipped decoder selected
				double mclipped = 0, sclipped = 0;
				mclipped = channel[ch].mark_mag > channel[ch].mark_env ? 
							channel[ch].mark_env : channel[ch].mark_mag;
				sclipped = channel[ch].space_mag > channel[ch].space_env ? 
							channel[ch].space_env : channel[ch].space_mag;
				if (mclipped < channel[ch].noise_floor) mclipped = channel[ch].noise_floor;
				if (sclipped < channel[ch].noise_floor) sclipped = channel[ch].noise_floor;

// Optimal ATC
//				int v = (((mclipped - channel[ch].noise_floor) * (channel[ch].mark_env - channel[ch].noise_floor) -
//						(sclipped - channel[ch].noise_floor) * (channel[ch].space_env - channel[ch].noise_floor)) -
//				0.25 * ((channel[ch].mark_env - channel[ch].noise_floor) * 
//						(channel[ch].mark_env - channel[ch].noise_floor) -
//						(channel[ch].space_env - channel[ch].noise_floor) * 
//						(channel[ch].space_env - channel[ch].noise_floor)));
//				bit = (v > 0);
// Kahn Square Law demodulator
				bit = norm(zp_mark[i]) >= norm(zp_space[i]);

				channel[ch].mark_history[channel[ch].inp_ptr] = zp_mark[i];
				channel[ch].space_history[channel[ch].inp_ptr] = zp_space[i];
				channel[ch].inp_ptr = (channel[ch].inp_ptr + 1) % MAXPIPE;

				if (channel[ch].state == RCVNG && rx( ch, reverse ? !bit : bit ) ) {
					if (channel[ch].sigsearch) channel[ch].sigsearch--;
					int mp0 = channel[ch].inp_ptr - 2;
					int mp1 = mp0 + 1;
					if (mp0 < 0) mp0 += MAXPIPE;
					if (mp1 < 0) mp1 += MAXPIPE;
					double ferr = (TWOPI * samplerate / rtty_baud) *
						(!reverse ? 
						arg(conj(channel[ch].mark_history[mp1]) * channel[ch].mark_history[mp0]) :
						arg(conj(channel[ch].space_history[mp1]) * channel[ch].space_history[mp0]));
					if (fabs(ferr) > rtty_baud / 2) ferr = 0;
					channel[ch].freqerr = decayavg ( channel[ch].freqerr, ferr / 4,
						progdefaults.rtty_afcspeed == 0 ? 8 :
						progdefaults.rtty_afcspeed == 1 ? 4 : 1 );
					if (channel[ch].metric > pow(10, progStatus.VIEWER_rttysquelch / 10.0))
						channel[ch].frequency -= ferr;
				}
			}
		}
	}

	find_signals();

	return 0;
}

char view_rtty::baudot_dec(int ch, unsigned char data)
{
	int out = 0;

	switch (data) {
	case 0x1F:		/* letters */
		channel[ch].rxmode = LETTERS;
		break;
	case 0x1B:		/* figures */
		channel[ch].rxmode = FIGURES;
		break;
	case 0x04:		/* unshift-on-space */
		if (progdefaults.UOSrx)
			channel[ch].rxmode = LETTERS;
		return ' ';
		break;
	default:
		if (channel[ch].rxmode == LETTERS)
			out = letters[data];
		else
			out = figures[data];
		break;
	}

	return out;
}

//=====================================================================
// RTTY transmit
//=====================================================================

int view_rtty::tx_process()
{
	return 0;
}

