#ifndef RIG_SUPPORT_H
#define RIG_SUPPORT_H

#include <FL/Fl_Double_Window.H>

#include <string>
#include <list>
#include <sstream>
#include <iostream>
#include <ctype.h>

#include "serial.h"

#if USE_HAMLIB
	#include "hamlib.h"
#endif

using namespace std;

extern Fl_Double_Window *rigcontrol;
extern string windowTitle;
extern Cserial rigio;

extern long int freqlist[];
extern void initOptionMenus();
extern void setMode();
extern void setBW();
extern void selMode(int);
extern void selBW(int);
extern void selFreq(long int);
extern void sortList();
extern void clearList();
extern void updateSelect();
extern void addtoList(long val);
extern void buildlist();
extern int  movFreq();
extern void selectFreq();
extern void delFreq();
extern void addFreq();
extern void saveFreqList();

extern bool readRigXML();
extern bool init_Xml_RigDialog();
#if USE_HAMLIB
extern bool init_Hamlib_RigDialog();
extern void selMode(rmode_t m);
#endif

extern Fl_Double_Window * createRigDialog();

#endif
