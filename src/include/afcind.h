//
// afcind.h
//
// AFC indicator widget routines.
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

#ifndef _AFCind_H_
#  define _AFCind_H_

//
// Include necessary headers.
//

#include <FL/Fl_Widget.H>

//
// AFCind class...
//

class AFCind : public Fl_Widget
{
public:
	enum PTYPE {HORIZONTAL, VERTICAL};
private:
	double	value_, range_;
	PTYPE	direction;
	
protected:

	virtual void draw();

public:

	AFCind(int x, int y, int w, int h, const char *l = 0);

	void	type(PTYPE direc) { direction = direc;}
	
	void	range(double v) { range_ = v; redraw(); }

	void	value(double v) { value_ = v; redraw(); }
};

#endif // !_AFCind_H_

