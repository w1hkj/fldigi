#ifndef _HAVE_DOCK_GRP_HDR_
#define _HAVE_DOCK_GRP_HDR_

#include <FL/Fl_Group.H>
#include <FL/Fl_Pack.H>

class dockgroup : public Fl_Group
{
protected:
	Fl_Window *win;
	Fl_Pack *pack;
	int children;
	int vis_h;

public:
	// Normal FLTK constructors
	dockgroup(int x, int y, int w, int h, const char *l = 0);
	
	// point back to our parent
	void set_window(Fl_Window *w) {win = w;}

	// methods for adding or removing toolgroups from the dock
	void add(Fl_Widget *w);
	void remove(Fl_Widget *w);
	
	// dock diagnostic
	char *dock_check(void);
};

#endif // _HAVE_DOCK_GRP_HDR_

