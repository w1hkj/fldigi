// ----------------------------------------------------------------------------
// raster.h, Raster scan Widget for display of fuzzy modes
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
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

#ifndef _RASTER_H
#define _RASTER_H

#include <FL/Fl_Widget.H>

class Raster : public Fl_Widget { 
public:
private:
	unsigned char *vidbuf;
	int		width;
	int		height;
	int		col;
	int		row;
	int		Nrows;
	int		rowheight;
	int		space;
	int		vidpos;	 // column start position 
	int		numcols; // number of columns to redraw
	int		yp;
public:
	Raster(int, int, int, int);
	~Raster();
	void	draw();
	void	resize(int x, int y, int w, int h);
	unsigned char *buffer() { return vidbuf;}
	int		size() { return width * height;}
	void	data(int data[], int len);
	void	clear();
	void	show() { Fl_Widget::show();}
	void	hide() { Fl_Widget::hide();}
};

#endif
