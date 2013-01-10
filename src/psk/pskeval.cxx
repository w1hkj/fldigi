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
	int ihbw = (int)(0.6*bw);
	int ibw = 2 * ihbw;

	double *vals = new double[ibw];
	double sig = 0.0;
	double val = 0.0;

	int low = progdefaults.LowFreqCutoff;
	if (low < ihbw) low = ihbw;
	int high = progdefaults.HighFreqCutoff;
	if (high > FFT_LEN - ihbw) high = FFT_LEN - ihbw;
	int nbr = high - low;

	sigmin = 1e6;

	for (int i = 0; i < ibw; i++) {
		val = vals[i] = wf->Pwr(i + low - ihbw);
		sig += val;
	}
	for (int i = 0, j = 0; i < nbr; i++) {
		sigpwr[i + low] = decayavg(sigpwr[i + low], sig, 32);
		sig -= vals[j];
		val = vals[j] = wf->Pwr(i + ihbw + low);
		sig += val;
		if (++j == ibw) j = 0;
		if (sig < sigmin) sigmin = sig;
	}

	if (sigmin < 1e-8) sigmin = 1e-8;
	delete [] vals;
}

double pskeval::sigpeak(int &f, int f1, int f2)
{
	double peak = 0;
	f1 -= bw;
	if (f1 <= progdefaults.LowFreqCutoff) f1 = progdefaults.LowFreqCutoff;
	f2 += bw;
	if (f2 >= progdefaults.HighFreqCutoff) f2 = progdefaults.HighFreqCutoff;

	int fa = f2, fb = f1;

	for (int i = f1; i < f2; i++) if (sigpwr[i] > peak) peak = sigpwr[i];
	if (!peak) return 0;
	for (int i = f1; i < f2; i++)
		if (sigpwr[i] > peak*0.75) fb = i;
	for (int i = f2; i > f1; i--)
		if (sigpwr[i] > peak*0.75) fa = i;
	if (fa > fb) return 0;
	f = (fa + fb) / 2;
	return peak / sigmin / bw;
}

double pskeval::peak(int &f0, int f1, int f2, double db)
{
	double peak = 0;

	int fa = f2, fb = f1;
	double level = pow(10, (10 + db) / 10.0);

//step 1
	for (int i = f1; i < f2; i++) if (sigpwr[i] > peak) peak = sigpwr[i];

	if (((peak-sigmin) / sigmin ) < level) {
		return 0;
	}

	for (int i = f1; i < f2; i++)
		if (sigpwr[i] > peak*0.75) fb = i;
	for (int i = f2; i > f1; i--)
		if (sigpwr[i] > peak*0.75) fa = i;
	if (fa > fb) {
		return 0;
	}
	f0 = (fa + fb) / 2;
//step 2
	f1 = f0 - 1.5*bw;
	if (f1 < bw) f1 = bw;
	f2 = f0 + 1.5*bw;
	fb = f1; fa = f2;
	peak = 0;
	for (int i = f1; i < f2; i++) if (sigpwr[i] > peak) peak = sigpwr[i];
	for (int i = f1; i < f2; i++)
		if (sigpwr[i] > peak*0.75) fb = i;
	for (int i = f2; i > f1; i--)
		if (sigpwr[i] > peak*0.75) fa = i;
	if (fa > fb) {
		return 0;
	}
	f0 = (fa + fb) / 2;
	return (peak - sigmin) / sigmin ;
}

void pskeval::clear() {
	for (int i = 0; i < FFT_LEN; i++) sigpwr[i] = 0.0;
}
