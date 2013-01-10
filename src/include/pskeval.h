// ----------------------------------------------------------------------------
// pskeval.cxx  --  psk signal evaluator
//
// Copyright (C) 2008
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

#ifndef _PSKEVAL_H
#define _PSKEVAL_H

#include "complex.h"
#include "filters.h"
#include "waterfall.h"

#define FLOWER 200
#define FUPPER 4000

class pskeval {
private:
	double	sigpwr[FFT_LEN];
	double	sigmin;
	double	bw;
public:
	pskeval();
	~pskeval();
	void	clear();
	void	setbw(double w) { bw = w;}
	void	sigdensity();
	double	sigpeak(int &f, int f1, int f2);
	double	peak(int &f, int f1, int f2, double level);
	double	power(int f1, int f2);
};

#endif
