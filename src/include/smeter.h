//
// smeter.h
//
//  Smeter bar widget routines.
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


#ifndef SMETER
#define SMETER

//
// Include necessary headers.
//

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>

//
// Smeter class...
//

class Smeter : public Fl_Widget
{
private:
	double	value_,
			minimum_,
			maximum_;
	int		sval;			// Size of sval bar...
	int		bx, by, bw, bh;	// Box areas...
	int		tx, tw;			// Temporary X + width
	int		ty, th;			// Temporary Y + height
	int		sx;				// meter left offset
	int		meter_width;
	int		meter_height;
	Fl_Color bgnd_;
	Fl_Color fgnd_;
	Fl_Color scale_color;
	static const char *meter_face;

	void (*cbFunc)(Fl_Widget *, void *);

protected:

	virtual void draw();

public:

	Smeter(int x, int y, int w, int h, const char *l = 0);

	void	maximum(double v) { maximum_ = v; redraw(); }
	double	maximum() const { return (maximum_); }

	void	minimum(double v) { minimum_ = v; redraw(); }
	double	minimum() const { return (minimum_); }

	void	value(double v) { 
		value_ = v;
		if (value_ < minimum_) value_ = minimum_;
		if (value_ > maximum_) value_ = maximum_;
		redraw();
	}
	double	value() const { return (value_); }
	void	resize(int x, int y, int w, int h);
	int		handle(int);

	void	set_background(Fl_Color c1) { bgnd_ = c1; redraw(); }
	void	set_metercolor(Fl_Color c2) { fgnd_ = c2; redraw(); }
	void	set_scalecolor(Fl_Color c3) { scale_color = c3; redraw(); }

	void callback (void (*cbf)(Fl_Widget *, void *) ){ cbFunc = cbf;}
	void do_callback() { 
		if (cbFunc) cbFunc(this, (void*)0);
	}

};

#endif // !smeter

