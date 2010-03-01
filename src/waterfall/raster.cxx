// ----------------------------------------------------------------------------
// raster.cxx, Raster scan Widget for display of fuzzy modes
//
// Copyright (C) 2006-2007
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

#include <config.h>

#include <FL/fl_draw.H>

#include "raster.h"
#include "modem.h"

#include <cmath>
#include <cstring>

#include "raster.h"
#include "qrunner.h"

bool rowschanged = false;

Raster::Raster (int X, int Y, int W, int H) :
	Fl_Widget (X, Y, W, H) {
	width = W - 4;
	height = H - 4;
	space = 2;
	rowheight = 60;
	Nrows = (int)(height / (rowheight + space) - 0.5);
	vidbuf = new unsigned char[width * height];
	memset(vidbuf, 255, width * height);
	col = 0;
	vidpos = 0;
	numcols = 0;
	yp = Nrows * (space + rowheight);
	box(FL_DOWN_BOX);
}

Raster::~Raster()
{
	delete [] vidbuf;
}


void Raster::data(int data[], int len)
{
	if (data == NULL || len == 0)
		return;
	int h = len;
	int pos;
	int zeropos;

	FL_LOCK_D();

	if (h > height) 
		h = height;
	col++;
	if (col >= width) {
		unsigned char *from = vidbuf + (space + rowheight) * width;
		int numtocopy = Nrows * (space + rowheight) * width;
		memmove(vidbuf, from, numtocopy);
		memset(	vidbuf + yp * width, 
				255, (space + rowheight) * width);
		col = 0;
		damage(FL_DAMAGE_USER2);
	}
	else
		if (damage() != FL_DAMAGE_ALL)
			damage(FL_DAMAGE_USER1);
	zeropos = Nrows * (space + rowheight) * width;
	for (int i = 0; i < h; i++) {
		pos = zeropos + width * (h - i - 1) + col;
		vidbuf[pos] = (unsigned char)data[i];
	}
	numcols++;
	vidpos = col - numcols;
	if (vidpos < 0) vidpos = 0;
	
//	redraw();
	REQ_DROP(&Raster::redraw, this);
	FL_UNLOCK_D();

	FL_AWAKE_D();
}

void Raster::clear()
{
	FL_LOCK_D();
	for (int i = 0; i < width * height; i++)
		vidbuf[i] = 255;
	col = width;
	REQ_DROP(&Raster::redraw, this);
//	redraw();
	FL_UNLOCK_D();
	FL_AWAKE_D();
}

void Raster::resize(int x, int y, int w, int h)
{
	int Wdest = w - 4, Hdest = h - 4;
	int Ndest = (int)(Hdest / (rowheight + space) - 0.5);
	unsigned char *tempbuf = new unsigned char [Wdest * Hdest];
	unsigned char *oldbuf;
	int xfrcols, xfrrows;
	int from, to;

	memset(tempbuf, 255, Wdest * Hdest);

	if (Wdest >= width)
		xfrcols = width;
	else
		xfrcols = Wdest;

	if (Ndest <= Nrows) {
		xfrrows = (Ndest + 1)*(rowheight + space);
		from = (Nrows - Ndest) * (rowheight + space);
		to = 0;
		for (int r = 0; r < xfrrows; r++)
			for (int c = 0; c < xfrcols; c++)
				tempbuf[(to + r) * Wdest + c] = vidbuf[(from + r) * width + c];
	} else {
		xfrrows = (Nrows + 1)*(rowheight + space);
		from = 0;
		to = (Ndest - Nrows) * (rowheight + space);
		for (int r = 0; r < xfrrows; r++)
			for (int c = 0; c < xfrcols; c++)
				tempbuf[(to + r) * Wdest + c] = vidbuf[(from + r) * width + c];
	}
	
	oldbuf = vidbuf;
	vidbuf = tempbuf;

	width = Wdest; height = Hdest;
	Nrows = Ndest;
	yp = Ndest * (space + rowheight);

	delete [] oldbuf;

	Fl_Widget::resize(x,y,w,h);

	redraw();
}

void Raster::draw()
{
	if ((damage() & FL_DAMAGE_USER2)) {
		draw_box();
		fl_draw_image_mono(
			vidbuf, 
			x() + 2, y() + 2, 
			width, height,
			1, width );
	} else if ((damage() & FL_DAMAGE_USER1)) {
		fl_draw_image_mono(
			vidbuf + vidpos + Nrows * (space + rowheight) * width,
			x() + vidpos + 2, y() + yp + 2,
			numcols, rowheight, 
			1, width);
		numcols = 0;
	} else {
		draw_box();
		fl_draw_image_mono(
			vidbuf, 
			x() + 2, y() + 2, 
			width, height,
			1, width );
	}
}

