// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

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

extern void initOptionMenus();
extern void clearList();
extern void updateSelect();
extern size_t addtoList(long val);
extern void buildlist();
extern void qso_movFreq(Fl_Widget* w, void*);
extern int	cb_qso_opMODE();
extern int  cb_qso_opBW();
extern void qso_setMode();
extern void setTitle();

extern void qso_addFreq();
extern void qso_delFreq();
extern void qso_selectFreq();
extern void qso_setFreq();
extern void qso_clearList();
extern void saveFreqList();

extern bool readRigXML();
extern bool init_Xml_RigDialog();
extern bool init_NoRig_RigDialog();
extern bool init_rigMEM_RigDialog();

#if USE_HAMLIB
extern bool init_Hamlib_RigDialog();
extern void selMode(rmode_t m);
extern std::string modeString(rmode_t m);
#endif

#endif
