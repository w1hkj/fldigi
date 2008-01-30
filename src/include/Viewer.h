#ifndef Viewer_h
#define Viewer_h
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>

extern Fl_Double_Window *dlgViewer;
extern Fl_Button *btnCloseViewer;
extern Fl_Button *btnClearViewer;
extern Fl_Hold_Browser *brwsViewer;

extern Fl_Double_Window* createViewer();

extern void openViewer();
extern void viewaddchr(int ch, int freq, char c);

#endif
