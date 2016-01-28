//
// pwrmeter.h
//
//  PWRmeter bar widget routines.
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


#ifndef PWRMETER
#define PWRMETER

//
// Include necessary headers.
//

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>

//
// PWRmeter class...
//

class PWRmeter : public Fl_Widget
{
public:
enum {P25, P50, P100, P200, AUTO};

private:
	double	value_,
			maximum_;
	int		sval;			// Size of sval bar...
	int		bx, by, bw, bh;	// Box areas...
	int		tx, tw;			// Temporary X + width
	int		ty, th;			// Temporary Y + height
	int		sx;				// meter left offset
	int		meter_width;
	int		meter_height;
	int		select_;
	Fl_Color bgnd_;
	Fl_Color fgnd_;
	Fl_Color scale_color;
	static const char *W25_face;
	static const char *W50_face;
	static const char *W100_face;
	static const char *W200_face;

	void (*cbFunc)(Fl_Widget *, void *);

protected:
	virtual void draw();

public:
	PWRmeter(int x, int y, int w, int h, const char *l = 0);

	void	value(double v) { value_ = v; redraw(); }
	double	value() const { return (value_); }
	void	resize(int x, int y, int w, int h);
	int		handle(int);

	void	set_background(Fl_Color c1) { bgnd_ = c1; redraw(); }
	void	set_metercolor(Fl_Color c2) { fgnd_ = c2; redraw(); }
	void	set_scalecolor(Fl_Color c3) { scale_color = c3; redraw(); }

	void	select(int sel);

	void callback (void (*cbf)(Fl_Widget *, void *) ){ cbFunc = cbf;}
	void do_callback() { 
		if (cbFunc) cbFunc(this, (void*)0);
	}

private:
	void	select_auto();
	void	select_25W();
	void	select_50W();
	void	select_100W();
	void	select_200W();
};


#endif // !pwrmeter

