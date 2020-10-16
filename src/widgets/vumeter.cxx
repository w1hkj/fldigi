//
// vumeter.cxx
//
// vumeter bar widget routines.
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

#include <iostream>
#include <config.h>
#include <cmath>

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include "vumeter.h"

#define min(a,b) ((a) <= (b) ? (a) : (b) )
#define max(a,b) ((a) >= (b) ? (a) : (b) )

//
// vumeter is a vumeter bar widget based off Fl_Widget that shows a
// standard vumeter bar in horizontal format

const char * vumeter::meter_face = "|.-120.-110.-100..-90..-80..-70..-60..-50..-40..-30..-20..-10...|";
void vumeter::draw()
{
	int	bx, by, bw;//, bh;	// Box areas...
	int	tx, tw, th;		// Temporary X + width
	int meter_width, meter_height;

// Get the box borders...
	bx = Fl::box_dx(box());
	by = Fl::box_dy(box());
	bw = Fl::box_dw(box());
//	bh = Fl::box_dh(box());
// Defne the inner box
	tx = x() + bx;
	tw = w() - bw;
	th = h() - 2 * by - 2;//bh;
// Determine optimal meter face height
	int fsize = 1;
	fl_font(FL_COURIER, fsize);
	meter_height = fl_height();
	while (meter_height < th) {
		fsize++;
		fsize++;
		fl_font(FL_COURIER, fsize);
		meter_height = fl_height();
	}
	fsize--;
	fl_font(FL_COURIER, fsize);
	meter_height = fl_height();
// Find visible scale
	const char *meter = meter_face;
	minimum_ = -130;
	maximum_ = 0;

	meter_width = fl_width(meter);
	while (meter_width > tw && *meter != 0) {
		meter++;
		meter_width = fl_width(meter);
		minimum_ += 2;
	}

	int mwidth = round(meter_width * (value_ - minimum_) / (maximum_ - minimum_));
	int PeakPos = round (meter_width * (peakv_ - minimum_) / (maximum_ - minimum_));

	mwidth = max ( min ( mwidth, meter_width), 0 );
	PeakPos = max ( min ( PeakPos, meter_width), 0 );

// Draw the box and label...
	fl_push_clip(x(), y(), w(), h());
	draw_box(box(), x(), y(), w(), h(), bgnd_);
	draw_box(FL_FLAT_BOX, tx, y() + by, tw, th, bgnd_);
	if (mwidth > 0) {
		draw_box(FL_FLAT_BOX,
			tx + (w() - meter_width) / 2, y() + by + (th - meter_height) / 2 + 1,
			mwidth, 
			meter_height,
			fgnd_);
		draw_box(FL_FLAT_BOX,
			tx + (w() - meter_width) / 2 + PeakPos, y() + by + (th - meter_height) / 2 + 1,
			2, 
			meter_height,
			peak_color);
	}
	label(meter);
	labelfont(FL_COURIER);
	labelsize(fsize);
	labelcolor(scale_color);
	draw_label();
	fl_pop_clip();
}

vumeter::vumeter(int X, int Y, int W, int H, const char *label)
: Fl_Widget(X, Y, W, H, "")
{
	align(FL_ALIGN_INSIDE);
	box(FL_DOWN_BOX);
	bgnd_ = FL_BACKGROUND2_COLOR;
	fgnd_ = FL_GREEN;
	peak_color  = FL_RED;
	scale_color = FL_BLACK;

	minimum_ = -100.0;
	maximum_ = 0.0;
	value_ = -50;
	avg_ = 10;
	aging_ = 10;
	clear();
	cbFunc = 0;
}

void vumeter::value(double v) {
	double vdb = 20 * log10(v == 0 ? 1e-9 : v);
//	if (vdb < minimum_) vdb = minimum_;
//	if (vdb > maximum_) vdb = maximum_;

	peakv_ = -100;
	for (int i = 1; i < aging_; i++) {
		peak_[i-1] = peak_[i];
		if (peakv_ < peak_[i])
			peakv_ = peak_[i];
	}
	peak_[aging_ - 1] = vdb;
	if (peakv_ < peak_[aging_ - 1])
		peakv_ = peak_[aging_ - 1];

	value_ -= vals_[0];
	for (int i = 1; i < avg_; i++)
		vals_[i-1] = vals_[i];
	value_ += (vals_[avg_- 1] = vdb / avg_); 

	redraw();
}

double vumeter::value()
{
	return (value_);
}

void vumeter::aging (int n)
{ 
	if (n <= 10 && n > 0) aging_ = n;
	else aging_ = 5;
	for (int i = 0; i < aging_; i++) peak_[i] = peakv_;
}

void vumeter::avg (int n)
{
	if (n <= 10 && n > 0) avg_ = n;
	else avg_ = 5;
	for (int i = 0; i < avg_; i++) vals_[i] = value_ / avg_;
}

void vumeter::clear ()
{
	for (int i = 0; i < 10; i++) {
		vals_[i] = peak_[i] = 0;
	}
	peakv_ = value_ = 0;
}

//
// End of vumeter.cxx
//
