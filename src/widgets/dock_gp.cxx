#include <stdio.h>

#include <FL/Fl.H>

#include "dock_gp.h"
#include "dropwin.h"

// basic fltk constructors
dockgroup::dockgroup(int x, int y, int w, int h, const char *l) 
  : Fl_Group(x, y, w, h, l) 
{
	pack = new Fl_Pack(x, y, w, h);
	pack->type(Fl_Pack::HORIZONTAL);
	children = 0;
	resizable(pack);
	vis_h = h;
}

void dockgroup::add(Fl_Widget *grp)
{
	int wd = w();
	int ht = h();
	
	// if the dock is "closed", open it back up
	if (ht < vis_h)
	{
		dropwin *dw = (dropwin *)win;
		size(wd, vis_h);
		pack->size(wd, vis_h);
		dw->dock_resize(ht - vis_h);
	}
	pack->add(grp); 
	pack->resizable(grp);
	children++;
	if (callback() != NULL) do_callback();
}

void dockgroup::remove(Fl_Widget *grp)
{
	int wd = w();
	pack->remove(grp);
	children--;
	pack->resizable(pack->child(pack->children() - 1));
	// If the dock is empty, close it down
	if (children <= 0) {
		dropwin *dw = (dropwin *)win;
		children = 0;
		size(wd, 2);
		dw->dock_resize(vis_h - 2);
	}
	if (callback() != NULL) do_callback();
}

char *dockgroup::dock_check(void)
{
	static char szcheck[50];
	snprintf(szcheck, sizeof(szcheck), "DG: %d - %dx%d", children, pack->w(), pack->h());
	return szcheck;
}
