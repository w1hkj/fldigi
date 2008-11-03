#ifndef ICONS_H_
#define ICONS_H_

#define USE_IMAGE_LABELS 1

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

void set_active(Fl_Menu_Item* item, bool v);
void set_active(Fl_Widget* w, bool v);

#endif // ICONS_H_
