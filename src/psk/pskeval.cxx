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

int countdown = 500;

void pskeval::sigdensity() {
	double sig = 0.0;
	double val;
	int ihbw = (int)(bw / 2 + 0.5);
	int ibw = 2 * ihbw;
	int fstart = progdefaults.LowFreqCutoff + ihbw;
	double *vals = new double[ibw];
	for (int i = 0; i < ibw; i++) vals[i] = 0.0;
	int j = -1;
	sigavg = 0.0;
	for (int i = progdefaults.LowFreqCutoff; i < progdefaults.HighFreqCutoff; i++) {
		if (++j == ibw) j = 0;
		val = wf->Pwr(i);
		if (i >= fstart) {
			sigpwr[i - ihbw] = decayavg(sigpwr[i - ihbw], sig, 64);
			sig -= vals[j];
		}
		vals[j] = val;
		sig += val;
		sigavg += val;
	}		
	sigavg /= (progdefaults.HighFreqCutoff - progdefaults.LowFreqCutoff);
	if (sigavg == 0) sigavg = 1e-20;
	delete [] vals;
}

double pskeval::sigpeak(int &f, int f1, int f2, int w)
{
	double peak = 0;
	f = (f1 + f2) / 2;
	for (int i = f1; i <= f2; i++)
		if (sigpwr[i] > peak) {
			peak = sigpwr[i];
			f = i;
		}
	return peak / sigavg / bw;
}


double pskeval::peak(int &f0, int f1, int f2, double val)
{
	double peak = 0;
	double testval = val * sigavg * bw;
	int fa = f2, fb = f1;
	for (int i = 0; i < (f2 - f1); i++) {
		if (sigpwr[f1+i] > testval) fb = f1 + i;
		if (sigpwr[f2-i] > testval) fa = f2 - i;
		if (sigpwr[f1+i] > peak) peak = sigpwr[f1+i];
	}
	if (fb > fa) {
		f0 = (fa + fb) / 2;
		return peak / sigavg / bw;
	} else
		return 0;
//	int f1 = f0 - bw;
//	int f2 = f0 + bw;
//	f1 = f1 < bw ? bw : f1;
//	return sigpeak(f0, f1, f2, bw);
/*
	double amp = 0;
	int f = f0;
	if ((amp = sigpeak(f, f1, f2, bw))) {
		if (f <= 0) { return 0; }
		f1 = f - bw;
		f2 = f + bw;
			if ((amp = sigpeak(f, f1, f2, bw))) {
				if (f <= 0) { f = f1; return 0; }
				f1 = f - bw;
				f2 = f + bw;
				amp = sigpeak(f, f1, f2, bw);
			}
		}
	if (f <= 0) { return 0; }
	f0 = f;
	return amp;
*/
}

void pskeval::clear() {
	for (int i = 0; i < FFT_LEN; i++) sigpwr[i] = 0.0;
}
