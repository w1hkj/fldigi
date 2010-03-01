// ----------------------------------------------------------------------------
// tune.cxx  -- create a single sinewave output with controlled start/end shape
//
// Copyright (C) 2006-2007
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

#include <math.h>
#include "sound.h"

namespace xmttune {

// use same wave shaping for tune key down / key up as for the CW tx_process
// produces a 4 msec leading / trailing edge
#define KNUM 32
// keydown wave shape

double kdshape[KNUM] = {
	0.00240750255310301, 0.00960708477768751,
	0.02152941088003600, 0.03805966253618680,
	0.05903864465505320, 0.08426431851158830, 
	0.11349374748686800, 0.14644543667658500,
	0.18280204383628200, 0.22221343555548300, 
	0.26430005922814900, 0.30865659834558700,
	0.35485587590940700, 0.40245296837259500, 
	0.45098949048925500, 0.49999800980765500,
	0.54900654829266300, 0.59754312772456200, 
	0.64514031509964400, 0.69133972425796200,
	0.73569643038517400, 0.77778325487450100, 
	0.81719487928327800, 0.85355174876454100,
	0.88650372738152000, 0.91573347010241700, 
	0.94095947900139100, 0.96193881423287900,
	0.97846943367117300, 0.99039213868324900, 
	0.99759210729604500, 0.99999999999295900
};

// keyup wave shape
double kushape[KNUM] = {
	0.99999999999295900, 0.99759210729604500, 
	0.99039213868324900, 0.97846943367117300,
	0.96193881423287900, 0.94095947900139100, 
	0.91573347010241700, 0.88650372738152000,
	0.85355174876454100, 0.81719487928327800, 
	0.77778325487450100, 0.73569643038517400,
	0.69133972425796200, 0.64514031509964400, 
	0.59754312772456200, 0.54900654829266300,
	0.49999800980765500, 0.45098949048925500, 
	0.40245296837259500, 0.35485587590940700,
	0.30865659834558700, 0.26430005922814900, 
	0.22221343555548300, 0.18280204383628200,
	0.14644543667658500, 0.11349374748686800, 
	0.08426431851158830, 0.05903864465505320,
	0.03805966253618680, 0.02152941088003600, 
	0.00960708477768751, 0.00240750255310301
};

#define BUFLEN 512
double phaseacc = 0.0;
double phaseincr = 0.0;
double outbuf[BUFLEN];

//===========================================================================

inline double nco()
{
	phaseacc += phaseincr;
	if (phaseacc > M_PI)
		phaseacc -= 2.0 * M_PI;
	return cos(phaseacc);
}

//=====================================================================


//=====================================================================

void keydown(double freq, SoundBase *scard)
{
	int i;
	phaseincr = 2.0 * M_PI * freq / active_modem->get_samplerate();
	for (i = 0; i < KNUM; i++)
		outbuf[i] = nco() * kdshape[i];
	for (; i < BUFLEN; i++)
		outbuf[i] = nco();
	active_modem->ModulateXmtr(outbuf, BUFLEN);
}

//=====================================================================

void keyup(double freq, SoundBase *scard)
{
	int i;
	phaseincr = 2.0 * M_PI * freq / active_modem->get_samplerate();
	for (i = 0; i < KNUM; i++)
		outbuf[i] = nco() * kushape[i];
	for (; i < BUFLEN; i++)
		outbuf[i] = 0.0;
	active_modem->ModulateXmtr(outbuf, BUFLEN);
}

//=====================================================================

void tune(double freq, SoundBase *scard)
{
	int i;
	phaseincr = 2.0 * M_PI * freq / active_modem->get_samplerate();
	for (i = 0; i < BUFLEN; i++)
		outbuf[i] = nco();
	active_modem->ModulateXmtr(outbuf, BUFLEN);
}

}  // namespace tune
