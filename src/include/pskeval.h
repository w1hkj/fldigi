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

#ifndef _PSKEVAL_H
#define _PSKEVAL_H

#include "complex.h"
#include "trx.h"
#include "filters.h"
#include "fldigi-config.h"
#include "waterfall.h"

#define FLOWER 200
#define FUPPER 4000

class pskeval {
private:
	double	sigpwr[FFT_LEN];
	double	integral[FFT_LEN];
	double	sigavg;
	double	bw;
public:
	pskeval();
	~pskeval();
	void	clear();
	void	setbw(double w) { bw = w;}
	void	sigdensity();
	double	sigpeak(int &f, int f1, int f2);
};

#endif
