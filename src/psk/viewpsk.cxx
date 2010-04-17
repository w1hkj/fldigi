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

extern waterfall *wf;

//=====================================================================
// Change the following for DCD low pass filter adjustment
#define SQLCOEFF 0.01
//=====================================================================

viewpsk::viewpsk(pskeval* eval, trx_mode pskmode)
{
	for (int i = 0; i < MAXCHANNELS; i++) {
		fir1[i] = (C_FIR_filter *)0;
		fir2[i] = (C_FIR_filter *)0;
	}

	evalpsk = eval;
	viewmode = MODE_PREV;	
	restart(pskmode);
}

viewpsk::~viewpsk()
{
	for (int i = 0; i < MAXCHANNELS; i++) {
		if (fir1[i]) delete fir1[i];
		if (fir2[i]) delete fir2[i];
	}
}

void viewpsk::init()
{
	for (int i = 0; i < MAXCHANNELS; i++) {
		phaseacc[i] = 0;
		prevsymbol[i]	= complex (1.0, 0.0);
		shreg[i] = 0;
		dcdshreg[i] = 0;
		dcd[i] = false;
		bitclk[i] = 0;
		freqerr[i] = 0.0;
		waitcount[i] = VWAITCOUNT;
		nomfreq[i] = progdefaults.VIEWERstart + 100 * i;
		frequency[i] = nomfreq[i];
		for (int j = 0; j < 16; j++)
			syncbuf[i * 16 + j] = 0.0;
		timeout[i] = 0;
	}
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
		symbollen = 128;
		dcdbits = 64;
		break;
	case MODE_PSK125:
		symbollen = 64;
		dcdbits = 128;
		break;
	case MODE_PSK250:
		symbollen = 32;
		dcdbits = 256;
		break;
	case MODE_PSK500:
		symbollen = 16;
		dcdbits = 512;
		break;
	default: // MODE_PSK31;
		symbollen = 256;
		dcdbits = 32;
	}

	wsincfilt(fir1c, 1.0 / symbollen, true);	// creates fir1c matched sin(x)/x filter w blackman
	wsincfilt(fir2c, 1.0 / 16.0, true);			// creates fir2c matched sin(x)/x filter w blackman

	for (int i = 0; i < MAXCHANNELS; i++) {
		if (fir1[i]) delete fir1[i];
		fir1[i] = new C_FIR_filter();
		fir1[i]->init(FIRLEN, symbollen / 16, fir1c, fir1c);

		if (fir2[i]) delete fir2[i];
		fir2[i] = new C_FIR_filter();
		fir2[i]->init(FIRLEN, 1, fir2c, fir2c);
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
	shreg[ch] = (shreg[ch] << 1) | !!bit;
	if ((shreg[ch] & 3) == 0) {
		c = psk_varicode_decode(shreg[ch] >> 2);
		shreg[ch] = 0;
		if (c == '\n' || c == '\r') c = ' ';
		if (isprint(c)) {
			REQ(&viewaddchr, ch, (int)frequency[ch], c, viewmode);
			timeout[ch] = now + progdefaults.VIEWERtimeout;
		}
	}
}

void viewpsk::findsignal(int ch)
{
	if (waitcount[ch] > 0) {
			waitcount[ch]--;
			return;
	}
		
	int ftest;
	int f1, f2;
	if (ch == 0)
		f1 = (int)nomfreq[ch];
	else if (dcd[ch-1])
		f1 = (int)(frequency[ch-1] + 2 * bandwidth);
	else
		f1 = (int)(nomfreq[ch] - bandwidth);
		
	if (ch == progdefaults.VIEWERchannels - 1)
		f2 = (int)(nomfreq[ch] + 100);
	else
		f2 = (int)(nomfreq[ch+1]);

	frequency[ch] = nomfreq[ch];
	if (evalpsk) {
		if (evalpsk->sigpeak(ftest, f1, f2) > pow(10, progdefaults.VIEWERsquelch / 10.0) ) {
			if (ftest < (nomfreq[ch+1]- bandwidth)) {
				frequency[ch] = ftest;
				timeout[ch] = now + progdefaults.VIEWERtimeout;
			}
		}
	}
	freqerr[ch] = 0.0;
}

void viewpsk::afc(int ch)
{
	double error;
	if (dcd[ch] == true) {
		error = (phase[ch] - bits[ch] * M_PI / 2);
		if (error < M_PI / 2)
			error += 2 * M_PI;
		if (error > M_PI / 2)
			error -= 2 * M_PI;
		error *= ((VPSKSAMPLERATE / (symbollen * 2 * M_PI)/16));
		if (fabs(error) < bandwidth) {
			freqerr[ch] = decayavg( freqerr[ch], error, VAFCDECAY);
			frequency[ch] -= freqerr[ch];
		}
	}
}


void viewpsk::rx_symbol(int ch, complex symbol)
{
	int n;
	phase[ch] = (prevsymbol[ch] % symbol).arg();
	prevsymbol[ch] = symbol;

	if (phase[ch] < 0) 
		phase[ch] += 2 * M_PI;

	bits[ch] = (((int) (phase[ch] / M_PI + 0.5)) & 1) << 1;
	n = 2;

// simple low pass filter for quality of signal
	
	quality[ch].re = SQLCOEFF * cos(n * phase[ch]) + (1.0 - SQLCOEFF) * quality[ch].re;
	quality[ch].im = SQLCOEFF * sin(n * phase[ch]) + (1.0 - SQLCOEFF) * quality[ch].im;

	metric[ch] = 100.0 * quality[ch].norm();
	
	dcdshreg[ch] = (dcdshreg[ch] << 2) | bits[ch];

	switch (dcdshreg[ch]) {
	case 0xAAAAAAAA:	/* DCD on by preamble */
		dcd[ch] = true;
		quality[ch] = complex (1.0, 0.0);
		break;

	case 0:			/* DCD off by postamble */
		dcd[ch] = false;
		quality[ch] = complex (0.0, 0.0);
		break;

	default:
		if (metric[ch] > progdefaults.VIEWERsquelch)
			dcd[ch] = true;
		else
			dcd[ch] = false;
	}

	if (dcd[ch] == true)
		rx_bit(ch, !bits[ch]);
}

int viewpsk::rx_process(const double *buf, int len)
{
	if (!(dlgViewer && dlgViewer->shown()))
		return 0;

	double sum;
	double ampsum;
	int idx;
	complex z[MAXCHANNELS];

	while (len-- > 0) {
// process all CHANNELS (25)
		for (int channel = 0; channel < progdefaults.VIEWERchannels; channel++) {
			
// Mix with the internal NCO for each channel
			z[channel] = complex ( *buf * cos(phaseacc[channel]), *buf * sin(phaseacc[channel]) );

			phaseacc[channel] += 2.0 * M_PI * frequency[channel] / VPSKSAMPLERATE;
			if (phaseacc[channel] > M_PI)
				phaseacc[channel] -= 2.0 * M_PI;
// filter & decimate
			if (fir1[channel]->run( z[channel], z[channel] )) { 
				fir2[channel]->run( z[channel], z[channel] ); 
			
				idx = (int) bitclk[channel];
				sum = 0.0;
				ampsum = 0.0;
				int syncbase = channel * 16;
				syncbuf[syncbase + idx] = 0.8 * syncbuf[syncbase + idx] + 0.2 * z[channel].mag();
			
				for (int i = 0; i < 8; i++) {
					sum += (syncbuf[syncbase + i] - syncbuf[syncbase + (i+8)]);
					ampsum += (syncbuf[syncbase + i] + syncbuf[syncbase + (i+8)]);
				}
				sum = (ampsum == 0 ? 0 : sum / ampsum);
			
				bitclk[channel] -= sum / 5.0;
				bitclk[channel] += 1;
			
				if (bitclk[channel] < 0) bitclk[channel] += 16.0;
				if (bitclk[channel] >= 16.0) {
					bitclk[channel] -= 16.0;
					rx_symbol(channel, z[channel]);
					afc(channel);
				}
			}
		}
		buf++;
	}

	now = time(NULL);
	for (int channel = 0; channel < progdefaults.VIEWERchannels; channel++) {
		if (!dcd[channel])
			findsignal(channel);
		if (timeout[channel] > 0 && timeout[channel] < now) {
			frequency[channel] = nomfreq[channel];
			REQ( &viewclearchannel, channel);
			timeout[channel] = 0;
			dcd[channel] = false;
		}
	}
	return 0;
}

