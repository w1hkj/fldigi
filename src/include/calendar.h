/* -*-C++-*- 

   "$Id: Fl_Calendar.H,v 1.4 2000/02/13 04:43:56 jamespalmer Exp $"
   
   Copyright 1999-2000 by the Flek development team.
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.
   
   Please report all bugs and problems to "flek-devel@sourceforge.net".

*/

#ifndef _FL_CALENDAR_H
#define _FL_CALENDAR_H

#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>

#include "flinput2.h"
#include "date.h"

class Fl_Calendar_Base : public Fl_Group , public Date
{
 private:

 protected:

  int cal_x;
  int cal_y;
  int cal_w;
  int cal_h;

  Fl_Button * days[6*7];

public:
  Fl_Widget *target;
  int calfmt;

  /**
   * The constructor for an empty Fl_Calendar_Base.
   */
  Fl_Calendar_Base (int x, int y, int w = (7*20), int h = (6*20),   
		    const char *l = 0);

  Fl_Button * day_button (int i);
  
  void update ();
  void csize (int cx, int cy, int cw, int ch);
  void setTarget (Fl_Widget *tgt);
};

class Fl_Calendar : public Fl_Calendar_Base 
{
public:
  /**
   * The constructor for an empty Fl_Calendar.
   */
  Fl_Calendar (int x, int y, int w = (7*20), int h = (8*20), 
  		    const char *l = 0);
 
  void today ();
  void previous_month ();
  void next_month ();
  void previous_year ();
  void next_year ();
  void setDate (int, int, int);
  
  void update ();
  void csize (int cx, int cy, int cw, int ch);
  int  handle (int);

protected:
//  Fl_Button * weekdays[7];
//  Fl_Button * caption;
  Fl_Box * weekdays[7];
  Fl_Box * caption;
  Fl_Button * nxt_month;
  Fl_Button * prv_month;
  Fl_Button * nxt_year;
  Fl_Button * prv_year;
};

class Fl_PopCal : public Fl_Window {

  friend void popcal_cb(Fl_Widget *, long);

  protected:
    int popcalfmt_;
//    Fl_Window popcal_form;
    Fl_Calendar *popcal;
    Fl_Input2 *target;
  public: 
    Fl_PopCal (int x, int y, int w, int h, Fl_Input2 *inp = 0);
    ~Fl_PopCal ();
    void popposition (int, int);
    void popshow ();
    void pophide ();
    void popcal_cb_i (Fl_Widget *, long);
    void popcalfmt (int);
    int  popcalfmt ();
    int  handle (int);   
    void setDate (int, int, int);
};

class Fl_DateInput : public Fl_Group  {
  
  protected:
    Fl_Button  *Btn;
    Fl_Input2   *Input;
    Fl_PopCal  *Cal;
    
    Fl_Window *popcal_form;
    Fl_Calendar *popcal;
    int popcalfmt_;

    void makepopcal();

  public:
    Fl_DateInput (int x, int y, int w, int h, const char * = 0);

    void format (int);
    const char *value ();
    void value (const char *);
    void color (Fl_Color);
    void textfont (int);
    void textsize (int);
    void textcolor (Fl_Color);
    void labelfont (int);
    void labelsize (int);
    void labelcolor (int);
    void align (Fl_Align);
    void fl_popcal();
    void take_focus();

};


#endif
