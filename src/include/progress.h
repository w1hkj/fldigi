//
// progress.h
//
//  Progress bar widget routines.
//
// Based on Fl_Progress widget, Copyright 2000-2005 by Michael Sweet.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

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

