//
// afcind.cxx
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
//
// Include necessary header files...
//

#include <config.h>
#include <math.h>

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include "afcind.h"

//
// afcind is a center bar indicator widget that shows a center mark and 
// two colored ratio members that slide to the left (below) or right (above)
// the center mark.  Used in place of closing eye type of display for AFC.
// either horizontal or vertical format
//
// if direction == VERTICAL the indicator goes from lower to upper
// if direction == HORIZONTAL the indicator goes from left to right

void AFCind::draw()
{
	double dVal;
	int	AFCval;	// Size of AFCind bar...
	int	bx, by, bw, bh;	// Box areas...
	int	tx, tw;		// Temporary X + width
	int	ty, th;		// Temporary Y + height


  // Get the box borders...
	bx = Fl::box_dx(box());
	by = Fl::box_dy(box());
	bw = Fl::box_dw(box());
	bh = Fl::box_dh(box());

	tx = x() + bx;
	tw = w() - bw;
	ty = y() + by;
	th = h() - bh;  

	dVal = CLAMP( (value_ + range_) / (2.0 * range_), -1.0, 1.0);
	AFCval = (int)((direction == HORIZONTAL ? tw : th) * dVal);

// draw the center marker either to the left or above the primary display

	if (direction == HORIZONTAL) {
//clear the display && draw the scale

		fl_clip (x(), y(), w(), h());
		draw_box(box(), x(), y(), w(), h(), FL_BLACK);

		fl_rectf(tx + tw / 2 - 1, ty, 2, 6, 238,238,238);
		fl_rectf(tx, ty + 7, AFCval - 1, th - 7, 238,232,170);
		fl_rectf(tx + AFCval + 1, ty + 7, tw - AFCval - 1, th - 7, 124,205,124);

		fl_pop_clip();

	} else {
		fl_clip (x(), y(), w(), h());
		draw_box(box(), x(), y(), w(), h(), FL_BLACK);
		
		fl_rectf(tx, ty + th/2 - 1, 6, 2, 238,238,238);
		fl_rectf(tx + 7, ty + th - (AFCval - 1), tw - 7,  AFCval - 1, 238,232,170);
		fl_rectf(tx + 7, ty, tw - 7, AFCval - 1, 124,205,124);

		fl_pop_clip();
	}
}


AFCind::AFCind(int X, int Y, int W, int H, const char* l)
: Fl_Widget(X, Y, W, H, l)
{
  align(FL_ALIGN_INSIDE);
  box(FL_DOWN_BOX);
  range(1.0);
  value(0.0);
  direction = HORIZONTAL;
}

//
// End of "$Id: AFCind.cxx 4288 2005-04-16 00:13:17Z mike $".
//
