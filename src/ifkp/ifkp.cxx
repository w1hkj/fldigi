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

#define IFKP_SR 16000

#include "ifkp-pic.cxx"

static fre_t call("([[:alnum:]]?[[:alpha:]/]+[[:digit:]]+[[:alnum:]/]+)", REG_EXTENDED);
static string teststr = "";
static string allowed = " 0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ/";
static char sz[21];

int ifkp::IMAGEspp = IMAGESPP;

static string valid_callsign(char ch)
{
	if (allowed.find(ch) == std::string::npos) ch = ' ';
	teststr += tolower(ch);
	if (teststr.length() > 20) teststr.erase(0,1);
	// wait for ' de '
	size_t p1;
	if ((p1 = teststr.find(" de ")) != std::string::npos) { // test for callsign
		p1 += 4;
		if (p1 >= teststr.length()) return "";
		while (p1 < teststr.length() && teststr[p1] == ' ') p1++;
		if (p1 == teststr.length()) return "";
		size_t p2 = teststr.rfind(' ');
		if ((p2 > p1) && (p2 - p1 < 21)) { // found a word, test for callsign
			memset(sz, 0, 21);
			strcpy(sz, teststr.substr(p1, p2-p1).c_str());
			teststr.erase(0, p2);
			if (call.match(sz)) {
				return sz;
			}
			return "";
		}
	}
	return "";
}

// nibbles table used for fast conversion from tone difference to symbol

void ifkp::init_nibbles()
{
	int nibble = 0;
	for (int i = 0; i < 199; i++) {
		nibble = floor(0.5 + (i - 99.0)/IFKP_SPACING);
		// allow for wrap-around (33 tones for 32 tone differences)
		if (nibble < 0) nibble += 33;
		if (nibble > 32) nibble -= 33;
		// adjust for +1 symbol at the transmitter
		nibble--;
		nibbles[i] = nibble;
	}
}

ifkp::ifkp(trx_mode md) : modem()
{
	samplerate = IFKP_SR;
	symlen = IFKP_SYMLEN;

	cap |= CAP_IMG;

	if (progdefaults.StartAtSweetSpot) {
		frequency = progdefaults.PSKsweetspot;
	} else
		frequency = wf->Carrier();
	REQ(put_freq, frequency);

	mode = md;
	fft = new g_fft<double>(IFKP_FFTSIZE);
	snfilt = new Cmovavg(200);
	movavg_size = 4;
	for (int i = 0; i < IFKP_NUMBINS; i++) binfilt[i] = new Cmovavg(movavg_size);
	txphase = 0;
	basetone = 197;

	rxfilter = new C_FIR_filter();
	float lo = frequency - 0.75 * bandwidth;
	float hi = frequency + 0.75 * bandwidth;
	rxfilter->init_bandpass(129, 1, lo/samplerate, hi/samplerate);

	picfilter = new C_FIR_filter();
	picfilter->init_lowpass(257, 1, 1.0 * bandwidth / samplerate);

	phase = 0;
	phidiff = 2.0 * M_PI * frequency / samplerate;

	IMAGEspp = IMAGESPP;
	pixfilter = new Cmovavg(IMAGEspp);
	ampfilter = new Cmovavg(IMAGEspp);
	syncfilter = new Cmovavg(3*IMAGEspp);

	bkptr = 0;
	peak_counter = 0;
	peak = last_peak = 0;
	max = 0;
	curr_nibble = prev_nibble = 0;
	s2n = 0;

	memset(rx_stream, 0, sizeof(rx_stream));
	rx_text.clear();

	for (int i = 0; i < IFKP_BLOCK_SIZE; i++)
		a_blackman[i] = blackman(1.0 * i / IFKP_BLOCK_SIZE);

	state = TEXT;

	init_nibbles();

	TX_IMAGE = TX_AVATAR = false;

	heard_log_fname = progdefaults.ifkp_heard_log;
	std::string sheard = TempDir;
	sheard.append(heard_log_fname);
	heard_log.open(sheard.c_str(), ios::app);

	audit_log_fname = progdefaults.ifkp_audit_log;
	std::string saudit = TempDir;
	saudit.append(audit_log_fname);
	audit_log.open(saudit.c_str(), ios::app);

	audit_log << "\n==================================================\n";
	audit_log << "Audit log: " << zdate() << ", " << ztime() << "\n";
	audit_log << "==================================================\n";
	audit_log.flush();

	restart();

	activate_ifkp_image_item(true);

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
	heard_log.close();
	audit_log.close();

	activate_ifkp_image_item(false);

}

void  ifkp::tx_init(SoundBase *sc)
{
	scard = sc;
	tone = prevtone = 0;
	txphase = 0;
	send_bot = true;
	mycall = progdefaults.myCall;
	if (progdefaults.ifkp_lowercase)
		for (size_t n = 0; n < mycall.length(); n++) mycall[n] = tolower(mycall[n]);
	videoText();
}

void  ifkp::rx_init()
{
	bkptr = 0;
	peak_counter = 0;
	peak = last_peak = 0;
	max = 0;
	curr_nibble = prev_nibble = 0;
	s2n = 0;

	memset(rx_stream, 0, sizeof(rx_stream));

	prevz = cmplx(0,0);
	image_counter = 0;
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
	peak_hits = 4;

	mycall = progdefaults.myCall;
	if (progdefaults.ifkp_lowercase)
		for (size_t n = 0; n < mycall.length(); n++) mycall[n] = tolower(mycall[n]);

	movavg_size = 3;

	for (int i = 0; i < IFKP_NUMBINS; i++) binfilt[i]->setLength(movavg_size);

	rx_init();
}

void ifkp::set_freq(double f)
{
	if (progdefaults.ifkp_freqlock)
		frequency = 1500;
	else
		frequency = f;

	if (frequency < 100 + 0.5 * bandwidth) frequency = 100 + 0.5 * bandwidth;
	if (frequency > 3900 - 0.5 * bandwidth) frequency = 3900 - 0.5 * bandwidth;

	tx_frequency = frequency;
	
	REQ(put_freq, frequency);
	
	set_bandwidth(33 * IFKP_SPACING * samplerate / symlen);
	basetone = ceil((frequency - bandwidth / 2.0) * symlen / samplerate);

	float lo = frequency - 0.75 * bandwidth;
	float hi = frequency + 0.75 * bandwidth;
	rxfilter->init_bandpass(129, 1, lo/samplerate, hi/samplerate);
	picfilter->init_lowpass(257, 1, 1.0 * bandwidth / samplerate);
	phase = 0;
	phidiff = 2.0 * M_PI * frequency / samplerate;

	std::ostringstream it;
	it << "\nSamplerate..... " << samplerate;
	it << "\nBandwidth...... " << bandwidth;
	it << "\nlower cutoff... " << lo;
	it << "\nupper cutoff... " << hi;
	it << "\ncenter ........ " << frequency;
	it << "\nSymbol length.. " << symlen    << "\nBlock size..... " << IFKP_SHIFT_SIZE;
	it << "\nMinimum Hits... " << peak_hits << "\nBasetone....... " << basetone << "\n";
	LOG_VERBOSE("%s", it.str().c_str());
}

void ifkp::show_mode()
{
	if (progdefaults.ifkp_baud == 0)
		put_MODEstatus("IFKP 0.5");
	else if (progdefaults.ifkp_baud == 1)
		put_MODEstatus("IFKP 1.0");
	else
		put_MODEstatus("IFKP 2.0");
	return;
}

void ifkp::restart()
{
	set_freq(wf->Carrier());

	peak_hits = 4;

	mycall = progdefaults.myCall;
	if (progdefaults.ifkp_lowercase)
		for (size_t n = 0; n < mycall.length(); n++) mycall[n] = tolower(mycall[n]);

	movavg_size = progdefaults.ifkp_baud == 2 ? 3 : 4;

	for (int i = 0; i < IFKP_NUMBINS; i++) binfilt[i]->setLength(movavg_size);

	show_mode();

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
	b_ava = false;
	image_mode = 0;
	if (pic_str.find("pic%") == 0) {
		switch (pic_str[4]) {
			case 'A':	picW = 59; picH = 74; b_ava = true; break;
			case 'T':	picW = 59; picH = 74; break;
			case 'S':	picW = 160; picH = 120; break;
			case 'L':	picW = 320; picH = 240; break;
			case 'F':	picW = 640; picH = 480; break;
			case 'V':	picW = 640; picH = 480; break;
			case 'P':	picW = 240; picH = 300; break;
			case 'p':	picW = 240; picH = 300; image_mode = 1; break;
			case 'M':	picW = 120; picH = 150; break;
			case 'm':	picW = 120; picH = 150; image_mode = 1; break;
			default: return;
		}
	} else
		return;
	if (!b_ava)
		REQ( ifkp_showRxViewer, pic_str[4]);
	else
		REQ( ifkp_clear_avatar );
	image_counter = -symlen / 2;
	col = row = rgb = 0;
	syncfilter->reset();
	pixfilter->reset();
	ampfilter->reset();
	state = IMAGE_START;
}

void ifkp::process_symbol(int sym)
{
	int nibble = 0;
	int curr_ch = -1	;

	symbol = sym;

	nibble = symbol - prev_symbol;
	if (nibble < -99 || nibble > 99) {
		prev_symbol = symbol;
		return;
	}
	nibble = nibbles[nibble + 99];

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
//			if (ch_sqlch_open || metric >= progStatus.sldrSquelchValue) {
			if (metric >= progStatus.sldrSquelchValue) {
				put_rx_char(curr_ch, FTextBase::RECV);
				if (progdefaults.ifkp_enable_audit_log) {
					audit_log << ifkp_ascii[curr_ch];
					if (curr_ch == '\n') audit_log << '\n';
				}
				parse_pic(curr_ch);
				station_calling = valid_callsign(curr_ch);
				if (!station_calling.empty()) {
					snprintf(szestimate, sizeof(szestimate), "%.0f db", s2n );
					REQ(add_to_heard_list, station_calling.c_str(), szestimate);
					if (progdefaults.ifkp_enable_heard_log) {
						std::string sheard = zdate();
						sheard.append(":").append(ztime());
						sheard.append(",").append(station_calling);
						sheard.append(",").append(szestimate).append("\n");
						heard_log << sheard;
						heard_log.flush();
					}
				}
			}
		}
		prev_nibble = curr_nibble;
	}

	prev_symbol = symbol;
}

static double sig = 0;

void ifkp::process_tones()
{
	max = 0;
	peak = 0;

	max = 0;
	peak = IFKP_NUMBINS / 2;

	int firstbin = frequency * IFKP_SYMLEN / samplerate - IFKP_NUMBINS / 2;

	double sigval = 0;

	double mins[4];
	double min = 1e8;
	double temp;
	int k = 0;
	for (int i = 0; i < IFKP_NUMBINS; ++i) {
		val = norm(fft_data[i + firstbin]);

		tones[i] = binfilt[i]->run(val);
		if (tones[i] > max) {
			max = tones[i];
			peak = i;
		}
// looking for minimum signal in a 3 bin sequence
		mins[k++] = val;
		if (k == 3) {
			temp = mins[0] + mins[1] + mins[2];
			if (temp < min) min = temp;
			k = 0;
		}
	}

	sigval = tones[peak-1] + tones[peak] + tones[peak+1];
	sigval /= 3;
	min /= 3;

	if (min <= 0.001 || sigval <= 0.001) {
		sig = 1;
		noise = 1;
	} else {
		sig = .95 * sig + .05 * sigval;
		noise = .99 * noise + .01 * min;
	}

	s2n = 10 * snfilt->run(log10(sig / noise)) - 36;

	if (s2n > 0) s2n *= 1.3;  // very empirical

//if (s2n > -30) {
//FILE *ifkptxt = fopen("ifkp.txt", "a");
//fprintf(ifkptxt, "%d,%f,%f,%f,%f,%f\n", peak, sigval, sig, min, noise, s2n);
//fclose(ifkptxt);
//}

//scale to -25 to +45 db range
// -25 -> 0 linear
// 0 - > 45 compressed by 2

	if (s2n < -25) s2n = -25;
	if (s2n > 45) s2n = 45;

	if (s2n <= 0) metric = 2 * (25 + s2n);
	if (s2n > 0) metric = 50 *( 1 + s2n / 45);

	display_metric(metric);

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
	pixel = (samplerate / TWOPI) * pixfilter->run(arg(conj(prevz) * currz));
	sync = (samplerate / TWOPI) * syncfilter->run(arg(conj(prevz) * currz));
	prevz = currz;
	amplitude = ampfilter->run(norm(currz));

	image_counter++;
	if (image_counter < 0) return;

	if (state == IMAGE_START) {
		if (sync < -0.59 * bandwidth) {
			state = IMAGE_SYNC;
		}
		return;
	}
	if (state == IMAGE_SYNC) {
		if (sync > -0.51 * bandwidth) {
			state = IMAGE;
		}
		return;
	}

	if ((image_counter % IMAGEspp) == 0) {

		byte = pixel * 256.0 / bandwidth + 128;
		byte = (int)CLAMP( byte, 0.0, 255.0);

		if (image_mode == 1) { // bw transmission
			pixelnbr = 3 * (col + row * picW);
			if (b_ava) {
				REQ(ifkp_update_avatar, byte, pixelnbr);
				REQ(ifkp_update_avatar, byte, pixelnbr + 1);
				REQ(ifkp_update_avatar, byte, pixelnbr + 2);
			} else {
				REQ(ifkp_updateRxPic, byte, pixelnbr);
				REQ(ifkp_updateRxPic, byte, pixelnbr + 1);
				REQ(ifkp_updateRxPic, byte, pixelnbr + 2);
			}
			if (++ col == picW) {
				col = 0;
				row++;
				if (row >= picH) {
					state = TEXT;
					REQ(ifkp_enableshift);
				}
			}
		} else { // color transmission
			pixelnbr = rgb + 3 * (col + row * picW);
			if (b_ava)
				REQ(ifkp_update_avatar, byte, pixelnbr);
			else
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

		amplitude *= (samplerate/2)*(.734); // sqrt(3000 / (11025/2))
		s2n = 10 * log10(snfilt->run( amplitude * amplitude / noise));

		metric = 2 * (s2n + 20);
		metric = CLAMP(metric, 0, 100.0);  // -20 to +30 db range
		display_metric(metric);
		amplitude = 0;

	}
}

int ifkp::rx_process(const double *buf, int len)
{
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
		if (state != TEXT) {
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
				memcpy(	rx_stream,								// to
						&rx_stream[IFKP_SHIFT_SIZE],			// from
						IFKP_BLOCK_SIZE*sizeof(*rx_stream));	// # bytes
				memset(fft_data, 0, sizeof(fft_data));
				for (int i = 0; i < IFKP_BLOCK_SIZE; i++) {
					double d = rx_stream[i] * a_blackman[i];
					fft_data[i] = cmplx(d,d);
				}
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

	frequency = (basetone + tone * IFKP_SPACING) * samplerate / symlen;
	if (grpNoise->visible() && btnOffsetOn->value()==true)
		frequency += freq_error;
	phaseincr = 2.0 * M_PI * frequency / samplerate;
	prevtone = tone;

	int send_symlen = symlen * (
			progdefaults.ifkp_baud == 2 ? 0.5 :
			progdefaults.ifkp_baud == 0 ? 2.0 : 1.0);

	for (int i = 0; i < send_symlen; i++) {
		outbuf[i] = cos(txphase);
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

void ifkp::send_avatar()
{
	int W = 59, H = 74;  // grey scale transfer (FAX)
	float freq, phaseincr;
	float radians = 2.0 * M_PI / samplerate;

	freq = frequency - 0.6 * bandwidth;
	#define PHASE_CORR  (3 * symlen / 2)
	phaseincr = radians * freq;
	for (int n = 0; n < PHASE_CORR; n++) {
		outbuf[n] = cos(txphase);
		txphase -= phaseincr;
		if (txphase < 0) txphase += TWOPI;
	}
	transmit(outbuf, PHASE_CORR);

	for (int row = 0; row < H; row++) {
		for (int color = 0; color < 3; color++) {
			memset(outbuf, 0, IMAGEspp * sizeof(*outbuf));
			for (int col = 0; col < W; col++) {
				if (stopflag) return;
				tx_pixelnbr = col + row * W;
				tx_pixel = ifkp_get_avatar_pixel(tx_pixelnbr, color);
				freq = frequency + (tx_pixel - 128) * bandwidth / 256.0;
				phaseincr = radians * freq;
				for (int n = 0; n < IMAGEspp; n++) {
					outbuf[n] = cos(txphase);
					txphase -= phaseincr;
					if (txphase < 0) txphase += TWOPI;
				}
				transmit(outbuf, IMAGEspp);
			}
			Fl::awake();
		}
	}
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
		case 0 : W = 59; H = 74; break;
		case 1 : W = 160; H = 120; break;
		case 2 : W = 320; H = 240; break;
		case 3 : W = 640; H = 480; color = false; break;
		case 4 : W = 640; H = 480; break;
		case 5 : W = 240; H = 300; break;
		case 6 : W = 240; H = 300; color = false; break;
		case 7 : W = 120; H = 150; break;
		case 8 : W = 120; H = 150; color = false; break;
	}

	while (!ifkp_image_header.empty()) {
		send_char(ifkp_image_header[0]);
		ifkp_image_header.erase(0,1);
	}

	REQ(ifkp_clear_tximage);

	freq = frequency - 0.6 * bandwidth;
	#define PHASE_CORR  (3 * symlen / 2)
	phaseincr = radians * freq;
	for (int n = 0; n < PHASE_CORR; n++) {
		outbuf[n] = cos(txphase);
		txphase -= phaseincr;
		if (txphase < 0) txphase += TWOPI;
	}
	transmit(outbuf, PHASE_CORR);

	if (color == false) {  // grey scale image
		for (int row = 0; row < H; row++) {
			memset(outbuf, 0, IMAGEspp * sizeof(*outbuf));
			for (int col = 0; col < W; col++) {
				if (stopflag) return;
				tx_pixelnbr = col + row * W;
				tx_pixel =	0.3 * ifkppic_TxGetPixel(tx_pixelnbr, 0) +   // red
							0.6 * ifkppic_TxGetPixel(tx_pixelnbr, 1) +   // green
							0.1 * ifkppic_TxGetPixel(tx_pixelnbr, 2);    // blue
				REQ(ifkp_updateTxPic, tx_pixel, tx_pixelnbr*3 + 0);
				REQ(ifkp_updateTxPic, tx_pixel, tx_pixelnbr*3 + 1);
				REQ(ifkp_updateTxPic, tx_pixel, tx_pixelnbr*3 + 2);
				freq = frequency + (tx_pixel - 128) * bandwidth / 256.0;
				phaseincr = radians * freq;
				for (int n = 0; n < IMAGEspp; n++) {
					outbuf[n] = cos(txphase);
					txphase -= phaseincr;
					if (txphase < 0) txphase += TWOPI;
				}
				transmit(outbuf, IMAGEspp);
				Fl::awake();
			}
		}
	} else {
		for (int row = 0; row < H; row++) {
			for (int color = 0; color < 3; color++) {
				memset(outbuf, 0, IMAGEspp * sizeof(*outbuf));
				for (int col = 0; col < W; col++) {
					if (stopflag) return;
					tx_pixelnbr = col + row * W;
					tx_pixel = ifkppic_TxGetPixel(tx_pixelnbr, color);
					REQ(ifkp_updateTxPic, tx_pixel, tx_pixelnbr*3 + color);
					freq = frequency + (tx_pixel - 128) * bandwidth / 256.0;
					phaseincr = radians * freq;
					for (int n = 0; n < IMAGEspp; n++) {
						outbuf[n] = cos(txphase);
						txphase -= phaseincr;
						if (txphase < 0) txphase += TWOPI;
					}
					transmit(outbuf, IMAGEspp);
				}
				Fl::awake();
			}
		}
	}
}

std::string img_str;

void ifkp::ifkp_send_image(std::string image_str) {
	img_str = image_str;
	TX_IMAGE = true;
	start_tx();
}

void ifkp::ifkp_send_avatar() {
	TX_AVATAR = true;
	start_tx();
}

int ifkp::tx_process()
{
	if (send_bot) {
		send_bot = false;
		send_char(0);
		send_char(0);
	}

	int c = get_tx_char();

//	if (c == GET_TX_CHAR_ETX || enable_image) {
	if (TX_IMAGE || TX_AVATAR) {
		if (img_str.length()) {
			for (size_t n = 0; n < img_str.length(); n++)
				send_char(img_str[n]);
		}
		if (TX_IMAGE) send_image();
		if (TX_AVATAR) send_avatar();
		send_char(0);
		stopflag = false;
		TX_IMAGE = false;
		TX_AVATAR = false;
		if (img_str.length()) {
			ifkppicTxWin->hide();
			img_str.clear();
		}
		return 0;
	}
	if ( stopflag || c == GET_TX_CHAR_ETX) { // aborts transmission
		send_char(0);
		TX_IMAGE = false;
		TX_AVATAR = false;
		stopflag = false;
		return -1;
	}
	send_char(c);
	return 0;
}

