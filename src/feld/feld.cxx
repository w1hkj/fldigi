//
//    feld.cxx  --  FELDHELL modem
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
//  Copyright (C) 2004
//  Lawrence Glaister (ve7it@shaw.ca)
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------


#include <config.h>

#include <stdlib.h>
#include <stdio.h>

#include "feld.h"
#include "fontdef.h"
#include "confdialog.h"
#include "qrunner.h"

char feldmsg[80];

void feld::tx_init(SoundBase *sc)
{
	scard = sc;
	txcounter = 0.0;
	tx_state = PREAMBLE;
	preamble = 3;
	prevsymb = false;
	videoText();
	return;
}

void feld::rx_init()
{
	rxcounter = 0.0;
	peakhold = 0.0;
	for (int i = 0; i < 2*RxColumnLen; i++ )
		col_data[i] = 0;
	col_pointer = 0;
	peakhold = 0.0;
	minhold = 1.0;
	agc = 0.0;
	return;
}

void feld::init()
{
	modem::init();
	initKeyWaveform();
	digiscope->mode(Digiscope::BLANK);
	put_MODEstatus(mode);
}

feld::~feld()
{
	if (hilbert) delete hilbert;
	if (bpfilt) delete bpfilt;
	if (bbfilt) delete bbfilt;
}

feld::feld(trx_mode m)
{
	double lp;
	mode = m;
	samplerate = FeldSampleRate;
	FL_LOCK_D();
	bandwidth = sldrHellBW->value();
	FL_UNLOCK_D();
	
 	switch (mode) { 
		case MODE_FSKHELL: 
			bandwidth = 122.5; 
			feldcolumnrate = 17.5; 
			break; 
		case MODE_FSKH105: 
			bandwidth = 55; 
			feldcolumnrate = 17.5; 
			break; 
		case MODE_HELL80: 
			bandwidth = 300; 
			feldcolumnrate = 35; 
			break;
		case MODE_FELDHELL: 
 		default :
 			bandwidth = 122.5; 
 			feldcolumnrate = 17.5;
 			break; 
 	}
 	hell_bandwidth = bandwidth;

	rxpixrate = (RxColumnLen * feldcolumnrate);
	txpixrate = (TxColumnLen * feldcolumnrate);
	downsampleinc = (double)(rxpixrate/samplerate);
	upsampleinc = (double)(txpixrate/samplerate);

/////////////
	hell_bandwidth = bandwidth;
	
	hilbert = new C_FIR_filter();
	hilbert->init_hilbert(37, 1);

	lp = 1.5 * bandwidth / 2.0 / samplerate;
	
	bpfilt = new fftfilt(0, lp, 1024);
	
	bbfilt = new Cmovavg(8);
	
	minmaxfilt = new Cmovavg(120);
	
	blackboard = false;
	hardkeying = false;

	rxphacc = 0.0;
	txphacc = 0.0;

}

// rx section

complex feld::mixer(complex in)
{

	complex z;

	z.re = cos(rxphacc);
	z.im = sin(rxphacc);

	z = z * in;

	rxphacc -= 2.0 * M_PI * frequency / samplerate;

	if (rxphacc > M_PI)
		rxphacc -= 2.0 * M_PI;
	else if (rxphacc < M_PI)
		rxphacc += 2.0 * M_PI;

	return z;
}


void feld::FSKHELL_rx(complex z)
{
	double f;
	int vid;

	f = (prev % z).arg() * samplerate / M_PI / bandwidth / 2.0;
	prev = z;
	
	f = bbfilt->run(f);

	rxcounter += downsampleinc;
	if (rxcounter < 1.0)
		return;

	rxcounter -= 1.0;
	
	f = CLAMP(f + 0.5, 0.0, 1.0);
	if (mode == MODE_HELL80)
		f = 1.0 - f;
	
	if (reverse)
		f = 1.0 - f;
	if (blackboard)
		f = 1.0 - f;
	vid = (int)(255 * f);

	col_data[col_pointer + RxColumnLen] = vid;
	col_pointer++;
	if (col_pointer == RxColumnLen) {
		REQ(put_rx_data, col_data, col_data.size());
		if (!halfwidth)
			REQ(put_rx_data, col_data, col_data.size());
		col_pointer = 0;
		for (int i = 0; i < RxColumnLen; i++)
			col_data[i] = col_data[i + RxColumnLen];
	}

}

void feld::rx(complex z)
{
    double x;

	x = bbfilt->run(z.mag());
	
	rxcounter += downsampleinc;
	if (rxcounter < 1.0)
		return;

	rxcounter -= 1.0;

	if (x > peakhold)
		peakhold = x;
    else
	peakhold *= (1 - 0.02 / RxColumnLen);
	if (x < minhold)
		minhold = x;
	else
		minhold *= (1 - 0.02 / RxColumnLen);
	
	x = CLAMP (x / peakhold, 0.0, 1.0);

	agc = minmaxfilt->run(peakhold - minhold);
	
	metric = CLAMP(100*agc, 0.0, 100.0); 
	display_metric(metric);
	
	if (blackboard)
		x = 255 * x;
	else
		x = 255 * (1.0 - x);
	
	col_data[col_pointer + RxColumnLen] = (int)x;
	col_pointer++;
	if (col_pointer == RxColumnLen) {
		if (metric > progdefaults.sldrSquelchValue || progdefaults.sqlonoff == false) {
			REQ(put_rx_data, col_data, col_data.size());
			if (!halfwidth)
				REQ(put_rx_data, col_data, col_data.size());
		}
		col_pointer = 0;
		for (int i = 0; i < RxColumnLen; i++)
			col_data[i] = col_data[i + RxColumnLen];
	}
}

int feld::rx_process(const double *buf, int len)
{

	complex z, *zp;
	int i, n;

	FL_LOCK_D();
	halfwidth = btnHellRcvWidth->value();
	blackboard = btnBlackboard->value();
//	squelch = progdefaults.sldrSquelchValue;
//	squelchon = progdefaults.sqlonoff;
	FL_UNLOCK_D();
	
	switch (mode) {
		default:
			if (bandwidth != hell_bandwidth) {
				double lp;
				hell_bandwidth = bandwidth;
				lp = 1.5 * bandwidth / 2.0 / samplerate;
				bpfilt->create_filter(0, lp);
			}
			break;
		case MODE_FSKHELL:
		case MODE_FSKH105:
		case MODE_HELL80:
			break;
	}

	while (len-- > 0) {
		/* create analytic signal... */
		z.re = z.im = *buf++;

		hilbert->run(z, z);

		/* ...so it can be shifted in frequency */
		z = mixer(z);

		n = bpfilt->run(z, &zp);

		switch (mode) {
			case MODE_FSKHELL:
			case MODE_FSKH105:
			case MODE_HELL80:
				for (i = 0; i < n; i++) {
					FSKHELL_rx(zp[i]);
				}
				break;
			default:
				for (i = 0; i < n; i++)
					rx(zp[i]);
				break;
		}
	}

	return 0;
}

//=====================================================================
// tx section

// returns value = column bits with b0 ... b13 the transmit rows respecfully
// 1 = on, 0 = off
// if all bits are 0
// and no lesser bits are set then character is complete
// then return -1;

int feld::get_font_data(unsigned char c, int col)
{
	int bits = 0;
	int mask;
	int bin;
	int ordbits = 0;
	fntchr *font = 0;
	
	if (col > 15 || c < ' ' || c > '~') 
		return -1;
	mask = 1 << (15 - col);
	switch (fntnbr) {
		case 0: font = feld7x7_14; break;
		case 1: font = feld7x7n_14; break;
		case 2: font = feldDx_14; break;
		case 3: font = feldfat_14; break;
		case 4: font = feldhell_12; break;
		case 5: font = feldlittle_12; break;
		case 6: font = feldlo8_14; break;
		case 7: font = feldlow_14; break;
		case 8: font = feldmodern_14; break;
		case 9: font = feldmodern8_14; break;
		case 10: font = feldnarr_14; break;
		case 11: font = feldreal_14; break;
		case 12: font = feldstyl_14; break;
		case 13: font = feldvert_14; break;
		case 14: font = feldwide_14; break;
		default: font = feld7x7_14;
	}
	for (int i = 0; i < 14; i++) ordbits |= font[c-' '].byte[i];
	
	for (int row = 0; row < 14; row ++) {
		bin =  font[c - ' '].byte[13 - row] & mask;
		if ( bin != 0)
			bits |= 1 << row;
	}
	int testval = (1 << (15 - col)) - 1;
	if ( (bits == 0) && ((ordbits & testval) == 0) )
		return -1;
	return bits;
}

double feld::nco(double freq)
{
	double x = sin(txphacc);

	txphacc += 2.0 * M_PI * freq / samplerate;

	if (txphacc > M_PI)
		txphacc -= 2.0 * M_PI;

	return x;
}

void feld::send_symbol(int currsymb, int nextsymb)
{
	double tone = get_txfreq_woffset();
	double Amp;
	int outlen = 0;
	
	for (;;) {
		Amp = 1.0;
		switch (mode) {
			case MODE_FSKHELL :
			case MODE_FSKH105 :
				tone = get_txfreq_woffset() + (reverse ? -1 : 1) * (currsymb ? -1 : 1) * bandwidth / 2.0;
				break;
			case MODE_HELL80:
				tone = get_txfreq_woffset() - (reverse ? -1 : 1) * (currsymb ? -1 : 1) * bandwidth / 2.0;
				break;
			case MODE_FELDHELL :
			default :
				if (prevsymb == 0 && currsymb == 1) {
					Amp = OnShape[outlen];
				} else if (currsymb == 1 && nextsymb == 0) {
					Amp = OffShape[outlen];
				} else 
					Amp = currsymb;
				break;
		}
		outbuf[outlen++] = Amp * nco(tone);

		if (outlen >= OUTBUFSIZE)
			break;
		txcounter += upsampleinc;
		if (txcounter < 1.0)
			continue;
		txcounter -= 1.0;
		break;
	}
	prevsymb = currsymb;

// write to soundcard & display
	ModulateXmtr(outbuf, outlen);

// rx echo
	rx_process(outbuf, outlen);
	
}

void feld::send_null_column()
{
	for (int i = 0; i < 14; i++)
		send_symbol(0, 0);
}

void feld::tx_char(char c)
{
	int column = 0;
	int bits, colbits;
	int currbit, nextbit;
	send_null_column();
	if (c == ' ') {
		send_null_column();
		send_null_column();
		send_null_column();
	} else {
		while ((bits = get_font_data(c, column)) != -1) {
			for (int col = 0; col < dxmode; col++) {
				colbits = bits;
				for (int i = 0; i < 14; i++) {
					currbit = colbits & 1;
					colbits = colbits >> 1;
					nextbit = colbits & 1;
					send_symbol(currbit, nextbit);
				}
			}
			column++;
		}
	}
	send_null_column();
	return;
}

int feld::tx_process()
{
	int c;
	bool hdkey;

	FL_LOCK_D();
	dxmode = 1 + btnHellXmtWidth->value();
	hdkey = btnHellFastAttack->value();
	FL_UNLOCK_D();
	fntnbr = progdefaults.feldfontnbr;
	if (hardkeying != hdkey) {
		hardkeying = hdkey;
		initKeyWaveform();
	}
	
	if (tx_state == PREAMBLE) {
		if (preamble-- > 0) {
			tx_char('.');
			return 0;
		}
		tx_state = DATA;
	}
	
	if (tx_state == POSTAMBLE) {
		if (postamble-- > 0) {
			tx_char('.');
			return 0;
		}
		tx_char(' ');
		tx_state = PREAMBLE;
		cwid();
		return -1;
	}
	
	c = get_tx_char();

	if (c == 0x03 || stopflag) {
		tx_state = POSTAMBLE;
		postamble = 3;
		return 0;
	}

// if TX buffer empty
// send idle character
	if (c == -1) {
		if (progdefaults.FELD_IDLE == true)
			c = '.';
		else {
			send_null_column();
			send_null_column();
			return 0;
		}
	}
	if (c == '\r' || c == '\n')
		c = ' ';

	tx_char(c);

	return 0;
}

void feld::initKeyWaveform()
{
	for (int i = 0; i < 80; i++) {
		OnShape[i] = 1.0;
		OffShape[i] = 0.0;
	}
	for (int i = 0; i < 32; i++) {
		if (hardkeying == false)
			OnShape[i] = 0.5*(1.0 - cos(M_PI * i / 33)); // raised cosine with 4 msec rise
		else if (i < 16)
			OnShape[i] = 0.5*(1.0 - cos(M_PI * i / 16)); // raised cosine with 2 msec rise
		OffShape[31 - i] = OnShape[i];
	}
}
