#include <config.h>

#include <cstdlib>
#include <cstring>
#include <cstdarg>

#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Tooltip.H>
#include <FL/x.H>

#include "flmisc.h"

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

#if !defined(__APPLE__) && !defined(__WOE32__)
#  include <FL/Fl_Window.H>
#  include <FL/Fl_Pixmap.H>
#  include <FL/fl_draw.H>
void make_pixmap(Pixmap *xpm, const char **data)
{
	// We need a displayed window to provide a GC for X_CreatePixmap
	Fl_Window w(0, 0, PACKAGE_NAME);
	w.xclass(PACKAGE_NAME);
	w.border(0);
	w.show();

	Fl_Pixmap icon(data);
	int maxd = MAX(icon.w(), icon.h());
	w.make_current();
	*xpm = fl_create_offscreen(maxd, maxd);
	w.hide();

	fl_begin_offscreen(*xpm);
	// fl_color(FL_BACKGROUND_COLOR);
	// fl_rectf(0, 0, maxd, maxd);
	icon.draw(maxd - icon.w(), maxd - icon.h());
	fl_end_offscreen();
}
#endif
