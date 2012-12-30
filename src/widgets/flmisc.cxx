// ----------------------------------------------------------------------------
// flmisc.cxx
//
// Copyright (C) 2008-2010
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <string>
#include <cstdlib>
#include <cstring>
#include <cstdarg>

#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Tooltip.H>

#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Return_Button.H>
#include <FL/fl_draw.H>

#include <FL/x.H>

#include "flmisc.h"
#include "pixmaps.h"

using namespace std;

unsigned quick_choice_menu(const char* title, unsigned sel, const Fl_Menu_Item* menu)
{
	unsigned n = menu->size();
	sel = CLAMP(sel - 1, 0, n - 1);
	int t = Fl_Tooltip::enabled();
	Fl_Tooltip::disable();
	const Fl_Menu_Item* p = menu->popup(Fl::event_x(), Fl::event_y(), title, menu + sel);
	Fl_Tooltip::enable(t);
	return p ? p - menu + 1 : 0;
}

unsigned quick_choice(const char* title, unsigned sel, ...)
{
	const char* item;
	const Fl_Menu_Item* menu = NULL;
	Fl_Menu_Item* p = NULL;

	va_list ap;
	va_start(ap, sel);
	for (size_t n = 0; (item = va_arg(ap, const char*)); n++) {
		if ((p = (Fl_Menu_Item*)realloc(p, (n+2) * sizeof(Fl_Menu_Item))) == NULL) {
			free((Fl_Menu_Item*)menu);
			va_end(ap);
			return 0;
		}
		memset(p + n, 0, 2 * sizeof(Fl_Menu_Item));
		p[n].label(item);
		p[n+1].label(NULL);
		menu = p;
	}
	va_end(ap);

	sel = quick_choice_menu(title, sel, menu);
	free(p);
	return sel;
}

// Adjust and return fg color to ensure good contrast with bg
Fl_Color adjust_color(Fl_Color fg, Fl_Color bg)
{
	Fl_Color adj;
	unsigned max = 24;
	while ((adj = fl_contrast(fg, bg)) != fg  &&  max--)
		fg = (adj == FL_WHITE) ? fl_color_average(fg, FL_WHITE, .9)
				       : fl_color_average(fg, FL_BLACK, .9);
	return fg;
}

// invert colour (bg1r, bg1g, bg1b); return def if new colour does not make
// good contrast with bg2
void adjust_color_inv(unsigned char& bg1r, unsigned char& bg1g, unsigned char& bg1b,
		      Fl_Color bg2, Fl_Color def)
{
	bg1r = 255 - bg1r; bg1g = 255 - bg1g; bg1b = 255 - bg1b;
	Fl_Color adj = fl_rgb_color(bg1r, bg1g, bg1b);
	if (fl_contrast(adj, bg2) != adj)
		Fl::get_color((def >= 0 ? def : adj), bg1r, bg1g, bg1b);
}

#if !defined(__APPLE__) && !defined(__WOE32__) && USE_X
#  include <FL/Fl_Window.H>
#  include <FL/Fl_Pixmap.H>
#  include <FL/fl_draw.H>
void make_pixmap(Pixmap *xpm, const char **data, int argc, char** argv)
{
	// We need a displayed window to provide a GC for X_CreatePixmap
	Fl_Window w(0, 0, PACKAGE_NAME);
	w.xclass(PACKAGE_NAME);
	w.border(0);
	w.show(argc, argv);

	Fl_Pixmap icon(data);
	int maxd = MAX(icon.w(), icon.h());
	w.make_current();
	*xpm = fl_create_offscreen(maxd, maxd);
	w.hide();

	fl_begin_offscreen(*xpm);
	// Fl_Color(FL_BACKGROUND_COLOR);
	// fl_rectf(0, 0, maxd, maxd);
	icon.draw(maxd - icon.w(), maxd - icon.h());
	fl_end_offscreen();
}
#endif


notify_dialog::notify_dialog(int X, int Y, const char* l)
	: Fl_Window(X, Y, l), icon(10, 10, 50, 50), message(70, 25, 330, 35),
	  dial(277, 70, 23, 23), button(309, 70, 90, 23, "Close"), resize_box(399, 26, 1, 1)
{
	end();

	icon.image(new Fl_Pixmap(dialog_information_48_icon));

	message.type(FL_MULTILINE_OUTPUT);
	message.box(FL_FLAT_BOX);
	message.color(FL_BACKGROUND_COLOR);

	button.callback(button_cb);
	newx = button.x();

	dial.box(FL_FLAT_BOX);
	dial.type(FL_FILL_DIAL);
	dial.selection_color(adjust_color(fl_lighter(FL_BACKGROUND_COLOR), FL_BACKGROUND_COLOR));
	dial.angle1(180);
	dial.angle2(-180);
	dial.minimum(0.0);

	xclass(PACKAGE_TARNAME);
	resizable(resize_box);
}
notify_dialog::~notify_dialog()
{
	delete icon.image();
	Fl::remove_timeout(dial_timer, &dial);
}
int notify_dialog::handle(int event)
{
	if (event == FL_HIDE && delete_on_hide) {
		Fl::delete_widget(this);
		return 1;
	}
	else if (event == FL_PUSH) {
		dial.hide();
		return Fl_Window::handle(event);
	}
	return Fl_Window::handle(event);
}

void notify_dialog::button_cb(Fl_Widget* w, void*)
{
	w->window()->hide();
}
void notify_dialog::dial_timer(void* arg)
{
	Fl_Dial* dial = reinterpret_cast<Fl_Dial*>(arg);
	double v = dial->value();
	if (!dial->visible())
		return;
	if (v == dial->minimum())
		return dial->window()->hide();
	dial->value(dial->clamp(v - 0.05));
	return Fl::repeat_timeout(0.05, dial_timer, arg);
}

Fl_Button* notify_dialog::make_button(int W, int H)
{
	Fl_Group* cur = Fl_Group::current();
	Fl_Group::current(this);
	Fl_Button* b = 0;

	int pad = 10;
	int X = newx - pad - W;
	if (X - pad - dial.w() > 0) {
		b = new Fl_Button(newx = X, button.y(), W, H);
		dial.position(b->x() - dial.w() - pad, dial.y());
	}

	Fl_Group::current(cur);
	return b;
}

void notify_dialog::notify(const char* msg, double timeout, bool delete_on_hide_)
{
	delete_on_hide = delete_on_hide_;
	message.value(msg);
	const char* p;
	if ((p = strchr(msg, '\n'))) { // use first line as label
		string l(msg, p - msg);
		copy_label(l.c_str());
	}
	else
		label("Notification");

	fl_font(message.textfont(), message.textsize());
	int H = 0;
	for (const char* p = msg; (p = strchr(p, '\n')); p++)
		H++;
	size(w(), h() + max(H - 1, 0) * fl_height());

	if (timeout > 0.0) {
		dial.maximum(timeout);
		dial.value(timeout);
		dial.show();
		Fl::add_timeout(0.0, dial_timer, &dial);
	}
	else
		dial.hide();
	button.take_focus();
	show();
}

// =============================================================================

#ifdef BUILD_FLDIGI

#include "icons.h"
#include "gettext.h"

Mode_Browser::Mode_Browser(void)
	: Fl_Double_Window(170, 460), changed_cb(NULL), changed_args(NULL)
{
	int bw = 80, bh = 20, pad = 2;

	modes = new Fl_Check_Browser(pad, pad, w() - pad, h() - 2 * (bh + 2 * pad));
	for (int i = 0; i < NUM_RXTX_MODES; i++)
		modes->add(mode_info[i].name);
	modes->callback(modes_cb, this);
	modes->when(FL_WHEN_CHANGED);

	all_button = new Fl_Button(modes->x(), modes->y() + modes->h() + pad,
				   bw, bh, _("Select All"));
	all_button->callback(button_cb, this);

	none_button = new Fl_Button(all_button->x(), all_button->y() + all_button->h() + pad,
				    all_button->w(), all_button->h(), _("Clear All"));
	none_button->callback(button_cb, this);

	close_button = new Fl_Button(w() - none_button->w() - pad, none_button->y(),
				     none_button->w(), none_button->h(),
				     make_icon_label(_("Close"), close_icon));
	set_icon_label(close_button);
	close_button->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	close_button->callback(button_cb, this);

	end();
	resizable(modes);
	xclass(PACKAGE_TARNAME);
}

Mode_Browser::~Mode_Browser(void)
{
	free_icon_label(close_button);
	delete close_button;
	delete all_button;
	delete none_button;
	delete modes;
}

void Mode_Browser::show(mode_set_t* b)
{
	store = b;
	modes->check_none();
	for (size_t i = 0; i < b->size(); i++)
		modes->checked(i + 1, store->test(i));
	modes->position(0);
	Fl_Double_Window::show();
}

void Mode_Browser::callback(Fl_Callback* cb, void* args)
{
	changed_cb = cb;
	changed_args = args;
}

void Mode_Browser::modes_cb(Fl_Widget* w, void* arg)
{
	Mode_Browser* m = static_cast<Mode_Browser*>(arg);
	int sel = m->modes->value();
	m->store->set(sel - 1, m->modes->checked(sel));
	if (m->changed_cb)
		m->changed_cb(m, m->changed_args);
}

void Mode_Browser::button_cb(Fl_Widget* w, void* arg)
{
	Mode_Browser* m = static_cast<Mode_Browser*>(arg);
	if (w == m->close_button)
		m->hide();
	else {
		if (w == m->all_button) {
			m->store->set();
			m->modes->check_all();
		}
		else {
			m->store->reset();
			m->modes->check_none();
		}
		if (m->changed_cb)
			m->changed_cb(m, m->changed_args);
	}
}

#endif // BUILD_FLDIGI
