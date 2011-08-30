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

#include "dtmf.h"
#include "filters.h"
#include "fft.h"
#include "misc.h"
#include "trx.h"
#include "waterfall.h"

#include "fl_digi.h"
#include "configuration.h"
#include "confdialog.h"
#include "qrunner.h"
#include "notify.h"
#include "debug.h"

#include "main.h"

LOG_FILE_SOURCE(debug::LOG_MODEM);

// tones in 4x4 array
// 697  770  842  941  1209  1336  1447  1633

int cDTMF::tones[] = {697, 770, 842, 941, 1209, 1336, 1477, 1633};

int cDTMF::row[] = {
941, 697, 697, 697, 770, 770, 770, 852, 852, 852, 941, 941, 697, 770, 852, 941};
//0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   *,   #,   A,   B,   C,   D

int cDTMF::col[] = { 
1336, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1477, 1633, 1633, 1633, 1633};
// 0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    *,    #,    A,    B,    C,    D

cDTMF::PAIRS cDTMF::pairs[] = {
{0, 0, '1'}, {0, 1, '2'}, {0, 2, '3'}, {0, 3, 'A'},
{1, 0, '4'}, {1, 1, '5'}, {1, 2, '6'}, {1, 3, 'B'},
{2, 0, '7'}, {2, 1, '8'}, {2, 2, '9'}, {2, 3, 'C'},
{3, 0, '*'}, {3, 1, '0'}, {3, 2, '#'}, {3, 3, 'D'} };

void cDTMF::makeshape()
{
	for (int i = 0; i < 128; i++) shape[i] = 1.0;
	for (int i = 0; i < RT; i++)
		shape[i] = 0.5 * (1.0 - cos (M_PI * i / RT));
}


cDTMF::cDTMF()
{
}

//rx is a work in progress :>)

void cDTMF::receive(const float* buf, size_t len)
{
	int binlo = 0, binhi = 0;
	double val1 = 0, val2 = 0, avg = 1e-6;
//this doesn't work!
	for (int i = 0; i < 8; i++) {
		bins[i] = wf->powerDensity(tones[i], 4);
		avg += bins[i] / 8;
	}
	for (int i = 0; i < 4; i++) {
		if (bins[i] > val1) { val1 = bins[i]; binlo = i; }
		if (bins[i+4] > val2) { val2 = bins[i+4]; binhi = i;}
	}
	if ((val1 / avg > 0.5) && (val2 / avg > 0.5)) {
		for (int i = 0; i < 16; i++) {
			if (pairs[i].lo == binlo && pairs[i].hi == binhi) {
				printf("DTMF %c\n", pairs[i].ch);
				break;
			}
		}
	}
}


//======================================================================
// DTMF tone transmit
//======================================================================

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

void cDTMF::two_tones(int rc)
{
	if (rc < 0 || rc > 15) return;
	double sr = active_modem->get_samplerate();
	double phaseincr = 2.0 * M_PI * row[rc] / sr;
	double phase = 0;
	int length = duration * sr / 1000;
	if (length > 16384) length = 16384;

	for (int i = 0; i < length; i++) {
		outbuf[i] = 0.5 * sin(phase);
		phase += phaseincr;
	}

	phaseincr = 2.0 * M_PI * col[rc] / sr;
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
	if (progdefaults.DTMFstr.empty()) return;

	int c = 0, delay = 0;
	duration = 50;
	RT = (int)(active_modem->get_samplerate() * 4 / 1000.0); // 4 msec edge
	makeshape();
	size_t colon = std::string::npos;

	size_t modifier = progdefaults.DTMFstr.find('D');
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
		else if (c >= '0' && c <= '9')        two_tones(c - '0');
		else if (c == '*')                    two_tones(10);
		else if (c == '#')                    two_tones(11);
		else if (c >= 'A' && c <= 'D')        two_tones(12 + c - 'A');
		else if (c >= 'a' && c <= 'd')        two_tones(12 + c - 'a');
		silence(duration);
	}
	progdefaults.DTMFstr.clear();
}

