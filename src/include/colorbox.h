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

#ifndef COLORBOX_H
#define COLORBOX_H

#include <FL/Fl_Button.H>

#include "waterfall.h"

extern void loadPalette();
extern void savePalette();
extern void selectColor(int);
extern void setColorButtons();

extern void selectFHColor(int);
extern void loadFHPalette();
extern void saveFHPalette();
extern void setFH_ColorButtons();

extern RGB FHpalette[];

class colorbox : public Fl_Button  {
	RGB mag[256];
	void draw();
public:
	colorbox(int x, int y, int w, int h, const char *label = 0) : Fl_Button(x,y,w,h,label) {
		Fl_Button::box(FL_DOWN_BOX);
		for (int i = 0; i < 256; i++) mag[i].R = mag[i].G = mag[i].B = 0;
	};
	void mag_RGBcolors(RGB *rgb);
	void mag_RGBIcolors(RGBI *rgbi);
	void palette_to_mag(RGB *pal);

	void end(){};
};

#endif

