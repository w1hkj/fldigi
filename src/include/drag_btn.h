#ifndef _HAVE_DRAG_BTN_HDR_
#define _HAVE_DRAG_BTN_HDR_

#include <FL/Fl_Box.H>

class drag_btn : public Fl_Box
{
private:
	int x1, y1; 	// click posn., used for dragging and docking checks
	int xoff, yoff; // origin used for dragging calcs
	int was_docked; // used in handle to note that we have just undocked

protected:
	// override box draw method to do our textured dragger look
	void draw();
	// override handle method to catch drag/dock operations
	int handle(int event);

public:
	// basic constructor
	drag_btn(int x, int y, int w, int h, const char *l = 0);
};

#endif // _HAVE_DRAG_BTN_HDR_

/* End of File */

