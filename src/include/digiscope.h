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

#ifndef DIGISCOPE_H
#define DIGISCOPE_H

#include <FL/Fl_Widget.H>

#include "complex.h"

class Digiscope : public Fl_Widget {
public:
#define DEFAULT_WIDTH	100
#define DEFAULT_HEIGHT	100
#define	MAX_LEN			4096 //1024
#define MAX_ZLEN		1024
#define NUM_GRIDS		100
	enum scope_mode {
		SCOPE,
		PHASE,
		PHASE1,
		PHASE2,
		PHASE3,
		RTTY,
		XHAIRS,
		WWV,
		DOMDATA,
		DOMWF,
		BLANK
	};

private:
	scope_mode _mode;
	double _buf[MAX_LEN];
	cmplx _zdata[MAX_ZLEN];
	//int _zlen;
	int _zptr;
	unsigned char *vidbuf;
	unsigned char *vidline;
	int _len;
	int linecnt;
	double _phase;
	double _quality;
	double _flo, _fhi, _amp;
	double _x[NUM_GRIDS], _y[NUM_GRIDS];
	bool _highlight;
	scope_mode phase_mode;

public:
	Digiscope(int, int, int, int);
	~Digiscope();
	int handle(int);
	void resize(int x, int y, int w, int h);
	void draw();
	void draw_scope();
	void draw_phase();
	void draw_rtty();
	void draw_xy();
	void draw_video();
	void data(double *data, int len, bool scale = true);
	void phase(double ph, double ql, bool hl);
	void video(double *data, int len, bool dir );
	void zdata(cmplx *z, int len);
	void rtty(double flo, double fhi, double amp);
	void mode(scope_mode md);
	scope_mode mode() { return _mode;};\
	void xaxis(int n, double y1) {
		if (n < NUM_GRIDS) _y[n] = y1;
	}
	void yaxis(int n, double x1) {
		if (n < NUM_GRIDS) _x[n] = x1;
	}
	void xaxis_1(double y1) { _y[1] = y1; }
	void xaxis_2(double y2) { _y[2] = y2; }
	void yaxis_1(double x1) { _x[1] = x1; }
	void yaxis_2(double x2) { _x[2] = x2; }
	void clear_axis() {
		for (int i = 0; i < NUM_GRIDS; i++)
			_x[i] = _y[i] = 0;
	}
};

#endif
