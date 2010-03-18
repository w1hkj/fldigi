// ----------------------------------------------------------------------------
//      flslider2.h
//
// Copyright (C) 2010
//              Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
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

#ifndef FL_SLIDER2_
#define FL_SLIDER2_

#include <FL/Fl_Slider.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Spinner.H>

class Fl_Slider2 : public Fl_Slider
{
public:
	Fl_Slider2(int x, int y, int w, int h, const char* l = 0)
		: Fl_Slider(x, y, w, h, l) { }
	int handle(int event);
};

class Fl_Value_Slider2 : public Fl_Value_Slider
{
public:
	Fl_Value_Slider2(int x, int y, int w, int h, const char* l = 0)
		: Fl_Value_Slider(x, y, w, h, l) { }
	int handle(int event);
};

class Fl_Counter2 : public Fl_Counter
{
public:
	Fl_Counter2(int x, int y, int w, int h, const char* l = 0)
		: Fl_Counter(x, y, w, h, l) { }
	int handle(int event);
};

class Fl_Value_Input2 : public Fl_Value_Input
{
public:
	Fl_Value_Input2(int x, int y, int w, int h, const char* l = 0)
		: Fl_Value_Input(x, y, w, h, l) { }
	int handle(int event);
};

class Fl_Spinner2 : public Fl_Spinner
{
public:
	Fl_Spinner2(int x, int y, int w, int h, const char* l = 0)
		: Fl_Spinner(x, y, w, h, l) { }
	int handle(int event);
};

#endif // FL_SLIDER2_
