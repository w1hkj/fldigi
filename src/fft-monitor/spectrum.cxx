// ----------------------------------------------------------------------------
// spectrum   Spectrum display Widget based on Digiscope widget
//
// Copyright (C) 2006-2017
//		Dave Freese, W1HKJ
// Copyright (C) 2008
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code
// distribution.
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

#include <config.h>

#include <iostream>
#include <cmath>
#include <cstring>

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include "spectrum.h"
#include "modem.h"
#include "trx.h"
#include "fl_digi.h"
#include "qrunner.h"

#include "configuration.h"

spectrum::spectrum (int x, int y, int w, int h) :
	Digiscope (x, y, w, h)
{
	_paused = false;
	_freq = 0;
	_db = 0;
	_db_diff = 0;
	_f_diff = 0;
	_gofreq = 0;
	mode(spectrum::SCOPE);
}

spectrum::~spectrum()
{
}

void spectrum::handle_shift_leftclick(int x1)
{
	_gofreq = progdefaults.fftviewer_fcenter - progdefaults.fftviewer_frng / 2
			+ (1.0 * x1 / w()) * progdefaults.fftviewer_frng;
}

void spectrum::handle_leftclick( int x1, int y1)
{
	if (Fl::event_state() & FL_SHIFT) {
		handle_shift_leftclick(x1);
		return;
	}
	if (Fl::event_key('1') || Fl::event_key(FL_KP + '1')) {
		_y_user1 = 1.0 * y1 / h();
		_x_user1 = 1.0 * x1 / w();
	} else if (Fl::event_key('2') || Fl::event_key(FL_KP + '2')) {
		_y_user2 = 1.0 * y1 / h();
		_x_user2 = 1.0 * x1 / w();
	} else if (Fl::event_key('c') || Fl::event_key('3') || Fl::event_key(FL_KP + '3')) {
		_y_user1 = _y_user2 = -1;
		_x_user1 = _x_user2 = -1;
	} else {
		_freq = progdefaults.fftviewer_fcenter - progdefaults.fftviewer_frng / 2
				+ (1.0 * x1 / w()) * progdefaults.fftviewer_frng;
		_db = progdefaults.fftviewer_maxdb - (1.0 * y1 / h()) * progdefaults.fftviewer_range;
		return;
	}

	if ((_y_user1 != -1) && (_y_user2 != -1))
		_db_diff = (_y_user1 - _y_user2) * progdefaults.fftviewer_range;
	else
		_db_diff = 0;

	if ((_x_user1 != -1) && (_x_user2 != -1))
		_f_diff = (_x_user2 - _x_user1) * progdefaults.fftviewer_frng;
	else
		_db_diff = 0;

}

void spectrum::handle_rightclick( int x, int y)
{
	_paused = !_paused;
}

int spectrum::handle(int event)
{
	if (event == FL_ENTER) {
		fl_cursor(FL_CURSOR_CROSS);
		redraw();
		return 1;
	}
	if (event == FL_LEAVE) {
		fl_cursor(FL_CURSOR_ARROW);
		redraw();
		return 1;
	}

	if (!Fl::event_inside(this))
		return 0;

	switch (event) {
		case FL_PUSH :
			break;
		case FL_RELEASE :
			if (!Fl::event_inside(this))
				break;
			switch (Fl::event_button()) {
				case FL_LEFT_MOUSE:
					handle_leftclick(Fl::event_x() - x(), Fl::event_y() - y());
					break;
				case FL_RIGHT_MOUSE :
					handle_rightclick(Fl::event_x() - x(), Fl::event_y() - y());
					break;
				default :
					break;
			}
		default :
			break;
	}
	return 1;
}

