#ifndef fl_misc_h_
#define fl_misc_h_

#include <config.h>

#include <FL/Enumerations.H>
#include <FL/Fl_Menu_Item.H>

unsigned quick_choice_menu(const char* title, unsigned sel, const Fl_Menu_Item* menu);
unsigned quick_choice(const char* title, unsigned sel, ...);

Fl_Color adjust_color(Fl_Color fg, Fl_Color bg);

#if !defined(__APPLE__) && !defined(__WOE32__)
#  include <FL/x.H>
void make_pixmap(Pixmap *xpm, const char **data);
#endif

#endif // fl_misc_h_
