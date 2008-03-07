#ifndef Viewer_h
#define Viewer_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>
#include <Fl/Fl_Group.H>
#include <Fl/Fl_Pack.H>

class pskBrowser : public Fl_Hold_Browser{
public:
	pskBrowser(int x, int y, int w, int h, const char *l = "")
		:Fl_Hold_Browser(x,y,w,h,l) {}
	~pskBrowser() {};
	void resize(int x, int y, int w, int h);
};

extern Fl_Double_Window *dlgViewer;
extern Fl_Button *btnCloseViewer;
extern Fl_Button *btnClearViewer;
extern pskBrowser *brwsViewer;

extern Fl_Double_Window* createViewer();

extern void openViewer();
extern void viewaddchr(int ch, int freq, char c);
extern void initViewer();
extern void viewclearchannel(int ch);
extern void viewer_paste_freq(int freq);

#endif
