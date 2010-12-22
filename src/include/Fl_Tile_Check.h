// ----------------------------------------------------------------------------
//      Fl_Tile_Check.h
//
// Copyright (C) 2007
//              Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
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

#ifndef Fl_Tile_Check_h_
#define Fl_Tile_Check_h_

#include <FL/Fl_Group.H>

class Tile_ : public Fl_Group {
public:
  int handle(int);
  Tile_(int X,int Y,int W,int H,const char*l=0) : Fl_Group(X,Y,W,H,l) {}
  void resize(int, int, int, int);
  void position(int, int, int, int);
  void newx(int);
};

/// A version of Fl_Tile that runs check callbacks and moves the boundary
/// between its child widgets only all resize checks return true.
class Fl_Tile_Check : public Tile_
{
public:
	typedef bool (*resize_check_func)(void *, int, int);

	Fl_Tile_Check(int x, int y, int w, int h, const char* l = 0);

	int handle(int event);
	void add_resize_check(resize_check_func f, void *a);
	void remove_resize_check(resize_check_func f, void *a);
	void remove_checks(void);
	bool do_checks(int xd, int yd);

protected:
	int xstart, ystart;
	resize_check_func resize_checks[8];
	void *resize_args[8];
};

#endif // Fl_Tile_Check_h_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
