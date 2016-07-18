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

#include <string>

#include <cstring>
#include <cstdlib>
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Output.H>

#include "config.h"
#include "combo.h"

void popbrwsr_cb (Fl_Widget *v, long d);

Fl_PopBrowser::Fl_PopBrowser (int X, int Y, int W, int H, const char *label)
 : Fl_Window (X, Y, W, H, label)
{
	hRow  = H;
	wRow  = W;
	clear_border();
	box(FL_BORDER_BOX);
	align(FL_ALIGN_INSIDE);

	popbrwsr = new Fl_Select_Browser(0,0,wRow,hRow, "");
	popbrwsr->callback ( (Fl_Callback*)popbrwsr_cb);
	popbrwsr->align(FL_ALIGN_INSIDE);
	parentCB = 0;
	end();
	set_modal();
}

Fl_PopBrowser::~Fl_PopBrowser ()
{
	delete popbrwsr;
}

int Fl_PopBrowser::handle(int event)
{
	Fl_ComboBox *cbx = (Fl_ComboBox*)this->parent();
	if (!Fl::event_inside( child(0) ) && event == FL_PUSH) {
		pophide();
		return 1;
 	}
	if (event == FL_KEYDOWN) {
		int kbd = Fl::event_key();
		int key = Fl::event_text()[0];
		if (kbd == FL_Down) {
			if (popbrwsr->value() < popbrwsr->size())
				popbrwsr->select(popbrwsr->value() + 1);
			popbrwsr->bottomline(popbrwsr->value());
			return 1;
		}
		if (kbd == FL_Up && popbrwsr->value() > 1) {
			popbrwsr->select(popbrwsr->value() - 1);
			return 1;
		}
		if (key == '\r' || kbd == FL_Enter) { // kbd test for OS X
			int n = popbrwsr->value() - 1;
			pophide();
			cbx->index(n);
			cbx->do_callback();
			return 1;
		}
		if (key == '\b' || kbd == FL_BackSpace) { // kbd test for OS X
			if (!keystrokes.empty())
				keystrokes.erase(keystrokes.length() - 1, 1);
			return 1;
		}
		if (key == 0x1b || kbd == FL_Escape) { // kbd test for OS X
			pophide();
			return 0;
		}
		if (key >= ' ' && key <= 0x7f) {
			keystrokes += key;
			for (int i = 0; i < cbx->listsize; i++) {
				if (strncasecmp(keystrokes.c_str(),
					cbx->datalist[i]->s,
					keystrokes.length()) == 0) {
					popbrwsr->select(i+1);
					popbrwsr->show(i+1);
					return 1;
				}
			}
			return 0;
		}
	}
	return Fl_Group::handle(event);
}

void Fl_PopBrowser::add(char *s, void *d)
{
	popbrwsr->add(s,d);
}

void Fl_PopBrowser::clear()
{
	popbrwsr->clear();
}

void Fl_PopBrowser::sort()
{
	return;
}

void Fl_PopBrowser::popshow (int x, int y)
{
	int nRows = parentCB->numrows();
	int fh = fl_height();
	int height = nRows * fh + 4;

	if (popbrwsr->size() == 0) return;
	if (nRows > parentCB->lsize()) nRows = parentCB->lsize();

// locate first occurance of inp string value in the list
// and display that if found
	int i = parentCB->index();
	if (!(i >= 0 && i < parentCB->listsize)) {
		for (i = 0; i < parentCB->listsize; i++)
			if (parentCB->type_ == COMBOBOX) {
				if (!strcmp(parentCB->val->value(), parentCB->datalist[i]->s))
					break;
			} else {
				if (!strcmp(parentCB->valbox->label(), parentCB->datalist[i]->s))
					break;
			}
		if (i == parentCB->listsize)
			i = 0;
	}

// resize and reposition the popup to insure that it is within the bounds
// of the uppermost parent widget
// preferred position is just below and at the same x position as the
// parent widget

	Fl_Widget *gparent = parentCB;
	int hp = gparent->h();

	while ((gparent = gparent->parent())) {
		hp = gparent->h();
		parentWindow = gparent;
	}

	int nu = nRows, nl = nRows;
	int hu = nu * fh + 4, hl = nl * fh + 4;
	int yu = parentCB->y() - hu;
	int yl = y;

	while (nl > 1 && (yl + hl > hp)) { nl--; hl -= fh; }
	while (nu > 1 && yu < 0) { nu--; yu += fh; hu -= fh; }

	y = yl; height = hl;
	if (nl < nu) {
		y = yu;
		height = hu;
	}

	popbrwsr->size (wRow, height);
	resize (x, y, wRow, height);

	popbrwsr->topline (i);

	keystrokes.empty();
	popbrwsr->show();
	show();
	for (const Fl_Widget* o = popbrwsr; o; o = o->parent())
		((Fl_Widget *)o)->set_visible();

	parentWindow->damage(FL_DAMAGE_ALL);
	parentWindow->redraw();

	Fl::grab(this);
}

void Fl_PopBrowser::pophide ()
{
	hide ();

	parentWindow->damage(FL_DAMAGE_ALL);
	parentWindow->redraw();

	Fl::grab(0);
	Fl::focus(((Fl_ComboBox*)parent())->btn);
}

void Fl_PopBrowser::popbrwsr_cb_i (Fl_Widget *v, long d)
{
// update the return values
	Fl_Select_Browser *SB = (Fl_Select_Browser *)(v);
	Fl_PopBrowser *PB = (Fl_PopBrowser *)(SB->parent());
	Fl_ComboBox *CB = (Fl_ComboBox *)(PB->parent());

	int row = SB->value();

	if (row == 0) return;
	SB->deselect();

	CB->value(SB->text(row));
	CB->idx = row - 1;

	PB->pophide();

	CB->do_callback();

	return;
}

void popbrwsr_cb (Fl_Widget *v, long d)
{
	((Fl_PopBrowser *)(v))->popbrwsr_cb_i (v, d);
	return;
}


void Fl_ComboBox::fl_popbrwsr(Fl_Widget *p)
{
	int xpos = p->x(), ypos = p->h() + p->y();
// pass the calling widget to the popup browser so that the
// correct callback function can be called when the user selects an item
// from the browser list
	Brwsr->parentCB = (Fl_ComboBox *) p;
	Brwsr->clear_kbd();
	Brwsr->popshow(xpos, ypos);
	return;
}

void btnComboBox_cb (Fl_Widget *v, void *d)
{
	Fl_ComboBox *p = (Fl_ComboBox *)(v->parent());
	p->fl_popbrwsr (p);
	return;
}

void val_callback(Fl_Widget *w, void *d)
{
	Fl_Input *inp = (Fl_Input *)(w);
	Fl_ComboBox *cbx = (Fl_ComboBox *)(d);
	cbx->add(inp->value());
	cbx->sort();
}

Fl_ComboBox::Fl_ComboBox (int X,int Y,int W,int H, const char *lbl, int wtype)
 : Fl_Group (X, Y, W, H, lbl)
{
	width = W; height = H;

	type_ = wtype;
	if (type_ == LISTBOX) {
		valbox = new Fl_Box (FL_DOWN_BOX, X, Y, W-H, H, "");
		valbox->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
		valbox->color(FL_BACKGROUND2_COLOR);
	} else {
		val = new Fl_Input (X, Y, W-H, H, "");
		val->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
		val->callback((Fl_Callback *)val_callback, this);
		val->when(FL_WHEN_RELEASE);
	}

	btn = new Fl_Button (X + W - H + 1, Y, H - 1, H, "@2>");
	btn->callback ((Fl_Callback *)btnComboBox_cb, 0);

	Brwsr = 0;
	datalist = new datambr *[FL_COMBO_LIST_INCR];
	maxsize = FL_COMBO_LIST_INCR;
	for (int i = 0; i < FL_COMBO_LIST_INCR; i++) datalist[i] = 0;
	listsize = 0;
	listtype = 0;

	Brwsr = new Fl_PopBrowser(X, Y, W, H, "");
	Brwsr->align(FL_ALIGN_INSIDE);

	idx = 0;

	end();

	numrows_ = 8;
}

Fl_ComboBox::~Fl_ComboBox()
{
	delete Brwsr;
	for (int i = 0; i < listsize; i++) {
		if (datalist[i]) {
			if (datalist[i]->s) delete [] datalist[i]->s;
			delete datalist[i];
		}
	}
	delete [] datalist;
}

int Fl_ComboBox::handle(int event)
{
	if (event == FL_KEYDOWN) {
		int  kbd = Fl::event_key();
		if (kbd == FL_Down) {
			fl_popbrwsr (this);
			return 1;
		}
	}
	return Fl_Group::handle(event);
}

void Fl_ComboBox::type (int t)
{
	listtype = t;
}

void Fl_ComboBox::readonly(bool yes)
{
	if (type_ == LISTBOX) return;
	val->readonly(yes);
	if (yes)
		val->selection_color(fl_rgb_color(173,216,230));
	else
		val->selection_color(FL_SELECTION_COLOR);
}

// ComboBox value is contained in the val widget

const char *Fl_ComboBox::value()
{
	if (type_ == LISTBOX)
		return valbox->label();
	else
		return val->value();
}

void Fl_ComboBox::value( const char *s )
{
	int i;
	if ((listtype & FL_COMBO_UNIQUE_NOCASE) == FL_COMBO_UNIQUE_NOCASE) {
		for (i = 0; i < listsize; i++) {
			if (strcasecmp (s, datalist[i]->s) == 0)
				break;
		}
	} else {
		for (i = 0; i < listsize; i++) {
			if (strcmp (s, datalist[i]->s) == 0)
				break;
		}
	}
	if ( i < listsize) {
		idx = i;
		if (type_ == LISTBOX) {
			valbox->label(datalist[idx]->s);
			valbox->redraw_label();
		} else
			val->value(datalist[idx]->s);
	} else {
		if (type_ != LISTBOX && !val->readonly()) {
			insert(s, 0);
			for (i = 0; i < listsize; i++) {
				if (strcmp (s, datalist[i]->s) == 0) {
					idx = i;
					val->value(datalist[idx]->s);
					break;
				}
			}
		} else if (type_ == LISTBOX) {
			valbox->label("");
			valbox->redraw_label();
		} else
			val->value("");
	}
}

void Fl_ComboBox::put_value(const char *s)
{
	value(s);
}

void Fl_ComboBox::index(int i)
{
	if (i >= 0 && i < listsize) {
		idx = i;
		if (type_ == LISTBOX) {
			valbox->label(datalist[idx]->s);
			valbox->redraw_label();
		} else
			val->value( datalist[idx]->s);
	}
}

int Fl_ComboBox::index() {
	return idx;
}

void * Fl_ComboBox::data() {
	return retdata;
}

void Fl_ComboBox::insert(const char *str, void *d)
{
	datalist[listsize] = new datambr;
	datalist[listsize]->s = new char [strlen(str) + 1];
	datalist[listsize]->s[0] = 0;
	strcpy (datalist[listsize]->s, str);
	datalist[listsize]->d = 0;
	Brwsr->add(datalist[listsize]->s,d);
	listsize++;
	if (listsize == maxsize) {
		int nusize = maxsize + FL_COMBO_LIST_INCR;
		datambr **temparray = new datambr *[nusize];
		for (int i = 0; i < listsize; i++)
			temparray[i] = datalist[i];
		delete [] datalist;
		datalist = temparray;
		maxsize = nusize;
	}
}

void Fl_ComboBox::add( const char *s, void * d)
{
	std::string str = s;
	std::string sinsert;
	size_t p = str.find("|");
	bool found = false;
	if (p != std::string::npos) {
		while (p != std::string::npos) {
			sinsert = str.substr(0, p);
			found = false;
// test for in list
			if ((listtype & FL_COMBO_UNIQUE_NOCASE) == FL_COMBO_UNIQUE_NOCASE) {
				for (int i = 0; i < listsize; i++) {
					if (sinsert == datalist[i]->s) {
						found = true;
						break;
					}
				}
			}
// not in list, so add this entry
			if (!found) insert(sinsert.c_str(), 0);
			str.erase(0, p+1);
			p = str.find("|");
		}
		if (str.length())
			insert(str.c_str(), 0);
	} else
		insert( s, d );
}

void Fl_ComboBox::clear()
{
	Brwsr->clear();

	if (listsize == 0) return;
	for (int i = 0; i < listsize; i++) {
		delete [] datalist[i]->s;
		delete datalist[i];
	}
	listsize = 0;
}

int DataCompare( const void *x1, const void *x2 )
{
	int cmp;
	datambr *X1, *X2;
	X1 = *(datambr **)(x1);
	X2 = *(datambr **)(x2);
	cmp = strcasecmp (X1->s, X2->s);
	if (cmp < 0)
		return -1;
	if (cmp > 0)
		return 1;
	return 0;
}

void Fl_ComboBox::sort() {
	Brwsr->clear ();
	qsort (&datalist[0],
		 listsize,
		 sizeof (datambr *),
		 DataCompare);
	for (int i = 0; i < listsize; i++)
		Brwsr->add (datalist[i]->s, datalist[i]->d);
}

void Fl_ComboBox::textfont (int fnt)
{
	if (type_ == LISTBOX)
		valbox->labelfont (fnt);
	else
		val->textfont (fnt);
}

void Fl_ComboBox::textsize (uchar n)
{
	if (type_ == LISTBOX)
		valbox->labelsize(n);
	else
		val->textsize (n);
}

void Fl_ComboBox::textcolor( Fl_Color c)
{
	if (type_ == LISTBOX)
		valbox->labelcolor (c);
	else
		val->textcolor (c);
}

void Fl_ComboBox::color(Fl_Color c)
{
	_color = c;
	if (type_ == LISTBOX)
		valbox->color(c);
	else
		val->color(c);
	if (Brwsr) Brwsr->color(c);
}

int Fl_ComboBox::find_index(const char *str)
{
	if((listsize < 1) || !str)
		return -1;

	for (int i = 0; i < listsize; i++) {
		if(datalist[i]->s)
			if(strncmp(datalist[i]->s, str, FILENAME_MAX) == 0)
				return i;
	}

	return -1;
}
