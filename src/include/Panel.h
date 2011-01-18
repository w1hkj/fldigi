// ----------------------------------------------------------------------------
//      Panel_Check.h
//
// Copyright (C) 2011
//              Dave Freese, W1HKJ
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

#ifndef Panel_h_
#define Panel_h_

#include <FL/Fl_Group.H>

class Panel : public Fl_Group {
public:
	int handle(int);
	Panel(int X,int Y,int W,int H,const char*l=0) : Fl_Group(X,Y,W,H,l) {
		clip_children(true);
	}
	void resize(int, int, int, int);
	void position(int, int, int, int);
	int  orgx();
	int  orgy();
};

#endif
