// ----------------------------------------------------------------------------
// viewpsk.cxx
//
// Copyright (C) 2008
//		Dave Freese, W1HKJ
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
// of the complete audio spectrum from 200 to 3500 Hz in equal 100 Hz
// channels.  Each channel is separately decoded and the decoded characters
// passed to the user interface routines for presentation.  The number of
// channels can be up to and including 30.

#include <config.h>

#include <stdlib.h>
#include <stdio.h>

#include "viewpsk.h"
#include "pskeval.h"
#include "pskcoeff.h"
#include "pskvaricode.h"
#include "misc.h"
#include "configuration.h"
#include "Viewer.h"
#include "qrunner.h"
#include "status.h"

extern waterfall *wf;

//=====================================================================
// Change the following for DCD low pass filter adjustment
#define SQLCOEFF 0.01
//#define SQLDECAY 50
#define SQLDECAY 20
//=====================================================================

viewpsk::viewpsk(pskeval* eval, trx_mode pskmode)
{
	for (int i = 0; i < MAXCHANNELS; i++) {
		channel[i].fir1 = (C_FIR_filter *)0;
		channel[i].fir2 = (C_FIR_filter *)0;
	}

	evalpsk = eval;
	viewmode = MODE_PREV;
	restart(pskmode);
}

viewpsk::~viewpsk()
{
	for (int i = 0; i < MAXCHANNELS; i++) {
		if (channel[i].fir1) delete channel[i].fir1;
		if (channel[i].fir2) delete channel[i].fir2;
	}
}

void viewpsk::init()
{
	nchannels = progdefaults.VIEWERchannels;
	lowfreq = progdefaults.LowFreqCutoff;

	for (int i = 0; i < MAXCHANNELS; i++) {
		channel[i].phaseacc = 0;
		channel[i].prevsymbol = cmplx (1.0, 0.0);
		channel[i].quality = cmplx (0.0, 0.0);
		channel[i].shreg = 0;
		channel[i].dcdshreg = 0;
		channel[i].dcd = false;
		channel[i].bitclk = 0;
		channel[i].freqerr = 0.0;
		channel[i].timeout = 0;
		channel[i].frequency = NULLFREQ;
		channel[i].reset = false;
		channel[i].acquire = 0;
		for (int j = 0; j < 16; j++)
			channel[i].syncbuf[j] = 0.0;
	}
	for (int i = 0; i < nchannels; i++)
		REQ(&viewclearchannel, i);

	evalpsk->clear();
	reset_all = false;
}

void viewpsk::restart(trx_mode pskmode)
{
	if (viewmode == pskmode) return;
	viewmode = pskmode;

	double			fir1c[64];
	double			fir2c[64];

	switch (viewmode) {
	case MODE_PSK31:
		symbollen = 256;
		dcdbits = 32;
		break;
	case MODE_PSK63:
	case MODE_PSK63F:
		symbollen = 128;
		dcdbits = 64;
		break;
	case MODE_PSK125:
	case MODE_PSK125R:
		symbollen = 64;
		dcdbits = 128;
		break;
	case MODE_PSK250:
	case MODE_PSK250R:
		symbollen = 32;
		dcdbits = 256;
		break;
	case MODE_PSK500:
	case MODE_PSK500R:
		symbollen = 16;
		dcdbits = 512;
		break;
	default: // punt! mode not one of the above.
		symbollen = 512;
		dcdbits = 32;
		break;
	}

	wsincfilt(fir1c, 1.0 / symbollen, true);	// creates fir1c matched sin(x)/x filter w blackman
	wsincfilt(fir2c, 1.0 / 16.0, true);			// creates fir2c matched sin(x)/x filter w blackman

	for (int i = 0; i < MAXCHANNELS; i++) {
		if (channel[i].fir1) delete channel[i].fir1;
		channel[i].fir1 = new C_FIR_filter();
		channel[i].fir1->init(FIRLEN, symbollen / 16, fir1c, fir1c);

		if (channel[i].fir2) delete channel[i].fir2;
		channel[i].fir2 = new C_FIR_filter();
		channel[i].fir2->init(FIRLEN, 1, fir2c, fir2c);
	}

	bandwidth = VPSKSAMPLERATE / symbollen;

	init();
}

//=============================================================================
//========================= viewpsk receive routines ==========================
//=============================================================================

void viewpsk::rx_bit(int ch, int bit)
{
	int c;
	channel[ch].shreg = (channel[ch].shreg << 1) | !!bit;
	if ((channel[ch].shreg & 3) == 0) {
		c = psk_varicode_decode(channel[ch].shreg >> 2);
		channel[ch].shreg = 0;
		if (c == -1) return;
		if (c == '\n' || c == '\r') c = ' ';
		if (iscntrl(c & 0xFF)) return;
		REQ(&viewaddchr, ch, (int)channel[ch].frequency, c, viewmode);
	}
}

void viewpsk::afc(int ch)
{
	if (channel[ch].dcd == true || channel[ch].acquire) {
		double error;
		double lower_bound = (lowfreq + ch * 100) - bandwidth;
		if (lower_bound < bandwidth) lower_bound = bandwidth;
		double upper_bound = lowfreq + (ch+1)*100 + bandwidth;

		error = (channel[ch].phase - channel[ch].bits * M_PI / 2);
		if (error < M_PI / 2.0) error += 2 * M_PI;
		if (error > M_PI / 2.0) error -= 2 * M_PI;
		error *= (VPSKSAMPLERATE / (symbollen * 2 * M_PI))/16.0;
		channel[ch].frequency -= error;
		channel[ch].frequency = CLAMP(channel[ch].frequency, lower_bound, upper_bound);
	}
	if (channel[ch].acquire) channel[ch].acquire--;
}

void viewpsk::clearch(int n)
{
	channel[n].reset = true;
	evalpsk->clear();
}

void viewpsk::clear()
{
	for (int i = 0; i < nchannels; i++)
		channel[i].reset = true;
	evalpsk->clear();
}

inline void viewpsk::timeout_check()
{
	for (int ch = 0; ch < nchannels; ch++) {
		if (channel[ch].timeout) channel[ch].timeout--;
		if (channel[ch].frequency == NULLFREQ) continue;
		if (channel[ch].reset || (!channel[ch].timeout && !channel[ch].acquire) ||
			(ch && (fabs(channel[ch-1].frequency - channel[ch].frequency) < bandwidth))) {
			channel[ch].reset = false;
			channel[ch].dcd = 0;
			channel[ch].frequency = NULLFREQ;
			channel[ch].acquire = 0;
			REQ(&viewclearchannel, ch);
			REQ(&viewaddchr, ch, NULLFREQ, 0, viewmode);
		}
	}
}

void viewpsk::findsignals()
{
	if (!evalpsk) return;
	double level = progStatus.VIEWER_psksquelch;
	int nomfreq = 0;
	int lfreq = 0;
	int hfreq = 0;
	int ftest;
	int f1, f2;

	timeout_check();

	for (int i = 0; i < nchannels; i++) {
		nomfreq = lowfreq + 100 * i;
		lfreq = nomfreq - 20;
		hfreq = nomfreq + 120; // suppress detection outside of this range
		if (!channel[i].dcd && !channel[i].timeout) {
			if (!channel[i].acquire) {
				channel[i].frequency = NULLFREQ;
				f1 = nomfreq - 0.5 * bandwidth;
				if (f1 < 2 * bandwidth) f1 = 2 * bandwidth;
				f2 = f1 + 100;
				ftest = (f1 + f2) / 2;
			} else {
				if (channel[i].frequency < lfreq || channel[i].frequency >= hfreq) 
					channel[i].frequency = nomfreq + 50;
				ftest = channel[i].frequency;
				f1 = ftest - bandwidth;
				f2 = ftest + bandwidth;
				if (f1 < 2 * bandwidth) {
					f1 = 2 * bandwidth;
					f2 = f1 + bandwidth;
				}
			}
			if (evalpsk->peak(ftest, f1, f2, level)) {
				if (ftest < lfreq || ftest >= hfreq) goto nexti;
				f1 = ftest - bandwidth;
				f2 = ftest + bandwidth;
				if (evalpsk->peak(ftest, f1, f2, level)) {
					if (ftest < lfreq || ftest >= hfreq) goto nexti;
					if (i && 
						(channel[i-1].dcd || channel[i-1].acquire) && 
						fabs(channel[i-1].frequency - ftest) < bandwidth) goto nexti;
					if ((i < nchannels - 1) &&
						(channel[i+1].dcd || channel[i+1].acquire) &&
						fabs(channel[i+1].frequency - ftest) < bandwidth) goto nexti;
					channel[i].frequency = ftest;
					channel[i].freqerr = 0.0;
					channel[i].metric = 0.0;
					if (!channel[i].acquire)
						channel[i].acquire = 2 * 8000 / 512;
				}
			}
		}
nexti: ;
	}
}

void viewpsk::rx_symbol(int ch, cmplx symbol)
{
	int n;

	channel[ch].phase = arg ( conj(channel[ch].prevsymbol) * symbol );
	channel[ch].prevsymbol = symbol;

	if (channel[ch].phase < 0)
		channel[ch].phase += 2 * M_PI;

	channel[ch].bits = (((int) (channel[ch].phase / M_PI + 0.5)) & 1) << 1;
	n = 2; // psk

	channel[ch].dcdshreg = (channel[ch].dcdshreg << 2) | channel[ch].bits;

	switch (channel[ch].dcdshreg) {
	case 0xAAAAAAAA:	/* DCD on by preamble */
		if (!channel[ch].dcd)
			REQ(&viewaddchr, ch, (int)channel[ch].frequency, 0, viewmode);
		channel[ch].dcd = true;
		channel[ch].quality = cmplx (1.0, 0.0);
		channel[ch].metric = 100;
		channel[ch].timeout = progdefaults.VIEWERtimeout * VPSKSAMPLERATE / WFBLOCKSIZE;
		channel[ch].acquire = 0;
		break;

	case 0:			/* DCD off by postamble */
		channel[ch].dcd = false;
		channel[ch].quality = cmplx (0.0, 0.0);
		channel[ch].metric = 0;
		channel[ch].acquire = 0;
//		channel[ch].frequency = NULLFREQ;
		break;

	default:
		channel[ch].quality = cmplx (
			decayavg(channel[ch].quality.real(), cos(n*channel[ch].phase), SQLDECAY),
			decayavg(channel[ch].quality.imag(), sin(n*channel[ch].phase), SQLDECAY));
//		channel[ch].quality.re = 
//			decayavg(channel[ch].quality.re, cos(n*channel[ch].phase), SQLDECAY);
//		channel[ch].quality.im = 
//			decayavg(channel[ch].quality.im, sin(n*channel[ch].phase), SQLDECAY);
		channel[ch].metric = norm(channel[ch].quality);
		if (channel[ch].metric > (progStatus.VIEWER_psksquelch + 6.0)/26.0) {
			channel[ch].dcd = true;
		} else {
			channel[ch].dcd = false;
		}
	}

	if (channel[ch].dcd == true) {
		channel[ch].timeout = progdefaults.VIEWERtimeout * VPSKSAMPLERATE / WFBLOCKSIZE;
		rx_bit(ch, !channel[ch].bits);
		channel[ch].acquire = 0;
	}
}

int viewpsk::rx_process(const double *buf, int len)
{
	double sum;
	double ampsum;
	int idx;
	cmplx z, z2;

	if (nchannels != progdefaults.VIEWERchannels || lowfreq != progdefaults.LowFreqCutoff)
		init();

// process all channels
	for (int ch = 0; ch < nchannels; ch++) {
		if (channel[ch].frequency == NULLFREQ) continue;
		for (int ptr = 0; ptr < len; ptr++) {
// Mix with the internal NCO for each channel
			z = cmplx ( buf[ptr] * cos(channel[ch].phaseacc), buf[ptr] * sin(channel[ch].phaseacc) );
			channel[ch].phaseacc += 2.0 * M_PI * channel[ch].frequency / VPSKSAMPLERATE;
// filter & decimate
			if (channel[ch].fir1->run( z, z )) {
				channel[ch].fir2->run( z, z2 );
				idx = (int) channel[ch].bitclk;
				sum = 0.0;
				ampsum = 0.0;
				channel[ch].syncbuf[idx] = 0.8 * channel[ch].syncbuf[idx] + 0.2 * abs(z2);

				for (int i = 0; i < 8; i++) {
					sum += (channel[ch].syncbuf[i] - channel[ch].syncbuf[i+8]);
					ampsum += (channel[ch].syncbuf[i] + channel[ch].syncbuf[i+8]);
				}
				sum = (ampsum == 0 ? 0 : sum / ampsum);

				channel[ch].bitclk -= sum / 5.0;
				channel[ch].bitclk += 1;

				if (channel[ch].bitclk < 0) channel[ch].bitclk += 16.0;
				if (channel[ch].bitclk >= 16.0) {
					channel[ch].bitclk -= 16.0;
					rx_symbol(ch, z2);
					afc(ch);
				}
			}
		}
	}

	findsignals();


	return 0;
}

int viewpsk::get_freq(int n) 
{
	if (channel[n].dcd)
		return (int)channel[n].frequency;
	return NULLFREQ;
}
