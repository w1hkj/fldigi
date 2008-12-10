// ----------------------------------------------------------------------------
// pskeval.cxx  --  psk signal evaluator
//
// Copyright (C) 2008
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
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
#include <iostream>
#include <config.h>

#include "pskeval.h"

using namespace std;
//=============================================================================
//========================== psk signal evaluation ============================
//=============================================================================

pskeval::pskeval() {
	bw = 31.25;
	clear();
}

pskeval::~pskeval() {
}


void pskeval::sigdensity() {
/*	
	double sig = 0.0;
	double val;
	int ihbw = (int)(bw / 2 + 0.5) + 1;
	int ibw = 2 * ihbw;
	int fstart = FLOWER + ibw;
	double *vals = new double[ibw + 1];
	for (int i = 0; i < ibw + 1; i++) vals[i] = 0.0;
	int j = -1;
	sigavg = 0.0;
	for (int i = FLOWER; i < IMAGE_WIDTH; i++) {
		j++;
		if (j == ibw + 1) j = 0;
		if (i >= fstart) {
			sigpwr[i - ihbw - 1] = sig;
			sig -= vals[j];
		}
		vals[j] = val = wf->Pwr(i);
		sig += val;
		sigavg += val;
	}		
	sigavg /= (FFT_LEN - FLOWER);
	if (sigavg == 0) sigavg = 1e-20;
	delete [] vals;
*/
	int ibw = (int)bw;
	if (ibw % 2) ibw++;
	int ihbw = ibw / 2;
	double val;
	sigavg = 0;
	for (int i = FLOWER+1; i < IMAGE_WIDTH - ibw - 1; i++) {
		val = sigpwr[i+ihbw] = (wf->Pwr(i-1) + wf->Pwr(i) + wf->Pwr(i + ibw) + wf->Pwr(i+ibw+1))/4;
		sigavg += val;
	}
	sigavg /= (IMAGE_WIDTH - FLOWER - 2);
}

double pskeval::sigpeak(int &f, int f1, int f2)
{
	double peak = 0;
	double halfpower = 0;
	for (int i = f1; i <= f2; i++) {
		if (i == f1) integral[i] = sigpwr[i];
		else integral[i] = integral[i-1] + sigpwr[i];
		if (sigpwr[i] > peak)
			peak = sigpwr[i];
//std::cout << i << "\t" << sigpwr[i] << "\t" << i << "\t" << integral[i] << std::endl;
	}
	halfpower = integral[f2] / 2.0;
	for (f = f1; f < f2; f++)
		if (integral[f] > halfpower)
			break;
	return peak / sigavg;// / bw;
}

void pskeval::clear() {
	for (int i = 0; i < FFT_LEN; i++) sigpwr[i] = 0.0;
}
