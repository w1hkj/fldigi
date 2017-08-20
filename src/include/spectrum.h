// ----------------------------------------------------------------------------
// digiscope.h, Miniature Oscilloscope/Phasescope Widget
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted in part from code contained in
// gmfsk source code distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
//  Copyright (C) 2004
//  Lawrence Glaister (ve7it@shaw.ca)
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

#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <FL/Fl_Widget.H>

#include "digiscope.h"

class spectrum : public Digiscope {
public:

private:

	bool _paused;
	double _freq;
	double _db;
	double _db_diff;
	double _f_diff;
	double _gofreq;

	void handle_shift_leftclick(int x);
	void handle_leftclick( int x, int y);
	void handle_rightclick( int x, int y);

public:
	spectrum(int, int, int, int);
	~spectrum();
	bool paused() { return _paused; }
	void paused(bool on) { _paused = on; }
	double freq() { return _freq; }
	double db() { return _db; }
	double db_diff() { return _db_diff; }
	double f_diff() { return _f_diff; }
	double gofreq() { return _gofreq; }
	void   gofreq(double f) { _gofreq = f; }

	int handle(int);
};

#endif
