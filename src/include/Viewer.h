#ifndef Viewer_h
#define Viewer_h

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>

enum { VIEWER_LABEL_AF, VIEWER_LABEL_RF, VIEWER_LABEL_CH, VIEWER_LABEL_NTYPES };

class pskBrowser : public Fl_Hold_Browser{
public:
	pskBrowser(int x, int y, int w, int h, const char *l = "")
		:Fl_Hold_Browser(x,y,w,h,l) {}
	~pskBrowser() {};
	void resize(int x, int y, int w, int h);
};

extern Fl_Double_Window *dlgViewer;

extern Fl_Double_Window* createViewer();

extern void openViewer();
extern void viewaddchr(int ch, int freq, char c, int md);
extern void initViewer();
extern void viewclearchannel(int ch);
extern void viewer_paste_freq(int freq);

#endif
