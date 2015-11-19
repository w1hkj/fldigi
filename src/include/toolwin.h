#ifndef _HAVE_TOOLWIN_HDR_
#define _HAVE_TOOLWIN_HDR_

/* fltk includes */
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>

class toolwin : public Fl_Double_Window
{
#define TW_MAX_FLOATERS	16

protected:
	void create_dockable_window(void);
	short idx;
	static toolwin* active_list[TW_MAX_FLOATERS];
	static short active;
	void *tool_group;

public:
	// Normal FLTK constructors
	toolwin(int w, int h, const char *l = 0);
	toolwin(int x, int y, int w, int h, const char *l = 0);
	
	// destructor
	~toolwin();

	// methods for hiding/showing *all* the floating windows
	static void show_all(void);
	static void hide_all(void);

	// set the inner group
	void set_inner(void *v) {tool_group = v;}
};

#endif // _HAVE_TOOLWIN_HDR_

// End of file //
