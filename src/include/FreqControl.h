// ----------------------------------------------------------------------------
//
// Frequency Control Widget for the Fast Light Tool Kit
//
// Copyright (C) 2023
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
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
// 'lbl' param used to specify # digits to display
// The widget can be used in Fluid & initialized with the
// number of digits as the label string
// default is 7; min number is 4, max number is 10.
//
// cFreqControl myFreqControl(x0, y0, w0, h0, "N");  where N is # digits
// cFreqControl *pMyFreqControl = new cFreqControl(x0,y0,w0,h0,"N");

#ifndef _FREQCONTROL_H_
#define _FREQCONTROL_H_

#include <FL/Enumerations.H>
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Repeat_Button.H>
#include <FL/Fl_Widget.H>

#include <cmath>
#include <string>

#ifdef MAX_DIGITS
#undef MAX_DIGITS
#endif
#define MAX_DIGITS 10

#ifdef MIN_DIGITS
#undef MIN_DIGITS
#endif
#define MIN_DIGITS 4

class cFreqControl : public Fl_Group {
friend void cbSelectDigit (Fl_Widget *btn, void * nbr);
public:
	cFreqControl(int x, int y, int w, int h, const char *lbl = "7");
	~cFreqControl();
	void updatevalue();
	void value(unsigned long long lv);
	unsigned long long value() {return val;};

	std::string strval() {
		char szfreq[20];
		snprintf(szfreq, sizeof(szfreq), "%f", val / 1e6);
		return szfreq;
	}

	unsigned long long maximum(void);
	void font(Fl_Font fnt);
	void SetBGCOLOR (uchar r, uchar g, uchar b);
	void GetBGCOLOR (uchar &r, uchar &g, uchar &b) {
			Fl::get_color(BGCOLOR, r, g, b);
	};

	void SetLBLCOLOR (uchar r, uchar g, uchar b);
		void GetLBLCOLOR (uchar &r, uchar &g, uchar &b) {
			Fl::get_color(LBLCOLOR, r, g, b);
	};

	void SetCOLORS(Fl_Color, Fl_Color);
	void UpdateCOLORS(Fl_Color, Fl_Color);

	void callback (void (*cbf)(Fl_Widget *, void *) ){ cbFunc = cbf;}
	void do_callback() { if (cbFunc) cbFunc(this, (void*)0); }

	int  handle(int event);

	void visual_beep();
	void set_hrd(bool b) {hrd_buttons = b;}

	void reverse_colors();
	void restore_colors();
	bool is_reversed_colors() { return colors_reversed; }

	void resize (int X, int Y, int W, int H);

	void set_ndigits(int nbr);

	void set_lsd(int lsd) {
		if (lsd < 0) return;
		if (lsd > 3) return;
		set_precision ((int) pow(10, lsd));
	}

	void set_precision(int prec) {
		switch (prec) {
			case 1000:
				dpoint = 0; precision = lsd = 1000; break;
			case 100:
				dpoint = 1; precision = lsd = 100; break;
			case 10:
				dpoint = 2; precision = lsd = 10; break;
			default:
				dpoint = 3; precision = lsd = 1; break;
		}
		set_ndigits(nD);
	}

	void activate() { active = true; }		// This function overrides the standard fltk widget func of same name
	void deactivate() { active = false; }	// This function overrides the standard fltk widget func of same name
	bool isactive() { return active; }
	bool numeric_entry_mode() { return numeric_entry_active; }
	void numeric_entry_mode(bool);


private:
	Fl_Repeat_Button		*Digit[MAX_DIGITS];
	Fl_Boxtype				Digit_box_type;
	Fl_Float_Input			*finp;
	static const char		*Label[];
	unsigned long long		mult[MAX_DIGITS];

	Fl_Box					*decbx;
	Fl_Box					*hfill;

	Fl_Font					font_number;

	Fl_Color				LBLCOLOR;
	Fl_Color				BGCOLOR;
	Fl_Color				REVBGCOLOR;
	Fl_Color				REVLBLCOLOR;

	Fl_Color				SELCOLOR;

	int						nD;
	unsigned long long		maxVal; // Hz
	unsigned long long		minVal; // Hz

	bool					active;

	int X, Y, W, H;		// Usable space inside FreqControl border
	int bdr_x, bdr_y;	// Thickness of X and Y Freq Control box borders in pixels

	int fw; // font width - pixels
	int fh; // font height - pixels
	int fs; // font size - pixels
	int dw; // (Fl_Repeat_Button) digit box width - pixels
	int pw; // (Fl_Box) decimal point box width - pixels

	void DecFreq(int n);
	void IncFreq(int n);

	void (*cbFunc)(Fl_Widget *, void *);
	static void freq_input_cb(Fl_Widget* input, void* arg);

	void cancel_kb_entry(void);

protected:
	unsigned long long val, oldval;	// Value displayed in Repeat Button boxes; units are Hz
	int  precision;			// Really, resolution - Number of Hz represented by least sig digit displayed; for flrig compatibility
	int  dpoint;			// Number of digits to the right of decimal point
	int  lsd;				// Least significant digit value displayed is 10^lsd Hz
	bool hrd_buttons;		// Toggle between Left/Right mouse click for inc/dec and click top/bottom of digit
	bool colors_reversed;	// Colors (label and background) are reversed to indicate control has focus
	bool numeric_entry_active;  // User pressed numeric keys for freq entry
};

#endif
