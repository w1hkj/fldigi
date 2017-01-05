// ----------------------------------------------------------------------------
// raster.h, Raster scan Widget for display of fuzzy modes
//
// Copyright (C) 2006-2008
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
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

#ifndef _RASTER_H
#define _RASTER_H

#include <FL/Fl_Widget.H>
#include "feld.h"

class Raster : public Fl_Widget {
public:
private:
	unsigned char *vidbuf;
	int		width;
	int		height;
	int		col;
	int		Nrows;
	int		rowheight;
	int		rhs;
	int		space;
	int		vidpos;	 // column start position
	int		numcols; // number of columns to redraw
	int		yp;
	bool	marquee;
	bool	_reverse;
public:
	Raster(int X, int Y, int W, int H, int rh = 2, bool rv = false);
	~Raster();
	void	draw();
	int		handle(int);
	void	resize(int x, int y, int w, int h);
	unsigned char *buffer() { return vidbuf;}
	int		size() { return width * height;}
	int		change_rowheight( int rh );
	void	data(int data[], int len);
	void	clear();
	void	show() { Fl_Widget::show();}
	void	hide() { Fl_Widget::hide();}
	void	set_marquee(bool val) { marquee = val; }
	bool	get_marquee() { return marquee; }
	void	reverse(bool val) { _reverse = val; }
};

#endif
