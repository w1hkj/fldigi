// ----------------------------------------------------------------------------
// ifkp.cxx  --  ifkp modem
//
// Copyright (C) 2015
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

#include <config.h>

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <libgen.h>

#include <FL/filename.H>
#include "progress.h"
#include "ifkp.h"
#include "complex.h"
#include "fl_digi.h"
#include "ascii.h"
#include "misc.h"
#include "fileselect.h"
#include "threads.h"
#include "debug.h"

#include "configuration.h"
#include "qrunner.h"
#include "fl_digi.h"
#include "status.h"
#include "main.h"
#include "icons.h"

#include "confdialog.h"

using namespace std;

#include "ifkp_varicode.cxx"

#define IFKPDEBUG		1

#include "ifkp-pic.cxx"

// nibbles table used for fast conversion from tone difference to symbol

static int nibbles[331];
static void init_nibbles()
{
	int nibble = 0;
	for (int i = -165; i < 166; i++) {
		nibble = floor(0.5 + 1.0*i/IFKP_SPACING);
		// allow for wrap-around (33 tones for 32 tone differences)
		if (nibble < 0) nibble += 33;
		// adjust for +1 symbol at the transmitter
		nibble -= IFKP_OFFSET;
		nibbles[i + 165] = nibble;
	}
}

ifkp::ifkp(trx_mode md) : modem()
{
	frequency = 1500;
	modem::set_freq(frequency);

	baud = progdefaults.ifkpbaud;
	symlen = IFKP_SYMLEN / baud;

	cap |= CAP_IMG;

	mode = md;
	samplerate = IFKP_SR;
	fft = new g_fft<double>(IFKP_FFTSIZE);
	snfilt = new Cmovavg(200);
	movavg_size = progdefaults.ifkpbaud == 2 ? 3 : 4;
	for (int i = 0; i < IFKP_NUMBINS; i++) binfilt[i] = new Cmovavg(movavg_size);
	txphase = 0;
	basetone = 197;

	rxfilter = new C_FIR_filter();
	rxfilter->init_bandpass(129, 1, 945.0/IFKP_SR, 2055.0/IFKP_SR);

	picfilter = new C_FIR_filter();
	picfilter->init_lowpass(257, 1, 500.0 / IFKP_SR);
	phase = 0;
	phidiff = 2.0 * M_PI * frequency / samplerate;

	bkptr = 0;
	peak_counter = 0;
	peak = last_peak = 0;
	max = 0;
	curr_nibble = prev_nibble = 0;
	s2n = 0;
	ch_sqlch_open = false;
	memset(rx_stream, 0, sizeof(rx_stream));
	rx_text.clear();

	for (int i = 0; i < IFKP_BLOCK_SIZE; i++)
		a_blackman[i] = blackman(1.0 * i / IFKP_BLOCK_SIZE);

	keyshape = new double[IFKP_SHAPE_SIZE];
	for (int i = 0; i < IFKP_SHAPE_SIZE; i++)
		keyshape[i] = blackman(0.5 * i / IFKP_SHAPE_SIZE);

	state = TEXT;
//	TX_IMAGE = false;

	init_nibbles();

	restart();
}

ifkp::~ifkp()
{
	delete fft;
	delete snfilt;
	delete rxfilter;
	delete picfilter;
	for (int i = 0; i < IFKP_NUMBINS; i++)
		delete binfilt[i];
	ifkp_deleteTxViewer();
	ifkp_deleteRxViewer();
}

void  ifkp::tx_init(SoundBase *sc)
{
	scard = sc;
	tone = prevtone = 0;
	txphase = 0;
	send_bot = true;
	mycall = progdefaults.myCall;
	for (size_t n = 0; n < mycall.length(); n++) mycall[n] = tolower(mycall[n]);
}

void  ifkp::rx_init()
{
	bandwidth = 33 * IFKP_SPACING * samplerate / IFKP_SYMLEN;
	bkptr = 0;
	peak_counter = 0;
	peak = last_peak = 0;
	max = 0;
	curr_nibble = prev_nibble = 0;
	s2n = 0;
	ch_sqlch_open = false;
	memset(rx_stream, 0, sizeof(rx_stream));

	prevz = cmplx(0,0);
	image_counter = 0;
	RXspp = 10; // 10 samples per pixel
	state = TEXT;

	rx_text.clear();
	for (int i = 0; i < IFKP_NUMBINS; i++) {
		tones[i] = 0.0;
		binfilt[i]->reset();
	}
	pixel = 0;
	pic_str = "     ";
}

void ifkp::init()
{
	modem::init();
	frequency = 1500;
	modem::set_freq(frequency);

	baud = progdefaults.ifkpbaud;
	symlen = IFKP_SYMLEN / baud;

	bandwidth = 33 * IFKP_SPACING * samplerate / IFKP_SYMLEN;
	basetone = ceil(1.0*(frequency - bandwidth / 2) * IFKP_SYMLEN / samplerate);

	peak_hits = 4;

	mycall = progdefaults.myCall;
	for (size_t n = 0; n < mycall.length(); n++) mycall[n] = tolower(mycall[n]);

	movavg_size = 3;

	for (int i = 0; i < IFKP_NUMBINS; i++) binfilt[i]->setLength(movavg_size);

	rx_init();
}

void ifkp::set_freq(double f)
{
	frequency = 1500;
	basetone = ceil(1.0*(frequency - bandwidth / 2) * IFKP_SYMLEN / samplerate);
	modem::set_freq(frequency);
}

void ifkp::show_mode()
{
	if (progdefaults.ifkpbaud == 0.5)
		put_MODEstatus("IFKP 0.5");
	else if (progdefaults.ifkpbaud == 1.0)
		put_MODEstatus("IFKP 1.0");
	else if (progdefaults.ifkpbaud == 1.5)
		put_MODEstatus("IFKP 1.5");
	else
		put_MODEstatus("IFKP 2.0");
}

void ifkp::restart()
{
	frequency = 1500;
	modem::set_freq(frequency);

	baud = progdefaults.ifkpbaud;
	symlen = IFKP_SYMLEN / baud;
	bandwidth = 33 * IFKP_SPACING * samplerate / IFKP_SYMLEN;
	basetone = ceil(1.0*(frequency - bandwidth / 2) * IFKP_SYMLEN / samplerate);

	peak_hits = 4;

	mycall = progdefaults.myCall;
	for (size_t n = 0; n < mycall.length(); n++) mycall[n] = tolower(mycall[n]);

	movavg_size = progdefaults.ifkpbaud == 2 ? 3 : 4;

	for (int i = 0; i < IFKP_NUMBINS; i++) binfilt[i]->setLength(movavg_size);

	show_mode();

//	std::ostringstream it;
//	it << "\nBandwidth...... " << bandwidth;
//	it << "\nSymbol length.. " << symlen    << "\nBlock size..... " << IFKP_SHIFT_SIZE;
//	it << "\nMinimum Hits... " << peak_hits << "\nBasetone....... " << basetone << "\n";
//	LOG_INFO("%s", it.str().c_str());

}

// valid printable character

bool ifkp::valid_char(int ch)
{
	if ( ! (ch ==  10 || ch == 163 || ch == 176 ||
		ch == 177 || ch == 215 || ch == 247 ||
		(ch > 31 && ch < 128)))
		return false;
	return true;
}

//=====================================================================
// receive processing
//=====================================================================
void ifkp::parse_pic(int ch)
{
	pic_str.erase(0,1);
	pic_str += ch;
	if (pic_str.find("pic%") == 0) {
		switch (pic_str[4]) {
			case 'S':	picW = 160; picH = 120; break;
			case 'L':	picW = 320; picH = 240; break;
			case 'F':	picW = 640; picH = 480; break;
			case 'V':	picW = 640; picH = 480; break;
			case 'P':	picW = 240; picH = 300; break;
			case 'p':	picW = 240; picH = 300; break;
			case 'M':	picW = 120; picH = 150; break;
			case 'm':	picW = 120; picH = 150; break;
			default: return;
		}
	} else
		return;
	REQ( ifkp_showRxViewer, pic_str[4]);
	image_counter = 0;
	col = row = rgb = 0; 
	state = IMAGE;
}

void ifkp::process_symbol(int sym)
{
	int nibble = 0;
	int curr_ch = -1;

	symbol = sym;

	nibble = symbol - prev_symbol;
	if (nibble < -165) nibble = -165;
	if (nibble > 165) nibble = 165;
	nibble = nibbles[nibble + 165];

	if (nibble >= 0) { // process nibble
		curr_nibble = nibble;

// single-nibble characters
		if ((prev_nibble < 29) & (curr_nibble < 29)) {
			curr_ch = ifkp_varidecode[prev_nibble];

// double-nibble characters
		} else if ( (prev_nibble < 29) &&
					 (curr_nibble > 28) &&
					 (curr_nibble < 32)) {
			curr_ch = ifkp_varidecode[prev_nibble * 32 + curr_nibble];
		}
		if (curr_ch > 0) {
			if (ch_sqlch_open || metric >= progStatus.sldrSquelchValue) {
				put_rx_char(curr_ch, FTextBase::RECV);
				parse_pic(curr_ch);
			}
		}
		prev_nibble = curr_nibble;
	}

	prev_symbol = symbol;
}

void ifkp::process_tones()
{
	noise = 0;
	max = 0;
	peak = 0;
	int firstbin = basetone - 21;
// time domain moving average filter for each tone bin
	for (int i = 0; i < IFKP_NUMBINS; ++i) {
		val = norm(fft_data[i + firstbin]);
		tones[i] = binfilt[i]->run(val);
		if (tones[i] > max) {
			max = tones[i];
			peak = i;
		}
	}

	noise += (tones[0] + tones[IFKP_NUMBINS - 1]) / 2.0;
	noise *= IFKP_FFTSIZE / 2.0;

	if (noise < 1e-8) noise = 1e-8;

	s2n = 10 * log10(snfilt->run(tones[peak]/noise)) + 3.0;

	snprintf(szestimate, sizeof(szestimate), "%.0f db", s2n );

	metric = 2 * (s2n + 20);
	metric = CLAMP(metric, 0, 100.0);  // -20 to +30 db range
	display_metric(metric);

	if (metric < progStatus.sldrSquelchValue && ch_sqlch_open)
		ch_sqlch_open = false;

	if (peak == prev_peak) {
		peak_counter++;
	} else {
		peak_counter = 0;
	}

	if ((peak_counter >= peak_hits) && 
		(peak != last_peak) &&
		(metric >= progStatus.sldrSquelchValue ||
		 progStatus.sqlonoff == false)) {
		process_symbol(peak);
		peak_counter = 0;
		last_peak = peak;
	}

	prev_peak = peak;
}

void ifkp::recvpic(double smpl)
{
	phase -= phidiff;
	if (phase < 0) phase += 2.0 * M_PI;

	cmplx z = smpl * cmplx( cos(phase), sin(phase ) );
	picfilter->run( z, currz);
	pixel += arg(conj(prevz) * currz);
	prevz = currz;

	image_counter++;

	if ((image_counter % RXspp) == 0) {
		pixel /= RXspp;
		pixel *= (samplerate / TWOPI);
		byte = pixel / 3.125 + 128;
		byte = (int)CLAMP( byte, 0.0, 255.0);

		if (image_mode == 2 || image_mode == 5 || image_mode == 7) {
			pixelnbr = 3 * (col + row * picW);
			REQ(ifkp_updateRxPic, byte, pixelnbr);
			REQ(ifkp_updateRxPic, byte, pixelnbr + 1);
			REQ(ifkp_updateRxPic, byte, pixelnbr + 2);
			if (++ col == picW) {
				col = 0;
				row++;
				if (row >= picH) {
					state = TEXT;
					REQ(ifkp_enableshift);
				}
			}
		} else {
			pixelnbr = rgb + 3 * (col + row * picW);
			REQ(ifkp_updateRxPic, byte, pixelnbr);
			if (++col == picW) {
				col = 0;
				if (++rgb == 3) {
					rgb = 0;
					++row;
				}
			}
			if (row > picH) {
				state = TEXT;
				REQ(ifkp_enableshift);
			}
		}
		pixel = 0;

		s2n = 10 * log10(snfilt->run(12000*amplitude/noise)) + 3.0;

		snprintf(szestimate, sizeof(szestimate), "%.0f db", s2n );

		metric = 2 * (s2n + 20);
		metric = CLAMP(metric, 0, 100.0);  // -20 to +30 db range
		display_metric(metric);

	}
}

int ifkp::rx_process(const double *buf, int len)
{
//	if (peak_hits != progdefaults.ifkphits) restart();
//	if (movavg_size != progdefaults.ifkp_movavg) restart();
	if (baud != progdefaults.ifkpbaud) restart();

	double val;
	cmplx zin, z;

	if (bkptr < 0) bkptr = 0;
	if (bkptr >= IFKP_SHIFT_SIZE) bkptr = 0;

	if (progStatus.ifkp_rx_abort) {
		state = TEXT;
		progStatus.ifkp_rx_abort = false;
		REQ(ifkp_clear_rximage);
	}

	while (len) {
		if (state == IMAGE) {
			recvpic(*buf);
			len--;
			buf++;
		} else {
			rxfilter->Irun(*buf, val);
			rx_stream[IFKP_BLOCK_SIZE + bkptr] = val;
			len--;
			buf++;
			bkptr++;

			if (bkptr == IFKP_SHIFT_SIZE) {
				bkptr = 0;
				memcpy(	rx_stream,							// to
						&rx_stream[IFKP_SHIFT_SIZE],				// from
						IFKP_BLOCK_SIZE*sizeof(*rx_stream));	// # bytes
				memset(fft_data, 0, sizeof(fft_data));
				for (int i = 0; i < IFKP_BLOCK_SIZE; i++)
					fft_data[i].real() = fft_data[i].imag() =
						rx_stream[i] * a_blackman[i];
				fft->ComplexFFT(fft_data);
				process_tones();
			}
		}
	}
	return 0;
}

//=====================================================================
// transmit processing
//=====================================================================

void ifkp::transmit(double *buf, int len)
{
//	if (xmtfilt && progdefaults.ifkp_xmtfilter)
//		for (int i = 0; i < len; i++) xmtfilt->Irun(buf[i], buf[i]);
	ModulateXmtr(buf, len);
}

void ifkp::send_tone(int tone)
{
	double phaseincr;
	double frequency;
	double freq_error = ctrl_freq_offset->value();

	frequency = (basetone + tone * IFKP_SPACING) * samplerate / IFKP_SYMLEN;
	if (grpNoise->visible() && btnOffsetOn->value()==true)
		frequency += freq_error;
	phaseincr = 2.0 * M_PI * frequency / samplerate;
	prevtone = tone;

	int send_symlen = symlen;

	for (int i = 0; i < send_symlen; i++) {
		outbuf[i] = cos(txphase);
		if (i < IFKP_SHAPE_SIZE) outbuf[i] *= keyshape[i];
		if ((send_symlen - i) < IFKP_SHAPE_SIZE) outbuf[i] *= keyshape[send_symlen - i - 1];
		txphase -= phaseincr;
		if (txphase < 0) txphase += TWOPI;
	}
	transmit(outbuf, send_symlen);
}

void ifkp::send_symbol(int sym)
{
	tone = (prevtone + sym + IFKP_OFFSET) % 33;
	send_tone(tone);
}

void ifkp::send_idle()
{
	send_symbol(0);
}

void ifkp::send_char(int ch)
{
	if (ch <= 0) return send_idle();

	int sym1 = ifkp_varicode[ch][0];
	int sym2 = ifkp_varicode[ch][1];

	send_symbol(sym1);
	if (sym2 > 28)
		send_symbol(sym2);
	put_echo_char(ch);
}

void ifkp::send_image()
{
	int W = 640, H = 480;  // grey scale transfer (FAX)
	bool color = true;
	float freq, phaseincr;
	float radians = 2.0 * M_PI / samplerate;

	if (!ifkppicTxWin || !ifkppicTxWin->visible()) {
		return;
	}

	switch (selifkppicSize->value()) {
		case 0 : W = 160; H = 120; break;
		case 1 : W = 320; H = 240; break;
		case 2 : W = 640; H = 480; color = false; break;
		case 3 : W = 640; H = 480; break;
		case 4 : W = 240; H = 300; break;
		case 5 : W = 240; H = 300; color = false; break;
		case 6 : W = 120; H = 150; break;
		case 7 : W = 120; H = 150; color = false; break;
	}

	REQ(ifkp_clear_tximage);

	freq = frequency - 400;
	#define PHASE_CORR  400
	phaseincr = radians * freq;
	for (int n = 0; n < PHASE_CORR; n++) {
		outbuf[n] = cos(txphase);
		txphase -= phaseincr;
		if (txphase < 0) txphase += TWOPI;
	}
	transmit(outbuf, 10);

	if (color == false) {  // grey scale image
		for (int row = 0; row < H; row++) {
			memset(outbuf, 0, 10 * sizeof(*outbuf));
			for (int col = 0; col < W; col++) {
				if (stopflag) return;
				tx_pixelnbr = col + row * W;
				tx_pixel =	0.3 * ifkppic_TxGetPixel(tx_pixelnbr, 0) +   // red
							0.6 * ifkppic_TxGetPixel(tx_pixelnbr, 1) +   // green
							0.1 * ifkppic_TxGetPixel(tx_pixelnbr, 2);    // blue
				REQ(ifkp_updateTxPic, tx_pixel, tx_pixelnbr*3 + 0);
				REQ(ifkp_updateTxPic, tx_pixel, tx_pixelnbr*3 + 1);
				REQ(ifkp_updateTxPic, tx_pixel, tx_pixelnbr*3 + 2);
				freq = frequency - 400 + tx_pixel * 3.125; // 800/256
				phaseincr = radians * freq;
				for (int n = 0; n < 10; n++) {
					outbuf[n] = cos(txphase);
					txphase -= phaseincr;
					if (txphase < 0) txphase += TWOPI;
				}
				transmit(outbuf, 10);
				Fl::awake();
			}
		}
	} else {
		for (int row = 0; row < H; row++) {
			for (int color = 0; color < 3; color++) {
				memset(outbuf, 0, 10 * sizeof(*outbuf));
				for (int col = 0; col < W; col++) {
					if (stopflag) return;
					tx_pixelnbr = col + row * W;
					tx_pixel = ifkppic_TxGetPixel(tx_pixelnbr, color);
					REQ(ifkp_updateTxPic, tx_pixel, tx_pixelnbr*3 + color);
					freq = frequency - 400 + tx_pixel * 3.125;
					phaseincr = radians * freq;
					for (int n = 0; n < 10; n++) {
						outbuf[n] = cos(txphase);
						txphase -= phaseincr;
						if (txphase < 0) txphase += TWOPI;
					}
					transmit(outbuf, 10);
				}
				Fl::awake();
			}
		}
	}
}

void ifkp::ifkp_send_image() {
	TX_IMAGE = true;
	start_tx();
}

int ifkp::tx_process()
{
	if (baud != progdefaults.ifkpbaud) restart();

	if (send_bot) {
		send_bot = false;
		send_char(0);
		send_char(0);
	}
	int c = get_tx_char();
	if (c == GET_TX_CHAR_ETX) {
		if (TX_IMAGE) send_image();
		send_char(0);
		stopflag = false;
		TX_IMAGE = false;
		return -1;
	}
	if ( stopflag ) { // aborts transmission
		TX_IMAGE = false;
		stopflag = false;
		return -1;
	}
	send_char(c);
	return 0;
}

//#include "bitmaps.cxx"
