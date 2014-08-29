//
// progress.h
//
//  Progress bar widget routines.
//
// Based on Fl_Progress widget, Copyright 2000-2005 by Michael Sweet.
//
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


#ifndef _Progress_H_
#  define _Progress_H_

//
// Include necessary headers.
//

#include <FL/Fl_Widget.H>

//
// Progress class...
//

class Progress : public Fl_Widget
{
public:
	enum PTYPE {HORIZONTAL, VERTICAL};
private:
	double	value_,
			minimum_,
			maximum_;
	PTYPE	direction;
	
protected:

	virtual void draw();

public:

	Progress(int x, int y, int w, int h, const char *l = 0);

	void	type(PTYPE direc) { direction = direc;}
	
	void	maximum(double v) { maximum_ = v; redraw(); }
	double	maximum() const { return (maximum_); }

	void	minimum(double v) { minimum_ = v; redraw(); }
	double	minimum() const { return (minimum_); }

	void	value(double v) { value_ = v; redraw(); }
	double	value() const { return (value_); }
};

#endif // !_Progress_H_

