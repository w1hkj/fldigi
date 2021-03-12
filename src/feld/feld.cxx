//
//	feld.cxx  --  FELDHELL modem
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
//
// This modem code owes much to the efforts of Joe Veldhuis, N8FQ
//
// Adapted from code contained in gmfsk source code distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
//  Copyright (C) 2004
//  Lawrence Glaister (ve7it@shaw.ca)
// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <stdlib.h>
#include <stdio.h>

#include <iostream>

using namespace std;

#include "feld.h"
#include "fl_digi.h"
#include "fontdef.h"
#include "confdialog.h"
#include "qrunner.h"
#include "status.h"
#include "debug.h"
#include "threads.h"

#include <FL/Fl.H>
#include <FL/Fl_Value_Slider.H>

LOG_FILE_SOURCE(debug::LOG_MODEM);

pthread_mutex_t feld_mutex = PTHREAD_MUTEX_INITIALIZER;

char feldmsg[80];

void feld::tx_init()
{
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
	for (int i = 0; i < 2 * MAX_RX_COLUMN_LEN; i++ )
		col_data[i] = 0;
	col_pointer = 0;
	peakhold = 0.0;
	agc = 0.0;

	RxColumnLen = progdefaults.HellRcvHeight;
	switch (mode) {
// Amplitude modulation modes
		case MODE_FELDHELL:
			rxpixrate = (RxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			break;
		case MODE_SLOWHELL:
			rxpixrate = (RxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			break;
		case MODE_HELLX5:
			rxpixrate = (RxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			break;
		case MODE_HELLX9:
			rxpixrate = (RxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			break;
// Frequency modulation modes
		case MODE_FSKH245:
			rxpixrate = (RxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			break;
		case MODE_FSKH105:
			rxpixrate = (RxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			break;
		case MODE_HELL80:
			rxpixrate = (RxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			break;
		default :
			rxpixrate = (RxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			break;
	}

	return;
}

void feld::init()
{
	modem::init();
	initKeyWaveform();
	set_scope_mode(Digiscope::BLANK);
	put_MODEstatus(mode);

	if (progdefaults.StartAtSweetSpot)
		set_freq(progdefaults.PSKsweetspot);
	else if (progStatus.carrier != 0) {
		set_freq(progStatus.carrier);
#if !BENCHMARK_MODE
		progStatus.carrier = 0;
#endif
	} else
		set_freq(wf->Carrier());

	rx_init();

}

static void set_HellBW(double val)
{
	sldrHellBW->value(val);
}

void feld::restart()
{
	RxColumnLen = progdefaults.HellRcvHeight;
	TxColumnLen = FELD_COLUMN_LEN;

	switch (mode) {
// Amplitude modulation modes
		case MODE_FELDHELL:
 			feldcolumnrate = 17.5;
			rxpixrate = (RxColumnLen * feldcolumnrate);
			txpixrate = (TxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			upsampleinc = (double)(txpixrate/samplerate);
			hell_bandwidth = txpixrate;
			filter_bandwidth = 5 * round(1.2 * hell_bandwidth / 5.0);
			progdefaults.HELL_BW_FH = filter_bandwidth;
			break;
		case MODE_SLOWHELL:
			feldcolumnrate = 2.1875;
			rxpixrate = (RxColumnLen * feldcolumnrate);
			txpixrate = (TxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			upsampleinc = (double)(txpixrate/samplerate);
			hell_bandwidth = txpixrate;
			filter_bandwidth = 5 * round(1.2 * hell_bandwidth / 5.0);
			progdefaults.HELL_BW_FH = filter_bandwidth;
			break;
		case MODE_HELLX5:
			feldcolumnrate = 87.5;
			rxpixrate = (RxColumnLen * feldcolumnrate);
			txpixrate = (TxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			upsampleinc = (double)(txpixrate/samplerate);
			hell_bandwidth = txpixrate;
			filter_bandwidth = 5 * round(1.2 * hell_bandwidth / 5.0);
			progdefaults.HELL_BW_X5 = filter_bandwidth;
			break;
		case MODE_HELLX9:
			feldcolumnrate = 157.5;
			rxpixrate = (RxColumnLen * feldcolumnrate);
			txpixrate = (TxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			upsampleinc = (double)(txpixrate/samplerate);
			hell_bandwidth = txpixrate;
			filter_bandwidth = 5 * round(1.2 * hell_bandwidth / 5.0);
			progdefaults.HELL_BW_X9 = filter_bandwidth;
			break;
// Frequency modulation modes
		case MODE_FSKH245:
			feldcolumnrate = 17.5;
			rxpixrate = (RxColumnLen * feldcolumnrate);
			txpixrate = (TxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			upsampleinc = (double)(txpixrate/samplerate);
			hell_bandwidth = 122.5;
			phi2freq = samplerate / M_PI / (hell_bandwidth / 2.0);
			filter_bandwidth = 5 * round(4.0 * hell_bandwidth / 5.0);
			progdefaults.HELL_BW_FSKH245 = filter_bandwidth;
			cap |= CAP_REV;
			break;
		case MODE_FSKH105:
			feldcolumnrate = 17.5;
			rxpixrate = (RxColumnLen * feldcolumnrate);
			txpixrate = (TxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			upsampleinc = (double)(txpixrate/samplerate);
			hell_bandwidth = 55;
			phi2freq = samplerate / M_PI / (hell_bandwidth / 2.0);
			filter_bandwidth = 5 * round(4.0 * hell_bandwidth / 5.0);
			progdefaults.HELL_BW_FSKH105 = filter_bandwidth;
			cap |= CAP_REV;
			break;
		case MODE_HELL80:
			feldcolumnrate = 35;
			rxpixrate = (RxColumnLen * feldcolumnrate);
			txpixrate = (TxColumnLen * feldcolumnrate);
			downsampleinc = (double)(rxpixrate/samplerate);
			upsampleinc = (double)(txpixrate/samplerate);
			hell_bandwidth = 300;
			phi2freq = samplerate / M_PI / (hell_bandwidth / 2.0);
			filter_bandwidth = 5 * round(4.0 * hell_bandwidth / 5.0);
			progdefaults.HELL_BW_HELL80 = filter_bandwidth;
			cap |= CAP_REV;
			break;
		default :
			feldcolumnrate = 17.5;
			break;
	}
	set_bandwidth(hell_bandwidth);
	lpfilt->create_filter(0, 1.0 * filter_bandwidth / samplerate);//0.5 * filter_bandwidth / samplerate);
	average->setLength( 500 * samplerate / rxpixrate);

	rx_init();

	wf->redraw_marker();

	REQ(set_HellBW, filter_bandwidth);

/*
std::cout <<
"HellRcvHeight:      " << progdefaults.HellRcvHeight << "\n" <<
"rx column length:   " << RxColumnLen << "\n" <<
"tx column length:   " << TxColumnLen << "\n" <<
"feldcolumrate:      " << feldcolumnrate << "\n" <<
"rxpixrate:          " << rxpixrate << "\n" <<
"txpixrate:          " << txpixrate << "\n" <<
"downsampleinc:      " << downsampleinc << "\n" <<
"upsampleinc:        " << upsampleinc << "\n" <<
"hell_bandwidth:     " << hell_bandwidth << "\n" <<
"filter bandwidth:   " << filter_bandwidth << "\n" <<
"baud:               " << 0.5 * txpixrate << "\n";
*/

}

feld::~feld()
{
	if (hilbert) delete hilbert;
	if (lpfilt)  delete lpfilt;
	if (average) delete average;
}

feld::feld(trx_mode m)
{
	mode = m;
	samplerate = FeldSampleRate;

	cap |= CAP_BW;

	lpfilt  = new fftfilt (0, 0.5, 1024);
	average = new Cmovavg (200);
	bbfilt = new Cmovavg(8);
	hilbert = new C_FIR_filter();
	hilbert->init_hilbert (37, 1);

	rxphacc = 0.0;
	txphacc = 0.0;

	wf->redraw_marker();

	REQ(&Raster::set_marquee, FHdisp, progdefaults.HellMarquee);

	col_data.alloc(2 * MAX_RX_COLUMN_LEN);

	restart();

}

// rx section

cmplx feld::mixer(cmplx in)
{

	cmplx z;

	z = cmplx( cos(rxphacc), sin(rxphacc) );

	z = z * in;

	rxphacc -= 2.0 * M_PI * frequency / samplerate;

	if (rxphacc < 0) rxphacc += TWOPI;

	return z;
}

void feld::FSKH_rx(cmplx z)
{
	guard_lock raster_lock(&feld_mutex);

	double f;
	double vid;
	double avg;

	f = arg(conj(prev) * z) * phi2freq;
	prev = z;
	f = bbfilt->run(f);

	avg = average->run(abs(z));

	rxcounter += downsampleinc;
	if (rxcounter < 1.0)
		return;
	rxcounter -= 1.0;

	if (avg > agc)
		agc = avg;
	else
		switch (progdefaults.hellagc) {
			case 3:
				agc *= (1.0 - 0.2 / RxColumnLen);
				break;
			case 2:
				agc *= (1.0 - 0.075 / RxColumnLen);
				break;
			case 1:
			default:
				agc *= (1.0 - 0.01 / RxColumnLen);
		}

	metric = CLAMP(1000*agc, 0.0, 100.0);
	display_metric(metric);

	vid = CLAMP(0.5*(f + 1), 0.0, 1.0);

	if (reverse)
		vid = 1.0 - vid;
	if (progdefaults.HellBlackboard)
		vid = 1.0 - vid;

	col_data[col_pointer + RxColumnLen] = (int)(vid * 255.0);
	col_pointer++;
	if (col_pointer == RxColumnLen) {
		if (metric > progStatus.sldrSquelchValue || progStatus.sqlonoff == false) {
			switch (progdefaults.HellRcvWidth) {
				case 4:
					REQ(put_rx_data, col_data, 2 * RxColumnLen);
				case 3:
					REQ(put_rx_data, col_data, 2 * RxColumnLen);
				case 2:
					REQ(put_rx_data, col_data, 2 * RxColumnLen);
				case 1:
				default:
					REQ(put_rx_data, col_data, 2 * RxColumnLen);
			}
		}
		col_pointer = 0;
		for (int i = 0; i < RxColumnLen; i++)
			col_data[i] = col_data[i + RxColumnLen];
	}
}

void feld::rx(cmplx z)
{
	guard_lock raster_lock(&feld_mutex);

	double x, avg;
	int ix;

	x = abs(z);
	if (x > peakval) peakval = x;
	avg = average->run(x);

	rxcounter += downsampleinc;
	if (rxcounter < 1.0)
		return;

	rxcounter -= 1.0;

	x = peakval;
	peakval = 0;
	if (x > peakhold)
		peakhold = x;
	else
		peakhold *= (1.0 - 0.02 / RxColumnLen);
	ix = CLAMP(255 * x / peakhold, 0, 255);

	if (avg > agc)
		agc = avg;
	else
		switch (progdefaults.hellagc) {
			case 3:
				agc *= (1.0 - 0.2 / RxColumnLen);
				break;
			case 2:
				agc *= (1.0 - 0.075 / RxColumnLen);
				break;
			case 1:
			default:
				agc *= (1.0 - 0.01 / RxColumnLen);
		}

	metric = CLAMP(1000*agc, 0.0, 100.0);
	display_metric(metric);

	if (!progdefaults.HellBlackboard)
		ix = 255 - ix;

	col_data[col_pointer + RxColumnLen] = ix;
	col_pointer++;
	if (col_pointer >= RxColumnLen) {
		if (metric > progStatus.sldrSquelchValue || progStatus.sqlonoff == false) {
			switch (progdefaults.HellRcvWidth) {
				case 4:
					REQ(put_rx_data, col_data, 2 * RxColumnLen);
				case 3:
					REQ(put_rx_data, col_data, 2 * RxColumnLen);
				case 2:
					REQ(put_rx_data, col_data, 2 * RxColumnLen);
				case 1:
				default:
					REQ(put_rx_data, col_data, 2 * RxColumnLen);
			}
		}
		col_pointer = 0;
		for (int i = 0; i < RxColumnLen; i++)
			col_data[i] = col_data[i + RxColumnLen];
	}

}

int feld::rx_process(const double *buf, int len)
{

	cmplx z, *zp;
	int i, n;

	if (progdefaults.HELL_BW != filter_bandwidth) {
		filter_bandwidth = progdefaults.HELL_BW;
		switch (mode) {
			case MODE_FELDHELL:
				progdefaults.HELL_BW_FH = filter_bandwidth;
				break;
			case MODE_SLOWHELL:
				progdefaults.HELL_BW_SH = filter_bandwidth;
				break;
			case MODE_HELLX5:
				progdefaults.HELL_BW_X5 = filter_bandwidth;
				break;
			case MODE_HELLX9:
				progdefaults.HELL_BW_X9 = filter_bandwidth;
				break;
			case MODE_FSKH245:
				progdefaults.HELL_BW_FSKH245 = filter_bandwidth;
				break;
			case MODE_FSKH105:
				progdefaults.HELL_BW_FSKH105 = filter_bandwidth;
				break;
			case MODE_HELL80:
				progdefaults.HELL_BW_HELL80 = filter_bandwidth;
		}

		lpfilt->create_filter(0, 0.5 * filter_bandwidth / samplerate);
		wf->redraw_marker();
	}

	while (len-- > 0) {
		/* create analytic signal... */
		z = cmplx( *buf, *buf );
		buf++;

/// hilbert transform need for complex frequency shift

		hilbert->run(z, z);
		z = mixer(z);

		n = lpfilt->run(z, &zp);

		switch (mode) {
			case MODE_FSKH245:
			case MODE_FSKH105:
			case MODE_HELL80:
				for (i = 0; i < n; i++) {
					FSKH_rx(zp[i]);
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
	switch (progdefaults.feldfontnbr) {
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

	if (txphacc > TWOPI) txphacc -= TWOPI;

	return x;
}

void feld::send_symbol(int currsymb, int nextsymb)
{
	int outlen = 0;

	double Amp = 1.0;
	double tone = get_txfreq_woffset();

	if (hardkeying != progdefaults.HellPulseFast) {
		hardkeying = progdefaults.HellPulseFast;
		initKeyWaveform();
	}

	if (mode == MODE_FSKH245 || mode == MODE_FSKH105 || mode == MODE_HELL80)
		tone += (reverse ? -1 : 1) * (currsymb ? -1 : 1) * bandwidth / 2.0;
	for (;;) {
		switch (mode) {
			case MODE_FSKH245 : case MODE_FSKH105 : case MODE_HELL80 :
				break;
			case MODE_HELLX5 : case MODE_HELLX9 :
				Amp = currsymb;
				break;
			case MODE_FELDHELL : case MODE_SLOWHELL :
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

		if (outlen >= OUTBUFSIZE) {
			LOG_DEBUG("feld reset");
			outlen = 0;
			break;
		}
		txcounter += upsampleinc;
		if (txcounter < 1.0)
			continue;
		txcounter -= 1.0;
		break;
	}
	prevsymb = currsymb;

// write to soundcard & display
	ModulateXmtr(outbuf, outlen);

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
			for (int col = 0; col < progdefaults.HellXmtWidth; col++) {
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
	modem::tx_process();

	int c;

	if (hardkeying != progdefaults.HellPulseFast) {
		hardkeying = progdefaults.HellPulseFast;
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
		return -1;
	}

	c = get_tx_char();

	if (c == GET_TX_CHAR_ETX || stopflag) {
		tx_state = POSTAMBLE;
		postamble = 3;
		return 0;
	}

// if TX buffer empty
// send idle character
	if (c == GET_TX_CHAR_NODATA) {
		if (progdefaults.HellXmtIdle == true)
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
	for (int i = 0; i < MAXLEN; i++) {
		OnShape[i] = 1.0;
		OffShape[i] = 0.0;
	}
	for (int i = 0; i < 32; i++) {
		switch (hardkeying) {
			case 0:
				OnShape[i] = 0.5*(1.0 - cos(M_PI * i / 33)); // raised cosine with 4 msec rise
				break;
			case 1:
				if (i < 16)
					OnShape[i] = 0.5*(1.0 - cos(M_PI * i / 16)); // raised cosine with 2 msec rise
				break;
			case 2:
				if (i < 8)
					OnShape[i] = 0.5*(1.0 - cos(M_PI * i / 8)); // raised cosine with 1 msec rise
			case 3:
			default:	// square shaped pulse
				break;
		}
		OffShape[31 - i] = OnShape[i];
	}
}
