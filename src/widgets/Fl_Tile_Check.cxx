// ----------------------------------------------------------------------------
// Fl_Tile_Check.cxx
//
// Copyright (C) 2007-2009
//		Stelios Bounanos, M0GLD
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

#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Tile.H>

#include "Fl_Tile_Check.h"

Fl_Tile_Check::Fl_Tile_Check(int x, int y, int w, int h, const char* l)
	: Fl_Tile(x, y, w, h, l)
{
	remove_checks();
}

int Fl_Tile_Check::handle(int event)
{
	switch (event) {
	case FL_DRAG:
		return 1;
	case FL_RELEASE:
		if (!do_checks(Fl::event_x() - xstart, Fl::event_y() - ystart))
			return 1;
		// fall through to reset [xy]start
	case FL_PUSH:
		xstart = Fl::event_x();
		ystart = Fl::event_y();
	}

	return Fl_Tile::handle(event);
}

void Fl_Tile_Check::add_resize_check(resize_check_func f, void *a)
{
	for (size_t i = 0; i < sizeof(resize_checks) / sizeof(resize_checks[0]); i++) {
		if (resize_checks[i] == 0) {
			resize_checks[i] = f;
			resize_args[i] = a;
			break;
		}
	}
}
void Fl_Tile_Check::remove_resize_check(resize_check_func f, void *a)
{
	for (size_t i = 0; i < sizeof(resize_checks) / sizeof(resize_checks[0]); i++)
		if (resize_checks[i] == f && resize_args[i] == a)
			resize_checks[i] = 0;
}
void Fl_Tile_Check::remove_checks(void)
{
	for (size_t i = 0; i < sizeof(resize_checks) / sizeof(resize_checks[0]); i++) {
		resize_checks[i] = 0;
		resize_args[i] = 0;
	}
}
bool Fl_Tile_Check::do_checks(int xd, int yd)
{
	for (size_t i = 0; i < sizeof(resize_checks) / sizeof(resize_checks[0]); i++)
		if (resize_checks[i] && !resize_checks[i](resize_args[i], xd, yd))
			return false;
	return true;
}
