#include <stdio.h>
#include <FL/Fl.H>

#include "dropwin.h"
#include "dock_events.h"

// basic fltk constructors
dropwin::dropwin(int x, int y, int w, int h, const char *l)
  : Fl_Double_Window(x, y, w, h, l)
{
	init_dropwin();
}

dropwin::dropwin(int w, int h, const char *l)
  : Fl_Double_Window(w, h, l)
{
	init_dropwin();
}

void dropwin::init_dropwin(void)
{
	dock = (dockgroup *)0;
	workspace = (Fl_Group *)0;
	Wdrop = DROP_REGION_HEIGHT;
	Hdrop = DROP_REGION_HEIGHT;
}

void dropwin::dock_resize(int delta_h)
{
	int xo = workspace->x();
	int yo = workspace->y();
	int wo = workspace->w();
	int ho = workspace->h();

	yo = yo - delta_h;
	ho = ho + delta_h;
	workspace->resize(xo, yo, wo, ho);
	workspace->redraw();
	redraw();
}


int dropwin::handle(int evt)
{
	int res = Fl_Double_Window::handle(evt);

	// Is this a dock_drop event?
	if((evt == FX_DROP_EVENT) && (dock)) {
		// Did the drop happen on us?
		// Get our co-ordinates
		int ex = x_root() + dock->x();
		int ey = y_root() + dock->y();
		// get the drop event co-ordinates
		int cx = Fl::event_x_root();
		int cy = Fl::event_y_root();
		// Is the event inside the boundary of this window?
		if (visible() &&
			abs(cx - ex) < Wdrop &&
			abs(cy - ey) < Hdrop) {
			res = 1;
		} else {
			res = 0;
		}
	}
	return res;
}

