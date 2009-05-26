#ifndef Fl_Tile_Check_h_
#define Fl_Tile_Check_h_

#include <FL/Fl_Tile.H>

/// A version of Fl_Tile that runs check callbacks and moves the boundary
/// between its child widgets only all resize checks return true.
class Fl_Tile_Check : public Fl_Tile
{
public:
	typedef bool (*resize_check_func)(void *, int, int);

	Fl_Tile_Check(int x, int y, int w, int h, const char* l = 0);

	int handle(int event);
	void add_resize_check(resize_check_func f, void *a);
	void remove_resize_check(resize_check_func f, void *a);
	void remove_checks(void);
	bool do_checks(int xd, int yd);

protected:
	int xstart, ystart;
	resize_check_func resize_checks[8];
	void *resize_args[8];
};

#endif // Fl_Tile_Check_h_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
