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

#include "fl_digi.h"
#include "viewpsk.h"
#include "pskeval.h"
#include "pskcoeff.h"
#include "pskvaricode.h"
#include "misc.h"
#include "configuration.h"
#include "Viewer.h"
#include "qrunner.h"
#include "status.h"
#include "trx.h"

extern waterfall *wf;

//=====================================================================
// Change the following for DCD low pass filter adjustment
#define SQLCOEFF 0.01
//#define SQLDECAY 50
#define SQLDECAY 20

#define K		5
#define POLY1	0x17
#define POLY2	0x19

#define PSKR_K		7
#define PSKR_POLY1	0x6d
#define PSKR_POLY2	0x4f

//=====================================================================

viewpsk::viewpsk(pskeval* eval, trx_mode pskmode)
{
	for (int i = 0; i < MAXCHANNELS; i++) {
		channel[i].fir1 = (C_FIR_filter *)0;
		channel[i].fir2 = (C_FIR_filter *)0;
		channel[i].dec = (viterbi *)0;
		channel[i].dec2 = (viterbi *)0;
		channel[i].Rxinlv = (interleave *)0;
		channel[i].Rxinlv2 = (interleave *)0;
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
		if (channel[i].dec) delete channel[i].dec;
		if (channel[i].dec2) delete channel[i].dec2;
		if (channel[i].Rxinlv) delete channel[i].Rxinlv;
		if (channel[i].Rxinlv2) delete channel[i].Rxinlv2;
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
		if (_pskr) {
		// MFSK varicode instead of psk
			channel[i].shreg = 1;
			channel[i].shreg2 = 1;
		} else {
			channel[i].shreg = 0;
			channel[i].shreg2 = 0;
		}
		channel[i].dcdshreg = 0;
		channel[i].dcdshreg2 = 0;
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

	double			fir1c[FIRLEN+1];
	double			fir2c[FIRLEN+1];

	int idepth = 2;
	int isize = 2;

	_pskr = false;
	_qpsk = false;
	symbits = 1;

	switch (viewmode) {
	case MODE_PSK31:
		symbollen = 256;
		dcdbits = 32;
		break;

	case MODE_PSK63F:
		_pskr = true;
	case MODE_PSK63:
		symbollen = 128;
		dcdbits = 64;
		break;

	case MODE_PSK125R:
		_pskr = true;
		idepth = 40;  // 2x2x40 interleaver
	case MODE_PSK125:
		symbollen = 64;
		dcdbits = 128;
		break;

	case MODE_PSK250R:
		_pskr = true;
		idepth = 80;  // 2x2x80 interleaver
	case MODE_PSK250:
		symbollen = 32;
		dcdbits = 256;
		break;
	case MODE_PSK500R:
		_pskr = true;
		idepth = 160; // 2x2x160 interleaver
	case MODE_PSK500:
		symbollen = 16;
		dcdbits = 512;
		break;

	case MODE_QPSK31:
		symbollen = 256;
		_qpsk = true;
		symbits = 2;
		dcdbits = 32;
		break;
	case MODE_QPSK63:
		symbollen = 128;
		symbits = 2;
		_qpsk = true;
		dcdbits = 64;
		break;
	case MODE_QPSK125:
		symbollen = 64;
		symbits = 2;
		_qpsk = true;
		dcdbits = 128;
		break;
	case MODE_QPSK250:
		symbollen = 32;
		symbits = 2;
		_qpsk = true;
		dcdbits = 256;
		break;
	case MODE_QPSK500:
		symbollen = 16;
		symbits = 2;
		_qpsk = true;
		dcdbits = 512;
		break;

	default: // punt! mode not one of the above.
		symbollen = 512;
		dcdbits = 32;
		break;
	}

	raisedcosfilt(fir1c, FIRLEN);
	for (int i = 0; i <= FIRLEN; i++)
		fir2c[i] = pskcore_filter[i];

	for (int i = 0; i < MAXCHANNELS; i++) {
		if (channel[i].fir1) delete channel[i].fir1;
		channel[i].fir1 = new C_FIR_filter();
		channel[i].fir1->init(FIRLEN+1, symbollen / 16, fir1c, fir1c);

		if (channel[i].fir2) delete channel[i].fir2;
		channel[i].fir2 = new C_FIR_filter();
		channel[i].fir2->init(FIRLEN+1, 1, fir2c, fir2c);

		if (_qpsk) {
			if (channel[i].dec) delete channel[i].dec;
			channel[i].dec = new viterbi(K, POLY1, POLY2);

			if (channel[i].dec2) delete channel[i].dec;
			channel[i].dec2 = 0;
		} else {
			if (channel[i].dec) delete channel[i].dec;
			channel[i].dec = new viterbi(PSKR_K, PSKR_POLY1, PSKR_POLY2);
			channel[i].dec->setchunksize(4);

			if (channel[i].dec2) delete channel[i].dec;
			channel[i].dec2 = new viterbi(PSKR_K, PSKR_POLY1, PSKR_POLY2);
			channel[i].dec2->setchunksize(4);
		}

	// 2x2x(20,40,80,160)
		channel[i].Rxinlv = new interleave (isize, idepth, INTERLEAVE_REV);
	// 2x2x(20,40,80,160)
		channel[i].Rxinlv2 = new interleave (isize, idepth, INTERLEAVE_REV);

	}

	bandwidth = VPSKSAMPLERATE / symbollen;

	init();
}

//=============================================================================
//========================= viewpsk receive routines ==========================
//=============================================================================

bool viewpsk::is_valid_char(int &c)
{
	if (c == '\n' || c == '\r') {
		c = ' ';
		return true;
	}
	if (c <= 0) return false;
	if (c > 0x7F) return false;
	if (iscntrl(c & 0xFF)) return false;
	return true;
}

void viewpsk::rx_bit(int ch, int bit)
{
	int c;
	channel[ch].shreg = (channel[ch].shreg << 1) | !!bit;
	if (_pskr) {
		// MFSK varicode instead of PSK Varicode
		if ((channel[ch].shreg & 7) == 1) {
			c = varidec(channel[ch].shreg >> 1);
			channel[ch].shreg = 1;
			// Voting at the character level
			if (channel[ch].fecmet >= channel[ch].fecmet2) {
				if (is_valid_char(c))
					REQ(&viewaddchr, ch, (int)channel[ch].frequency, c, viewmode);
			}
		}
	} else {
		if ((channel[ch].shreg & 3) == 0) {
			c = psk_varicode_decode(channel[ch].shreg >> 2);
			channel[ch].shreg = 0;
			if (is_valid_char(c))
				REQ(&viewaddchr, ch, (int)channel[ch].frequency, c, viewmode);
		}
	}
}

void viewpsk::rx_bit2(int ch, int bit)
{
	int c;

	channel[ch].shreg2 = (channel[ch].shreg2 << 1) | !!bit;
	// MFSK varicode instead of PSK Varicode
	if ((channel[ch].shreg2 & 7) == 1) {
		c = varidec(channel[ch].shreg2 >> 1);
		// Voting at the character level
		if (channel[ch].fecmet < channel[ch].fecmet2) {
			if (is_valid_char(c))
				REQ(&viewaddchr, ch, (int)channel[ch].frequency, c, viewmode);
		}
		channel[ch].shreg2 = 1;
	}
}

void viewpsk::rx_pskr(int ch, unsigned char symbol)
{
	int met;
	unsigned char twosym[2];
	unsigned char tempc;
	int c;

	// Accumulate the soft bits for the interleaver THEN submit to Viterbi
	// decoder in alternance so that each one is processed one bit later.
	// Only two possibilities for sync: current bit or previous one since
	// we encode with R = 1/2 and send encoded bits one after the other
	// through the interleaver.

	channel[ch].symbolpair[1] = channel[ch].symbolpair[0];
	channel[ch].symbolpair[0] = symbol;

	if (channel[ch].rxbitstate == 0) {
		// process bit 1
		// copy to avoid scrambling symbolpair for the next bit
		channel[ch].rxbitstate = 1;
		twosym[0] = channel[ch].symbolpair[0];
		twosym[1] = channel[ch].symbolpair[1];
		// De-interleave for Robust modes only
		if (viewmode != MODE_PSK63F) channel[ch].Rxinlv2->symbols(twosym);
		// pass de-interleaved bits pair to the decoder, reversed
		tempc = twosym[1];
		twosym[1] = twosym[0];
		twosym[0] = tempc;
		// Then viterbi decoder
		c = channel[ch].dec2->decode(twosym, &met);
		if (c != -1) {
			// FEC only take metric measurement after backtrace
			// Will be used for voting between the two decoded streams
			channel[ch].fecmet2 = decayavg(channel[ch].fecmet2, met, 20);
			rx_bit2(ch, c & 0x08);
			rx_bit2(ch, c & 0x04);
			rx_bit2(ch, c & 0x02);
			rx_bit2(ch, c & 0x01);
		}
	} else {
		// process bit 0
		// copy to avoid scrambling symbolpair for the next bit
		channel[ch].rxbitstate = 0;
		twosym[0] = channel[ch].symbolpair[0];
		twosym[1] = channel[ch].symbolpair[1];
		// De-interleave
		if (viewmode != MODE_PSK63F) channel[ch].Rxinlv->symbols(twosym);
		tempc = twosym[1];
		twosym[1] = twosym[0];
		twosym[0] = tempc;
		// Then viterbi decoder
		c = channel[ch].dec->decode(twosym, &met);
		if (c != -1) {
			channel[ch].fecmet = decayavg(channel[ch].fecmet, met, 20);
			rx_bit(ch, c & 0x08);
			rx_bit(ch, c & 0x04);
			rx_bit(ch, c & 0x02);
			rx_bit(ch, c & 0x01);
		}
	}
}

void viewpsk::rx_qpsk(int ch, int bits)
{
	unsigned char sym[2];
	int c;

	if (!active_modem->get_reverse())
		bits = (4 - bits) & 3;

	sym[0] = (bits & 1) ? 255 : 0;
	sym[1] = (bits & 2) ? 0 : 255;	// top bit is flipped

	c = channel[ch].dec->decode(sym, NULL);

	if (c != -1) {
		rx_bit(ch, c & 0x80);
		rx_bit(ch, c & 0x40);
		rx_bit(ch, c & 0x20);
		rx_bit(ch, c & 0x10);
		rx_bit(ch, c & 0x08);
		rx_bit(ch, c & 0x04);
		rx_bit(ch, c & 0x02);
		rx_bit(ch, c & 0x01);
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
	int n = 2; // psk
	unsigned char softbit = 128;
	double softangle;
	double softamp;
	double sigamp = norm(symbol);

	channel[ch].phase = arg ( conj(channel[ch].prevsymbol) * symbol );
	channel[ch].prevsymbol = symbol;

	if (channel[ch].phase < 0)
		channel[ch].phase += 2 * M_PI;

	if (_qpsk) {
		n = 4;
		channel[ch].bits = ((int) (channel[ch].phase / M_PI_2 + 0.5)) & (n-1);
	} else {
		channel[ch].bits = (((int) (channel[ch].phase / M_PI + 0.5)) & (n-1)) << 1;

	// hard decode if needed
	// softbit = (bits & 2) ? 0 : 255;  
	// reversed as we normally pass "!bits" when hard decoding
	// Soft decode section below
		channel[ch].averageamp = decayavg(channel[ch].averageamp, sigamp, SQLDECAY);
		if (sigamp > 0 && channel[ch].averageamp > 0) {
			softamp = clamp( channel[ch].averageamp / sigamp, 1.0, 1e6);
		} else {
			softamp = 1; // arbritary number (50% impact)
		}
	// Compute values between -128 and +127 for phase value only
		if (channel[ch].phase > M_PI) {
			softangle = (127 - (((2 * M_PI - channel[ch].phase) / M_PI) * (double) 255));
		} else {
			softangle = (127 - ((channel[ch].phase / M_PI) * (double) 255));
		}
	// Then apply impact of amplitude. Finally, re-centre on 127-128
	// as the decoder needs values between 0-255
		softbit = (unsigned char) ((softangle / (1 + softamp)) - 127);
	}

	channel[ch].dcdshreg <<= (symbits + 1);
	channel[ch].dcdshreg |= channel[ch].bits;

	switch (channel[ch].dcdshreg) {

	// bpsk DCD on

		case 0xAAAAAAAA:	/* DCD on by preamble */
			if (_pskr) break;
			if (!channel[ch].dcd)
				REQ(&viewaddchr, ch, (int)channel[ch].frequency, 0, viewmode);
			channel[ch].dcd = 1;
			channel[ch].quality = cmplx (1.0, 0.0);
			channel[ch].metric = 100;
			channel[ch].timeout = progdefaults.VIEWERtimeout * VPSKSAMPLERATE / WF_BLOCKSIZE;
			channel[ch].acquire = 0;
			break;

	// pskr DCD on
		case 0x0A0A0A0A:
			if (!_pskr) break;
			if (!channel[ch].dcd)
				REQ(&viewaddchr, ch, (int)channel[ch].frequency, 0, viewmode);
			channel[ch].dcd = 1;
			channel[ch].quality = cmplx (1.0, 0.0);
			channel[ch].metric = 100;
			channel[ch].timeout = progdefaults.VIEWERtimeout * VPSKSAMPLERATE / WF_BLOCKSIZE;
			channel[ch].acquire = 0;
			break;

	case 0:			/* DCD off by postamble */
		channel[ch].dcd = false;
		channel[ch].quality = cmplx (0.0, 0.0);
		channel[ch].metric = 0;
		channel[ch].acquire = 0;
		break;

	default:
		channel[ch].quality = cmplx (
			decayavg(channel[ch].quality.real(), cos(n*channel[ch].phase), SQLDECAY),
			decayavg(channel[ch].quality.imag(), sin(n*channel[ch].phase), SQLDECAY));
		channel[ch].metric = norm(channel[ch].quality);
		if (channel[ch].metric > (progStatus.VIEWER_psksquelch + 6.0)/26.0) {
			channel[ch].dcd = true;
		} else {
			channel[ch].dcd = false;
		}
	}

	if (channel[ch].dcd == true) {
		channel[ch].timeout = progdefaults.VIEWERtimeout * VPSKSAMPLERATE / WF_BLOCKSIZE;
		if (_qpsk) rx_qpsk(ch, channel[ch].bits);
		else if (_pskr) rx_pskr(ch, softbit);
		else rx_bit(ch, !channel[ch].bits);
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

				double bitsteps = (symbollen >= 16 ? 16 : symbollen);
				int symsteps = (int) (bitsteps / 2);

				for (int i = 0; i < symsteps; i++) {
					sum += (channel[ch].syncbuf[i] - channel[ch].syncbuf[i+8]);
					ampsum += (channel[ch].syncbuf[i] + channel[ch].syncbuf[i+8]);
				}
				sum = (ampsum == 0 ? 0 : sum / ampsum);

				channel[ch].bitclk -= sum / 5.0;
				channel[ch].bitclk += 1;

				if (channel[ch].bitclk < 0) channel[ch].bitclk += bitsteps;
				if (channel[ch].bitclk >= bitsteps) {
					channel[ch].bitclk -= bitsteps;
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
