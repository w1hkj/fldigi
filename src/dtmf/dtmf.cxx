// ----------------------------------------------------------------------------
//
//	DTMF.cxx
//
// Copyright (C) 2011
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

#include <cmath>
#include <cstring>
#include <float.h>
#include <samplerate.h>

#include "trx.h"

#include "dtmf.h"
#include "misc.h"

#include "fl_digi.h"
#include "configuration.h"
#include "qrunner.h"
#include "debug.h"
#include "status.h"

#include "main.h"

LOG_FILE_SOURCE(debug::LOG_MODEM);

// tones in 4x4 array
// 697  770  842  941  1209  1336  1447  1633

int cDTMF::row[] = {697, 770, 852, 941};

int cDTMF::col[] = {1209, 1336, 1477, 1633}; 

const char cDTMF::rc[] = "123A456B789C*0#D";

//======================================================================
// DTMF tone receive
//======================================================================
// tone #s and coefficients
// 8000 Hz sampling N == 240
// 11025 Hz sampling N == 331

/*
 * calculate the power of each tone using Goertzel filters
 */
void cDTMF::calc_power()
{
	double sr = active_modem->get_samplerate();
// reset row freq filters
	for (int i = 0; i < 4; i++) filt[i]->reset(240, row[i], sr);
// reset col freq filters
	for (int i = 0; i < 4; i++) filt[i+4]->reset(240, col[i], sr);

	for (int i = 0; i < framesize; i++)
		for (int j = 0; j < NUMTONES; j++)
			filt[j]->run(data[i]);

	for (int i = 0; i < NUMTONES; i++)
		power[i] = filt[i]->mag();

	maxpower = 0;
	minpower = 1e6;
	for (int i = 0; i < NUMTONES;i++) {
		if (power[i] > maxpower)
			maxpower = power[i];
		if (power[i] < minpower)
			minpower = power[i];
	}
	if ( minpower == 0 ) minpower = 1e-3;
}


/*
 * detect which signals are present.
 *
 */

int cDTMF::decode()
{ 
	calc_power();

	if (maxpower < (10 * progStatus.sldrSquelchValue))
		return ' ';

	int r = -1, c = -1;
	double pwr = 0;
	for (int i = 0; i < 4; i++)
		if (power[i] > pwr) {
			pwr = power[i];
			r = i;
		}
	pwr = 0;
	for (int i = 0; i < 4; i++)
		if (power[i+4] > pwr) {
			pwr = power[i+4];
			c = i;
		}
	if (r == -1 || c == -1) 
		return ' ';
	return rc[r*4 + c];
}

/*
 * read in frames, output the decoded
 * results
 */
void cDTMF::receive(const float* buf, size_t len)
{
	int x;
	static size_t dptr = 0;
	size_t bufptr = 0;

	if (!progdefaults.DTMFdecode) return;

	framesize = (active_modem->get_samplerate() == 8000) ? 240 : 331;

	while (1) {
		int i;
		for ( i = dptr; i < framesize; i++) {
			data[i] = buf[bufptr];
			bufptr++;
			if (bufptr == len) break;
		}
		if (i < framesize) {
			dptr = i + 1;
			return;
		}
		dptr = 0;

		x = decode(); 
		if (x == ' ') {
			silence_time++;
			if (silence_time == 4 && !dtmfchars.empty()) dtmfchars += ' ';
			if (silence_time == FLUSH_TIME) {
				if (!dtmfchars.empty()) {
					REQ(showDTMF, dtmfchars);
					dtmfchars.clear();
				}
				silence_time = 0;
			}
		} else {
			silence_time = 0;
			if (x != last && last == ' ')
				dtmfchars += x;
		}
		last = x;
	}
}

//======================================================================
// DTMF tone transmit
//======================================================================

void cDTMF::makeshape()
{
	for (int i = 0; i < 128; i++) shape[i] = 1.0;
	for (int i = 0; i < RT; i++)
		shape[i] = 0.5 * (1.0 - cos (M_PI * i / RT));
}

//----------------------------------------------------------------------
// transmit silence for specified time duration in milliseconds
//----------------------------------------------------------------------

void cDTMF::silence(int len)
{
	double sr = active_modem->get_samplerate();
	int length = len * sr / 1000;
	if (length > 16384) length = 16384;
	memset(outbuf, 0, length * sizeof(*outbuf));
	active_modem->ModulateXmtr(outbuf, length);
}

//----------------------------------------------------------------------
// transmit DTMF tones for specific time interval
//----------------------------------------------------------------------

void cDTMF::two_tones(int ch)
{
	if (!strchr(rc, ch)) return;
	int pos = strchr(rc, ch) - rc;
	int r = pos / 4;
	int c = pos % 4;
	double sr = active_modem->get_samplerate();
	double phaseincr = 2.0 * M_PI * row[r] / sr;
	double phase = 0;
	int length = duration * sr / 1000;
	if (length > 16384) length = 16384;

	for (int i = 0; i < length; i++) {
		outbuf[i] = 0.5 * sin(phase);
		phase += phaseincr;
	}

	phaseincr = 2.0 * M_PI * col[c] / sr;
	phase = 0;
	for (int i = 0; i < length; i++) {
		outbuf[i] += 0.5 * sin(phase);
		phase += phaseincr;
	}
	for (int i = 0; i < RT; i++) {
		outbuf[i] *= shape[i];
		outbuf[length - 1 - i] *= shape[i];
	}

	active_modem->ModulateXmtr(outbuf, length);
}

//----------------------------------------------------------------------
// transmit the string contained in progdefaults.DTMFstr and output as 
// dtmf valid characters, 0-9, *, #, A-D
// each pair of tones is separated by 50ms silence
// 500 msec silence for ' ', ',' or '-'
// 50 msec silence for invalid characters
//----------------------------------------------------------------------

void cDTMF::send()
{
	int c = 0, delay = 0;
	duration = 50;
	RT = (int)(active_modem->get_samplerate() * 4 / 1000.0); // 4 msec edge
	makeshape();
	size_t colon = std::string::npos;

	size_t modifier = progdefaults.DTMFstr.find("W");
	if (modifier != std::string::npos) {
		delay = atoi(&progdefaults.DTMFstr[modifier + 1]);
		colon = progdefaults.DTMFstr.find(':', modifier);
		progdefaults.DTMFstr.erase(modifier, colon - modifier + 1);
	}

	modifier = progdefaults.DTMFstr.find('L');
	if (modifier != std::string::npos) {
		duration = atoi(&progdefaults.DTMFstr[modifier + 1]);
		colon = progdefaults.DTMFstr.find(':', modifier);
		progdefaults.DTMFstr.erase(modifier, colon - modifier + 1);
	}

	while (delay > 50) { silence(50); delay -= 50;}
	if (delay) silence(delay);

	for(size_t i = 0; i < progdefaults.DTMFstr.length(); i++) {
		c = progdefaults.DTMFstr[i];
		if (c == ' ' || c == ',' || c == '-') 
			silence(duration);
		else if ( (c >= '0' && c <= '9') || 
					c == '*' ||
					c == '#' ||
					(c >= 'A' && c <= 'D') )
			two_tones(c);
		silence(duration);
	}
	progdefaults.DTMFstr.clear();
}

