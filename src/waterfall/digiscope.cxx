// ----------------------------------------------------------------------------
// digiscope.cxx, Miniature Oscilloscope/Phasescope Widget
//
// Copyright (C) 2006-2009
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

#include "digiscope.h"
#include "modem.h"
#include "trx.h"
#include "fl_digi.h"
#include "qrunner.h"


Digiscope::Digiscope (int X, int Y, int W, int H) :
	Fl_Widget (X, Y, W, H) {
	_phase = _quality = _flo = _fhi = _amp = 0.0;
	box(FL_DOWN_BOX);
	vidbuf = new unsigned char[ 3 * (W-4) * (H-4)];
	vidline = new unsigned char[ 3 * (W-4)];
	_highlight = false;
	_len = MAX_LEN;
	_zptr = 0;
	_x1 = _x2 = _y1 = _y2;
	phase_mode = PHASE1;
}

Digiscope::~Digiscope()
{
	if (vidbuf) delete [] vidbuf;
	if (vidline) delete [] vidline;
}

void Digiscope::video(double *data, int len , bool dir)
{
	if (active_modem->HistoryON()) return;
	
	if (data == NULL || len == 0)
		return;
	
	FL_LOCK_D();
	int W = w() - 4;
	int H = h() - 4;
	for (int i = 0; i < W; i++) 
		vidline[3*i] = vidline[3*i+1] = 
		vidline[3*i+2] = (unsigned char)(data[i * len / W]);
	vidline[3*W/2] = 255;
	vidline[3*W/2+1] = 0;
	vidline[3*W/2+2] = 0;
	if (dir) {
		if (linecnt == H) {
			linecnt--;
			unsigned char *p = &vidbuf[3*W];
			memmove (vidbuf, p, 3*(W * (H-1))*sizeof(unsigned char));
			memcpy (&vidbuf[3*W*(H-1)], vidline, 3*W * sizeof (unsigned char));
		}
		else
			memcpy (&vidbuf[3*W*linecnt], vidline, 3*W * sizeof(unsigned char));
		linecnt++;
	} else {
		unsigned char *p = &vidbuf[3*W];
		memmove (p, vidbuf, 3 * (W * (H-1)) * sizeof(unsigned char));
		memcpy(vidbuf, vidline, 3 * W * sizeof(unsigned char));
	}

	REQ_DROP(&Digiscope::redraw, this);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void Digiscope::zdata(cmplx *zarray, int len )
{
	if (active_modem->HistoryON()) return;
	
	if (zarray == NULL || len == 0)
		return;
	
	FL_LOCK_D();
	for (int i = 0; i < len; i++) {
		_zdata[_zptr++] = zarray[i];
		if (_zptr == MAX_ZLEN) _zptr = 0;
	}
	REQ_DROP(&Digiscope::redraw, this);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void Digiscope::data(double *data, int len, bool scale)
{
	if (active_modem->HistoryON()) return;
	
	if (data == 0) {
		memset(_buf, 0, MAX_LEN * sizeof(*_buf));
    	REQ_DROP(&Digiscope::redraw, this);
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
		if (max == min) max *= 1.001;
		for (int i = 0; i < _len; i++)
			_buf[i] = (_buf[i] - min) / (max - min);
	}
	REQ_DROP(&Digiscope::redraw, this);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void Digiscope::phase(double ph, double ql, bool hl)
{
	if (active_modem->HistoryON()) return;
	
	FL_LOCK_D();
	_phase = ph;
	_quality = ql;
	_highlight = hl;
	REQ_DROP(&Digiscope::redraw, this);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void Digiscope::rtty(double flo, double fhi, double amp)
{
	if (active_modem->HistoryON()) return;
	
	FL_LOCK_D();
	_flo = flo;
	_fhi = fhi;
	_amp = amp;
	REQ_DROP(&Digiscope::redraw, this);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}


void Digiscope::mode(scope_mode md)
{
	if (md == PHASE) {
		if (phase_mode >= PHASE1 && phase_mode <= PHASE3)
			md = phase_mode;
		else
			md = phase_mode = PHASE1;
	}
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
	REQ_DROP(&Digiscope::redraw, this);
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void Digiscope::draw_phase()
{
	// max number of shown vectors is first dimension
	static double pvecstack[8][2];
	static const size_t psz = sizeof(pvecstack)/sizeof(*pvecstack);
	static unsigned pszn = 0;

	fl_clip(x()+2,y()+2,w()-4,h()-4);
	fl_color(FL_BLACK);
	fl_rectf(x()+2,y()+2,w()-4,h()-4);
	fl_push_matrix();
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

	if (_highlight) {
		if (_mode > PHASE1) {
			if (pszn == psz - 1)
				memmove(pvecstack, pvecstack + 1, (psz - 1) * sizeof(*pvecstack));
			else
				pszn++;
			pvecstack[pszn][0] = _phase;
			pvecstack[pszn][1] = _quality;

			// draw the stack in progressively brighter green
			for (unsigned i = 0; i <= pszn; i++) {
//				fl_color(fl_color_average(FL_GREEN, FL_BLACK, 1.0 - 0.8 * (n-i)/ pszn));
				fl_color(fl_color_average(FL_GREEN, FL_BLACK, 0.2 + 0.8 * i / pszn));
				fl_begin_line();
				fl_vertex(0.0, 0.0);
				if (_mode == PHASE3) // scale length by quality
					fl_vertex(0.9 * cos(pvecstack[i][0] - M_PI / 2) * pvecstack[i][1],
						  0.9 * sin(pvecstack[i][0] - M_PI / 2) * pvecstack[i][1]);
				else
					fl_vertex(0.9 * cos(pvecstack[i][0] - M_PI / 2),
						  0.9 * sin(pvecstack[i][0] - M_PI / 2));
				fl_end_line();
			}
		}
		else { // original style
			fl_color(FL_GREEN);
			fl_begin_line();
                        fl_vertex(0.0, 0.0);
                        fl_vertex(0.9 * cos(_phase - M_PI / 2), 0.9 * sin( _phase - M_PI / 2));
			fl_end_line();
		}
	} else {
		fl_color(FL_GREEN);
		fl_circle( 0.0, 0.0, 0.1);
	}
	fl_pop_matrix();
	fl_pop_clip();
}

void Digiscope::draw_scope()
{
	int npts, np;
	fl_clip(x()+2,y()+2,w()-4,h()-4);
	fl_color(FL_BLACK);
	fl_rectf(x()+2,y()+2,w()-4,h()-4);
	fl_push_matrix();
	npts = MIN(w(), _len);
	npts = MAX(1, npts);
	fl_translate(x()+2, y() + h() - 2);
	fl_scale ((w()-4), - (h() - 4));
	fl_color(FL_GREEN);
	fl_begin_line();
	for (int i = 0; i < npts; i++) {
		np = i * _len / npts;
		np = np < MAX_LEN ? np : MAX_LEN - 1;
		fl_vertex( (double)i / npts, _buf[np] );
	}
	fl_end_line();

// x & y axis'
	if (_x1) {
		fl_color(FL_WHITE);
		fl_begin_line();
			fl_vertex(_x1, 0.0);
			fl_vertex(_x1, 1.0);
		fl_end_line();
	}
	if (_x2) {
		fl_color(FL_YELLOW);
		fl_begin_line();
			fl_vertex(_x2, 0.0);
			fl_vertex(_x2, 1.0);
		fl_end_line();
	}
	if (_y1) {
		fl_color(FL_WHITE);
		fl_begin_line();
			fl_vertex(0.0, _y1);
			fl_vertex(1.0, _y1);
		fl_end_line();
	}
	if (_y2) {
		fl_color(FL_YELLOW);
		fl_begin_line();
			fl_vertex(0.0, _y2);
			fl_vertex(1.0, _y2);
		fl_end_line();
	}

	fl_pop_matrix();
	fl_pop_clip();
}


void Digiscope::draw_xy()
{
	fl_clip(x()+2,y()+2,w()-4,h()-4);
	fl_color(FL_BLACK);
	fl_rectf(x()+2,y()+2,w()-4,h()-4);
	fl_push_matrix();
	fl_translate(x() + w() / 2.0, y() + w() / 2.0);
	fl_scale( w()/2.0, -w()/2.0);
// x & y axis	
	fl_color(FL_LIGHT1);
	fl_begin_line();
		fl_vertex(-0.6, 0.0);
		fl_vertex(-1.0, 0.0);
	fl_end_line();
	fl_begin_line();
		fl_vertex(0.6, 0.0);
		fl_vertex(1.0, 0.0);
	fl_end_line();
	fl_begin_line();
		fl_vertex(0.0, -0.6);
		fl_vertex(0.0, -1.0);
	fl_end_line();
	fl_begin_line();
		fl_vertex(0.0, 0.6);
		fl_vertex(0.0, 1.0);
	fl_end_line();
// data
	int W = w() / 2;
	int H = h() / 2;
	int X = x();
	int Y = y();
	int xp, yp, xp1, yp1;
	int j = _zptr;
	if (++j == MAX_ZLEN) j = 0;
	xp = X + (int)((_zdata[j].real() + 1.0) * W);
	yp = Y + (int)((_zdata[j].imag() + 1.0) * H);

	fl_color(fl_rgb_color(0, 230,0));
	for (int i = 0; i <  MAX_ZLEN; i++ ) {
		if (++j == MAX_ZLEN) j = 0;
		xp1 = X + (int)((_zdata[j].real() + 1.0) * W);
		yp1 = Y + (int)((_zdata[j].imag() + 1.0) * H);
		fl_line(xp, yp, xp1, yp1);
		xp = xp1; yp = yp1;
	}

	fl_pop_matrix();
	fl_pop_clip();
}

void Digiscope::draw_rtty()
{
	int npts, np;
	fl_clip(x()+2,y()+2,w()-4,h()-4);
	fl_color(FL_BLACK);
	fl_rectf(x()+2,y()+2,w()-4,h()-4);
	fl_push_matrix();
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
	for (int i = 0; i < npts; i++) {
		np = i * _len / npts;
		np = np < MAX_LEN ? np : MAX_LEN - 1;
		fl_vertex( (double)i / npts, 0.5 + 0.75 * _buf[np] );
	}
	fl_end_line();
	fl_pop_matrix();
	fl_pop_clip();
}

void Digiscope::draw_video()
{
	fl_draw_image(
		vidbuf, 
		x() + 2, y() + 2, 
		w() - 4, h() - 4);
}

void Digiscope::draw()
{
	draw_box();
	if (_mode == WWV || _mode == DOMWF)
		draw_video();
	else {
		switch (_mode) {
			case SCOPE :	draw_scope(); break;
			case PHASE1:
			case PHASE2:
			case PHASE3:	draw_phase(); break;
			case RTTY :		draw_rtty(); break;
			case XHAIRS :	draw_xy(); break;
			case DOMDATA :	draw_scope(); break;
			case BLANK : 
			default: 
				fl_clip(x()+2,y()+2,w()-4,h()-4);
				fl_color(FL_BLACK);
				fl_rectf(x()+2,y()+2,w()-4,h()-4);
				fl_push_matrix();
				fl_pop_matrix();
				fl_pop_clip();
				break;
		}
	}
}

int Digiscope::handle(int event)
{
	if (!Fl::event_inside(this))
		return 0;

	switch (event) {
	case FL_RELEASE:
		switch (_mode) {
		case PHASE1: case PHASE2:
			_mode = (scope_mode)((int)_mode + 1);
			phase_mode = _mode;
			redraw();
			break;
		case PHASE3:
			_mode = PHASE1;
			phase_mode = _mode;
			redraw();
			break;
		case RTTY:
			_mode = XHAIRS;
			redraw();
			break;
		case XHAIRS:
			_mode = RTTY;
			redraw();
			break;
		case DOMDATA:
			_mode = DOMWF;
			redraw();
			break;
		case DOMWF:
			_mode = DOMDATA;
			redraw();
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
		if ((event = Fl::event_dy()) || (event = Fl::event_dx()))
			wf->handle_mouse_wheel(waterfall::WF_AFC_BW, event);
		break;
	default:
		break;
	}

	return 1;
}

void Digiscope::resize(int x, int y, int w, int h)
{
	delete [] vidbuf;
	delete [] vidline;
	vidbuf = new unsigned char[ 3 * (w-4) * (h-4)];
	vidline = new unsigned char[ 3 * (w-4)];

	Fl_Widget::resize(x, y, w, h);
	mode(_mode);
}
