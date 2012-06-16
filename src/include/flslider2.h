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

#include "config.h"

#if FLDIGI_FLTK_API_MAJOR == 1 && FLDIGI_FLTK_API_MINOR < 3
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
	Fl_Font textfont_;
	int textsize_;
	Fl_Color textcolor_;
	Fl_Color bkcolor_;
public:
	Fl_Counter2(int x, int y, int w, int h, const char* l = 0)
		: Fl_Counter(x, y, w, h, l) { }
	int handle(int event);
  /** Gets the text font */
  Fl_Font textfont() const {return textfont_;}
  /** Sets the text font to \p s */
  void textfont(Fl_Font s) {textfont_ = s;}

  /** Gets the font size */
  int  textsize() const {return textsize_;}
  /** Sets the font size to \p s */
  void textsize(int s) {textsize_ = s;}

  /** Gets the font color */
  Fl_Color textcolor() const {return textcolor_;}
  /** Sets the font color to \p s */
  void textcolor(Fl_Color s) {textcolor_ = s;}
  
  void textbkcolor(Fl_Color c) {bkcolor_ = c;}
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


#else
// support for 1.3.0 style slider
#include <FL/Fl_Slider.H>
#include <FL/Fl_Value_Slider.H>
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

//======================================================================
// Fl_Counter with 
// improved event handling,
// color rendering
//======================================================================

// values for type():
#define FL_NORMAL_COUNTER	0	/**< type() for counter with fast buttons */
#define FL_SIMPLE_COUNTER	1	/**< type() for counter without fast buttons */

/**
  Controls a single floating point value with button (or keyboard) arrows.
  Double arrows buttons achieve larger steps than simple arrows.
  \see Fl_Spinner for value input with vertical step arrows.
  <P align=center>\image html counter.png</P>
  \image latex counter.png "My_Counter" width=4cm

  \todo Refactor the doxygen comments for My_Counter type() documentation.

  The type of an My_Counter object can be set using type(uchar t) to:
  \li \c FL_NORMAL_COUNTER: Displays a counter with 4 arrow buttons.
  \li \c FL_SIMPLE_COUNTER: Displays a counter with only 2 arrow buttons.
*/
class FL_EXPORT My_Counter : public Fl_Valuator {

  Fl_Font textfont_;
  Fl_Fontsize textsize_;
  Fl_Color textcolor_;
  Fl_Color bkcolor_;
  double lstep_;
  uchar mouseobj;
  static void repeat_callback(void *);
  int calc_mouseobj();
  void increment_cb();

protected:

  void draw();

public:

  int handle(int);

  My_Counter(int X, int Y, int W, int H, const char* L = 0);
  ~My_Counter();

  /**
    Sets the increment for the large step buttons.
    The default value is 1.0.
    \param[in] a large step increment.
  */
  void lstep(double a) {lstep_ = a;}

  /**
    Sets the increments for the normal and large step buttons.
    \param[in] a, b normal and large step increments.
  */
  void step(double a,double b) {Fl_Valuator::step(a); lstep_ = b;}

  /**
    Sets the increment for the normal step buttons.
    \param[in] a normal step increment.
  */
  void step(double a) {Fl_Valuator::step(a);}

  /**
    Returns the increment for normal step buttons.
   */
  double step() const {return Fl_Valuator::step();}

  /** Gets the text font */
  Fl_Font textfont() const {return textfont_;}
  /** Sets the text font to \p s */
  void textfont(Fl_Font s) {textfont_ = s;}

  /** Gets the font size */
  Fl_Fontsize textsize() const {return textsize_;}
  /** Sets the font size to \p s */
  void textsize(Fl_Fontsize s) {textsize_ = s;}

  /** Gets the font color */
  Fl_Color textcolor() const {return textcolor_;}
  /** Sets the font color to \p s */
  void textcolor(Fl_Color s) {textcolor_ = s;}
  
  void textbkcolor(Fl_Color c) {bkcolor_ = c;}

};


class Fl_Counter2 : public My_Counter
{
public:
	Fl_Counter2(int x, int y, int w, int h, const char* l = 0)
		: My_Counter(x, y, w, h, l) { }
	int handle(int event);
};

#endif // fltk-1.3.0 style slider

#endif // FL_SLIDER2_
