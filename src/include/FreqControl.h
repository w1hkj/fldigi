// ----------------------------------------------------------------------------
//
// Frequency Control Widget for the Fast Light Tool Kit
//
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of flrig.
//
// flrig is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// flrig is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------
//
// Usage:
//	Create a multi-digit receiver / transceiver frequency control widget
//
// label used to pass # digits & decimal position to control
// the widget can be used in Fluid & initialized with the
// number of digits as the label string
// default is 7; min number is 1, max number is 9 as in
//
// cFreqControl myFreqConrol(x0, y0, w0, h0, "N");  where N is # digits
// cFreqControl *pMyFreqControl = new cFreqControl(x0,y0,w0,h0,"N");

#ifndef _FREQCONTROL_H_
#define _FREQCONTROL_H_

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Repeat_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Enumerations.H>

#ifdef MAX_DIGITS
#undef MAX_DIGITS
#endif
#define MAX_DIGITS 10

#ifdef MIN_DIGITS
#undef MIN_DIGITS
#endif
#define MIN_DIGITS 4

class Fl_Box;
class Fl_Float_Input;

class cFreqControl : public Fl_Group {
friend void cbSelectDigit (Fl_Widget *btn, void * nbr);
public:
	cFreqControl(int x, int y, int w, int h, const char *lbl = "9");
	~cFreqControl();
	void updatevalue();
	void value(long lv);
	long value(){return val;};
	long int maximum(void);
	void font(Fl_Font fnt);
	void SetONCOLOR (uchar r, uchar g, uchar b);
	void SetOFFCOLOR (uchar r, uchar g, uchar b);
	void GetONCOLOR (uchar &r, uchar &g, uchar &b) {
			Fl::get_color(ONCOLOR, r, g, b);
	};
	void GetOFFCOLOR (uchar &r, uchar &g, uchar &b) {
			Fl::get_color(OFFCOLOR, r, g, b);
	};
	void SetONOFFCOLOR( Fl_Color, Fl_Color);
//	void setCallBack (int (*cbf)() ){ cbFunc = cbf;};
	void callback (void (*cbf)(Fl_Widget *, void *) ){ cbFunc = cbf;}
	void do_callback() { if (cbFunc) cbFunc(this, (void*)0); }

//	void setCallBack (int (*cbf)(Fl_Widget *, void *) ){ cbFunc = cbf;};
//	void do_callback(Fl_Widget *w, void *d) { if (cbFunc) cbFunc(w, d); }

	int  handle(int event);

	void set_lsd(int val) {
		int temp = precision;
		if (val < 0) return;
		if (val > 3) return;
		if (val == 0) temp = 1;
		if (val == 1) temp = 10;
		if (val == 2) temp = 100;
		if (val == 3) temp = 1000;
		if (temp < precision) temp = precision;
		lsd = temp;
	}

	void visual_beep();
	void set_hrd(bool b) {hrd_buttons = b;}

	void reverse_colors();
	void restore_colors();
	bool  is_reversed_colors() { return enable_arrow_keys; }
	void show_focus() {reverse_colors();}
	void clear_focus() {restore_colors();}

	void resize (int X, int Y, int W, int H);

	void set_precision(int val) {
		switch (val) {
			case 100:
				dpoint = 1; precision = 100; break;
			case 10:
				dpoint = 2; precision = 10; break;
			default:
				dpoint = 3; precision = 1; break;
		}
	}

	void set_ndigits(int val);

private:
	Fl_Repeat_Button	  	*Digit[MAX_DIGITS];
	Fl_Float_Input			*finp;
	static const char	 	*Label[];
	long int		mult[MAX_DIGITS];
	Fl_Box				*decbx;
	Fl_Box				*hfill1;
	Fl_Box				*hfill2;
	Fl_Font  font_number;
	Fl_Color OFFCOLOR;
	Fl_Color ONCOLOR;
	Fl_Color SELCOLOR;
	Fl_Color ILLUMCOLOR;
	Fl_Color REVONCOLOR;
	Fl_Color REVOFFCOLOR;
	int nD;
	//int active;
	long int maxVal;
	long int minVal;

	int pw; // decimal width
	int wfill;
	int bdr;
	int fcWidth;
	int fcTop;
	int fcHeight;
	int W;

	void DecFreq(int n);
	void IncFreq(int n);
	void (*cbFunc)(Fl_Widget *, void *);
	static void freq_input_cb(Fl_Widget* input, void* arg);
	void cancel_kb_entry(void);

protected:
	long int val, oldval;
	int  precision;
	int  dpoint;

	int  lsd;

	bool hrd_buttons;
	bool enable_arrow_keys;
};

#endif
