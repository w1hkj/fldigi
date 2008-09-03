/* -*-C++-*-

   "$Id: Fl_Combobox.H,v 1.4 2000/02/13 04:43:56 dhfreese Exp $"
   
   Copyright 1999-2000 by the Dave Freese.
   
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

#ifndef _FL_COMBOBOX_H
#define _FL_COMBOBOX_H

#include <FL/Fl.H>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Pixmap.H>

#define FL_COMBO_UNIQUE 1
#define FL_COMBO_UNIQUE_NOCASE 2
#define FL_COMBO_LIST_INCR 100

class Fl_ComboBox;

struct datambr {
  char *s;
  void *d;
};

struct retvals {
//  Fl_Output * Inp;
  Fl_Input *Inp;
  void     * retval;
  int      * idx;};

class Fl_PopBrowser : public Fl_Window {

  friend void popbrwsr_cb(Fl_Widget *, long);

  protected:
    Fl_Select_Browser *popbrwsr;
    retvals  Rvals;
    int hRow;
    int wRow;
  public: 
    Fl_PopBrowser (int x, int y, int w, int h, retvals R);
    ~Fl_PopBrowser ();
    void popshow (int, int);
    void pophide ();
    void popbrwsr_cb_i (Fl_Widget *, long);

    void add (char *s, void *d = 0);
    void clear ();
    void sort ();
    int  handle (int);

    Fl_ComboBox *parent;

};

class Fl_ComboBox : public Fl_Group  {
  friend int DataCompare (const void *, const void *);
  friend class Fl_PopBrowser;
  
  protected:
    Fl_Button     *Btn;
//    Fl_Output     *Output;
    Fl_Input      *Output;
    Fl_PopBrowser *Brwsr;
    datambr       **datalist;
    int           listsize;
    int           maxsize;
    int           listtype;

  private:
    int width;
    int height;
    void *retdata;
    int  idx;
    retvals R;

  public:

    Fl_ComboBox (int x, int y, int w, int h, const char * = 0);

    const char *value ();
    void value (const char *);
    void put_value( const char *);
    void fl_popbrwsr(Fl_Widget *);

    void type (int = 0);
    void add (const char *s, void *d = 0);
    void clear ();
    void sort ();
    int  index ();
	void index (int i);
    void *data ();
    void textfont (int);
    void textsize (uchar);
    void readonly();

};


#endif
