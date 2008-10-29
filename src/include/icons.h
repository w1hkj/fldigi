#ifndef ICONS_H_
#define ICONS_H_

#ifdef __CYGWIN__
#  define USE_IMAGE_LABELS 0
#else
#  define USE_IMAGE_LABELS 1
#endif

#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_Item.H>
#include "pixmaps.h"

const char* make_icon_label(const char* text, const char** pixmap = 0);

void set_icon_label(Fl_Menu_Item* item);
void set_icon_label(Fl_Widget* w);

const char* get_icon_label_text(Fl_Menu_Item* item);
const char* get_icon_label_text(Fl_Widget* w);

void free_icon_label(Fl_Menu_Item* item);
void free_icon_label(Fl_Widget* w);

template <typename T>
void set_active(T* t, bool v) { if (v) t->activate(); else t->deactivate(); set_icon_label(t); }

#endif // ICONS_H_
