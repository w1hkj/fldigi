// ----------------------------------------------------------------------------
// Frequency Control Widget
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

#include <FL/fl_draw.H>
#include <FL/names.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <string.h>

#include "FreqControl.h"
#include "gettext.h"

std::string old_input_buffer;  // Hold contents of Fl_Float_Input widget prior to change in case of need to revert

const char *cFreqControl::Label[10] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };


static void blink_point(Fl_Widget* w)
{
	w->label(*w->label() ? "" : ".");
	Fl::repeat_timeout(0.5, (Fl_Timeout_Handler)blink_point, w);
}


void cFreqControl::IncFreq (int nbr) {
	unsigned long long v = val;
	v += mult[nbr] * precision;
	if (v <= maxVal) {
		val = v;
		updatevalue();
		numeric_entry_mode(false);
		do_callback();
	}
}

void cFreqControl::DecFreq (int nbr) {
	unsigned long long requested_decrement = mult[nbr] * precision;
	if (requested_decrement > val) {	// Handle case where user clicks low (decrement) on
										// high value blank digit leading to underflow
		return;
	} else {
		val -= requested_decrement;
		updatevalue();
		numeric_entry_mode(false);
		do_callback();
	}
}

void cbSelectDigit (Fl_Widget *btn, void * nbr)
{
	Fl_Button *b = (Fl_Button *)btn;
	int Nbr = reinterpret_cast<intptr_t> (nbr);

	cFreqControl *fc = (cFreqControl *)b->parent();
	if (fc->hrd_buttons) {
		int yclick = Fl::event_y();
		int fc_yc = fc->y() + fc->h()/2;
		if (yclick <= fc_yc)
			fc->IncFreq(Nbr);
		else
			fc->DecFreq(Nbr);
	} else {
		if (Fl::event_button1())
			fc->IncFreq(Nbr);
		else if (Fl::event_button3())
			fc->DecFreq(Nbr);
	}
	fc->redraw();
}


void cFreqControl::set_ndigits(int nbr)
{
	// If number of digits change, remove/delete prior constructs if any
	// and create desired number.

	if (nbr > MAX_DIGITS) nbr = MAX_DIGITS;
	if (nbr < MIN_DIGITS) nbr = MIN_DIGITS;

	if (nD && nbr != nD) {
		for (int n = 0; n < nD; n++) {
			this->remove(Digit[n]);
			delete Digit[n];
		}
	}

	if (nbr != nD) {
		for (int n = 0; n < nbr; n++) {
			Digit[n] = new Fl_Repeat_Button(0, 0, 1, 1, " ");
			Digit[n]->box(Digit_box_type);
			Digit[n]->labelcolor(LBLCOLOR);
			Digit[n]->color(BGCOLOR, SELCOLOR);
			Digit[n]->align(FL_ALIGN_INSIDE);
			Digit[n]->callback(cbSelectDigit, reinterpret_cast<void*>(n));
			this->add(Digit[n]);
		}
	}

	nD = nbr;

	// Usable space inside FreqControl box border
	X = this->x() + bdr_x;
	Y = this->y() + bdr_y;
	W = this->w() - 2 * bdr_x;
	H = this->h() - 2 * bdr_y;

	// While we allow user to select a font, we pay no attention
	// to the user's selection of font size; we size the font
	// to fill the available height in the Fl_Repeat_Button box,
	// constrained by the maximum width allowable per digit.

	fs = H;
	fl_font(font_number, fs);

	fw = fl_width("0");  // Assumes the '0' numeral is the widest
	fh = fl_height();

	while ( fs && ((fh - fl_descent() >= H) || ((nD + 0.5) * fw >= W))) {
		if (--fs <= 1) break;
		fl_font(font_number, fs);
		fh = fl_height();
		fw = fl_width("0");
	}
	dw = fw;
	pw = dw / 2;

	// Number display will be right-justified in available space.

	//	Working from right to left:
	//		Right-of-decimal Repeat Buttons (digits)
	//		Decimal box
	//		Left-of-decimal Repeat Buttons (digits)
	//		Fill box

	int xpos = X + W;
	for (int n = 0; n < dpoint; n++) {
		xpos -= dw;
		Digit[n]->resize (xpos, Y, dw, H);
		Digit[n]->labelfont(font_number);
		Digit[n]->labelsize(fs);
	}

	xpos -= pw;
	decbx->resize(xpos, Y, pw, H);
	decbx->labelfont(font_number);
	decbx->labelsize(fs);

	for (int n = dpoint; n < nD; n++) {
		xpos -= dw;
		Digit[n]->resize(xpos, Y, dw, H);
		Digit[n]->labelfont(font_number);
		Digit[n]->labelsize(fs);
	}

	hfill->resize(X, Y, xpos - X, H);

	redraw();

	// Compute max freq value

	double fmaxval_hz, fmaxval_khz;
	minVal = 0;
	fmaxval_hz = (pow(10.0, nD) - 1) * precision;
	unsigned long long UMAX = maximum();//(unsigned long int)(pow(2.0, 32) - 1);
	if (fmaxval_hz > UMAX) fmaxval_hz = UMAX;
	maxVal = fmaxval_hz;
	fmaxval_khz = (fmaxval_hz / 1000);  // For tooltip use; not used elsewhere

	const char* freq_steps[] = {
		"1", "10", "100", "1k", "10k", "100k", "1M", "10M", "100M", "1G", "10G", "100G"};

	static char tt[1000];

	snprintf(tt, sizeof(tt), "Set Frequency: (Max val: %.3f kHz)\n\
  * Mousewheel over digit\n\
  * MM click: paste from selection\n\
  - or -\n\
  * Click to set focus - colors reverse - then:\n\
      R/L arrow +/- %s; w/SHIFT %s; w/CTRL %s\n\
      U/D arrow +/- %s; w/SHIFT %s; w/CTRL %s\n\
      Pg U/D    +/- %s; w/SHIFT %s; w/CTRL %s\n\
      L/R mouse click (hold for rpt) in top/bot half of digit\n\
      Ctrl/Meta-v: paste from clipboard\n\
    ENTER, ESC, or click outside to release focus\n\
    - or -\n\
    * Enter frequency with number keys or Keypad\n\
        (decimal point blinks once entry has started)\n\
      ENTER to confirm and send to rig;\n\
      BSP/Ctrl-BSP erase last digit/all digits entered;\n\
      ESC to abort and revert to previous value",
	  fmaxval_khz,
	  freq_steps[3-dpoint], freq_steps[4-dpoint], freq_steps[5-dpoint],
	  freq_steps[6-dpoint], freq_steps[7-dpoint], freq_steps[8-dpoint],
	  freq_steps[9-dpoint], freq_steps[10-dpoint], freq_steps[11-dpoint]);
	tooltip(tt);
}



cFreqControl::cFreqControl(int x, int y, int w, int h, const char *lbl):
			  Fl_Group(x,y,w,h,"") {

	end();	// End automatic child widget addition to Group parent;
			// child widgets will be added explicitly as appropriate.

	font_number = FL_HELVETICA; // Font will be overridden with stored preference

	BGCOLOR = REVLBLCOLOR = fl_rgb_color ( 255, 253, 222); // Light yellow;
	LBLCOLOR = REVBGCOLOR = FL_BLACK;
	SELCOLOR = fl_rgb_color ( 100, 100, 100);  // Light gray

	oldval = val = 0;	// Hz
	precision = 1;		// Hz; Resolution of Frequency Control display; internal resolution always 1 Hz
	dpoint = 3;			// Number of digits to the right of decimal

	nD = 0;				// Number of digits to display

	// FreqControl box type and box border thicknesses
	box(FL_DOWN_BOX);
	color(BGCOLOR);
	bdr_x = Fl::box_dx(box());
	bdr_y = Fl::box_dy(box());

	// Repeat Button, decimal, and fill boxes
	Digit_box_type = FL_FLAT_BOX;

	// "Box" for decimal point - Created once; position and size will change as necessary
	decbx = new Fl_Box(0, 0, 1, 1, ".");
	decbx->box(Digit_box_type);
	decbx->color(BGCOLOR);
	decbx->labelcolor(LBLCOLOR);
	decbx->align(FL_ALIGN_INSIDE);
	add(decbx);

	// "Box" to fill empty space to the left of most significant digit - Created once; position and size will change as necessary
	hfill = new Fl_Box(0, 0, 1, 1, "");
	hfill->box(Digit_box_type);
	hfill->labelcolor(LBLCOLOR);
	hfill->color(BGCOLOR);
	add(hfill);

	mult[0] = 1;
	for (int n = 1; n < MAX_DIGITS; n++ )
		mult[n] = 10 * mult[n-1];

	// Create Repeat Buttons; size and position widgets
	set_ndigits(atoi(lbl));

	cbFunc = NULL;

	// Hidden widget used for managing text input and paste actions
	finp = new Fl_Float_Input(0, 0, 24,24);
	finp->callback(freq_input_cb, this);
	finp->when(FL_WHEN_CHANGED | FL_WHEN_NOT_CHANGED);
	finp->hide();

	// true - either mouse button upper half of digit to inc; lower half to dec
	// false - Left mouse button increments; right mouse button decrements
	hrd_buttons = true;

	colors_reversed = false;
	active = true;					// Frequency Control will accept changes
	numeric_entry_active = false;	// User is NOT in the process of entering numbers by keyboard
}

cFreqControl::~cFreqControl()
{
	delete decbx;
	delete hfill;
	delete finp;
	if (nD > 0) {
		for (int i = 0; i < nD; i++) {
			delete Digit[i];
		}
	}
}


void cFreqControl::updatevalue()
{
	unsigned long long v = val / precision;
	int i;
	if (v > 0ULL) {
		for (i = 0; i < nD; i++) {
			Digit[i]->label((v == 0 && i > dpoint) ? "" : Label[v % 10]);
			v /= 10;
		}
	}
	else {
		for (i = 0; i < (dpoint + 1); i++)
			Digit[i]->label("0");
		for (; i < nD; i++)
			Digit[i]->label("");
	}

	redraw();
}

void cFreqControl::font(Fl_Font fnt)
{
	font_number = fnt;
	set_ndigits(nD);
	updatevalue();
}

void cFreqControl::SetCOLORS( Fl_Color LBLcolor, Fl_Color BGcolor)
{
	if (this->contains(Fl::focus())) return;	// Protect against calls from highlight_vfo
												// that would restore colors while freq entry in progress

	LBLCOLOR = REVBGCOLOR = LBLcolor;
	BGCOLOR = REVLBLCOLOR = BGcolor;
	UpdateCOLORS(LBLCOLOR, BGCOLOR);
}

void cFreqControl::SetBGCOLOR (uchar r, uchar g, uchar b)
{
	BGCOLOR = fl_rgb_color (r, g, b);
	REVLBLCOLOR = BGCOLOR;
	UpdateCOLORS(LBLCOLOR, BGCOLOR);
}

void cFreqControl::SetLBLCOLOR (uchar r, uchar g, uchar b)
{
	LBLCOLOR = fl_rgb_color (r, g, b);
	REVBGCOLOR = LBLCOLOR;
	UpdateCOLORS(LBLCOLOR, BGCOLOR);
}

void cFreqControl::restore_colors()
{
	colors_reversed = false;
	UpdateCOLORS(LBLCOLOR, BGCOLOR);
}

void cFreqControl::reverse_colors()
{
	colors_reversed = true;
	UpdateCOLORS(REVLBLCOLOR, REVBGCOLOR);
}

void cFreqControl::UpdateCOLORS(Fl_Color lbl, Fl_Color bg) {
	for (int n = 0; n < nD; n++) {
		Digit[n]->labelcolor(lbl);
		Digit[n]->color(bg);
	}
	decbx->labelcolor(lbl);
	decbx->color(bg);
	decbx->redraw();
	decbx->redraw_label();
	hfill->labelcolor(lbl);
	hfill->color(bg);
	hfill->redraw();
	hfill->redraw_label();

	color(bg);
	redraw();
}


void cFreqControl::value(unsigned long long lv)
{
	if (numeric_entry_mode()) return;	// Don't allow third party to assign a value while user is entering via keyboard.
	if (lv > maxVal) return;			// Assume this is an erroneous entry and reject it.
	oldval = val = lv;
	numeric_entry_mode(false);
	updatevalue();
}

unsigned long long cFreqControl::maximum(void)
{
	return 9999999999ULL;  // Extreme frequency limit intended for non-rig control microwave case
}

void cFreqControl::cancel_kb_entry(void)
{
	val = oldval;
	numeric_entry_mode(false);
	updatevalue();
	do_callback();	// If user changes freq at rig while user is in the middle of keyboard freq entry,
					// and user aborts freq entry, FC-displayed freq will revert to original value
					// and the callback is needed to return the rig to the original value.
}

int cFreqControl::handle(int event)
{
	// std::cerr << this << ": " << time(NULL) << " :handle: with event: " << fl_eventnames[event] << ", Fl::focus(): " << Fl::focus() << std::endl;  // Use for debugging event processing
	//	if (Fl::focus()) {
	//		std::cerr << this << " The focus widget label: " << (Fl::focus()->label() ? Fl::focus()->label() : "no label") << std::endl;
	//		std::cerr << this << " The focus widget tooltip: " << (Fl::focus()->tooltip() ? Fl::focus()->tooltip() : "no tooltip") << std::endl;
	//	}

	if (!active) {
		if (event == FL_MOUSEWHEEL) {
			return 1;	// Prevent MW action over us from propagating
		} else {
			return 0;
		}
	}

	switch (event) {

	case FL_FOCUS:

		if (!Fl::focus()) return 0; // Reject a FOCUS event when no other widget has focus;
									// fltk platform variation shim principally to address the
									// click-outside-application-and-return case.

		// Upon application start, fltk sends FL_FOCUS to each widget in creation order until one accepts it.
		// Accept focus only if event is inside FC (this is the event triggering the FOCUS offer, which
		// should be the button click in the FC).

		if (Fl::event_inside(this)) {
			reverse_colors();
			return 1;	// Accept focus
		}
		return 0;
		//NOTREACHED

	case FL_UNFOCUS:
		if (Fl::focus() != this && this->contains(Fl::focus())) {	// Repeat button took focus so cancel numeric entry
			if (numeric_entry_mode())
				cancel_kb_entry();
			return 1;
		} else if (!this->contains(Fl::focus())) {			// Some other widget took focus; cancel numeric entry and restore colors.
			if (numeric_entry_mode())
				cancel_kb_entry();
			if (colors_reversed) restore_colors();
			return 1;
		}
		return 0;
		//NOTREACHED

	case FL_KEYBOARD:
		switch (Fl::event_key()) {
			case FL_Right:
				if (numeric_entry_mode()) return 1;
				if (Fl::event_ctrl()) IncFreq(2);
				else if (Fl::event_shift()) IncFreq(1);
				else IncFreq(0);
				return 1;
				//NOTREACHED
			case FL_Left:
				if (numeric_entry_mode()) return 1;
				if (Fl::event_ctrl()) DecFreq(2);
				else if (Fl::event_shift()) DecFreq(1);
				else DecFreq(0);
				return 1;
				//NOTREACHED
			case FL_Up:
				if (numeric_entry_mode()) return 1;
				if (Fl::event_ctrl()) IncFreq(5);
				else if (Fl::event_shift()) IncFreq(4);
				else IncFreq(3);
				return 1;
				//NOTREACHED
			case FL_Down:
				if (numeric_entry_mode()) return 1;
				if (Fl::event_ctrl()) DecFreq(5);
				else if (Fl::event_shift()) DecFreq(4);
				else DecFreq(3);
				return 1;
				//NOTREACHED
			case FL_Page_Up:
				if (numeric_entry_mode()) return 1;
				if (Fl::event_ctrl()) IncFreq(8);
				else if (Fl::event_shift()) IncFreq(7);
				else IncFreq(6);
				return 1;
				//NOTREACHED
			case FL_Page_Down:
				if (numeric_entry_mode()) return 1;
				if (Fl::event_ctrl()) DecFreq(8);
				else if (Fl::event_shift()) DecFreq(7);
				else DecFreq(6);
				return 1;
				//NOTREACHED
			case FL_BackSpace:
				if (numeric_entry_mode()) {
					return finp->handle(event);
				} else return 0;
				//NOTREACHED
			case FL_Enter: case FL_KP_Enter:
				if (numeric_entry_mode()) {
					finp->do_callback();
					numeric_entry_mode(false);
				}
				do_callback();
				Fl::focus(Fl::first_window());
				return 1;
				//NOTREACHED
			case FL_Escape:
				if (numeric_entry_mode())
					cancel_kb_entry();
				do_callback();
				Fl::focus(Fl::first_window());
				return 1;
				//NOTREACHED
			case 'v':
				if (Fl::event_command()) {					// FL_CTRL or OSX FL_META
					if (numeric_entry_mode()) {				// Ignore Ctrl/Meta-v while in numeric entry mode
						return 1;
					} else {
						old_input_buffer = finp->value();	// Protect against paste value > max allowed
						Fl::paste(*(this->finp), 1);		// 1 = Paste from clipboard
						do_callback();
					}
					Fl::focus(Fl::first_window());
				}
				return 1;
				//NOTREACHED
		default:
			// Keyboard entry processing

			// Accept numbers, decimal point, and 'e' or 'E' to indicate exponential notation only
			int ch = Fl::event_text()[0];
			if ((ch < '0' || ch > '9') && ch != '.' && ch != 'e' && ch!= 'E') return 1;

			if (!numeric_entry_mode()) {
				// User has begun entering a frequency
				numeric_entry_mode(true);	// Set blinking decimal point to signify user is in numeric frequency entry mode
				finp->static_value("");		// Clear Fl_Float_Input string contents
				oldval = val;				// Preserve current FC-internal integer val in case restoral is needed
			}

			old_input_buffer = finp->value();	// Preserve current Fl_Float_Input string value in case restoral is needed
												// if user entry > max allowed; pre-checks don't cover all cases.

			// If an exponent has been indicated ('e' or 'E' entered), allow one digit of exponent
			const char *e;
			if ((e = strchr(finp->value(), 'e')) || (e = strchr(finp->value(), 'E'))) {
				if (strlen(e) == 1) {
					return finp->handle(event);	// Allow entry of digit
				} else {
					return 1;
				}
			}

			// If decimal already entered, limit additional digits to those being displayed.
			const char *p = strchr(finp->value(), '.');
			if (p) {
				if (strlen(p) <= (size_t) dpoint) {
					return finp->handle(event);
				} else {
					// Allow the entry of 'e' or 'E' following the max decimal digits
					if (strlen(p) == (size_t) dpoint + 1 && (ch == 'e' || ch == 'E')) {
						return finp->handle(event);
					} else {
						return 1;
					}
				}
			}

			// Ignore excess digits left of decimal
			size_t inp_len = strlen(finp->value());
			if ((inp_len == (unsigned long) (nD - dpoint)) && (ch != '.')) return 1;

			// Otherwise, accept entry
			return finp->handle(event);

		}// End FL_KEYBOARD event processing
		//NOTREACHED

	case FL_MOUSEWHEEL:
		if (Fl::event_inside(this)) {			// Reject fltk's attempts to give us events that don't belong to us
			if (numeric_entry_mode()) {			// Ignore mousewheel in numeric entry mode
				return 1;
			} else {
				int d;
				if ( !((d = Fl::event_dy()) || (d = Fl::event_dx())) )
					return 1;

				for (int i = 0; i < nD; i++) {
					if (Fl::event_inside(Digit[i])) {
						d > 0 ? DecFreq(i) : IncFreq(i);
						return 1;
					}
				}
			}
		}
		return 1;  // Consume but ignore mousewheel action outside FreqControl or inside FreqControl but not over digit
		//NOTREACHED

	case FL_PUSH:
		if (numeric_entry_mode()) {				// Ignore mouse click in numeric entry mode
				return 1;
		}

		switch (Fl::event_button()) {

			case FL_MIDDLE_MOUSE:
				Fl::paste(*(this->finp), 0);	// Send FL_PASTE to the Fl_Float_Input widget; its parent
												// Fl_Input_::handle checks for valid floating point entry.
												// 0 = Paste from selection, not clipboard
				do_callback();
				return 1;
			//NOTREACHED

			default:

				if (!this->contains(Fl::focus())) {
					take_focus();	// Signal interest in focus if we don't have it; will result in an FL_FOCUS event
					return 1;
				} else {
					return Fl_Group::handle(event);	// The appropriate Repeat Button will handle the event
				}
				//NOTREACHED
		}
		//NOTREACHED

	default:
		return Fl_Group::handle(event);
	}
}

// Hidden Fl_Float_Input widget's callback called on keyboard number entry and on PASTE and ENTER actions.
//
// Compute the integer Hz representation of the Fl_Float_Input widget string value
// and update the values shown on the Repeat Button labels.

void cFreqControl::freq_input_cb(Fl_Widget*, void* arg)
{
	cFreqControl* fc = reinterpret_cast<cFreqControl*>(arg);
	double finp_val = strtod(fc->finp->value(), NULL);

	finp_val *= 1000;  // Frequency string is in kHz - convert here to Hz used internally by FreqControl
	finp_val += 0.5;
	unsigned long long lval = (unsigned long long) finp_val;

	if (lval <= fc->maxVal) {
		fc->val = lval;
		fc->updatevalue();
	} else { // Need to revert entry because we overran our max value
		fc->finp->value(old_input_buffer.c_str());
	}
}


void cFreqControl::numeric_entry_mode(bool flag) {
	if (flag) {
		numeric_entry_active = true;
		Fl::add_timeout(0.0, (Fl_Timeout_Handler)blink_point, decbx);
	} else {
		numeric_entry_active = false;
		Fl::remove_timeout((Fl_Timeout_Handler)blink_point, decbx);
		// Ensure we have a visible decimal point after completion of keyboard entry;
		// might otherwise be blank due to uncertain ending state of 'blink_point' function.
		decbx->label(".");
		decbx->redraw_label();
	}
}


static void restore_color(void* w)
{
	cFreqControl *fc = (cFreqControl *)w;
	fc->restore_colors();
}

static void reverse_color(void* w)
{
	cFreqControl *fc = (cFreqControl *)w;
	fc->reverse_colors();
}

void cFreqControl::visual_beep()
{
	if (this->colors_reversed) {
		restore_color(this);
		Fl::add_timeout(0.1, reverse_color, this);
	} else {
		reverse_colors();
		Fl::add_timeout(0.1, restore_color, this);
	}
}
void cFreqControl::resize(int x, int y, int w, int h)
{
	Fl_Group::resize(x,y,w,h);

	set_ndigits(nD);
	return;
}
