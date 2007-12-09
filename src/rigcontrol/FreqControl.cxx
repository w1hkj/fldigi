// "$Id: FreqControl.cpp,v  2006/02/26"
//
// Frequency Control Widget for the Fast Light Tool Kit (Fltk)
//
// Copyright 2005-2006, Dave Freese W1HKJ
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
// Please report all bugs and problems to "w1hkj@w1hkj.com".
//

#include <config.h>

#include "qrunner.h"

#include "FreqControl.h"
#include "rigdialog.h"

const char *cFreqControl::Label[10] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
	
void cFreqControl::IncFreq (int nbr) {
	long v = 1;
	v = val + mult[nbr];
	if (v <= maxVal) val = v;
	updatevalue();
	do_callback();
}

void cFreqControl::DecFreq (int nbr) {
	long v = 1;
	v = val - mult[nbr];
	if (v >= minVal)
 	  val = v;
	updatevalue();
	do_callback();
}

void cbSelectDigit (Fl_Widget *btn, void * nbr)
{
	Fl_Button *b = (Fl_Button *)btn;
	bool top = (Fl::event_y() < b->y() + b->h()/2);
	int Nbr = (int)(reinterpret_cast<long> (nbr));
	
	cFreqControl *fc = (cFreqControl *)b->parent();
	if (top)
		fc->IncFreq(Nbr);
	else
		fc->DecFreq(Nbr);
	fc->damage();
}

cFreqControl::cFreqControl(int x, int y, int w, int h, const char *lbl):
			  Fl_Group(x,y,w,h,"") {
	ONCOLOR = FL_YELLOW;
	OFFCOLOR = FL_BLACK;
	SELCOLOR = fl_rgb_color(100, 100, 100);
	ILLUMCOLOR = FL_GREEN;
	val = 0;
	nD = 9; // nD <= MAXDIGITS

	int pw = 6; // decimal width
	int fcWidth = (w - pw - 4)/nD;
	int fcFirst = x;
	int fcTop = y;
	int fcHeight = h;
	long int max;
	int xpos;

	box(FL_DOWN_BOX);
	max = 1;
	for (int n = 0; n < nD; n++) {
		xpos = fcFirst + (nD - 1 - n) * fcWidth + 2;
		if (n < 3) xpos += pw;
		Digit[n] = new Fl_Repeat_Button (
			xpos,
			fcTop + 2,
			fcWidth,
			fcHeight-4,
			" ");
		Digit[n]->box(FL_FLAT_BOX);	
		Digit[n]->labelfont(FL_COURIER);
		Digit[n]->labelcolor(ONCOLOR);
		Digit[n]->color(OFFCOLOR, SELCOLOR);
		Digit[n]->labelsize(fcHeight);
		Digit[n]->callback(cbSelectDigit, (void *) n);
		mult[n] = max;
		max *= 10;
	}
	decbx = new Fl_Box(fcFirst + (nD - 3) * fcWidth + 2, fcTop + 2, pw, fcHeight-4,".");
	decbx->box(FL_FLAT_BOX);
	decbx->labelfont(FL_COURIER);
	decbx->labelcolor(ONCOLOR);
	decbx->color(OFFCOLOR);
	decbx->labelsize(fcHeight);
	
	cbFunc = NULL;
	maxVal = max * 10 - 1;
	minVal = 0;
	end();
}

cFreqControl::~cFreqControl()
{
	for (int i = 0; i < nD; i++) {
		delete Digit[i];
	}
}


void cFreqControl::updatevalue()
{
	long v = val;
	for (int n = 0; n < nD; n++) {
		Digit[n]->label(v == 0 ? " " : Label[v % 10]);
		v /= 10;
	}
	damage();
	if (adjFreq)
		adjFreq->value(val);
}

void cFreqControl::SetONOFFCOLOR( Fl_Color ONcolor, Fl_Color OFFcolor)
{
    OFFCOLOR = OFFcolor;
    ONCOLOR = ONcolor;

    for (int n = 0; n < nD; n++) {
		Digit[n]->labelcolor(ONCOLOR);
		Digit[n]->color(OFFCOLOR);
	}
	decbx->labelcolor(ONCOLOR);
	decbx->color(OFFCOLOR);
	damage();
}

void cFreqControl::SetONCOLOR (uchar r, uchar g, uchar b) 
{
	ONCOLOR = fl_rgb_color (r, g, b);
    for (int n = 0; n < nD; n++) {
		Digit[n]->labelcolor(ONCOLOR);
		Digit[n]->color(OFFCOLOR);
	}
	decbx->labelcolor(ONCOLOR);
	decbx->color(OFFCOLOR);
	damage();
}

void cFreqControl::SetOFFCOLOR (uchar r, uchar g, uchar b) 
{
	OFFCOLOR = fl_rgb_color (r, g, b);
    for (int n = 0; n < nD; n++) {
		Digit[n]->labelcolor(ONCOLOR);
		Digit[n]->color(OFFCOLOR);
	}
	decbx->labelcolor(ONCOLOR);
	decbx->color(OFFCOLOR);
	damage();
}

void cFreqControl::value(long lv)
{
  val = lv;
  QUEUE(&cFreqControl::updatevalue, this); //updatevalue();
}

int cFreqControl::handle(int event)
{
	if (!Fl::event_inside(this))
		return Fl_Group::handle(event);

	switch (event) {
		int d;
	case FL_KEYBOARD:
		switch (Fl::event_key()) {
		case FL_Left:
			d = -1;
			break;
		case FL_Down:
			d = -10;
			break;
		case FL_Right:
			d = 1;
			break;
		case FL_Up:
			d = 10;
			break;
		default:
			return 1;
		}
		val += d;
		updatevalue();
		do_callback();
		break;
	case FL_MOUSEWHEEL:
		if ( !((d = Fl::event_dy()) || (d = Fl::event_dx())) )
			return 1;

		for (int i = 0; i < nD; i++) {
			if (Fl::event_inside(Digit[i])) {
				d > 0 ? DecFreq(i) : IncFreq(i);
				break;
			}
		}
		break;
	case FL_PUSH:
		return Fl_Group::handle(event);
	}

	return 1;
}
