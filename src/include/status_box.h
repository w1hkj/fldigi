//
// status_box.h
//
//  status_box bar widget routines.
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


#ifndef _status_box_H
#define _status_box_H

//
// Include necessary headers.
//

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>

//
// status_box class...
//

class status_box : public Fl_Box {
private:
	void (*cbFunc)(Fl_Widget *, void *);
protected:
public:
	status_box(int x, int y, int w, int h, const char *label = "") 
		: Fl_Box(x,y,w,h,label)
		{ };
	int		handle(int e) {
		if (Fl::event_inside( this )) {
			if (e == FL_RELEASE) {
				do_callback();
				return 1;
			}
		}
		return 0;
	}
	void callback (void (*cbf)(Fl_Widget *, void *) ){ cbFunc = cbf;}
	void do_callback() { 
		if (cbFunc) cbFunc(this, (void*)0);
	}
};

#endif // !status_box

