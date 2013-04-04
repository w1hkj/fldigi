#ifndef Viewer_h
#define Viewer_h

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/fl_show_colormap.H>

#include <string>

#include <config.h>
#include "viewpsk.h"
#include "psk_browser.h"
#include "flslider2.h"
#include "flinput2.h"

extern Fl_Double_Window *dlgViewer;
extern Fl_Value_Slider2 *sldrViewerSquelch;
extern Fl_Double_Window* createViewer();
extern Fl_Input2  *viewer_inp_seek;
extern pskBrowser *brwsViewer;

extern void openViewer();
extern void viewaddchr(int ch, int freq, char c, int md);
extern void viewerswap(int, int);
extern void initViewer();
extern void viewclearchannel(int ch);
extern void viewer_paste_freq(int freq);
extern void viewer_redraw();

#endif
