#ifndef RIGCAT_H_
#define RIGCAT_H_

#ifndef RIGCATTEST
#  error FIXME: file should not have been included
#endif

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <iostream>
#include <fstream>
#include <string>

extern Fl_Double_Window *window;
extern char *homedir;
extern string xmlfname;

extern void MilliSleep(long msecs);

#endif
