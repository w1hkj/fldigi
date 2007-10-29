// ----------------------------------------------------------------------------
// id.h  --  WATERFALL ID
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
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

#ifndef _ID_H
#define _ID_H

#include <string>
#include "complex.h"

using namespace std;

struct idfntchr { char c; int byte[5]; };

class id {
public:
#define NUMROWS				5
#define NUMCHARS			2
#define NUMTONES			5
#define TONESPACING			6
#define IDSYMLEN			3072

private:
	
	static	idfntchr idch[];
	static	int mask[];
	static  double	w[];
	static  double	txpulse[];
	static  double	outbuf[];
	
	void	make_pulse(double samplerate);
	void	make_tones(double frequency, double samplerate);
	void	send(long int);
	void	sendchars(std::string);

public:
	id();
	~id();
	void	text(string s, double frequency, int samplerate);
};

#endif
