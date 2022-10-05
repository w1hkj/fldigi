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
	color_vid = new RGB[width * height];

	_reverse = rv;

	box(FL_DOWN_BOX);
	marquee = false;
	_use_color = false;

	clear();
	col = 0;
}

Raster::~Raster()
{
	delete [] vidbuf;
	delete [] color_vid;
}

int Raster::change_rowheight( int rh )
{
	{
		guard_lock raster_lock(&raster_mutex);

		while ( (2*rh+space) > height) {
			rh--;
		}

		rowheight = 2 * rh;
		rhs = rowheight + space;
		Nrows = 0;

		while ((Nrows * rhs) < height) {
			Nrows++;
		}
		Nrows--;
	}

	clear();

	return rh;
}

void Raster::data(int data[], int len)
{
	{
		guard_lock raster_lock(&raster_mutex);

		if (data == NULL || len == 0 || (len > rowheight)) {
			return;
		}

		if (marquee) {
			for (int row = 0; row < Nrows; row++) {
				int rowstart = width * rhs * row;
				int nextrow = width * rhs * (row + 1);
				int pn = 0;
				for (int i = 0; i < len; i++) {
					pn = rowstart + i*width + width - 1;
					vidbuf[ pn ] = vidbuf[nextrow + i* width];
					color_vid[pn] = mag2RGB[ vidbuf[ pn ] ];
				}
			}
			int toppixel = width * (Nrows - 1) * rhs + width - 1;
			int pn = 0;
			for (int i = 0; i < len; i++) {
				pn = toppixel + width * (len - i);
				vidbuf[pn] = (unsigned char) data[i];
				color_vid[pn] = mag2RGB[ vidbuf[ pn ] ];
			}
		} else {
			int pos = 0;
			int zeropos = (Nrows - 1) * rhs * width;
			for (int i = 0; i < len; i++) {
				pos = zeropos + width * (len - i - 1) + col;
				vidbuf[pos] = (unsigned char)data[i];
				color_vid[pos] = mag2RGB[ vidbuf[ pos] ];
			}
			if (++col >= width) {
				unsigned char *from = vidbuf + rhs * width;
				int numtocopy = (Nrows -1) * rhs * width;

				memmove(vidbuf, from, numtocopy);
				for (int n = 0; n < numtocopy; n++)
					color_vid[ n ] = mag2RGB[ vidbuf[ n ] ];

				if (_use_color) {
					memset(vidbuf + zeropos, 0, rhs * width);
					for (int n = 0; n < width * height; n++)
						color_vid[n] = mag2RGB[ vidbuf[ n ] ];
				} else {
					if (_reverse)
						memset(vidbuf + zeropos, 0, rhs * width);
					else
						memset(vidbuf + zeropos, 255, rhs * width);
					}
				col = 0;
			}
		}
	}
	redraw();
}

void Raster::clear()
{
	{
		guard_lock raster_lock(&raster_mutex);

		if (_use_color) {
			memset(vidbuf, 0, width * height);
			for (int n = 0; n < width * height; n++)
				color_vid[n] = mag2RGB[ vidbuf[ n ] ];
		} else {
			if (_reverse)
				memset(vidbuf, 0, width * height);
			else
				memset(vidbuf, 255, width * height);
		}
		col = 0;
	}
	redraw();
}

void Raster::resize(int x, int y, int w, int h)
{
if (1) {
	{
		guard_lock raster_lock(&raster_mutex);

		if (w < 14) w = 14;
		if (h < (4 + rhs)) h = 4 + rhs;
		int Wdest = w - 4;
		int Hdest = h - 4;
		int Ndest = 0;
		while ((Ndest * rhs) < Hdest) Ndest++;
		Ndest--;

		unsigned char *tempbuf = new unsigned char [Wdest * Hdest];
		unsigned char *oldbuf = vidbuf;

		width = Wdest;
		height = Hdest;
		Nrows = Ndest;

		vidbuf = tempbuf;
		delete [] oldbuf;

		RGB *nurgb = new RGB[width * height];
		RGB *oldrgb = color_vid;

		color_vid = nurgb;
		delete [] oldrgb;

		Fl_Widget::resize(x,y,w,h);
	}
	clear();
	return;
} else {
	{
		guard_lock raster_lock(&raster_mutex);

		if (w < 14) w = 14;
		if (h < (4 + rhs)) h = 4 + rhs;
		int Wdest = w - 4;
		int Hdest = h - 4;
		int Ndest = 0;
		while ((Ndest * rhs) < Hdest) Ndest++;
		Ndest--;

		unsigned char *tempbuf = new unsigned char [Wdest * Hdest];
		unsigned char *oldbuf = vidbuf;

		if (_use_color) {
			memset(tempbuf, 0, Wdest * Hdest);
		} else {
			if (_reverse)
				memset(tempbuf, 0, Wdest * Hdest);
			else
				memset(tempbuf, 255, Wdest * Hdest);
		}
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

		RGB *nuRGB = new RGB[width * height];
		RGB *oldRGB = color_vid;
		color_vid = nuRGB;
		delete [] oldRGB;

		for (int n = 0; n < width * height; n++)
			color_vid[ n ] = mag2RGB[ vidbuf[ n ] ];

		Fl_Widget::resize(x,y,w,h);
		}
		redraw();
	}
}

void Raster::do_reverse()
{
	{
		guard_lock raster_lock(&raster_mutex);
		for (int n = 0; n < width * height; n++)
			vidbuf[n] = 255 - vidbuf[n];
		if (!_use_color)
			redraw();
	}
}

void Raster::draw()
{
	if (_use_color)
		fl_draw_image( (unsigned char *)color_vid, x() + 2, y() + 2, width, height );
	else
		fl_draw_image_mono( vidbuf, x() + 2, y() + 2, width, height, 1);
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

void Raster::set_colors(RGB *palette) // array of 9
{
	for (int n = 0; n < 8; n++) {
		for (int i = 0; i < 32; i++) {
			mag2RGB[i + 32*n].R = palette[n].R + (int)(1.0 * i * (palette[n+1].R - palette[n].R) / 32.0);
			mag2RGB[i + 32*n].G = palette[n].G + (int)(1.0 * i * (palette[n+1].G - palette[n].G) / 32.0);
			mag2RGB[i + 32*n].B = palette[n].B + (int)(1.0 * i * (palette[n+1].B - palette[n].B) / 32.0);
		}
	}
	redraw();
}
