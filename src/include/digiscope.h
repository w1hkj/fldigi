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

#ifndef DIGISCOPE_H
#define DIGISCOPE_H

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include "threads.h"
#include "complex.h"

class Digiscope : public Fl_Widget { 
public:
#define DEFAULT_WIDTH	100
#define DEFAULT_HEIGHT	100
#define	MAX_LEN			4096
//#define MAX_ZLEN		16184
//#define MAX_ZLEN		8192
#define MAX_ZLEN		4096
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
	Fl_Mutex *mutex;
private:
	scope_mode _mode;
	double _buf[MAX_LEN];
	complex _zdata[MAX_ZLEN];
	int _zlen;
	int _zptr;
	unsigned char *vidbuf;
	unsigned char *vidline;
	int _len;
	int linecnt;
	double _phase;
	double _quality;
	double _flo, _fhi, _amp;
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
	void draw_crosshairs();
	void draw_xy();
	void draw_video();
	void data(double *data, int len, bool scale = true);
	void phase(double ph, double ql, bool hl);
	void video(double *data, int len, bool dir );
	void zdata(complex *z, int len);
	void rtty(double flo, double fhi, double amp);
	void mode(scope_mode md);
	scope_mode mode() { return _mode;};
};

#endif
