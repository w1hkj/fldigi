// ----------------------------------------------------------------------------
// digiscope.cxx, Miniature Oscilloscope/Phasescope Widget
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code 
// distribution.
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

#include <config.h>

#include "digiscope.h"

#include "modem.h"

#include <iostream>

#include "qrunner.h"


Digiscope::Digiscope (int X, int Y, int W, int H) :
	Fl_Widget (X, Y, W, H) {
	_phase = _flo = _fhi = _amp = 0.0;
	box(FL_DOWN_BOX);
	vidbuf = new unsigned char[ 3 * (W-4) * (H-4)];
	vidline = new unsigned char[ 3 * (W-4)];
	_highlight = false;
	_len = MAX_LEN;
}

Digiscope::~Digiscope()
{
	if (vidbuf) delete [] vidbuf;
	if (vidline) delete [] vidline;
}

void Digiscope::video(double *data, int len )
{
	if (data == NULL || len == 0)
		return;
	
	FL_LOCK_D();
	int W = w() - 4;
	int H = h() - 4;
// video signal display
//	if (len < W)
//		for (int i = 0; i < W; i++) 
//			vidline[3*i] = vidline[3*i+1] = 
//			vidline[3*i+2] = (unsigned char)(data[i * W / len]);
//	else
		for (int i = 0; i < W; i++) 
			vidline[3*i] = vidline[3*i+1] = 
			vidline[3*i+2] = (unsigned char)(data[i * len / W]);
	vidline[3*W/2] = 255;
	vidline[3*W/2+1] = 0;
	vidline[3*W/2+2] = 0;
	if (linecnt == H) {
		linecnt--;
		unsigned char *p = &vidbuf[3*W];
		memmove (vidbuf, p, 3*(W * (H-1))*sizeof(unsigned char));
		memcpy (&vidbuf[3*W*(H-1)], vidline, 3*W * sizeof (unsigned char));
	}
	else
		memcpy (&vidbuf[3*W*linecnt], vidline, 3*W * sizeof(unsigned char));
	linecnt++;

	REQ(&Digiscope::redraw, this);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void Digiscope::data(double *data, int len, bool scale)
{
	if (data == 0) {
		memset(_buf, 0, MAX_LEN * sizeof(*_buf));
		return;
	}
	if (len == 0)
		return;
	FL_LOCK_D();
	if (len > MAX_LEN) _len = MAX_LEN;
	else _len = len;
	memcpy(_buf, data, len * sizeof(double));

	if (scale) {
		double max = 1E-6;
		double min = 1E6;
		for (int i = 0; i < _len; i++) {
			max = MAX(max, _buf[i]);
			min = MIN(min, _buf[i]);
		}
		for (int i = 0; i < _len; i++)
			if (_buf[i] > 0.01) // threshold
				_buf[i] = (_buf[i] - min) / (max - min);
			else
				_buf[i] = 0.0;
	}
	REQ(&Digiscope::redraw, this);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void Digiscope::phase(double ph, bool hl)
{
	FL_LOCK_D();
	_phase = ph;
	_highlight = hl;
	REQ(&Digiscope::redraw, this);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void Digiscope::rtty(double flo, double fhi, double amp)
{
	FL_LOCK_D();
	_flo = flo;
	_fhi = fhi;
	_amp = amp;
	REQ(&Digiscope::redraw, this);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}


void Digiscope::mode(scope_mode md)
{
	int W = w() - 4;
	int H = h() - 4;
	FL_LOCK_D();
	_mode = md;
	memset(_buf, 0, MAX_LEN * sizeof(double));
	linecnt = 0;
	memset (vidbuf, 0, 3*W*H * sizeof (unsigned char) );
	memset (vidline, 0, 3*W*sizeof(unsigned char) );
	vidline[3*W/2] = 255;
	vidline[3*W/2+1] = 0;
	vidline[3*W/2+2] = 0;
	for (int i = 0; i < H; i++)
		memcpy(&vidbuf[3*W*i], vidline, 3*W*sizeof(unsigned char) );
	REQ(&Digiscope::redraw, this);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void Digiscope::draw_phase()
{
	fl_translate(x() + w() / 2.0, y() + w() / 2.0);
	fl_scale( 0.9*w()/2, -0.9*w()/2);
	fl_color(FL_WHITE);
	fl_circle( 0.0, 0.0, 1.0);
	fl_begin_line();
		fl_vertex(-1.0, 0.0);
		fl_vertex(-0.9, 0.0);
	fl_end_line();
	fl_begin_line();
		fl_vertex(1.0, 0.0);
		fl_vertex(0.9, 0.0);
	fl_end_line();
	fl_begin_line();
		fl_vertex(0.0, -1.0);
		fl_vertex(0.0, -0.9);
	fl_end_line();
	fl_begin_line();
		fl_vertex(0.0, 1.0);
		fl_vertex(0.0, 0.9);
	fl_end_line();
	
	fl_color(FL_GREEN);
	if (_highlight) {
		fl_begin_line();
			fl_vertex(0.0, 0.0);
			fl_vertex(0.9 * cos(_phase - M_PI / 2), 0.9 * sin( _phase - M_PI / 2));
		fl_end_line();
	} else {
		fl_circle( 0.0, 0.0, 0.1);
	}
}

void Digiscope::draw_crosshairs()
{
	double phi, xp, yp;
	fl_translate(x() + w() / 2.0, y() + w() / 2.0);
	fl_scale( 0.9*w()/2, -0.9*w()/2);
	fl_color(FL_WHITE);
	fl_circle( 0.0, 0.0, 1.0);
	fl_begin_line();
		fl_vertex(-1.0, 0.0);
		fl_vertex(-0.9, 0.0);
	fl_end_line();
	fl_begin_line();
		fl_vertex(1.0, 0.0);
		fl_vertex(0.9, 0.0);
	fl_end_line();
	fl_begin_line();
		fl_vertex(0.0, -1.0);
		fl_vertex(0.0, -0.9);
	fl_end_line();
	fl_begin_line();
		fl_vertex(0.0, 1.0);
		fl_vertex(0.0, 0.9);
	fl_end_line();
	
	fl_color(FL_GREEN);
	phi = (_fhi - 1.0)*M_PI/4.0;
	xp = 0.9*cos(phi); yp = 0.9*sin(phi);
	fl_begin_line();
		fl_vertex(-xp, -yp);
		fl_vertex( xp,  yp);
	fl_end_line();
	
	fl_color(FL_RED);
	phi = M_PI / 2.0 + (_flo + 1.0)*M_PI/4.0; // -
	xp = 0.9*cos(phi); yp = 0.9*sin(phi);
	fl_begin_line();
		fl_vertex(-xp, -yp);
		fl_vertex( xp,  yp);
	fl_end_line();
}

void Digiscope::draw_scope()
{
	int npts;
	npts = MIN(w(), _len);
	npts = MAX(1, npts);
	fl_translate(x()+2, y() + h() - 2);
	fl_scale ((w()-4), - (h() - 4));
	fl_color(FL_GREEN);
	fl_begin_line();
	for (int i = 0; i < npts; i++)
		fl_vertex( (double)i / npts, _buf[i * _len / npts] );
	fl_end_line();
}

void Digiscope::draw_rtty()
{
	int npts;
	npts = MIN(w(), _len);
	npts = MAX(1, npts);
	fl_translate(x()+2, y() + h() - 2);
	fl_scale ((w()-4), - (h() - 4));
	fl_color(FL_YELLOW);
	fl_begin_line();
		fl_vertex( 0.0, 0.9);
		fl_vertex( 1.0, 0.9);
	fl_end_line();
	fl_begin_line();
		fl_vertex( 0.0, 0.1);
		fl_vertex( 1.0, 0.1);
	fl_end_line();
	fl_color(FL_GREEN);
	fl_begin_line();
	for (int i = 0; i < npts; i++)
		fl_vertex( (double)i / npts, 0.5 + 0.75 * _buf[i * _len / npts] );
	fl_end_line();
}

void Digiscope::draw_video()
{
	fl_draw_image(
		vidbuf, 
		x() + 2, y() + 2, 
		w() - 4, h() - 4);//, 
//		3, 3* (w() - 4));
}

void Digiscope::draw()
{
	draw_box();
	if (_mode == WWV || _mode == DOMWF)
		draw_video();
	else {
		fl_clip(x()+2,y()+2,w()-4,h()-4);
		fl_color(FL_BLACK);
		fl_rectf(x()+2,y()+2,w()-4,h()-4);
		fl_push_matrix();
		switch (_mode) {
			case SCOPE :	draw_scope(); break;
			case PHASE :	draw_phase(); break;
			case RTTY :		draw_rtty(); break;
			case XHAIRS :	draw_crosshairs(); break;
			case DOMDATA :	draw_scope(); break;
			case BLANK : 
			default: break;
		}
		fl_pop_matrix();
		fl_pop_clip();
	}
}

int Digiscope::handle(int event)
{
	if (!Fl::event_inside(this))
		return 0;

	switch (event) {
	case FL_RELEASE:
		switch (_mode) {
		case RTTY:
			_mode = XHAIRS;
			break;
		case XHAIRS:
			_mode = RTTY;
			break;
		case DOMDATA:
			_mode = DOMWF;
			break;
		case DOMWF:
			_mode = DOMDATA;
			break;
		case WWV:
			event = Fl::event_button();
			if (event == FL_LEFT_MOUSE)
				wwv_modem->set1(Fl::event_x() - x(), w());
			else if (event == FL_RIGHT_MOUSE)
				wwv_modem->set2(Fl::event_x() - x(), Fl::event_y() - y());
			break;
		default:
			break;
		}
		return 1;
	case FL_MOUSEWHEEL:
		change_modem_param(FL_CTRL);
		break;
	default:
		break;
	}

	return 1;
}
