#ifndef _RIG_CAT_H
#define _RIG_CAT_H

#include <string>
#include <string>
#include <fstream>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Enumerations.H>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

#include <FL/fl_ask.H>
#include <FL/fl_file_chooser.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/fl_draw.H>

using namespace std;

extern char homedir[];
extern void clean_exit();

extern Fl_Double_Window *window;

extern string xmlfname;

#endif
