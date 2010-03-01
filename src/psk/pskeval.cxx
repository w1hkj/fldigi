// ----------------------------------------------------------------------------
// pskeval.cxx  --  psk signal evaluator
//
// Copyright (C) 2008-2009
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
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

#include "fl_digi.h"
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
// restore the 3.03 algorithm for finding the signal density
    static double alpha = 0.125;
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
		val = wf->Pwr(i);
		if (i >= fstart) {
			sigpwr[i - ihbw - 1] = (1-alpha)*sigpwr[i - ihbw - 1] + alpha * sig;
			sig -= vals[j];
		}
		vals[j] = val;
		sig += val;
		sigavg += val;
	}		
	sigavg /= (FFT_LEN - FLOWER);
	if (sigavg == 0) sigavg = 1e-20;
	delete [] vals;
}

double pskeval::sigpeak(int &f, int f1, int f2)
{
// restore the 3.03 algorithm for finding the signal peak
// and frequency at which the peak occurs
	double peak = 0;
	f = (f1 + f2) / 2;
	for (int i = f1; i <= f2; i++)
		if (sigpwr[i] > peak) {
			peak = sigpwr[i];
			f = i;
		}
	return peak / sigavg / bw;
}

void pskeval::clear() {
	for (int i = 0; i < FFT_LEN; i++) sigpwr[i] = 0.0;
}
