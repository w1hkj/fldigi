// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef _FL_COMBOBOX_H
#define _FL_COMBOBOX_H

#include <string>

#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Box.H>

#define FL_COMBO_UNIQUE 1
#define FL_COMBO_UNIQUE_NOCASE 2
#define FL_COMBO_LIST_INCR 100

class Fl_ComboBox;

enum {COMBOBOX, LISTBOX};

struct datambr {
  char *s;
  void *d;
};

class Fl_PopBrowser : public Fl_Window {

friend void popbrwsr_cb(Fl_Widget *, long);

	Fl_Select_Browser *popbrwsr;
	int hRow;
	int wRow;
	std::string keystrokes;

public:
	Fl_PopBrowser (int x, int y, int w, int h, const char *label);
	~Fl_PopBrowser ();
	void popshow (int, int);
	void pophide ();
	void popbrwsr_cb_i (Fl_Widget *, long);

	void add (char *s, void *d = 0);
	void clear ();
	void sort ();
	int  handle (int);
	void clear_kbd() { keystrokes.clear(); }

	Fl_ComboBox *parentCB;
	Fl_Widget *parentWindow;

};

class Fl_ComboBox : public Fl_Group  {
  friend int DataCompare (const void *, const void *);
  friend class Fl_PopBrowser;
  friend void val_callback(Fl_Widget *, void *);

	Fl_Button		*btn;
	Fl_Box			*valbox;
	Fl_Input		*val;
	Fl_PopBrowser	*Brwsr;
	datambr			**datalist;
	int				listsize;
	int				maxsize;
	int				listtype;
	int				numrows_;
	int				type_;

	int				width;
	int				height;
	void			*retdata;
	int				idx;
	Fl_Color		_color;
	void			insert(const char *, void *);

public:

	Fl_ComboBox (int x, int y, int w, int h, const char *lbl = 0,
		int wtype = COMBOBOX);
	~Fl_ComboBox();

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
	int  find_index(const char *str);
	void *data ();
	void textfont (int);
	void textsize (uchar);
	void textcolor (Fl_Color c);
	void color (Fl_Color c);
	void readonly(bool yes = true);
	int  numrows() { return numrows_; }
	void numrows(int n) { numrows_ = n; }
	int  lsize() { return listsize; }
	void set_focus() { Fl::focus(btn); };
	void labelfont(Fl_Font fnt) { Fl_Group::labelfont(fnt); }
	Fl_Font labelfont() { return Fl_Group::labelfont(); }
	void labelsize(Fl_Fontsize n) { Fl_Group::labelsize(n); }
	Fl_Fontsize labelsize() { return Fl_Group::labelsize(); }

	int handle(int);

// Custom resize behavior -- input stretches, button doesn't
	inline int val_x() { return x(); }
	inline int val_y() { return y(); }
	inline int val_w() { return w() - h(); }
	inline int val_h() { return h(); }

	inline int btn_x() { return x() + w() - h(); }
	inline int btn_y() { return y(); }
	inline int btn_w() { return h(); }
	inline int btn_h() { return h(); }

	void resize(int X, int Y, int W, int H) {
		Fl_Group::resize(X,Y,W,H);
		if (type_ == LISTBOX)
			valbox->resize(val_x(), val_y(), val_w(), val_h());
		else
			val->resize(val_x(), val_y(), val_w(), val_h());
		btn->resize(btn_x(), btn_y(), btn_w(), btn_h());
	}

};

class Fl_ListBox : public Fl_ComboBox {
public:
	Fl_ListBox (int x, int y, int w, int h, const char *lbl = 0) :
		Fl_ComboBox(x,y,w,h,lbl,LISTBOX) {};
	~Fl_ListBox() {};
};

#endif
