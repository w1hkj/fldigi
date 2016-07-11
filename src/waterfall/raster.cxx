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

#include <iostream>

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include "raster.h"
#include "modem.h"

#include <cmath>
#include <cstring>

#include "raster.h"
#include "qrunner.h"

using namespace std;

bool rowschanged = false;

Raster::Raster (int X, int Y, int W, int H) :
	Fl_Widget (X, Y, W, H) {
	width = W - 4;
	height = H - 4;
	space = 0;//1;
	rowheight = 2 * FELD_RX_COLUMN_LEN + space;

	Nrows = 0;
	while ((Nrows * rowheight) < height) Nrows++;
	Nrows--;

	vidbuf = new unsigned char[width * height];
	memset(vidbuf, 255, width * height);
	col = 0;
	box(FL_DOWN_BOX);
}

Raster::~Raster()
{
	delete [] vidbuf;
}


void Raster::data(int data[], int len)
{
	if (data == NULL || len == 0 || (len > rowheight))
		return;

	FL_LOCK_D();

	for (int row = 0; row < Nrows; row++) {
		int rowstart = width * rowheight * row;
		int nextrow = width * rowheight * (row + 1);
		for (int i = 0; i < len; i++) {
			memmove(	vidbuf + rowstart + i*width,
						vidbuf + rowstart + i*width + 1, 
						width - 1);
			if (row < (Nrows - 1)) {
				vidbuf[rowstart + i*width + width - 1] =
					vidbuf[nextrow + i* width];
			}
		}
	}
	int toppixel = width * (Nrows - 1) * rowheight + width - 1;
	for (int i = 0; i < len; i++) {
		vidbuf[toppixel + width * (len - i)] = (unsigned char) data[i];
	}

	REQ_DROP(&Raster::redraw, this);
	FL_UNLOCK_D();

	FL_AWAKE_D();
}

void Raster::clear()
{
	memset(vidbuf, 255, width * height);
//	redraw();
	REQ_DROP(&Raster::redraw, this);
	FL_AWAKE_D();
}

void Raster::resize(int x, int y, int w, int h)
{
	int Wdest = w - 4;
	int Hdest = h - 4;
	int Ndest;
	unsigned char *tempbuf = new unsigned char [Wdest * Hdest];
	unsigned char *oldbuf;
	int xfrcols, xfrrows;
	int from, to;

	Ndest = 0;
	while ((Ndest * rowheight) < Hdest) Ndest++;
	Ndest--;

	memset(tempbuf, 255, Wdest * Hdest);

	if (Wdest >= width)
		xfrcols = width;
	else
		xfrcols = Wdest;

	if (Ndest <= Nrows) {
		xfrrows = Ndest * rowheight;
		from = (Nrows - Ndest) * rowheight;
		to = 0;
		for (int r = 0; r < xfrrows; r++)
			for (int c = 0; c < xfrcols; c++)
				tempbuf[(to + r) * Wdest + c] = vidbuf[(from + r) * width + c];
	} else {
		xfrrows = Nrows * rowheight;
		from = 0;
		to = (Ndest - Nrows) * rowheight;
		for (int r = 0; r < xfrrows; r++)
			for (int c = 0; c < xfrcols; c++)
				tempbuf[(to + r) * Wdest + c] = vidbuf[(from + r) * width + c];
	}

	oldbuf = vidbuf;
	vidbuf = tempbuf;

	width = Wdest;
	height = Hdest;
	Nrows = Ndest;

	delete [] oldbuf;

	Fl_Widget::resize(x,y,w,h);

	redraw();
}

void Raster::draw()
{
	draw_box();
	fl_draw_image_mono(
		vidbuf, 
		x() + 2, y() + 2, 
		width, height, 1);
}

int Raster::handle(int event)
{
	if (Fl::event_inside( this )) {
		if (event == FL_PUSH) {
			if (Fl::event_button() == FL_RIGHT_MOUSE) {
				clear();
				return 1;
			}
		}
	}
	return Fl_Widget::handle(event);
}
