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

static unsigned char letters[32] = {
	'\0',	'E',	'\n',	'A',	' ',	'S',	'I',	'U',
	'\r',	'D',	'R',	'J',	'N',	'F',	'C',	'K',
	'T',	'Z',	'L',	'W',	'H',	'Y',	'P',	'Q',
	'O',	'B',	'G',	'·',	'M',	'X',	'V',	'·'
};

// U.S. version of the figures case.
static unsigned char figures[32] = {
	'\0',	'3',	'\n',	'-',	' ',	'\a',	'8',	'7',
	'\r',	'$',	'4',	'\'',	',',	'!',	':',	'(',
	'5',	'"',	')',	'2',	'#',	'6',	'0',	'1',
	'9',	'?',	'&',	'·',	'.',	'/',	';',	'·'
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
		for (int i = 0; i < RTTYMaxSymLen; i++ ) {
			channel[ch].bbfilter[i] = 0.0;
		}
		channel[ch].bitfilt->reset();
		channel[ch].poserr = channel[ch].negerr = 0.0;
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
	if (hilbert) delete hilbert;
	for (int ch = 0; ch < MAX_CHANNELS; ch ++) {
		if (channel[ch].bitfilt) delete channel[ch].bitfilt;
		if (channel[ch].bpfilt) delete channel[ch].bpfilt;
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
		if (channel[ch].bpfilt) {
			channel[ch].bpfilt->create_filter(bp_filt_lo, bp_filt_hi);
			channel[ch].bitfilt->setLength(bflen);
		} else {
			channel[ch].bpfilt = new fftfilt(bp_filt_lo, bp_filt_hi, 1024);
			channel[ch].bitfilt = new Cmovavg(bflen);
		}
		channel[ch].state = IDLE;
		channel[ch].timeout = 0;
		channel[ch].freqerr = 0.0;
		channel[ch].filterptr = 0;
		channel[ch].poscnt = 0;
		channel[ch].negcnt = 0;
		channel[ch].posfreq = 0;
		channel[ch].negfreq = 0.0;
		channel[ch].metric = 0.0;
		channel[ch].sigpwr = 0.0;
		channel[ch].noisepwr = 0.0;
		channel[ch].freqerrlo = 0.0;
		channel[ch].freqerrhi = 0.0;
		channel[ch].sigsearch = 0;
		channel[ch].frequency = NULLFREQ;
		channel[ch].counter = symbollen / 2;
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
		channel[ch].bpfilt = (fftfilt *)0;
		channel[ch].bitfilt = (Cmovavg *)0;
	}
	hilbert = new C_FIR_filter();
	hilbert->init_hilbert(37, 1);

	restart();
}

complex view_rtty::mixer(int ch, complex in)
{
	complex z;
	z.re = cos(channel[ch].phaseacc);
	z.im = sin(channel[ch].phaseacc);
	z = z * in;

	channel[ch].phaseacc -= TWOPI * channel[ch].frequency / samplerate;
	if (channel[ch].phaseacc > M_PI)
		channel[ch].phaseacc -= TWOPI;
	else if (channel[ch].phaseacc < M_PI)
		channel[ch].phaseacc += TWOPI;

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
		return baudot_dec(ch, data);

	return data;
}

bool view_rtty::rx(int ch, bool bit)
{
	bool flag = false;
	unsigned char c;

	switch (channel[ch].rxstate) {
	case RTTY_RX_STATE_IDLE:
		if (!bit) {
			channel[ch].rxstate = RTTY_RX_STATE_START;
			channel[ch].counter = symbollen / 2;
		}
		break;

	case RTTY_RX_STATE_START:
		if (--channel[ch].counter == 0) {
			if (!bit) {
				channel[ch].rxstate = RTTY_RX_STATE_DATA;
				channel[ch].counter = symbollen;
				channel[ch].bitcntr = 0;
				channel[ch].rxdata = 0;
			} else {
				channel[ch].rxstate = RTTY_RX_STATE_IDLE;
			}
		} else
			if (bit) channel[ch].rxstate = RTTY_RX_STATE_IDLE;
		break;

	case RTTY_RX_STATE_DATA:
		if (--channel[ch].counter == 0) {
			channel[ch].rxdata |= bit << channel[ch].bitcntr++;
			channel[ch].counter = symbollen;
		}

		if (channel[ch].bitcntr == nbits) {
			if (rtty_parity == RTTY_PARITY_NONE) {
				channel[ch].rxstate = RTTY_RX_STATE_STOP;
			}
			else {
				channel[ch].rxstate = RTTY_RX_STATE_PARITY;
			}
		}
		break;

	case RTTY_RX_STATE_PARITY:
		if (--channel[ch].counter == 0) {
			channel[ch].rxstate = RTTY_RX_STATE_STOP;
			channel[ch].rxdata |= bit << channel[ch].bitcntr++;
			channel[ch].counter = symbollen;
		}
		break;

	case RTTY_RX_STATE_STOP:
		if (--channel[ch].counter == 0) {
			if (bit) {
				if (channel[ch].metric > rtty_squelch) {
					c = decode_char(ch);
// print this RTTY_CHANNEL
					if ( c != 0 )
						REQ(&viewaddchr, ch, (int)channel[ch].frequency, c, mode);
//						put_rx_char(progdefaults.rx_lowercase ? tolower(c) : c);
				}
				flag = true;
			}
			channel[ch].rxstate = RTTY_RX_STATE_STOP2;
			channel[ch].counter = symbollen / 2;
		}
		break;

	case RTTY_RX_STATE_STOP2:
		if (--channel[ch].counter == 0) {
			channel[ch].rxstate = RTTY_RX_STATE_IDLE;
		}
		break;
	}

	return flag;
}

void view_rtty::Metric(int ch)
{
	double delta = rtty_baud/2.0;
	double np =
		wf->powerDensity(channel[ch].frequency - shift * 1.5, delta) +
	 	wf->powerDensity(channel[ch].frequency + shift * 1.5, delta) + 1e-10;
	double sp =
		wf->powerDensity(channel[ch].frequency - shift/2, delta) +
		wf->powerDensity(channel[ch].frequency + shift/2, delta) + 1e-10;

	channel[ch].sigpwr = decayavg( channel[ch].sigpwr, sp, sp - channel[ch].sigpwr > 0 ? 2 : 8);

	channel[ch].noisepwr = decayavg( channel[ch].noisepwr, np, 32 );

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
	double rtty_squelch = pow(10, progdefaults.VIEWERsquelch / 10.0);
	for (int i = 0; i < progdefaults.VIEWERchannels; i++) {
		if (channel[i].state != IDLE) continue;
		int cf = progdefaults.LowFreqCutoff + 100 * i;
		if (cf < shift) cf = shift;
		for (int chf = cf; chf < cf + 100; chf += 5) {
			if (chf < shift) continue;
			spwrlo = wf->powerDensity(chf - shift/2, rtty_baud) / 2;
			spwrhi = wf->powerDensity(chf + shift/2, rtty_baud) / 2;
			npwr = wf->powerDensity(chf, rtty_baud / 2) + 1e-10;
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
}

void view_rtty::clearch(int ch)
{
	channel[ch].state = IDLE;
	channel[ch].rxstate = RTTY_RX_STATE_IDLE;
	channel[ch].rxmode = LETTERS;
	channel[ch].phaseacc = 0;
	channel[ch].frequency = NULLFREQ;
	for (int i = 0; i < RTTYMaxSymLen; i++ ) {
		channel[ch].bbfilter[i] = 0.0;
	}
	channel[ch].bitfilt->reset();
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
		for (int i = 0; i < RTTYMaxSymLen; i++ ) {
			channel[ch].bbfilter[i] = 0.0;
		}
		channel[ch].bitfilt->reset();
		channel[ch].poserr = channel[ch].negerr = 0.0;
	}
}

int view_rtty::rx_process(const double *buf, int buflen)
{
	complex z, *zp;
	double f = 0.0;
	double fin;
	static bool bit = true;
	int n = 0;
	double deadzone = shift/4;
	double ferr = 0;

	if (progdefaults.RTTY_BW != rtty_BW) {
		rtty_BW = progdefaults.RTTY_BW;
		bp_filt_lo = (shift/2.0 - rtty_BW/2.0) / samplerate;
		if (bp_filt_lo < 0) bp_filt_lo = 0;
		bp_filt_hi = (shift/2.0 + rtty_BW/2.0) / samplerate;
		for (int ch = 0; ch < MAX_CHANNELS; ch++)
			channel[ch].bpfilt->create_filter(bp_filt_lo, bp_filt_hi);
	}
	rtty_squelch = pow(10, progdefaults.VIEWERsquelch / 10.0);

	for (int ch = 0; ch < progdefaults.VIEWERchannels; ch++) {
		if (channel[ch].state == IDLE)
			continue;
		if (channel[ch].sigsearch) {
			channel[ch].sigsearch--;
			if (!channel[ch].sigsearch)
				channel[ch].state = RCVNG;
		}
		for (int len = 0; len < buflen; len++) {
			z.re = z.im = buf[len];
			hilbert->run(z, z);
			z = mixer(ch, z);
			n = channel[ch].bpfilt->run(z, &zp);
			if (n) {
				for (int i = 0; i < n; i++) {
					fin = (channel[ch].prevsmpl % zp[i]).arg() * samplerate / TWOPI;
					channel[ch].prevsmpl = zp[i];

					if (fin > 0.0) {
						channel[ch].poscnt++;
						channel[ch].posfreq += fin;
					}
					if (fin < 0.0) {
						channel[ch].negcnt++;
						channel[ch].negfreq += fin;
					}

					fin = CLAMP(fin, - rtty_shift, rtty_shift);
// filter the result with a moving average filter
					f = channel[ch].bitfilt->run(fin);
//	hysterisis dead zone in frequency discriminator bit detector
					if (f > deadzone )
						bit = true;
					if (f < -deadzone)
						bit = false;

					if (channel[ch].state == RCVNG) {
						if ( rx( ch, reverse ? !bit : bit ) ) {
							if (channel[ch].poscnt && channel[ch].negcnt) {
								channel[ch].poserr = channel[ch].posfreq / channel[ch].poscnt;
								channel[ch].negerr = channel[ch].negfreq / channel[ch].negcnt;

								ferr = -(channel[ch].poserr + channel[ch].negerr) /
												(2*(SIGSEARCH - channel[ch].sigsearch + 1));

								int fs = progdefaults.rtty_afcspeed;
								int avging;
								if (fs == 0) avging = 8;
								else if (fs == 1) avging = 4;
								else avging = 1;
								channel[ch].freqerr   = decayavg(channel[ch].freqerr, ferr,  avging);
								channel[ch].poscnt = channel[ch].negcnt = 0;
								channel[ch].posfreq = channel[ch].negfreq = 0.0;
							}
						}
					}
				}
			}
		}

		Metric(ch);
		if (channel[ch].metric > pow(10, progdefaults.VIEWERsquelch / 10.0))
			channel[ch].frequency -= ferr;
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

