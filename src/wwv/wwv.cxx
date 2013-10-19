// ----------------------------------------------------------------------------
// wwv.cxx  -- wwv monitoring modem
//
// Copyright (C) 2006-2009
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
//
// This modem is only used for reception of WWV "tick" signals to determine
// the correction factor to be applied to the sound card oscillator.

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

#include "wwv.h"
#include "fl_digi.h"

using namespace std;

void wwv::tx_init(SoundBase *sc)
{
	scard = sc;
	phaseacc = 0;
}

void wwv::rx_init()
{
	phaseacc = 0.0;
	smpl_ctr = 0;		// sample counter for timing wwv rx 
	agc = 0.0;			// threshold for tick detection 
	ticks = 0;
	calc = false;
	zoom = false;
	set_scope_mode(Digiscope::WWV);
	put_MODEstatus(mode);
}

void wwv::init()
{
	modem::init();
	rx_init();
}

wwv::~wwv() {
	if (hilbert) delete hilbert;
	if (lpfilter) delete lpfilter;
	if (vidfilter) delete vidfilter;
}


wwv::wwv() : modem()
{
	double lp;
	mode = MODE_WWV;
	frequency = 1000;
	bandwidth = 200;
	samplerate = 8000;	

// phase increment expected at the tick freq 
	phaseincr = 2.0 * M_PI * frequency / samplerate;

	hilbert = new C_FIR_filter();
	hilbert->init_hilbert(37, 1);

	lp = 0.5 * bandwidth / samplerate;
	lpfilter = new C_FIR_filter();
	lpfilter->init_lowpass (FIRLEN_1, DEC_1, lp);
	
	vidfilter = new Cmovavg(16);

	makeaudio();
}


//=======================================================================
//update_syncscope()
//Routine called to update the display on the sync scope display.
//For wwv this is video signal much like a FAX display
//=======================================================================
//
void wwv::update_syncscope()
{
	double max = 0, min = 1e6, range;
	for (int i = 0; i < 1000; i++ ) {
		if (max < buffer[i]) max = buffer[i];
		if (min > buffer[i]) min = buffer[i];
	}
	range = max - min;
	for (int i = 0; i < 1000; i++ ) {
		buffer[i] = 255*(buffer[i] - min) / range;
	}
	if (zoom)
		set_video(&buffer[400], 200);
	else
		set_video(buffer, 1000);
	buffer.next(); // change buffers
}


//=====================================================================
// wwv_rxprocess()
// Called with a block (512 samples) of audio.
// Nominal sound card sampling rate is set to 8000 Hz
//=======================================================================

int wwv::rx_process(const double *buf, int len)
{
	cmplx z, znco;

	while (len-- > 0) {
		z = cmplx ( *buf, *buf );
		buf++;

		hilbert->run(z, z);

		znco = cmplx ( cos(phaseacc), sin(phaseacc) );
		z = znco * z;
		
		phaseacc += phaseincr;
		if (phaseacc > M_PI)
			phaseacc -= 2.0 * M_PI;

		if (lpfilter->run ( z, z )) {
			buffer[smpl_ctr % 1000] = vidfilter->run( abs(z) );		
			if (++smpl_ctr >= 1000) {
				update_syncscope();
				smpl_ctr = 0;
			}
		}
	}
	return 0;
}

void wwv::set1(int x, int y)
{
	int zfactor = 500;
	if (zoom) zfactor = 100;
	smpl_ctr -= ((2*x - y) * zfactor) / y;
	if (smpl_ctr < 0) smpl_ctr += 1000;
	if (smpl_ctr > 1000) smpl_ctr -= 1000;
}

char strPPM[20];

void wwv::set2(int x, int y)
{
	zoom = !zoom;
}

//======================================================================
// transmit time tick
//======================================================================

void wwv::makeshape()
{
	for (int i = 0; i < 32; i++)
		keyshape[i] = 0.5 * (1.0 - cos (M_PI * i / 32));
}

double wwv::nco(double freq)
{
	phaseacc += 2.0 * M_PI * freq / samplerate;

	if (phaseacc > M_PI)
		phaseacc -= 2.0 * M_PI;

	return sin(phaseacc);
}

void wwv::makeaudio()
{
	phaseacc = 0.0;
	makeshape();
	for (int i = 0; i < 400; i++) {
		audio[i] = (i < 200 ? nco(1000) : 0);
		quiet[i] = 0;
	}
	for (int i = 0; i < 32; i++) {
		audio[i] *= keyshape[i];
		audio[199 - i] *= keyshape[i];
	}
}

int wwv::tx_process() 
{
	static int cycle = 4;
	int c = get_tx_char();

	if (c == GET_TX_CHAR_ETX || stopflag) {
		stopflag = false;
		return -1;
	}
	if (--cycle == 0) {
		memcpy(play, audio, 400 * sizeof(double));
		ModulateXmtr(play, 400);
		cycle = 4;
	} else
		ModulateXmtr(quiet, 400);
	ModulateXmtr(quiet, 400);
	ModulateXmtr(quiet, 400);
	ModulateXmtr(quiet, 400);
	ModulateXmtr(quiet, 400);
	return 0;
}
