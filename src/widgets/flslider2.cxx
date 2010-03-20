// ----------------------------------------------------------------------------
// flslider2.cxx
//
// Copyright (C) 2010
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <FL/Fl.H>
#include "flslider2.h"

inline static int handle_scroll(Fl_Valuator* w, int event)
{
	if (!(event == FL_MOUSEWHEEL && Fl::event_inside(w)))
		return 0;
	double d;
	if ((d = Fl::event_dy()) || (d = Fl::event_dx())) {
		if (Fl::event_state() & FL_SHIFT)
			d *= 10.0;
		if (!dynamic_cast<Fl_Value_Input*>(w) && !dynamic_cast<Fl_Counter*>(w) &&
		    !(w->type() & FL_HOR_SLIDER))
			d = -d;
		w->value(w->clamp(w->increment(w->value(), -d)));
		w->do_callback();
	}
	return 1;
}

int Fl_Slider2::handle(int event)
{
	return handle_scroll(this, event) ? 1 : Fl_Slider::handle(event);
}

int Fl_Value_Slider2::handle(int event)
{
	return handle_scroll(this, event) ? 1 : Fl_Value_Slider::handle(event);
}

int Fl_Counter2::handle(int event)
{
	return handle_scroll(this, event) ? 1 : Fl_Counter::handle(event);
}

int Fl_Value_Input2::handle(int event)
{
	return handle_scroll(this, event) ? 1 : Fl_Value_Input::handle(event);
}

inline static int handle_scroll(Fl_Spinner* w, int event)
{
	if (!(event == FL_MOUSEWHEEL && Fl::event_inside(w)))
		return 0;
	double d;
	if ((d = Fl::event_dy()) || (d = Fl::event_dx())) {
		if (Fl::event_state() & FL_SHIFT)
			d *= 10.0;
		d = w->value() - d * w->step();
		w->value(WCLAMP(d, w->minimum(), w->maximum()));
		w->do_callback();
	}
	return 1;
}

int Fl_Spinner2::handle(int event)
{
	return handle_scroll(this, event) ? 1 : Fl_Spinner::handle(event);
}
