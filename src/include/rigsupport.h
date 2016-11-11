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

#include "xmlrpcpp/XmlRpc.h"
#include "xmlrpcpp/XmlRpcValue.h"

#define DEFAULT_RIGXML_FILENAME "rig-not-assigned.xml"

extern std::string windowTitle;
extern Cserial rigio;

extern void initOptionMenus();
extern void clearList();
extern void updateSelect();
extern size_t addtoList(long val);
extern void build_frequencies2_list();
extern void qso_movFreq(Fl_Widget* w, void*);
extern int	cb_qso_opMODE();
extern int  cb_qso_opBW();
extern int  cb_qso_btnBW1();
extern int  cb_qso_opBW1();
extern int  cb_qso_btnBW2();
extern int  cb_qso_opBW2();
extern void qso_setMode();
extern void setTitle();

extern int  fwidths[];
extern void sendFreq(long int f);
extern void qso_addFreq();
extern void qso_delFreq();
extern void qso_selectFreq();
extern void qso_selectFreq(long int rfcarrier, int carrier);
extern void qso_setFreq();
extern void qso_setFreq(long int f);
extern void qso_clearList();
extern void saveFreqList();
extern void qso_updateEntry(int i, std::string usage);

extern bool readRigXML();
extern bool init_Xml_RigDialog();
extern bool init_NoRig_RigDialog();

extern bool ModeIsLSB(std::string);

#if USE_HAMLIB
extern bool init_Hamlib_RigDialog();
extern void selMode(rmode_t m);
extern std::string modeString(rmode_t m);
extern bool hamlib_USB();
extern bool hamlib_active();
#endif

// xmlrpc_rig specific

extern bool connected_to_flrig;

extern void xmlrpc_rig_set_qsy(long long rfc);
extern bool xmlrpc_USB();

extern void FLRIG_set_flrig_ab(int n);
extern void FLRIG_start_flrig_thread();

extern void stop_flrig_thread();
extern void reconnect_to_flrig();
extern void set_flrig_ptt(int on);
extern void set_flrig_freq(long int fr);
extern void set_flrig_mode(const char *md);
extern void set_flrig_bw(int bw1, int bw2 = 0);
extern void set_flrig_notch();

// GPIO export/unexport pin #
extern void export_gpio(int);
extern void unexport_gpio(int);

//------------------------------------------------------------------------------

#endif
