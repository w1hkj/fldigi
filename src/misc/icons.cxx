// ----------------------------------------------------------------------------
//      icons.cxx
//
// Copyright (C) 2008
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

#include <config.h>
#include "icons.h"

#include <FL/Fl.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>

#if USE_IMAGE_LABELS
#  include <map>
#  include <cassert>
#  include <cstring>

#  include <FL/Fl_Multi_Label.H>
#  include <FL/Fl_Image.H>
#  include <FL/Fl_Pixmap.H>

#endif


using namespace std;

#if USE_IMAGE_LABELS
typedef map<Fl_Multi_Label*, Fl_Image**> imap_t;
static imap_t* imap = 0;
#endif

#define FL_EMPTY_LABEL FL_FREE_LABELTYPE
static void draw_empty(const Fl_Label*, int, int, int, int, Fl_Align) { }
static void measure_empty(const Fl_Label*, int& w, int& h) { w = h = 0; }

// The following functions create image+text menu item labels.
// You've had too much FLTK if you already know how to do that.


// Return a multi_label pointer, cast to a string, for `text' and
// `pixmap'.  This goes into the label pointer of a widget or menu
// item. The text label is copied if we are using multi labels. You must
// call set_icon_label on the widget or menu item before its draw()
// function is called for the first time.
//
// A NULL pixmap means that the caller wants an empty, transparent, icon.
const char* make_icon_label(const char* text, const char** pixmap)
{
#if USE_IMAGE_LABELS
	static imap_t* imap_ = 0;
	if (unlikely(!imap_)) {
		imap = imap_ = new imap_t;
		Fl::set_labeltype(FL_EMPTY_LABEL, draw_empty, measure_empty);
	}

	// Create a multi label and associate it with an Fl_Image* array
	Fl_Multi_Label* mlabel = new Fl_Multi_Label;
	Fl_Image** images = new Fl_Image*[2];
	images[0] = new Fl_Pixmap(pixmap ? pixmap : clear_row_icon);
	images[1] = 0; // we create this on demand
	// set_icon_label_ will set mlabel->labela later
	mlabel->typea = _FL_IMAGE_LABEL;

	if (!text)
		text = "";
	size_t len = strlen(text);
	char* s = new char[len + 2];
	s[0] = ' ';
	memcpy(s + 1, text, len + 1);
	mlabel->labelb = s;
	mlabel->typeb = FL_NORMAL_LABEL;

	(*imap)[mlabel] = images;

	return (const char*)mlabel;
#else
	return text;
#endif
}

#if USE_IMAGE_LABELS
// Find the item's label, which should be something that was returned by
// make_icon_label, and set the active or inactive image.
template <typename T>
void set_icon_label_(T* item)
{
	imap_t::iterator j = imap->find((Fl_Multi_Label*)(item->label()));
	if (j == imap->end())
		return;

	Fl_Multi_Label* mlabel = j->first;
	Fl_Image** images = j->second;
	unsigned char i = !item->active();

	if (!images[i]) { // create inactive version of other image
		images[i] = images[!i]->copy();
		images[i]->inactive();
	}
	if (mlabel->typea == _FL_IMAGE_LABEL)
		mlabel->labela = (const char*)images[i];
	else
		mlabel->labelb = (const char*)images[i];
	item->image(images[i]);
	mlabel->label(item);
	item->labeltype(_FL_MULTI_LABEL);
}
#endif

void set_icon_label(Fl_Menu_Item* item)
{
#if USE_IMAGE_LABELS
	set_icon_label_(item);
#else
	// this isn't needed but it simplifies fldigi's UI setup code
	if (item->labeltype() == _FL_MULTI_LABEL)
		item->labeltype(FL_NORMAL_LABEL);
#endif
}

void set_icon_label(Fl_Widget* w)
{
#if USE_IMAGE_LABELS
	set_icon_label_(w);
	w->image(0);
#else
	if (w->labeltype() == _FL_MULTI_LABEL)
		w->labeltype(FL_NORMAL_LABEL);
#endif
}

void toggle_icon_labels(void)
{
#if USE_IMAGE_LABELS
	for (imap_t::iterator i = imap->begin(); i != imap->end(); ++i) {
		// swap sublabels
		const char* l = i->first->labela;
		i->first->labela = i->first->labelb;
		i->first->labelb = l;
		if (i->first->typea == _FL_IMAGE_LABEL) {
			i->first->typea = FL_NORMAL_LABEL;
			i->first->typeb = FL_EMPTY_LABEL;
			i->first->labela++;
		}
		else {
			i->first->typea = _FL_IMAGE_LABEL;
			i->first->typeb = FL_NORMAL_LABEL;
			i->first->labelb--;
		}
	}

#endif
}

template <typename T>
const char* get_icon_label_text_(T* item)
{
#if USE_IMAGE_LABELS
	if (item->labeltype() == _FL_MULTI_LABEL) {
		imap_t::iterator i = imap->find((Fl_Multi_Label*)(item->label()));
		if (i == imap->end())
			return 0;
		if (i->first->typeb == FL_NORMAL_LABEL)
			return i->first->labelb + 1;
		else // disabled icons
			return i->first->labela;
	}
	else
#endif
		return item->label();
}

const char* get_icon_label_text(Fl_Menu_Item* item)
{
	return get_icon_label_text_(item);
}
const char* get_icon_label_text(Fl_Widget* w)
{
	return get_icon_label_text_(w);
}

template <typename T>
void free_icon_label_(T* item)
{
#if USE_IMAGE_LABELS
	if (item->labeltype() == FL_NORMAL_LABEL) {
		delete [] item->label();
		item->label(0);
		return;
	}

	imap_t::iterator i = imap->find((Fl_Multi_Label*)item->label());
	if (i == imap->end())
		return;

	item->label(0);

	// delete the images
	delete i->second[0];
	delete i->second[1];
	delete [] i->second;

	// delete the multi label
	delete [] ((i->first->typeb == FL_NORMAL_LABEL) ? i->first->labelb : i->first->labela-1);
	delete i->first;

	imap->erase(i);
#endif
}

void free_icon_label(Fl_Menu_Item* item) { free_icon_label_(item); }
void free_icon_label(Fl_Widget* w) { free_icon_label_(w); }

template <typename T>
void set_active_(T* t, bool v) {
	if (v)
		t->activate();
	else
		t->deactivate();
	if (t->labeltype() == _FL_MULTI_LABEL)
		set_icon_label(t);
}

void set_active(Fl_Menu_Item* item, bool v) { set_active_(item, v); }
void set_active(Fl_Widget* w, bool v) { set_active_(w, v); }

static Fl_Image* msg_icon;
void set_message_icon(const char** pixmap)
{
	if (msg_icon && msg_icon->data() == pixmap)
		return;
	delete msg_icon;

	Fl_Widget* msg = fl_message_icon();
	msg->label("");
	msg->align(FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE);
	msg->color(msg->parent()->color());
	msg->box(FL_NO_BOX);
	msg->image(msg_icon = new Fl_Pixmap(pixmap));
}
