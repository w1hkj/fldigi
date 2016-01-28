//
// pwrmeter.cxx
//
// PWRmeter bar widget routines.
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

#include "pwrmeter.h"

//
// pwrmeter is a pwrmeter bar widget based off Fl_Widget that shows a
// standard pwrmeter bar in horizontal format

void PWRmeter::draw()
{
	if (select_ == 4) select_auto();

	sval = round(meter_width * (value_) / (maximum_));
	if (sval > meter_width) sval = meter_width;
	if (sval < 0) sval = 0;

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

void PWRmeter::select_25W()
{
	maximum_ = 25;
	label(W25_face);
	fl_font(FL_HELVETICA, labelsize());
	meter_width = fl_width(W25_face);
	sx = (tw - meter_width) / 2 + fl_width("|") / 2;
	meter_width -= fl_width("|");
}

void PWRmeter::select_50W()
{
	maximum_ = 50;
	label(W50_face);
	fl_font(FL_HELVETICA, labelsize());
	meter_width = fl_width(W50_face);
	sx = (tw - meter_width) / 2 + fl_width("|") / 2;
	meter_width -= fl_width("|");
}

void PWRmeter::select_100W()
{
	maximum_ = 100;
	label(W100_face);
	fl_font(FL_HELVETICA, labelsize());
	meter_width = fl_width(W100_face);
	sx = (tw - meter_width) / 2 + fl_width("|") / 2;
	meter_width -= fl_width("|");
}

void PWRmeter::select_200W()
{
	maximum_ = 200;
	label(W200_face);
	fl_font(FL_HELVETICA, labelsize());
	meter_width = fl_width(W200_face);
	sx = (tw - meter_width) / 2 + fl_width("|") / 2;
	meter_width -= fl_width("|");
}

void PWRmeter::select_auto()
{
	if (value_ <= 25.0) {
		select_25W();
	} else if (value_ <= 50.0) {
		select_50W();
	} else if (value_ <= 100) {
		select_100W();
	} else {
		select_200W();
	}
	redraw();
}

void PWRmeter::select( int sel ) { 
	switch (sel) {
		case P25:
			select_25W();
			select_ = 0;
			break;
		case P50:
			select_50W();
			select_ = 1;
			break;
		case P100:
			select_100W();
			select_ = 2;
			break;
		case P200:
			select_200W();
			select_ = 3;
			break;
		case AUTO:
		default:
			select_auto();
			select_ = 4;
	}
	redraw();
}

const char * PWRmeter::W25_face   = "| : : : : | : : : : | : : : : | : : : : | : : : 25|";
const char * PWRmeter::W50_face   = "|    :    |    :    |    :    |    :    |    :  50|";
const char * PWRmeter::W100_face  = "|    |    |    |    |    |    |    |    |    | 100|";
const char * PWRmeter::W200_face  = "|     :     |     :      |      :     |     :  200|";

PWRmeter::PWRmeter(int X, int Y, int W, int H, const char* l)
: Fl_Widget(X, Y, W, H, "")
{
	align(FL_ALIGN_INSIDE);
	box(FL_DOWN_BOX);
	bgnd_ = FL_BACKGROUND2_COLOR;
	fgnd_ = FL_GREEN;
	scale_color = FL_BLACK;

	maximum_ = 100.0;
	value_ = 0.0;

	select_ = 2; // 100 W scale

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
	meter_width = fl_width(W100_face);
	while ((meter_width < tw) && (fl_height() < th)) {
		fsize++;
		fl_font(FL_HELVETICA, fsize);
		meter_width = fl_width(W100_face);
	}
	fsize--;
	fl_font(FL_HELVETICA, fsize);
	meter_width = fl_width(W100_face);

	meter_height = fl_height();
	label(W100_face);
	labelfont(FL_HELVETICA);
	labelsize(fsize);
	labelcolor(scale_color);

	sx = (tw - meter_width) / 2 + fl_width("|") / 2;

	meter_width -= fl_width("|");
}

void PWRmeter::resize(int X, int Y, int W, int H) {
	Fl_Widget::resize(X,Y,W,H);

	bx = Fl::box_dx(box());
	by = Fl::box_dy(box());
	bw = Fl::box_dw(box());
	bh = Fl::box_dh(box());

	tx = X + bx;
	tw = W - bw;
	ty = Y + by;
	th = H - bh;

	const char *face;
	switch (select_) {
		case P25:
			face = W25_face;
			break;
		case P50:
			face = W50_face;
			break;
		case P100:
			face = W100_face;
			break;
		case P200:
			face = W200_face;
			break;
		case AUTO:
		default:
			face = W25_face;
	}

	static int fsize = 6;
	fl_font(FL_HELVETICA, fsize);
	meter_width = fl_width(face);
	while ((meter_width < tw) && (fl_height() < th)) {
		fsize++;
		fl_font(FL_HELVETICA, fsize);
		meter_width = fl_width(face);
	}
	fsize--;
	fl_font(FL_HELVETICA, fsize);
	meter_width = fl_width(face);

	meter_height = fl_height();
	label(face);
	labelfont(FL_HELVETICA);
	labelsize(fsize);
	labelcolor(scale_color);

	sx = (tw - meter_width) / 2 + fl_width("|") / 2;
	meter_width -= fl_width("|");

}

int PWRmeter::handle(int event)
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
// End of PWRmeter.cxx
//
