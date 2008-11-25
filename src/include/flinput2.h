#ifndef FL_INPUT2_
#define FL_INPUT2_

#include <FL/Fl_Input.H>

class Fl_Input2 : public Fl_Input
{
public:
	Fl_Input2(int x, int y, int w, int h, const char* l = 0);
	int handle(int event);
};

#endif // FL_INPUT2_
