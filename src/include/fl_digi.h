// ----------------------------------------------------------------------------
//
//	fl_digi.h
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#ifndef fl_digi_h

#define fl_digi_h

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Check_Button.H>

#include "combo.h"
#include "TextView.h"
#include "FTextView.h"
#include "raster.h"
#include "waterfall.h"
#include "digiscope.h"
#include "globals.h"
#include "mixer.h"

extern Fl_Double_Window *fl_digi_main;

extern ReceiveWidget		*ReceiveText;
extern TransmitWidget		*TransmitText;
extern Raster			*FHdisp;
extern Fl_Tile_check		*TiledGroup;
extern Fl_Text_Buffer	*rcvBuffer;
extern Fl_Text_Buffer	*xmtBuffer;
extern Fl_Box			*StatusBar;
extern Fl_Box			*Status2;
extern Fl_Box			*Status1;
extern Fl_Box			*WARNstatus;
extern Fl_Button		*MODEstatus;
extern Fl_Slider		*sldrSquelch;
extern Fl_Progress		*pgrsSquelch;
extern Fl_Button 		*btnMacro[];
extern Fl_Input			*inpFreq;
extern Fl_ComboBox		*cboBand;
extern Fl_Button		*btnSideband;
extern Fl_Input			*inpTime;
extern Fl_Input			*inpCall;
extern Fl_Input			*inpName;
extern Fl_Input			*inpRstIn;
extern Fl_Input			*inpRstOut;
extern Fl_Input			*inpQth;
extern Fl_Input			*inpLoc;
extern Fl_Input			*inpNotes;
extern Fl_Input			*inpAZ;	// WA5ZNU
extern Fl_Button		*qsoClear;
extern Fl_Button		*qsoSave;
extern Fl_Button		*btnMacroTimer;
extern Fl_Slider		*valRcvMixer;
extern Fl_Slider		*valXmtMixer;
extern Fl_Light_Button	*afconoff;
extern Fl_Light_Button	*sqlonoff;
extern Fl_Check_Button	*chk_afconoff;
extern Fl_Check_Button	*chk_sqlonoff;

extern bool				useCheckButtons;
extern bool				altMacros;

extern waterfall		*wf;
extern Digiscope		*digiscope;


extern void create_fl_digi_main();
extern Fl_Menu_Item menu_[];
extern Fl_Menu_Item *getMenuItem(const char *caption);
extern void activate_rig_menu_item(bool b);
extern void activate_test_menu_item(bool b);


extern void put_freq(double frequency);
extern void put_Bandwidth(int bandwidth);
extern void display_metric(double metric);
extern void put_cwRcvWPM(double wpm);
extern void set_scope(double *data, int len, bool autoscale = true);
extern void set_phase(double phase, bool highlight);
extern void set_rtty(double, double, double);
extern void set_video(double *, int);
extern void set_CWwpm();
extern void put_rx_char(unsigned int data);
extern void put_sec_char( char chr );

extern void put_status(const char *msg, double timeout = 0);
extern void clear_status_cb(void *);
extern void clear_StatusMessages();
extern void put_MODEstatus(trx_mode mode);
extern void put_Status1(const char *msg);
extern void put_Status2(const char *msg);

extern void put_WARNstatus(double);

extern void put_rx_data(int *data, int len);
extern int get_tx_char();
extern int  get_secondary_char();
extern void put_echo_char(unsigned int data);
extern void resetRTTY();
extern void resetOLIVIA();
extern void resetDOMEX();
extern void resetSoundCard();
extern void restoreFocus();
extern void setReverse(int);
extern void clearQSO();
extern void closeRigDialog();

extern void setAfcOnOff(bool b);
extern void setSqlOnOff(bool b);
extern bool QueryAfcOnOff();
extern bool QuerySqlOnOff();

extern void init_modem(trx_mode mode);
extern void init_modem_sync(trx_mode mode);

extern void change_modem_param(int state);

#endif
