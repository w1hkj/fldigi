//
// smeter.cxx
//
// Smeter bar widget routines.
//
// A part of the fldigi.
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
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>
#include <cmath>

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include "smeter.h"

//
// smeter is a smeter bar widget based off Fl_Widget that shows a
// standard smeter bar in horizontal format

void Smeter::draw()
{
	if (maximum_ > minimum_)
		sval = round(meter_width * (value_ - minimum_) / (maximum_ - minimum_));
	else
		sval = 0;

// Draw the box and label...
	draw_box();
	draw_box(box(), tx, ty, tw, th, bgnd_);
	if (sval > 0)
		draw_box(FL_FLAT_BOX,
			tx + sx, ty + 2,
			sval, 
			th - 4,
			fgnd_);
	labelcolor(scale_color);
	draw_label();
}

const char * Smeter::meter_face = "|  :  : S3 :  : S6 :  : S9  ::  20  ::  40  ::   |";

Smeter::Smeter(int X, int Y, int W, int H, const char* l)
: Fl_Widget(X, Y, W, H, "")
{
	align(FL_ALIGN_INSIDE);
	box(FL_DOWN_BOX);
	bgnd_ = FL_BACKGROUND2_COLOR;
	fgnd_ = FL_GREEN;
	scale_color = FL_BLACK;

	minimum_ = 0.0;
	maximum_ = 100.0;
	value_ = 0;

  // Get the box borders...
	bx = Fl::box_dx(box());
	by = Fl::box_dy(box());
	bw = Fl::box_dw(box());
	bh = Fl::box_dh(box());

	tx = X + bx;
	tw = W - bw;
	ty = Y + by;
	th = H - bh;

	static int fsize = 6;
	fl_font(FL_HELVETICA, fsize);
	meter_width = fl_width(meter_face);
	while ((meter_width < tw) && (fl_height() < th)) {
		fsize++;
		fl_font(FL_HELVETICA, fsize);
		meter_width = fl_width(meter_face);
	}
	fsize--;
	fl_font(FL_HELVETICA, fsize);
	meter_width = fl_width(meter_face);
	meter_height = fl_height();
	label(meter_face);
	labelfont(FL_HELVETICA);
	labelsize(fsize);
	labelcolor(scale_color);

	meter_width -= fl_width("|");
	sx = (tw - meter_width) / 2;
}

void Smeter::resize(int X, int Y, int W, int H) {
	Fl_Widget::resize(X,Y,W,H);

	bx = Fl::box_dx(box());
	by = Fl::box_dy(box());
	bw = Fl::box_dw(box());
	bh = Fl::box_dh(box());

	tx = X + bx;
	tw = W - bw;
	ty = Y + by;
	th = H - bh;

	static int fsize = 6;
	fl_font(FL_HELVETICA, fsize);
	meter_width = fl_width(meter_face);
	while ((meter_width < tw) && (fl_height() < th)) {
		fsize++;
		fl_font(FL_HELVETICA, fsize);
		meter_width = fl_width(meter_face);
	}
	fsize--;
	fl_font(FL_HELVETICA, fsize);
	meter_width = fl_width(meter_face);
	meter_height = fl_height();
	label(meter_face);
	labelfont(FL_HELVETICA);
	labelsize(fsize);
	labelcolor(scale_color);

	meter_width -= fl_width("|");
	sx = (tw - meter_width) / 2;
}

int Smeter::handle(int event)
{
	if (Fl::event_inside( this )) {
		if (event == FL_RELEASE) {
			do_callback();
			return 1;
		}
	}
	return 0;
}


//
// End of Smeter.cxx
//
