//
// fl_disp_slider.h
//
// Slider header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
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

#ifndef fl_disp_slider_H
#define fl_disp_slider_H

#include <FL/Fl_Valuator.H>

// values for type(), lowest bit indicate horizontal:
#define FL_VERT_SLIDER		0
#define FL_HOR_SLIDER		1
#define FL_VERT_FILL_SLIDER	2
#define FL_HOR_FILL_SLIDER	3
#define FL_VERT_NICE_SLIDER	4
#define FL_HOR_NICE_SLIDER	5
#define FL_VERT_DISP_SLIDER 6
#define FL_HOR_DISP_SLIDER  7

class fl_disp_slider : public Fl_Valuator {

  float value_;
  float disp_value_;
  uchar slider_;
  void _fl_disp_slider();
  void draw_bg(int, int, int, int);

protected:

  // these allow subclasses to put the slider in a smaller area:
  void draw_(int, int, int, int);
  int handle_(int, int, int, int, int);

public:

  void draw();
  int handle(int);
  fl_disp_slider(int x,int y,int w,int h, const char *l = 0);
  fl_disp_slider(uchar t,int x,int y,int w,int h, const char *l);

//  int scrollvalue(int windowtop,int windowsize,int first,int totalsize);
  void bounds(double a, double b);
  float value() const {return value_;}
  void value(double v);
  void disp_value(double v) { 
	disp_value_ = v;
    damage(FL_DAMAGE_EXPOSE);
  };
  Fl_Boxtype slider() const {return (Fl_Boxtype)slider_;}
  void slider(Fl_Boxtype c) {slider_ = c;}
};

#endif

