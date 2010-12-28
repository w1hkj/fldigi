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
#include "configuration.h"
#include "misc.h"

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

int countdown = 8;
int rows = 0;

void pskeval::sigdensity() {
	int ihbw = (int)(1.5*bw);
	int ibw = 2 * ihbw;

	double *vals = new double[ibw];
	double sig = 0.0;
	double val = 0.0;

	int low = progdefaults.LowFreqCutoff;
	if (low < ihbw) low = ihbw;
	int high = progdefaults.HighFreqCutoff;
	if (high > FFT_LEN - ihbw) high = FFT_LEN - ihbw;
	int nbr = high - low;

	sigavg = 0.0;

	for (int i = 0; i < ibw; i++) {
		val = vals[i] = wf->Pwr(i + low - ihbw);
		sig += val;
	}
	for (int i = 0, j = 0; i < nbr; i++) {
		sigpwr[i + low] = decayavg(sigpwr[i + low], sig, 64);
		sig -= vals[j];
		val = vals[j] = wf->Pwr(i + ihbw + low);
		sig += val;
		if (++j == ibw) j = 0;
		sigavg += val;
	}
	sigavg /= nbr;
	if (sigavg == 0) sigavg = 1e-5;
	delete [] vals;
/*
	if (!rows++) {
		for (int i = low; i < high-2; i++) printf("%d,",i);
		printf("%d\n", high - 1);
	}
	if (!countdown--) {
		countdown = 8;
		for (int i = low; i < high-2; i++) printf("%d,", (int)(1000*sigpwr[i]/sigavg/bw));
		printf("%d\n", (int)(1000*sigpwr[high - 1]/sigavg/bw));
	}
*/
}

double pskeval::sigpeak(int &f, int f1, int f2, int w)
{
	double peak = 0;
	f1 -= bw;
	if (f1 <= progdefaults.LowFreqCutoff) f1 = progdefaults.LowFreqCutoff;
	f2 += bw;
	if (f2 >= progdefaults.HighFreqCutoff) f2 = progdefaults.HighFreqCutoff;

	int fa = f2, fb = f1;


//	for (int i = (int)(f1-bw); i < (int)(f2+bw); i++)
//		printf("%d, %f\n", i, sigpwr[i]/ sigavg / bw);

	for (int i = f1; i < f2; i++) if (sigpwr[i] > peak) peak = sigpwr[i];
	if (!peak) return 0;
	for (int i = f1; i < f2; i++)
		if (sigpwr[i] > peak*0.75) fb = i;
	for (int i = f2; i > f1; i--)
		if (sigpwr[i] > peak*0.75) fa = i;
	if (fa > fb) return 0;
	f = (fa + fb) / 2;
	return peak / sigavg / bw;
}

double pskeval::peak(int &f0, int f1, int f2, double level)
{
	double peak = 0;

	int fa = f2, fb = f1;

//step 1
	for (int i = f1; i < f2; i++) if (sigpwr[i] > peak) peak = sigpwr[i];
//	if (!peak) return 0;
	if (peak < level*sigavg) return 0;

	for (int i = f1; i < f2; i++)
		if (sigpwr[i] > peak*0.75) fb = i;
	for (int i = f2; i > f1; i--)
		if (sigpwr[i] > peak*0.75) fa = i;
	if (fa > fb) return 0;
	f0 = (fa + fb) / 2;
//step 2
	fb = f1 = f0 - 1.5*bw;
	fa = f2 = f0 + 1.5*bw;
	peak = 0;
	for (int i = f1; i < f2; i++) if (sigpwr[i] > peak) peak = sigpwr[i];
	for (int i = f1; i < f2; i++)
		if (sigpwr[i] > peak*0.75) fb = i;
	for (int i = f2; i > f1; i--)
		if (sigpwr[i] > peak*0.75) fa = i;
	if (fa > fb) return 0;
	f0 = (fa + fb) / 2;

//	for (int i = (int)(fa); i < (int)(fb); i++)
//		printf("%d, %f\n", i, sigpwr[i]/ sigavg );

	return peak / sigavg / bw;
}

void pskeval::clear() {
	for (int i = 0; i < FFT_LEN; i++) sigpwr[i] = 0.0;
}
