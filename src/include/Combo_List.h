// $Id: Combo_List.h,v 1.6 2004/03/24 02:49:00 jbryan Exp $

/***************************************************************
 *                FLU - FLTK Utility Widgets 
 *  Copyright (C) 2002 Ohio Supercomputer Center, Ohio State University
 *
 * This file and its content is protected by a software license.
 * You should have received a copy of this license with this file.
 * If not, please contact the Ohio Supercomputer Center immediately:
 * Attn: Jason Bryan Re: FLU 1224 Kinnear Rd, Columbus, Ohio 43212
 * 
 ***************************************************************/



#ifndef _COMBO_LIST_H
#define _COMBO_LIST_H

#include <FL/Fl_Hold_Browser.H>

#include "Combo_Box.h"

//! Just like the Fl_Choice widget except the input area is editable
class Combo_List : public Combo_Box
{

public:

  //! Normal FLTK widget constructor
  Combo_List( int x, int y, int w, int h, const char *l = 0 );

  //! Default destructor
  ~Combo_List();

  //! Publicly exposed list widget (instance of Fl_Hold_Browser)
  Fl_Hold_Browser list;

 protected:

  bool _value( const char *v );
  const char* _next();
  const char* _previous();
  void _hilight( int x, int y );

  inline static void _cb( Fl_Widget *w, void *arg )
    { ((Combo_List*)arg)->cb(); }
  void cb();

};

#endif
