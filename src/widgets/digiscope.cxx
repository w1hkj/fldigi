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

#include "digiscope.h"

#include "modem.h"

#include <iostream>

inline double MAX(double m1, double m2) {
	if (m1 > m2) return m1;
	return m2;
}

inline int MAX(int m1, int m2) {
	if (m1 > m2) return m1;
	return m2;
}

inline double MIN(double m1, double m2) {
	if (m1 < m2) return m1;
	return m2;
}

inline int MIN (int m1, int m2) {
	if (m1 < m2) return m1;
	return m2;
}


Digiscope::Digiscope (int X, int Y, int W, int H) :
	Fl_Widget (X, Y, W, H) {
	_phase = _flo = _fhi = _amp = 0.0;
	box(FL_DOWN_BOX);
	vidbuf = new unsigned char[(W-4) * (H-4)];
	vidline = new unsigned char[W-4];
}

Digiscope::~Digiscope()
{
}

void Digiscope::video(double *data, int len )
{
	if (data == NULL || len == 0)
		return;
	
	FL_LOCK();
	int W = w() - 4;
	int H = h() - 4;
// video signal display
	if (len < W)
		for (int i = 0; i < W; i++) vidline[i] = (unsigned char)(data[i * W / len]);
	else
		for (int i = 0; i < W; i++) vidline[i] = (unsigned char)(data[i * len / W]);
	if (linecnt == H) {
		linecnt--;
		unsigned char *p = &vidbuf[W];
		memmove (vidbuf, p, (W * (H-1))*sizeof(unsigned char));
		memcpy (&vidbuf[W*(H-1)], vidline, W * sizeof (unsigned char));
	}
	else
		memcpy (&vidbuf[W*linecnt], vidline, W * sizeof(unsigned char));
	linecnt++;

	redraw();
	FL_UNLOCK();
	FL_AWAKE();
}

void Digiscope::data(double *data, int len, bool scale)
{
	if (data == NULL || len == 0)
		return;
	FL_LOCK();
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
	redraw();
	FL_UNLOCK();
	FL_AWAKE();
}

void Digiscope::phase(double ph, bool hl)
{
	FL_LOCK();
	_phase = ph;
	_highlight = hl;
	redraw();
	FL_UNLOCK();
	FL_AWAKE();
}

void Digiscope::rtty(double flo, double fhi, double amp)
{
	FL_LOCK();
	_flo = flo;
	_fhi = fhi;
	_amp = amp;
	redraw();
	FL_UNLOCK();
	FL_AWAKE();
}


void Digiscope::mode(scope_mode md)
{
	FL_LOCK();
	_mode = md;
	memset(_buf, 0, MAX_LEN * sizeof(double));
	linecnt = 0;
	memset (vidbuf, 0, (w() -4)*(h()-4) * sizeof (unsigned char) );
	memset (vidline, 0, (w() - 4)*sizeof(unsigned char) );
	redraw();
	FL_UNLOCK();
	FL_AWAKE();
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
	fl_draw_image_mono(
		vidbuf, 
		x() + 2, y() + 2, 
		w() - 4, h() - 4, 
		1, w() - 4);
}

void Digiscope::draw()
{
	draw_box();
	if (_mode == WWV)
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
			case BLANK : 
			default: break;
		}
		fl_pop_matrix();
		fl_pop_clip();
	}
}

int Digiscope::handle(int event)
{
	if (Fl::event_inside( this )) {
		if (event == FL_RELEASE) {
			if (_mode == RTTY || _mode == XHAIRS) {
				if (_mode == RTTY) {
					_mode = XHAIRS;
					return 1;
				}
				_mode = RTTY;
				return 1;
			}
			if (_mode == WWV ) {
				int xpos = Fl::event_x() - x();
				int ypos = Fl::event_y() - y();
				int evb = Fl::event_button();
				if (evb == 1)
					wwv_modem->set1(xpos,ypos);
//					std::cout << "#1 " << xpos << ", " << ypos << std::endl; fflush(stdout);
				if (evb == 3)
					wwv_modem->set2(xpos,ypos);
//					std::cout << "#2 " << xpos << ", " << ypos << std::endl; fflush(stdout);
				return 1;
			}
		}
		return 1;
	}
	return 0;
}
