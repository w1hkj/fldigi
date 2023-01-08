//
// vumeter.h
//
//  vumeter bar widget routines.
// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------


#ifndef _VUMETER_H
#define _VUMETER_H

//
// Include necessary headers.
//

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>

//
// vumeter class...
//

class vumeter : public Fl_Widget {
private:
	double	value_,
			minimum_,
			maximum_,
			peakv_,
			peak_[10],
			vals_[10];
	int		avg_;
	int		aging_;

	Fl_Color bgnd_;
	Fl_Color fgnd_;
	Fl_Color scale_color;
	Fl_Color peak_color;

	static const char *meter_face_A;
	static const char *meter_face_B;
	const char *meter_face;

	void (*cbFunc)(Fl_Widget *, void *);

protected:

	virtual void draw();

public:

	vumeter(int x, int y, int w, int h, const char *label = "");

	void	maximum(double v) { maximum_ = v; redraw(); }
	double	maximum() const { return (maximum_); }

	void	minimum(double v) { minimum_ = v; redraw(); }
	double	minimum() const { return (minimum_); }

	void	value(double v);
	double	value();
	void	aging (int n);
	void	avg (int n);
	void	clear();

	void	set_background(Fl_Color c1) { bgnd_ = c1; redraw(); }
	void	set_metercolor(Fl_Color c2) { fgnd_ = c2; redraw(); }
	void	set_scalecolor(Fl_Color c3) { scale_color = c3; redraw(); }
	void	set_peakcolor(Fl_Color c4) { peak_color = c4; redraw(); }

	int		handle(int e) {
		if (Fl::event_inside( this )) {
			if (e == FL_RELEASE) {
				do_callback();
				return 1;
			}
		}
		return 0;
	}

	void callback (void (*cbf)(Fl_Widget *, void *) ){ cbFunc = cbf;}
	void do_callback() { 
		if (cbFunc) cbFunc(this, (void*)0);
	}

	void wsjtx_meter_face (bool on) {
		if (on) {
			meter_face = meter_face_B;
			minimum_ = 0.0;
			maximum_ = 90.0;
			value_ = 50;
		} else {
			meter_face = meter_face_A;
			minimum_ = -100.0;
			maximum_ = 0.0;
			value_ = -50;
		}
	}
};

#endif // !vumeter

