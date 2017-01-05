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

#include "threads.h"

static pthread_mutex_t raster_mutex = PTHREAD_MUTEX_INITIALIZER;

using namespace std;

bool active = false;

Raster::Raster (int X, int Y, int W, int H, int rh, bool rv) :
	Fl_Widget (X, Y, W, H) {
	width = W - 4;
	height = H - 4;
	space = 1;
	rowheight = 2 * rh;
	rhs = rowheight + space;

	Nrows = 0;
	while ((Nrows * rhs) < height) Nrows++;
	Nrows--;

	vidbuf = new unsigned char[width * height];
	_reverse = rv;
	if (_reverse)
		memset(vidbuf, 0, width * height);
	else
		memset(vidbuf, 255, width * height);
	col = 0;

	box(FL_DOWN_BOX);
	marquee = false;
}

Raster::~Raster()
{
	delete [] vidbuf;
}

int Raster::change_rowheight( int rh )
{
	guard_lock raster_lock(&raster_mutex);

	while ( (2*rh+space) > height) rh--;

	rowheight = 2 * rh;
	rhs = rowheight + space;

	Nrows = 0;
	while ((Nrows * rhs) < height) Nrows++;
	Nrows--;

	if (_reverse)
		memset(vidbuf, 0, width * height);
	else
		memset(vidbuf, 255, width * height);
	col = 0;

	REQ_DROP(&Raster::redraw, this);

	return rh;
}

void Raster::data(int data[], int len)
{

	guard_lock raster_lock(&raster_mutex);

	if (data == NULL || len == 0 || (len > rowheight)) {
		return;
	}

	if (marquee) {
		for (int row = 0; row < Nrows; row++) {
			int rowstart = width * rhs * row;
			int nextrow = width * rhs * (row + 1);
			for (int i = 0; i < len; i++) {
				memmove(vidbuf + rowstart + i*width,
						vidbuf + rowstart + i*width + 1, 
						width - 1);
				if (row < (Nrows - 1)) {
					vidbuf[rowstart + i*width + width - 1] =
						vidbuf[nextrow + i* width];
				}
			}
		}
		int toppixel = width * (Nrows - 1) * rhs + width - 1;
		for (int i = 0; i < len; i++) {
			vidbuf[toppixel + width * (len - i)] = (unsigned char) data[i];
		}
	} else {
		int pos = 0;
		int zeropos = (Nrows - 1) * rhs * width;
		for (int i = 0; i < len; i++) {
			pos = zeropos + width * (len - i - 1) + col;
			vidbuf[pos] = (unsigned char)data[i];
		}
		if (++col >= width) {
			unsigned char *from = vidbuf + rhs * width;
			int numtocopy = (Nrows -1) * rhs * width;
			memmove(vidbuf, from, numtocopy);
			if (_reverse)
				memset(vidbuf + zeropos, 0, rhs * width);
			else
				memset(vidbuf + zeropos, 255, rhs * width);
			col = 0;
		}
	}

	REQ_DROP(&Raster::redraw, this);
}

void Raster::clear()
{
	guard_lock raster_lock(&raster_mutex);

	if (_reverse)
		memset(vidbuf, 0, width * height);
	else
		memset(vidbuf, 255, width * height);
	col = 0;

	REQ_DROP(&Raster::redraw, this);
}

void Raster::resize(int x, int y, int w, int h)
{
	guard_lock raster_lock(&raster_mutex);

	int Wdest = w - 4;
	int Hdest = h - 4;
	int Ndest = 0;
	while ((Ndest * rhs) < Hdest) Ndest++;
	Ndest--;

	unsigned char *tempbuf = new unsigned char [Wdest * Hdest];
	unsigned char *oldbuf = vidbuf;

	if (_reverse)
		memset(tempbuf, 0, Wdest * Hdest);
	else
		memset(tempbuf, 255, Wdest * Hdest);

	int Ato = Wdest * Hdest;
	int Afm = width * height;

	if (Ato >= Afm) {
	} else {
	}

	int xfrcols, xfrrows;
	int from, to;

	if (Wdest >= width)
		xfrcols = width;
	else
		xfrcols = Wdest;

	if (Ndest <= Nrows) {
		xfrrows = Ndest * rhs;
		from = (Nrows - Ndest) * rhs;
		to = 0;
		for (int r = 0; r < xfrrows; r++)
			for (int c = 0; c < xfrcols; c++)
				tempbuf[(to + r) * Wdest + c] = vidbuf[(from + r) * width + c];
	} else {
		xfrrows = Nrows * rhs;
		from = 0;
		to = (Ndest - Nrows) * rhs;
		for (int r = 0; r < xfrrows; r++)
			for (int c = 0; c < xfrcols; c++)
				tempbuf[(to + r) * Wdest + c] = vidbuf[(from + r) * width + c];
	}

	width = Wdest;
	height = Hdest;
	Nrows = Ndest;
	vidbuf = tempbuf;

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
