#ifndef RIG_SUPPORT_H
#define RIG_SUPPORT_H

#include <FL/Fl_Double_Window.H>

#include <string>

#include "serial.h"
#if USE_HAMLIB
	#include "hamlib.h"
#endif

extern std::string windowTitle;
extern Cserial rigio;

extern void clearList();
extern void qso_movFreq(Fl_Widget* w, void*);
extern int	cb_qso_opMODE();
extern int  cb_qso_opBW();
extern void qso_setMode();
extern void setTitle();


extern void qso_createFreqList(int frqHorPos, int frqVrtPos, int btnWidth, int totalHeight, int Hmenu, int pad );
extern void qso_updateFreqColor(int selLin);
extern void qso_addFreq();
extern void qso_delFreq();
extern void qso_displayFreq( long long freq );
extern void qso_selectFreq();
extern void qso_setFreq();
extern void qso_clearList();
extern void saveFreqList();

extern bool readRigXML();
extern bool init_Xml_RigDialog();
extern bool init_NoRig_RigDialog();

#if USE_HAMLIB
extern bool init_Hamlib_RigDialog();
extern void selMode(rmode_t m);
extern const std::string & modeString(rmode_t m);
#endif

#endif
