#ifndef COLORBOX_H
#define COLORBOX_H

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Button.H>

extern void loadPalette();
extern void savePalette();
extern void selectColor(int);
extern void setColorButtons();

class colorbox : public Fl_Button  {
	void draw();
public:
	colorbox(int x, int y, int w, int h, const char *label = 0) : Fl_Button(x,y,w,h,label) {
		Fl_Button::box(FL_DOWN_BOX);
	};
	void end(){};
};

#endif

