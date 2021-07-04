// ----------------------------------------------------------------------------
//
//	fl_digi.cxx
//
// Copyright (C) 2006-2021
//		Dave Freese, W1HKJ
// Copyright (C) 2007-2010
//		Stelios Bounanos, M0GLD
// Copyright (C) 2020-2021
//		John Phelps, KL4YFD
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <sys/types.h>

#ifdef __WOE32__
#  ifdef __CYGWIN__
#    include <w32api/windows.h>
#  else
#    include <windows.h>
#  endif
#endif

#include <cstdlib>
#include <cstdarg>
#include <string>
#include <fstream>
#include <algorithm>
#include <map>

#ifndef __WOE32__
#include <sys/wait.h>
#endif

//++++++++++++++++++
#include <FL/Fl_Scroll.H>
extern Fl_Scroll       *wefax_pic_rx_scroll;


#include "gettext.h"
#include "fl_digi.h"

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Pixmap.H>
#include <FL/Fl_Image.H>
//#include <FL/Fl_Tile.H>
#include <FL/x.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Tooltip.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Pack.H>

#include "waterfall.h"
#include "raster.h"
#include "progress.h"
#include "Panel.h"

#include "main.h"
#include "threads.h"
#include "trx.h"
#if USE_HAMLIB
	#include "hamlib.h"
#endif
#include "timeops.h"
#include "rigio.h"
#include "nullmodem.h"
#include "psk.h"
#include "cw.h"
#include "mfsk.h"
#include "wefax.h"
#include "wefax-pic.h"
#include "navtex.h"
#include "mt63.h"
#include "view_rtty.h"
#include "olivia.h"
#include "contestia.h"
#include "thor.h"
#include "dominoex.h"
#include "feld.h"
#include "throb.h"
//#include "pkt.h"
#include "fsq.h"
#include "ifkp.h"
#include "wwv.h"
#include "analysis.h"
#include "ssb.h"

#include "fmt.h"
#include "fmt_dialog.h"

#include "fileselect.h"

#include "smeter.h"
#include "pwrmeter.h"

#include "ascii.h"
#include "globals.h"
#include "misc.h"
#include "FTextRXTX.h"

#include "confdialog.h"
#include "configuration.h"
#include "status.h"

#include "macros.h"
#include "macroedit.h"
#include "logger.h"
#include "lookupcall.h"
#include "fd_logger.h"
#include "fd_view.h"

#include "font_browser.h"

#include "icons.h"
#include "pixmaps.h"

#include "rigsupport.h"

#include "logsupport.h"

#include "qrunner.h"

#include "Viewer.h"
#include "soundconf.h"

#include "htmlstrings.h"
#	include "xmlrpc.h"
#if BENCHMARK_MODE
#	include "benchmark.h"
#endif

#include "debug.h"
#include "re.h"
#include "network.h"
#include "spot.h"
#include "dxcc.h"
#include "locator.h"
#include "notify.h"
#include "strutil.h"

#include "test_signal.h"

#include "logbook.h"

#include "rx_extract.h"
#include "speak.h"
#include "flmisc.h"

#include "arq_io.h"
#include "data_io.h"
#include "kmlserver.h"
#include "psm/psm.h"
#include "n3fjp_logger.h"

#include "dx_cluster.h"
#include "dx_dialog.h"

#include "notifydialog.h"
#include "macroedit.h"
#include "rx_extract.h"
#include "wefax-pic.h"
#include "charsetdistiller.h"
#include "charsetlist.h"
#include "outputencoder.h"
#include "record_loader.h"
#include "record_browse.h"

#include "winkeyer.h"
#include "nanoIO.h"

#include "audio_alert.h"

#include "spectrum_viewer.h"

#include "contest.h"

#include "tabdefs.h"

#define CB_WHEN FL_WHEN_CHANGED | FL_WHEN_NOT_CHANGED | FL_WHEN_ENTER_KEY_ALWAYS | FL_WHEN_RELEASE_ALWAYS

#define LOG_TO_FILE_MLABEL     _("Log all RX/TX text")
#define RIGCONTROL_MLABEL      TAB_RIG_CONTROL
#define OPMODES_MLABEL         _("Op &Mode")
#define OPMODES_FEWER          _("Show fewer modes")
#define OPMODES_ALL            _("Show all modes")
#define OLIVIA_MLABEL            "Olivia"
#define CONTESTIA_MLABEL         "Contestia"
#define RTTY_MLABEL              "RTTY"
#define VIEW_MLABEL            _("&View")
#define MFSK_IMAGE_MLABEL      _("MFSK Image")
#define THOR_IMAGE_MLABEL      _("THOR Raw Image")
#define IFKP_IMAGE_MLABEL      _("IFKP Raw Image")
#define WEFAX_TX_IMAGE_MLABEL  _("Weather Fax Image TX")
#define CONTEST_MLABEL         _("Contest")
#define COUNTRIES_MLABEL       _("C&ountries")
#define UI_MLABEL              _("&UI")
#define RIGLOG_FULL_MLABEL     _("Full")
#define RIGLOG_PARTIAL_MLABEL  _("Partial")
#define RIGLOG_NONE_MLABEL     _("None")
#define DOCKEDSCOPE_MLABEL     _("Docked scope")
#define WF_MLABEL              _("Minimal controls")
#define SHOW_CHANNELS          _("Show channels")

#define LOG_CONNECT_SERVER     _("Connect to server")

// MAXIMUM allowable string lengths in log fields
#define MAX_FREQ 14
#define MAX_TIME 4
#define MAX_RST 3
#define MAX_CALL 30
#define MAX_NAME 30
#define MAX_AZ 3
#define MAX_QTH 50
#define MAX_STATE 2
#define MAX_LOC 8
#define MAX_SERNO 10
#define MAX_XCHG_IN 50
#define MAX_COUNTRY 50
#define MAX_COUNTY 100
#define MAX_NOTES 400
#define MAX_SECTION 20
#define MAX_CLASS   10

using namespace std;

void set599();

//regular expression parser using by mainViewer (pskbrowser)
fre_t seek_re("CQ", REG_EXTENDED | REG_ICASE | REG_NOSUB);

bool bWF_only = false;

Fl_Double_Window	*fl_digi_main				= (Fl_Double_Window *)0;
Fl_Double_Window	*scopeview					= (Fl_Double_Window *)0;
Fl_Double_Window	*field_day_viewer			= (Fl_Double_Window *)0;
Fl_Double_Window	*dxcluster_viewer			= (Fl_Double_Window *)0;
Fl_Double_Window	*rxaudio_dialog				= (Fl_Double_Window *)0;

Fl_Help_Dialog 		*help_dialog       = (Fl_Help_Dialog *)0;

Fl_Button			*btnDockMacro[48];

static Fl_Group		*mnuFrame;
Fl_Menu_Bar 		*mnu;

Fl_Box				*tx_timer = (Fl_Box *)0;
Fl_Light_Button		*btnAutoSpot = (Fl_Light_Button *)0;
Fl_Light_Button		*btnTune = (Fl_Light_Button *)0;
Fl_Light_Button		*btnRSID = (Fl_Light_Button *)0;
Fl_Light_Button		*btnTxRSID = (Fl_Light_Button *)0;
Fl_Button			*btnMacroTimer = (Fl_Button *)0;

Fl_Group			*center_group = (Fl_Group *)0;
Fl_Group			*text_group;
Fl_Group			*wefax_group = 0;
Fl_Group			*mvgroup = 0;

Panel				*text_panel = 0;

//------------------------------------------------------------------------------
// groups and widgets used exclusively for FSQCALL

Fl_Group			*fsq_group = 0;
Fl_Group			*fsq_upper = 0;
Fl_Group			*fsq_lower = 0;
Fl_Group			*fsq_upper_left = 0;
Fl_Group			*fsq_upper_right = 0;
Fl_Group			*fsq_lower_left = 0;
Fl_Group			*fsq_lower_right = 0;

Panel				*fsq_left = (Panel *)0;
Fl_Box				*fsq_minbox = (Fl_Box *)0;
FTextRX				*fsq_rx_text = (FTextRX *)0;
FTextTX				*fsq_tx_text = (FTextTX *)0;
Fl_Browser			*fsq_heard = (Fl_Browser *)0;

Fl_Light_Button		*btn_FSQCALL = (Fl_Light_Button *)0;
Fl_Light_Button		*btn_SELCAL = (Fl_Light_Button *)0;
Fl_Light_Button		*btn_MONITOR = (Fl_Light_Button *)0;
Fl_Button			*btn_FSQQTH = (Fl_Button *)0;
Fl_Button			*btn_FSQQTC = (Fl_Button *)0;
Fl_Button			*btn_FSQCQ = (Fl_Button *)0;
Progress			*ind_fsq_speed = (Progress *)0;
Progress			*ind_fsq_s2n = (Progress *)0;

//------------------------------------------------------------------------------
// groups and widgets used exclusively for IFKP
Fl_Group		*ifkp_group = (Fl_Group *)0;
Fl_Box			*ifkp_minbox = (Fl_Box *)0;
Fl_Group		*ifkp_left = (Fl_Group *)0;
FTextRX			*ifkp_rx_text = (FTextRX *)0;
FTextTX			*ifkp_tx_text = (FTextTX *)0;
Fl_Group		*ifkp_right = (Fl_Group *)0;
Fl_Browser		*ifkp_heard = (Fl_Browser *)0;
Progress		*ifkp_s2n_progress = (Progress *)0;
picture			*ifkp_avatar = (picture *)0;

//----------------------------------------------------------------------
// FMT group
Fl_Group		*fmt_group = (Fl_Group *)0;

//------------------------------------------------------------------------------
// thor avatar
picture			*thor_avatar = (picture *)0;

//------------------------------------------------------------------------------

Fl_Group			*macroFrame1 = (Fl_Group *)0;
Fl_Group			*macroFrame2 = (Fl_Group *)0;
Fl_Group			*mf_group1 = (Fl_Group *)0;
Fl_Group			*mf_group2 = (Fl_Group *)0;
Fl_Group			*tbar = (Fl_Group *)0;

FTextRX				*ReceiveText = 0;
FTextTX				*TransmitText = 0;
Raster				*FHdisp;
Fl_Box				*minbox;
int					oix;

pskBrowser			*mainViewer = (pskBrowser *)0;
Fl_Input2			*txtInpSeek = (Fl_Input2 *)0;

status_box			*StatusBar = (status_box *)0;
Fl_Box				*Status2 = (Fl_Box *)0;
Fl_Box				*Status1 = (Fl_Box *)0;
Fl_Counter2			*cntTxLevel = (Fl_Counter2 *)0;
Fl_Counter2			*cntCW_WPM=(Fl_Counter2 *)0;
Fl_Button			*btnCW_Default=(Fl_Button *)0;
Fl_Box				*WARNstatus = (Fl_Box *)0;
Fl_Button			*MODEstatus = (Fl_Button *)0;
Fl_Button 			*btnMacro[NUMMACKEYS * NUMKEYROWS];
Fl_Button			*btnAltMacros1 = (Fl_Button *)0;
Fl_Button			*btnAltMacros2 = (Fl_Button *)0;
Fl_Light_Button		*btnAFC = (Fl_Light_Button *)0;
Fl_Light_Button		*btnSQL = (Fl_Light_Button *)0;
Fl_Light_Button		*btnPSQL = (Fl_Light_Button *)0;
Fl_Box				*corner_box = (Fl_Box *)0;

vumeter				*VuMeter = (vumeter *)0;
Fl_Box				*VuBox   = (Fl_Box *)0;

Fl_Group			*RigControlFrame = (Fl_Group *)0;
Fl_Group			*RigViewerFrame = (Fl_Group *)0;

cFreqControl 		*qsoFreqDisp = (cFreqControl *)0;
Fl_Group			*qso_combos = (Fl_Group *)0;
Fl_ListBox			*qso_opMODE = (Fl_ListBox *)0;
Fl_Group			*qso_opGROUP = (Fl_Group *)0;
Fl_ListBox			*qso_opBW = (Fl_ListBox *)0;
Fl_Button			*qso_btnBW1 = (Fl_Button *)0;
Fl_ListBox			*qso_opBW1 = (Fl_ListBox *)0;
Fl_Button			*qso_btnBW2 = (Fl_Button *)0;
Fl_ListBox			*qso_opBW2 = (Fl_ListBox *)0;
Fl_Button			*qso_opPICK = (Fl_Button *)0;

Fl_Button			*qsoClear;
Fl_Button			*qsoSave;

Fl_Input2			*inpFreq = (Fl_Input2 *)0;
Fl_Input2			*inpTimeOff = (Fl_Input2 *)0;
Fl_Input2			*inpTimeOn = (Fl_Input2 *)0;
Fl_Button			*btnTimeOn;
Fl_Input2			*inpCall = (Fl_Input2 *)0;
Fl_Input2			*inpName = (Fl_Input2 *)0;
Fl_Input2			*inpRstIn = (Fl_Input2 *)0;
Fl_Input2			*inpRstOut = (Fl_Input2 *)0;
Fl_Input2			*inpQTH = (Fl_Input2 *)0;
Fl_Input2			*inpQth = (Fl_Input2 *)0;
Fl_Input2			*inpLoc = (Fl_Input2 *)0;
Fl_Input2			*inpState = (Fl_Input2 *)0;

Fl_Input2			*inpCounty = (Fl_Input2 *)0;

Fl_ComboBox			*cboCountyQSO = (Fl_ComboBox *)0;

Fl_ComboBox			*cboCountry = (Fl_ComboBox *)0;
Fl_ComboBox			*cboCountryQSO = (Fl_ComboBox *)0;
Fl_ComboBox			*cboCountryAICW2 = (Fl_ComboBox *)0;
Fl_ComboBox			*cboCountryAIDX = (Fl_ComboBox *)0;
Fl_ComboBox			*cboCountryWAE2 = (Fl_ComboBox *)0;
Fl_ComboBox			*cboCountryAIDX2 = (Fl_ComboBox *)0;
Fl_ComboBox			*cboCountryRTU2 = (Fl_ComboBox *)0;

Fl_Input2			*inpSerNo = (Fl_Input2 *)0;
Fl_Input2			*outSerNo = (Fl_Input2 *)0;
Fl_Input2			*inpXchgIn = (Fl_Input2 *)0;
// Field Day fields
Fl_Input2			*inpClass = (Fl_Input2 *)0;
Fl_Input2			*inpSection = (Fl_Input2 *)0;
// CQWW fields
Fl_Input2			*inp_CQzone = (Fl_Input2 *)0;
Fl_Input2			*inp_CQstate = (Fl_Input2 *)0;
// Kids Day fields
Fl_Input2			*inp_KD_age = (Fl_Input2 *)0;

Fl_Input2			*inpVEprov = (Fl_Input2 *)0;
Fl_Input2			*inpNotes = (Fl_Input2 *)0;
Fl_Input2			*inpAZ = (Fl_Input2 *)0;


Fl_Button			*qsoTime;
Fl_Button			*btnQRZ;
//Fl_Button			*CFtoggle = (Fl_Button *)0;

// Top Frame 1 group controls
Fl_Group			*Logging_frame = (Fl_Group *)0;
Fl_Group			*Logging_frame_1 = (Fl_Group *)0;

Fl_Tabs				*NFtabs = (Fl_Tabs *)0;
Fl_Group			*gGEN_QSO_1 = (Fl_Group *)0;
Fl_Group			*NotesFrame = (Fl_Group *)0;
Fl_Group			*Ccframe = (Fl_Group *)0;
Fl_Group			*TopFrame1 = (Fl_Group *)0;
Fl_Input2			*inpFreq1 = (Fl_Input2 *)0;
Fl_Input2			*inpTimeOff1 = (Fl_Input2 *)0;
Fl_Input2			*inpTimeOn1 = (Fl_Input2 *)0;
Fl_Button			*btnTimeOn1;
Fl_Input2			*inpCall1 = (Fl_Input2 *)0;
Fl_Input2			*inpName1 = (Fl_Input2 *)0;
Fl_Input2			*inpRstIn1 = (Fl_Input2 *)0;
Fl_Input2			*inpRstOut1 = (Fl_Input2 *)0;
Fl_Input2			*inpState1 = (Fl_Input2 *)0;
Fl_Input2			*inpLoc1 = (Fl_Input2 *)0;
// Generic contest sub frame
Fl_Group			*gGEN_CONTEST = (Fl_Group *)0;
Fl_Input2			*inpXchgIn1 = (Fl_Input2 *)0;
Fl_Input2			*outSerNo1 = (Fl_Input2 *)0;
Fl_Input2			*inpSerNo1 = (Fl_Input2 *)0;
// FD contest sub frame
Fl_Group			*gFD = (Fl_Group *)0;
Fl_Input2			*inp_FD_class1 = (Fl_Input2 *)0;
Fl_Input2			*inp_FD_section1 = (Fl_Input2 *)0;
// Kids Day fields
Fl_Group			*gKD_1 = (Fl_Group *)0;
Fl_Input2			*inp_KD_age1 = (Fl_Input2 *)0;
Fl_Input2			*inp_KD_state1 = (Fl_Input2 *)0;
Fl_Input2			*inp_KD_VEprov1 = (Fl_Input2 *)0;
Fl_Input2			*inp_KD_XchgIn1 = (Fl_Input2 *)0;
// CQWW RTTY contest sub frame
Fl_Group			*gCQWW_RTTY = (Fl_Group *)0;
Fl_Input2			*inp_CQzone1 = (Fl_Input2 *)0;
Fl_Input2			*inp_CQstate1 = (Fl_Input2 *)0;
// CQWW DX contest sub frame
Fl_Group			*gCQWW_DX = (Fl_Group *)0;
Fl_Input2			*inp_CQDXzone1 = (Fl_Input2 *)0;
// CW Sweepstakes contest sub frame
Fl_Group			*gCWSS = (Fl_Group *)0;
Fl_Input2			*outSerNo3 = (Fl_Input2 *)0;
Fl_Input2			*inp_SS_SerialNoR = (Fl_Input2 *)0;
Fl_Input2			*inp_SS_Precedence = (Fl_Input2 *)0;
Fl_Input2			*inp_SS_Check = (Fl_Input2 *)0;
Fl_Input2			*inp_SS_Section = (Fl_Input2 *)0;
Fl_Input2			*inp_SS_SerialNoR1 = (Fl_Input2 *)0;
Fl_Input2			*inp_SS_Precedence1 = (Fl_Input2 *)0;
Fl_Input2			*inp_SS_Check1 = (Fl_Input2 *)0;
Fl_Input2			*inp_SS_Section1 = (Fl_Input2 *)0;
// 1010 contest
Fl_Group			*g1010 = (Fl_Group *)0;
Fl_Input2			*inp_1010_nr = (Fl_Input2 *)0;
Fl_Input2			*inp_1010_nr1 = (Fl_Input2 *)0;
Fl_Input2			*inp_1010_XchgIn1 = (Fl_Input2 *)0;
// VHF contest
Fl_Group			*gVHF = (Fl_Group *)0;
Fl_Input2			*inp_vhf_RSTin1 = (Fl_Input2 *)0;
Fl_Input2			*inp_vhf_RSTout1 = (Fl_Input2 *)0;
Fl_Input2			*inp_vhf_Loc1 = (Fl_Input2 *)0;
// ARRL Round Up Contest
Fl_Group			*gARR = (Fl_Group *)0;
Fl_Input2			*inp_ARR_Name2 = (Fl_Input2 *)0;
Fl_Input2			*inp_ARR_check = (Fl_Input2 *)0;
Fl_Input2			*inp_ARR_check1 = (Fl_Input2 *)0;
Fl_Input2			*inp_ARR_check2 = (Fl_Input2 *)0;
Fl_Input2			*inp_ARR_XchgIn1 = (Fl_Input2 *)0;
Fl_Input2			*inp_ARR_XchgIn2 = (Fl_Input2 *)0;
// ARRL School Roundup - LOG_ASCR
Fl_Group			*gASCR = (Fl_Group *)0;
Fl_Input2			*inp_ASCR_class1 = (Fl_Input2 *)0;
Fl_Input2			*inp_ASCR_XchgIn1 = (Fl_Input2 *)0;
// LOG_NAQP - North American QSO Party
Fl_Group			*gNAQP = (Fl_Group *)0;
Fl_Input2			*inpSPCnum = (Fl_Input2 *)0;  // same name used in N3FJP loggers
Fl_Input2			*inpSPCnum_NAQP1 = (Fl_Input2 *)0;
// LOG_ARRL_RTTY - ARRL RTTY Roundup
Fl_Group			*gARRL_RTTY= (Fl_Group *)0;
Fl_Input2			*inpRTU_stpr1 = (Fl_Input2 *)0;
Fl_Input2			*inpRTU_serno1 = (Fl_Input2 *)0;
// LOG_IARI - Italian International DX
Fl_Group			*gIARI = (Fl_Group *)0;
Fl_Input2			*inp_IARI_PR1 = (Fl_Input2 *)0;
Fl_Input2			*out_IARI_SerNo1 = (Fl_Input2 *)0;
Fl_Input2			*inp_IARI_SerNo1 = (Fl_Input2 *)0;
Fl_Input2			*inp_IARI_RSTin2 = (Fl_Input2 *)0;
Fl_Input2			*inp_IARI_RSTout2 = (Fl_Input2 *)0;
Fl_Input2			*out_IARI_SerNo2 = (Fl_Input2 *)0;
Fl_Input2			*inp_IARI_SerNo2 = (Fl_Input2 *)0;
Fl_Input2			*inp_IARI_PR2= (Fl_Input2 *)0;
Fl_ComboBox			*cboCountryIARI2 = (Fl_ComboBox *)0;
// LOG_NAS - North American Sprint
Fl_Group			*gNAS = (Fl_Group *)0;
Fl_Input2			*outSerNo5 = (Fl_Input2 *)0;
Fl_Input2			*inp_ser_NAS1 = (Fl_Input2 *)0;
Fl_Input2			*inpSPCnum_NAS1 = (Fl_Input2 *)0;
// LOG_AIDX - African All Mode
Fl_Group			*gAIDX = (Fl_Group *)0;
Fl_Input2			*outSerNo7 = (Fl_Input2 *)0;
Fl_Input2			*inpSerNo3 = (Fl_Input2 *)0;
// LOG_JOTA - Jamboree On The Air
Fl_Group			*gJOTA = (Fl_Group *)0;
Fl_Input2			*inp_JOTA_troop = (Fl_Input2 *)0;
Fl_Input2			*inp_JOTA_scout = (Fl_Input2 *)0;
Fl_Input2			*inp_JOTA_scout1 = (Fl_Input2 *)0;
Fl_Input2			*inp_JOTA_troop1 = (Fl_Input2 *)0;
Fl_Input2			*inp_JOTA_spc = (Fl_Input2 *)0;
Fl_Input2			*inp_JOTA_spc1 = (Fl_Input2 *)0;
// LOG_AICW - ARRL International DX (cw)
Fl_Group			*gAICW = (Fl_Group *)0;
Fl_Input2			*inpSPCnum_AICW1 = (Fl_Input2 *)0;
// LOG_SQSO
Fl_Group			*gSQSO = (Fl_Group *)0;
Fl_Input2			*inpSQSO_state1 = (Fl_Input2 *)0;
Fl_Input2			*inpSQSO_state2 = (Fl_Input2 *)0;
Fl_Input2			*inpSQSO_county1 = (Fl_Input2 *)0;
Fl_Input2			*inpSQSO_county2 = (Fl_Input2 *)0;
Fl_Input2			*inpSQSO_serno1 = (Fl_Input2 *)0;
Fl_Input2			*inpSQSO_serno2 = (Fl_Input2 *)0;
Fl_Input2			*outSQSO_serno1 = (Fl_Input2 *)0;
Fl_Input2			*outSQSO_serno2 = (Fl_Input2 *)0;
Fl_Input2			*inpRstIn_SQSO2 = (Fl_Input2 *)0;
Fl_Input2			*inpRstOut_SQSO2 = (Fl_Input2 *)0;
Fl_Input2			*inpSQSO_name2 = (Fl_Input2 *)0;
Fl_Input2			*inpSQSO_category = (Fl_Input2 *)0;
Fl_Input2			*inpSQSO_category1 = (Fl_Input2 *)0;
Fl_Input2			*inpSQSO_category2 = (Fl_Input2 *)0;
// LOG_CQ_WPX
Fl_Group			*gCQWPX = (Fl_Group *)0;
Fl_Input2			*inpSerNo_WPX1 = (Fl_Input2 *)0;
Fl_Input2			*inpSerNo_WPX2 = (Fl_Input2 *)0;
Fl_Input2			*outSerNo_WPX1 = (Fl_Input2 *)0;
Fl_Input2			*outSerNo_WPX2 = (Fl_Input2 *)0;
Fl_Input2			*inpRstIn_WPX2 = (Fl_Input2 *)0;
Fl_Input2			*inpRstOut_WPX2 = (Fl_Input2 *)0;
// LOG_WAE
Fl_Group			*gWAE = (Fl_Group *)0;
Fl_Input2			*inpSerNo_WAE1 = (Fl_Input2 *)0;
Fl_Input2			*inpSerNo_WAE2 = (Fl_Input2 *)0;
Fl_Input2			*outSerNo_WAE1 = (Fl_Input2 *)0;
Fl_Input2			*outSerNo_WAE2 = (Fl_Input2 *)0;
Fl_Input2			*inpRstIn_WAE2 = (Fl_Input2 *)0;
Fl_Input2			*inpRstOut_WAE2 = (Fl_Input2 *)0;

//----------------------------------------------------------------------
// Single Line Rig / Logging Controls
cFreqControl 		*qsoFreqDisp1 = (cFreqControl *)0;

// Top Frame 2 group controls - no contest
Fl_Group			*TopFrame2 = (Fl_Group *)0;
cFreqControl		*qsoFreqDisp2 = (cFreqControl *)0;
Fl_Input2	        *inpTimeOff2 = (Fl_Input2 *)0;
Fl_Input2			*inpTimeOn2 = (Fl_Input2 *)0;
Fl_Button			*btnTimeOn2;
Fl_Input2			*inpCall2 = (Fl_Input2 *)0;
Fl_Input2			*inpName2 = (Fl_Input2 *)0;
Fl_Input2			*inpRstIn2 = (Fl_Input2 *)0;
Fl_Input2			*inpRstOut2 = (Fl_Input2 *)0;
Fl_Button			*qso_opPICK2;
Fl_Button			*qsoClear2;
Fl_Button			*qsoSave2;
Fl_Button			*btnQRZ2;

// Top Frame 3 group controls - contest
Fl_Group			*TopFrame3 = (Fl_Group *)0;
Fl_Group			*TopFrame3a = (Fl_Group *)0;

Fl_Group		*log_generic_frame = (Fl_Group *)0;
Fl_Group		*log_fd_frame = (Fl_Group *)0;
Fl_Group		*log_kd_frame = (Fl_Group *)0;
Fl_Group		*log_1010_frame = (Fl_Group *)0;
Fl_Group		*log_arr_frame = (Fl_Group *)0;
Fl_Group		*log_vhf_frame = (Fl_Group *)0;
Fl_Group		*log_cqww_frame = (Fl_Group *)0;
Fl_Group		*log_cqww_rtty_frame = (Fl_Group *)0;
Fl_Group		*log_cqss_frame = (Fl_Group *)0;
Fl_Group		*log_cqwpx_frame = (Fl_Group *)0;
Fl_Group		*log_ascr_frame = (Fl_Group *)0;
Fl_Group		*log_naqp_frame = (Fl_Group *)0;
Fl_Group		*log_rtty_frame = (Fl_Group *)0;
Fl_Group		*log_iari_frame = (Fl_Group *)0;
Fl_Group		*log_nas_frame = (Fl_Group *)0;
Fl_Group		*log_aidx_frame = (Fl_Group *)0;
Fl_Group		*log_jota_frame = (Fl_Group *)0;
Fl_Group		*log_aicw_frame = (Fl_Group *)0;
Fl_Group		*log_sqso_frame = (Fl_Group *)0;
Fl_Group		*log_wae_frame = (Fl_Group *)0;

cFreqControl 	*qsoFreqDisp3 = (cFreqControl *)0;
Fl_Button		*qso_opPICK3;
Fl_Button		*qsoClear3;
Fl_Button		*qsoSave3;

Fl_Group		*TopFrame3b = (Fl_Group *)0;
Fl_Input2		*inpCall3 = (Fl_Input2 *)0;

// Generic contest fields
Fl_Input2		*inpTimeOff3 = (Fl_Input2 *)0;
Fl_Input2		*inpTimeOn3 = (Fl_Input2 *)0;
Fl_Button		*btnTimeOn3;
Fl_Input2		*outSerNo2 = (Fl_Input2 *)0;
Fl_Input2		*inpSerNo2 = (Fl_Input2 *)0;
Fl_Input2		*inpXchgIn2 = (Fl_Input2 *)0;
// Field Day fields
Fl_Input2		*inpTimeOff4 = (Fl_Input2 *)0;
Fl_Input2		*inpTimeOn4 = (Fl_Input2 *)0;
Fl_Button		*btnTimeOn4;
Fl_Input2		*inp_FD_class2 = (Fl_Input2 *)0;
Fl_Input2		*inp_FD_section2 = (Fl_Input2 *)0;
// Kids Day fields
Fl_Input2		*inp_KD_name2 = (Fl_Input2 *)0;
Fl_Input2		*inp_KD_age2 = (Fl_Input2 *)0;
Fl_Input2		*inp_KD_state2 = (Fl_Input2 *)0;
Fl_Input2		*inp_KD_VEprov2 = (Fl_Input2 *)0;
Fl_Input2		*inp_KD_XchgIn2 = (Fl_Input2 *)0;
// CQWW RTTY fields
Fl_Input2		*inp_CQ_RSTin2 = (Fl_Input2 *)0;
Fl_Input2		*inp_CQ_RSTout2 = (Fl_Input2 *)0;
Fl_Input2		*inp_CQzone2 = (Fl_Input2 *)0;
Fl_Input2		*inp_CQstate2 = (Fl_Input2 *)0;
Fl_ComboBox		*cboCountryCQ2 = (Fl_ComboBox *)0;
// CQWW DX fields
Fl_Input2		*inp_CQDX_RSTin2 = (Fl_Input2 *)0;
Fl_Input2		*inp_CQDX_RSTout2 = (Fl_Input2 *)0;
Fl_Input2		*inp_CQDXzone2 = (Fl_Input2 *)0;
Fl_ComboBox		*cboCountryCQDX2 = (Fl_ComboBox *)0;
// CW Sweepstakes contest sub frame
Fl_Input2		*outSerNo4 = (Fl_Input2 *)0;
Fl_Input2		*inp_SS_SerialNoR2 = (Fl_Input2 *)0;
Fl_Input2		*inp_SS_Precedence2 = (Fl_Input2 *)0;
Fl_Input2		*inp_SS_Check2 = (Fl_Input2 *)0;
Fl_Input2		*inp_SS_Section2 = (Fl_Input2 *)0;
// 1010 contest
Fl_Input2		*inp_1010_name2 = (Fl_Input2 *)0;
Fl_Input2		*inp_1010_nr2 = (Fl_Input2 *)0;
Fl_Input2		*inp_1010_XchgIn2 = (Fl_Input2 *)0;
// VHF contest
Fl_Input2		*inp_vhf_RSTin2 = (Fl_Input2 *)0;
Fl_Input2		*inp_vhf_RSTout2 = (Fl_Input2 *)0;
Fl_Input2		*inp_vhf_Loc2 = (Fl_Input2 *)0;
// ARRL School Roundup - LOG_ASCR
Fl_Input2		*inp_ASCR_name2 = (Fl_Input2 *)0;
Fl_Input2		*inp_ASCR_class2 = (Fl_Input2 *)0;
Fl_Input2		*inp_ASCR_XchgIn2 = (Fl_Input2 *)0;
Fl_Input2		*inp_ASCR_RSTin2 = (Fl_Input2 *)0;
Fl_Input2		*inp_ASCR_RSTout2 = (Fl_Input2 *)0;
// LOG_NAQP
Fl_Input2		*inpTimeOff5 = (Fl_Input2 *)0;
Fl_Input2		*inpTimeOn5 = (Fl_Input2 *)0;
Fl_Button		*btnTimeOn5;
Fl_Input2		*inpNAQPname2;
Fl_Input2		*inpSPCnum_NAQP2 = (Fl_Input2 *)0;
// LOG_ARRL_RTTY - ARRL RTTY Roundup
Fl_Input2		*inpRTU_stpr2 = (Fl_Input2 *)0;
Fl_Input2		*inpRTU_RSTin2 = (Fl_Input2 *)0;
Fl_Input2		*inpRTU_RSTout2 = (Fl_Input2 *)0;
Fl_Input2		*inpRTU_serno2 = (Fl_Input2 *)0;
// LOG_NAS - NA Sprint
Fl_Input2		*outSerNo6 = (Fl_Input2 *)0;
Fl_Input2		*inp_ser_NAS2 = (Fl_Input2 *)0;
Fl_Input2		*inpSPCnum_NAS2 = (Fl_Input2 *)0;
Fl_Input2		*inp_name_NAS2 = (Fl_Input2 *)0;
// LOG_AIDX - African All Mode
Fl_Input2		*inpRstIn3 = (Fl_Input2 *)0;
Fl_Input2		*inpRstOut3 = (Fl_Input2 *)0;
Fl_Input2		*outSerNo8 = (Fl_Input2 *)0;
Fl_Input2		*inpSerNo4 = (Fl_Input2 *)0;
// LOG_JOTA - Jamboree On The Air
Fl_Input2		*inpRstIn4 = (Fl_Input2 *)0;
Fl_Input2		*inpRstOut4 = (Fl_Input2 *)0;
Fl_Input2		*inp_JOTA_scout2 = (Fl_Input2 *)0;
Fl_Input2		*inp_JOTA_troop2 = (Fl_Input2 *)0;
Fl_Input2		*inp_JOTA_spc2 = (Fl_Input2 *)0;
// LOG_AICW - ARRL International DX (cw)
Fl_Input2		*inpRstIn_AICW2 = (Fl_Input2 *)0;
Fl_Input2		*inpRstOut_AICW2 = (Fl_Input2 *)0;
Fl_Input2		*inpSPCnum_AICW2 = (Fl_Input2 *)0;

// Used when no logging frame visible
Fl_Input2		*inpCall4 = (Fl_Input2 *)0;

Fl_Browser		*qso_opBrowser = (Fl_Browser *)0;
Fl_Button		*qso_btnAddFreq = (Fl_Button *)0;
Fl_Button		*qso_btnSelFreq = (Fl_Button *)0;
Fl_Button		*qso_btnDelFreq = (Fl_Button *)0;
Fl_Button		*qso_btnClearList = (Fl_Button *)0;
Fl_Button		*qso_btnAct = 0;
Fl_Input2		*qso_inpAct = (Fl_Input2 *)0;
Fl_Group		*opUsageFrame = (Fl_Group *)0;
Fl_Output		*opOutUsage = (Fl_Output *)0;
Fl_Input2		*opUsage = (Fl_Input2 *)0;
Fl_Button		*opUsageEnter = (Fl_Button *)0;

Fl_Group	 	*wf_group = (Fl_Group *)0;
Fl_Group		*status_group = (Fl_Group *)0;

Fl_Value_Slider2	*mvsquelch = (Fl_Value_Slider2 *)0;
Fl_Button		*btnClearMViewer = 0;

static const int pad = 1;
static const int Hentry		= 24;
static const int Wbtn		= Hentry;
static int x_qsoframe	= Wbtn;
int Hmenu		= 22;
static const int Hqsoframe	= 2*pad + 3 * (Hentry + pad);

int Hstatus = 20;
int Hmacros = 20;

#define TB_HEIGHT 20
#define MACROBAR_MIN 18
#define MACROBAR_MAX 30

static int wf1			= 355;

static const int w_inpTime2	= 40;
static const int w_inpCall2	= 100;
static const int w_inpRstIn2	= 30;
static const int w_inpRstOut2	= 30;

// maximum 1 row height for raster display, FeldHell
static int minhtext = 42*2+6;

static int main_hmin;// = HMIN;

time_t program_start_time = 0;

bool xmlrpc_address_override_flag   = false;
bool xmlrpc_port_override_flag      = false;

bool arq_address_override_flag      = false;
bool arq_port_override_flag         = false;

bool kiss_address_override_flag     = false;
std::string override_xmlrpc_address = "";
std::string override_xmlrpc_port    = "";
std::string override_arq_address    = "";
std::string override_arq_port       = "";
std::string override_kiss_address   = "";
std::string override_kiss_io_port   = "";
std::string override_kiss_out_port  = "";
int override_kiss_dual_port_enabled = -1;          // Ensure this remains negative until assigned
int override_data_io_enabled        = DISABLED_IO;

int IMAGE_WIDTH;
int Hwfall;
int Wwfall;

int					altMacros = 0;

waterfall			*wf = (waterfall *)0;
Digiscope			*digiscope = (Digiscope *)0;

Fl_Slider2			*sldrSquelch = (Fl_Slider2 *)0;
Progress			*pgrsSquelch = (Progress *)0;

Smeter				*smeter = (Smeter *)0;
PWRmeter			*pwrmeter = (PWRmeter *)0;

Fl_Group			*pwrlevel_grp = (Fl_Group *)0;
Fl_Value_Slider2	*pwr_level = (Fl_Value_Slider2 *)0;
Fl_Button			*set_pwr_level = (Fl_Button *)0;

static Fl_Pixmap 		*addrbookpixmap = 0;

#if !defined(__APPLE__) && !defined(__WOE32__) && USE_X
Pixmap				fldigi_icon_pixmap;
#endif

// for character set conversion
int rxtx_charset;
static CharsetDistiller rx_chd;
static CharsetDistiller echo_chd;
static OutputEncoder    tx_encoder;

Fl_Menu_Item *getMenuItem(const char *caption, Fl_Menu_Item* submenu = 0);
void UI_select();
bool clean_exit(bool ask);

void cb_init_mode(Fl_Widget *, void *arg);

void cb_oliviaCustom(Fl_Widget *w, void *arg);

void cb_contestiaCustom(Fl_Widget *w, void *arg);

void cb_rtty45(Fl_Widget *w, void *arg);
void cb_rtty50(Fl_Widget *w, void *arg);
void cb_rtty75N(Fl_Widget *w, void *arg);
void cb_rtty75W(Fl_Widget *w, void *arg);
void cb_rtty100(Fl_Widget *w, void *arg);
void cb_rttyCustom(Fl_Widget *w, void *arg);

void cb_fsq2(Fl_Widget *w, void *arg);
void cb_fsq3(Fl_Widget *w, void *arg);
void cb_fsq4p5(Fl_Widget *w, void *arg);
void cb_fsq6(Fl_Widget *w, void *arg);
void cb_fsq1p5(Fl_Widget *w, void *arg);

void cb_ifkp0p5(Fl_Widget *w, void *arg);
void cb_ifkp1p0(Fl_Widget *w, void *arg);
void cb_ifkp2p0(Fl_Widget *w, void *arg);

void cb_ifkp0p5a(Fl_Widget *w, void *arg);
void cb_ifkp1p0a(Fl_Widget *w, void *arg);
void cb_ifkp2p0a(Fl_Widget *w, void *arg);

void set_colors();

//void cb_pkt1200(Fl_Widget *w, void *arg);
//void cb_pkt300(Fl_Widget *w, void *arg);
//void cb_pkt2400(Fl_Widget *w, void *arg);

Fl_Widget *modem_config_tab;
static const Fl_Menu_Item *quick_change;

static const Fl_Menu_Item quick_change_psk[] = {
	{ mode_info[MODE_PSK31].name, 0, cb_init_mode, (void *)MODE_PSK31 },
	{ mode_info[MODE_PSK63].name, 0, cb_init_mode, (void *)MODE_PSK63 },
	{ mode_info[MODE_PSK63F].name, 0, cb_init_mode, (void *)MODE_PSK63F },
	{ mode_info[MODE_PSK125].name, 0, cb_init_mode, (void *)MODE_PSK125 },
	{ mode_info[MODE_PSK250].name, 0, cb_init_mode, (void *)MODE_PSK250 },
	{ mode_info[MODE_PSK500].name, 0, cb_init_mode, (void *)MODE_PSK500 },
	{ mode_info[MODE_PSK1000].name, 0, cb_init_mode, (void *)MODE_PSK1000 },
	{ 0 }
};

static const Fl_Menu_Item quick_change_qpsk[] = {
	{ mode_info[MODE_QPSK31].name, 0, cb_init_mode, (void *)MODE_QPSK31 },
	{ mode_info[MODE_QPSK63].name, 0, cb_init_mode, (void *)MODE_QPSK63 },
	{ mode_info[MODE_QPSK125].name, 0, cb_init_mode, (void *)MODE_QPSK125 },
	{ mode_info[MODE_QPSK250].name, 0, cb_init_mode, (void *)MODE_QPSK250 },
	{ mode_info[MODE_QPSK500].name, 0, cb_init_mode, (void *)MODE_QPSK500 },
	{ 0 }
};

static const Fl_Menu_Item quick_change_8psk[] = {
	{ mode_info[MODE_8PSK125].name, 0, cb_init_mode, (void *)MODE_8PSK125 },
	{ mode_info[MODE_8PSK250].name, 0, cb_init_mode, (void *)MODE_8PSK250 },
	{ mode_info[MODE_8PSK500].name, 0, cb_init_mode, (void *)MODE_8PSK500 },
	{ mode_info[MODE_8PSK1000].name, 0, cb_init_mode, (void *)MODE_8PSK1000 },
	{ mode_info[MODE_8PSK125FL].name, 0, cb_init_mode, (void *)MODE_8PSK125FL },
	{ mode_info[MODE_8PSK125F].name, 0, cb_init_mode, (void *)MODE_8PSK125F },
	{ mode_info[MODE_8PSK250F].name, 0, cb_init_mode, (void *)MODE_8PSK250F },
	{ mode_info[MODE_8PSK250FL].name, 0, cb_init_mode, (void *)MODE_8PSK250FL },
	{ mode_info[MODE_8PSK500F].name, 0, cb_init_mode, (void *)MODE_8PSK500F },
	{ mode_info[MODE_8PSK1000F].name, 0, cb_init_mode, (void *)MODE_8PSK1000F },
	{ mode_info[MODE_8PSK1200F].name, 0, cb_init_mode, (void *)MODE_8PSK1200F },
	{ 0 }
};

static const Fl_Menu_Item quick_change_ofdm[] = {
	{ mode_info[MODE_OFDM_500F].name, 0, cb_init_mode, (void *)MODE_OFDM_500F },
	{ mode_info[MODE_OFDM_750F].name, 0, cb_init_mode, (void *)MODE_OFDM_750F },
//	{ mode_info[MODE_OFDM_2000F].name, 0, cb_init_mode, (void *)MODE_OFDM_2000F },
//	{ mode_info[MODE_OFDM_2000].name, 0, cb_init_mode, (void *)MODE_OFDM_2000 },
	{ mode_info[MODE_OFDM_3500].name, 0, cb_init_mode, (void *)MODE_OFDM_3500 },
	{ 0 }
};

static const Fl_Menu_Item quick_change_pskr[] = {
	{ mode_info[MODE_PSK125R].name, 0, cb_init_mode, (void *)MODE_PSK125R },
	{ mode_info[MODE_PSK250R].name, 0, cb_init_mode, (void *)MODE_PSK250R },
	{ mode_info[MODE_PSK500R].name, 0, cb_init_mode, (void *)MODE_PSK500R },
	{ mode_info[MODE_PSK1000R].name, 0, cb_init_mode, (void *)MODE_PSK1000R },
	{ 0 }
};

static const Fl_Menu_Item quick_change_psk_multiR[] = {
	{ mode_info[MODE_4X_PSK63R].name, 0, cb_init_mode, (void *)MODE_4X_PSK63R },
	{ mode_info[MODE_5X_PSK63R].name, 0, cb_init_mode, (void *)MODE_5X_PSK63R },
	{ mode_info[MODE_10X_PSK63R].name, 0, cb_init_mode, (void *)MODE_10X_PSK63R },
	{ mode_info[MODE_20X_PSK63R].name, 0, cb_init_mode, (void *)MODE_20X_PSK63R },
	{ mode_info[MODE_32X_PSK63R].name, 0, cb_init_mode, (void *)MODE_32X_PSK63R },

	{ mode_info[MODE_4X_PSK125R].name, 0, cb_init_mode, (void *)MODE_4X_PSK125R },
	{ mode_info[MODE_5X_PSK125R].name, 0, cb_init_mode, (void *)MODE_5X_PSK125R },
	{ mode_info[MODE_10X_PSK125R].name, 0, cb_init_mode, (void *)MODE_10X_PSK125R },
	{ mode_info[MODE_12X_PSK125R].name, 0, cb_init_mode, (void *)MODE_12X_PSK125R },
	{ mode_info[MODE_16X_PSK125R].name, 0, cb_init_mode, (void *)MODE_16X_PSK125R },

	{ mode_info[MODE_2X_PSK250R].name, 0, cb_init_mode, (void *)MODE_2X_PSK250R },
	{ mode_info[MODE_3X_PSK250R].name, 0, cb_init_mode, (void *)MODE_3X_PSK250R },
	{ mode_info[MODE_5X_PSK250R].name, 0, cb_init_mode, (void *)MODE_5X_PSK250R },
	{ mode_info[MODE_6X_PSK250R].name, 0, cb_init_mode, (void *)MODE_6X_PSK250R },
	{ mode_info[MODE_7X_PSK250R].name, 0, cb_init_mode, (void *)MODE_7X_PSK250R },

	{ mode_info[MODE_2X_PSK500R].name, 0, cb_init_mode, (void *)MODE_2X_PSK500R },
	{ mode_info[MODE_3X_PSK500R].name, 0, cb_init_mode, (void *)MODE_3X_PSK500R },
	{ mode_info[MODE_4X_PSK500R].name, 0, cb_init_mode, (void *)MODE_4X_PSK500R },

	{ mode_info[MODE_2X_PSK800R].name, 0, cb_init_mode, (void *)MODE_2X_PSK800R },

	{ mode_info[MODE_2X_PSK1000R].name, 0, cb_init_mode, (void *)MODE_2X_PSK1000R },
	{ 0 }
};

static const Fl_Menu_Item quick_change_psk_multi[] = {
	{ mode_info[MODE_12X_PSK125].name, 0, cb_init_mode, (void *)MODE_12X_PSK125 },
	{ mode_info[MODE_6X_PSK250].name, 0, cb_init_mode, (void *)MODE_6X_PSK250 },
	{ mode_info[MODE_2X_PSK500].name, 0, cb_init_mode, (void *)MODE_2X_PSK500 },
	{ mode_info[MODE_4X_PSK500].name, 0, cb_init_mode, (void *)MODE_4X_PSK500 },
	{ mode_info[MODE_2X_PSK800].name, 0, cb_init_mode, (void *)MODE_2X_PSK800 },
	{ mode_info[MODE_2X_PSK1000].name, 0, cb_init_mode, (void *)MODE_2X_PSK1000 },
	{ 0 }
};

static const Fl_Menu_Item quick_change_mfsk[] = {
	{ mode_info[MODE_MFSK4].name, 0, cb_init_mode, (void *)MODE_MFSK4 },
	{ mode_info[MODE_MFSK8].name, 0, cb_init_mode, (void *)MODE_MFSK8 },
	{ mode_info[MODE_MFSK16].name, 0, cb_init_mode, (void *)MODE_MFSK16 },
	{ mode_info[MODE_MFSK11].name, 0, cb_init_mode, (void *)MODE_MFSK11 },
	{ mode_info[MODE_MFSK22].name, 0, cb_init_mode, (void *)MODE_MFSK22 },
	{ mode_info[MODE_MFSK31].name, 0, cb_init_mode, (void *)MODE_MFSK31 },
	{ mode_info[MODE_MFSK32].name, 0, cb_init_mode, (void *)MODE_MFSK32 },
	{ mode_info[MODE_MFSK64].name, 0, cb_init_mode, (void *)MODE_MFSK64 },
	{ mode_info[MODE_MFSK128].name, 0, cb_init_mode, (void *)MODE_MFSK128 },
	{ mode_info[MODE_MFSK64L].name, 0, cb_init_mode, (void *)MODE_MFSK64L },
	{ mode_info[MODE_MFSK128L].name, 0, cb_init_mode, (void *)MODE_MFSK128L },
	{ 0 }
};

static const Fl_Menu_Item quick_change_wefax[] = {
	{ mode_info[MODE_WEFAX_576].name, 0, cb_init_mode, (void *)MODE_WEFAX_576 },
	{ mode_info[MODE_WEFAX_288].name, 0, cb_init_mode, (void *)MODE_WEFAX_288 },
	{ 0 }
};

static const Fl_Menu_Item quick_change_navtex[] = {
	{ mode_info[MODE_NAVTEX].name, 0, cb_init_mode, (void *)MODE_NAVTEX },
	{ mode_info[MODE_SITORB].name, 0, cb_init_mode, (void *)MODE_SITORB },
	{ 0 }
};

static const Fl_Menu_Item quick_change_mt63[] = {
	{ mode_info[MODE_MT63_500S].name, 0, cb_init_mode, (void *)MODE_MT63_500S },
	{ mode_info[MODE_MT63_500L].name, 0, cb_init_mode, (void *)MODE_MT63_500L },
	{ mode_info[MODE_MT63_1000S].name, 0, cb_init_mode, (void *)MODE_MT63_1000S },
	{ mode_info[MODE_MT63_1000L].name, 0, cb_init_mode, (void *)MODE_MT63_1000L },
	{ mode_info[MODE_MT63_2000S].name, 0, cb_init_mode, (void *)MODE_MT63_2000S },
	{ mode_info[MODE_MT63_2000L].name, 0, cb_init_mode, (void *)MODE_MT63_2000L },
	{ 0 }
};

static const Fl_Menu_Item quick_change_thor[] = {
	{ mode_info[MODE_THORMICRO].name, 0, cb_init_mode, (void *)MODE_THORMICRO },
	{ mode_info[MODE_THOR4].name, 0, cb_init_mode, (void *)MODE_THOR4 },
	{ mode_info[MODE_THOR5].name, 0, cb_init_mode, (void *)MODE_THOR5 },
	{ mode_info[MODE_THOR8].name, 0, cb_init_mode, (void *)MODE_THOR8 },
	{ mode_info[MODE_THOR11].name, 0, cb_init_mode, (void *)MODE_THOR11 },
	{ mode_info[MODE_THOR16].name, 0, cb_init_mode, (void *)MODE_THOR16 },
	{ mode_info[MODE_THOR22].name, 0, cb_init_mode, (void *)MODE_THOR22 },
	{ mode_info[MODE_THOR25x4].name, 0, cb_init_mode, (void *)MODE_THOR25x4 },
	{ mode_info[MODE_THOR50x1].name, 0, cb_init_mode, (void *)MODE_THOR50x1 },
	{ mode_info[MODE_THOR50x2].name, 0, cb_init_mode, (void *)MODE_THOR50x2 },
	{ mode_info[MODE_THOR100].name, 0, cb_init_mode, (void *)MODE_THOR100 },
	{ 0 }
};

static const Fl_Menu_Item quick_change_domino[] = {
	{ mode_info[MODE_DOMINOEXMICRO].name, 0, cb_init_mode, (void *)MODE_DOMINOEXMICRO },
	{ mode_info[MODE_DOMINOEX4].name, 0, cb_init_mode, (void *)MODE_DOMINOEX4 },
	{ mode_info[MODE_DOMINOEX5].name, 0, cb_init_mode, (void *)MODE_DOMINOEX5 },
	{ mode_info[MODE_DOMINOEX8].name, 0, cb_init_mode, (void *)MODE_DOMINOEX8 },
	{ mode_info[MODE_DOMINOEX11].name, 0, cb_init_mode, (void *)MODE_DOMINOEX11 },
	{ mode_info[MODE_DOMINOEX16].name, 0, cb_init_mode, (void *)MODE_DOMINOEX16 },
	{ mode_info[MODE_DOMINOEX22].name, 0, cb_init_mode, (void *)MODE_DOMINOEX22 },
	{ mode_info[MODE_DOMINOEX44].name, 0, cb_init_mode, (void *)MODE_DOMINOEX44 },
	{ mode_info[MODE_DOMINOEX88].name, 0, cb_init_mode, (void *)MODE_DOMINOEX88 },
	{ 0 }
};

static const Fl_Menu_Item quick_change_feld[] = {
	{ mode_info[MODE_FELDHELL].name, 0, cb_init_mode, (void *)MODE_FELDHELL },
	{ mode_info[MODE_SLOWHELL].name, 0, cb_init_mode, (void *)MODE_SLOWHELL },
	{ mode_info[MODE_HELLX5].name,   0, cb_init_mode, (void *)MODE_HELLX5 },
	{ mode_info[MODE_HELLX9].name,   0, cb_init_mode, (void *)MODE_HELLX9 },
	{ mode_info[MODE_FSKH245].name,  0, cb_init_mode, (void *)MODE_FSKH245 },
	{ mode_info[MODE_FSKH105].name,  0, cb_init_mode, (void *)MODE_FSKH105 },
	{ mode_info[MODE_HELL80].name,   0, cb_init_mode, (void *)MODE_HELL80 },
	{ 0 }
};

static const Fl_Menu_Item quick_change_throb[] = {
	{ mode_info[MODE_THROB1].name, 0, cb_init_mode, (void *)MODE_THROB1 },
	{ mode_info[MODE_THROB2].name, 0, cb_init_mode, (void *)MODE_THROB2 },
	{ mode_info[MODE_THROB4].name, 0, cb_init_mode, (void *)MODE_THROB4 },
	{ mode_info[MODE_THROBX1].name, 0, cb_init_mode, (void *)MODE_THROBX1 },
	{ mode_info[MODE_THROBX2].name, 0, cb_init_mode, (void *)MODE_THROBX2 },
	{ mode_info[MODE_THROBX4].name, 0, cb_init_mode, (void *)MODE_THROBX4 },
	{ 0 }
};

static const Fl_Menu_Item quick_change_olivia[] = {
	{ mode_info[MODE_OLIVIA_4_125].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_125 },
	{ mode_info[MODE_OLIVIA_4_250].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_250 },
	{ mode_info[MODE_OLIVIA_4_500].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_500 },
	{ mode_info[MODE_OLIVIA_4_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_1000 },
	{ mode_info[MODE_OLIVIA_4_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_2000 },

	{ mode_info[MODE_OLIVIA_8_125].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_125 },
	{ mode_info[MODE_OLIVIA_8_250].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_250 },
	{ mode_info[MODE_OLIVIA_8_500].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_500 },
	{ mode_info[MODE_OLIVIA_8_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_1000 },
	{ mode_info[MODE_OLIVIA_8_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_2000 },

	{ mode_info[MODE_OLIVIA_16_500].name, 0, cb_init_mode, (void *)MODE_OLIVIA_16_500 },
	{ mode_info[MODE_OLIVIA_16_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_16_1000 },
	{ mode_info[MODE_OLIVIA_16_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_16_2000 },

	{ mode_info[MODE_OLIVIA_32_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_32_1000 },
	{ mode_info[MODE_OLIVIA_32_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_32_2000 },

	{ mode_info[MODE_OLIVIA_64_500].name, 0, cb_init_mode, (void *)MODE_OLIVIA_64_500 },
	{ mode_info[MODE_OLIVIA_64_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_64_1000 },
	{ mode_info[MODE_OLIVIA_64_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_64_2000 },

	{ _("Custom..."), 0, cb_oliviaCustom, (void *)MODE_OLIVIA },
	{ 0 }
};

static const Fl_Menu_Item quick_change_contestia[] = {
	{ mode_info[MODE_CONTESTIA_4_125].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_125 },
	{ mode_info[MODE_CONTESTIA_4_250].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_250 },
	{ mode_info[MODE_CONTESTIA_4_500].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_500 },
	{ mode_info[MODE_CONTESTIA_4_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_1000 },
	{ mode_info[MODE_CONTESTIA_4_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_2000 },

	{ mode_info[MODE_CONTESTIA_8_125].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_125 },
	{ mode_info[MODE_CONTESTIA_8_250].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_250 },
	{ mode_info[MODE_CONTESTIA_8_500].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_500 },
	{ mode_info[MODE_CONTESTIA_8_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_1000 },
	{ mode_info[MODE_CONTESTIA_8_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_2000 },

	{ mode_info[MODE_CONTESTIA_16_250].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_16_250 },
	{ mode_info[MODE_CONTESTIA_16_500].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_16_500 },
	{ mode_info[MODE_CONTESTIA_16_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_16_1000 },
	{ mode_info[MODE_CONTESTIA_16_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_16_2000 },

	{ mode_info[MODE_CONTESTIA_32_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_32_1000 },
	{ mode_info[MODE_CONTESTIA_32_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_32_2000 },

	{ mode_info[MODE_CONTESTIA_64_500].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_64_500 },
	{ mode_info[MODE_CONTESTIA_64_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_64_1000 },
	{ mode_info[MODE_CONTESTIA_64_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_64_2000 },

	{ _("Custom..."), 0, cb_contestiaCustom, (void *)MODE_CONTESTIA },
	{ 0 }
};

static const Fl_Menu_Item quick_change_rtty[] = {
	{ "RTTY-45", 0, cb_rtty45, (void *)MODE_RTTY },
	{ "RTTY-50", 0, cb_rtty50, (void *)MODE_RTTY },
	{ "RTTY-75N", 0, cb_rtty75N, (void *)MODE_RTTY },
	{ "RTTY-75W", 0, cb_rtty75W, (void *)MODE_RTTY },
	{ "RTTY-100", 0, cb_rtty100, (void *)MODE_RTTY },
	{ _("Custom..."), 0, cb_rttyCustom, (void *)MODE_RTTY },
	{ 0 }
};

static const Fl_Menu_Item quick_change_fsq[] = {
	{ "FSQ1.5", 0, cb_fsq1p5, (void *)MODE_FSQ },
        { "FSQ2", 0, cb_fsq2, (void *)MODE_FSQ },
	{ "FSQ3", 0, cb_fsq3, (void *)MODE_FSQ },
	{ "FSQ4.5", 0, cb_fsq4p5, (void *)MODE_FSQ },
	{ "FSQ6", 0, cb_fsq6, (void *)MODE_FSQ },
	{ 0 }
};

static const Fl_Menu_Item quick_change_ifkp[] = {
	{ "IFKP 0.5", 0, cb_ifkp0p5a, (void *)MODE_IFKP },
	{ "IFKP 1.0", 0, cb_ifkp1p0a, (void *)MODE_IFKP },
	{ "IFKP 2.0", 0, cb_ifkp2p0a, (void *)MODE_IFKP },
	{ 0}
};

//Fl_Menu_Item quick_change_pkt[] = {
//    { " 300 baud", 0, cb_pkt300, (void *)MODE_PACKET },
//    { "1200 baud", 0, cb_pkt1200, (void *)MODE_PACKET },
//    { "2400 baud", 0, cb_pkt2400, (void *)MODE_PACKET },
//    { 0 }
//};

inline int minmax(int val, int min, int max)
{
	val = val < max ? val : max;
	return val > min ? val : min;
}

// Olivia
void set_olivia_default_integ()
{
	if (!progdefaults.olivia_reset_fec) return;

	int tones = progdefaults.oliviatones;
	int bw = progdefaults.oliviabw;

	if (tones < 1) tones = 1;
	int depth = minmax( (8 * (1 << bw)) / (1 << tones), 4, 4 * (1 << bw));

	progdefaults.oliviasinteg = depth;
	cntOlivia_sinteg->value(depth);
}

void set_olivia_tab_widgets()
{
	i_listbox_olivia_bandwidth->index(progdefaults.oliviabw);
	i_listbox_olivia_tones->index(progdefaults.oliviatones);
	set_olivia_default_integ();
}

void close_tree_items()
{
	std::string tabs[] = {
		_("Colors-Fonts"),
		_("Contests"),
		_("IDs"),
		_("Logging"),
		_("Modem/CW"),
		_("Modem/TTY"),
		_("Modem"),
		_("Misc"),
		_("Rig Control"),
		_("Soundcard"),
		_("UI"),
		_("Waterfall"),
		_("Web")
	};
	for (size_t n = 0; n < sizeof(tabs) / sizeof(*tabs); n++)
			tab_tree->close(tabs[n].c_str(),0);
}

void select_tab_tree(const char *tab)
{
	close_tree_items();

	std::string pname = tab;
	size_t p = pname.find("/");
	while (p != std::string::npos) {
		tab_tree->open(pname.substr(0,p).c_str());
		p = pname.find("/", p+1);
	}
	tab_tree->open(pname.c_str(),0);
	tab_tree->select(tab,1);
	SelectItem_CB(tab_tree);
}

void open_config(const char *tab)
{
	select_tab_tree(tab);
	dlgConfig->show();
}

void cb_oliviaCustom(Fl_Widget *w, void *arg)
{
	open_config(TAB_OLIVIA);
	cb_init_mode(w, arg);
}

// Contestia
void set_contestia_default_integ()
{
	if (!progdefaults.contestia_reset_fec) return;

	int tones = progdefaults.contestiatones;
	int bw = progdefaults.contestiabw;

	if (tones < 1) tones = 1;
	int depth = minmax( (8 * (1 << bw)) / (1 << tones), 4, 4 * (1 << bw));

	progdefaults.contestiasinteg = depth;
	cntContestia_sinteg->value(depth);
}

void set_contestia_tab_widgets()
{
	i_listbox_contestia_bandwidth->index(progdefaults.contestiabw);
	i_listbox_contestia_tones->index(progdefaults.contestiatones);
	set_contestia_default_integ();
}

void cb_contestiaCustom(Fl_Widget *w, void *arg)
{
	open_config(TAB_CONTESTIA);
	cb_init_mode(w, arg);
}

// rtty
void set_rtty_tab_widgets()
{
	selShift->index(progdefaults.rtty_shift);
	selCustomShift->deactivate();
	selBits->index(progdefaults.rtty_bits);
	selBaud->index(progdefaults.rtty_baud);
	selParity->index(progdefaults.rtty_parity);
	selStopBits->index(progdefaults.rtty_stop);
}

void enable_rtty_quickchange()
{
	if (active_modem->get_mode() == MODE_RTTY)
		quick_change = quick_change_rtty;
}

void disable_rtty_quickchange()
{
	if (active_modem->get_mode() == MODE_RTTY)
		quick_change = 0;
}

void cb_rtty45(Fl_Widget *w, void *arg)
{
	progdefaults.rtty_baud = 1;
	progdefaults.rtty_bits = 0;
	progdefaults.rtty_shift = 3;
	set_rtty_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_rtty50(Fl_Widget *w, void *arg)
{
	progdefaults.rtty_baud = 2;
	progdefaults.rtty_bits = 0;
	progdefaults.rtty_shift = 3;
	set_rtty_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_rtty75N(Fl_Widget *w, void *arg)
{
	progdefaults.rtty_baud = 4;
	progdefaults.rtty_bits = 0;
	progdefaults.rtty_shift = 3;
	set_rtty_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_rtty75W(Fl_Widget *w, void *arg)
{
	progdefaults.rtty_baud = 4;
	progdefaults.rtty_bits = 0;
	progdefaults.rtty_shift = 9;
	set_rtty_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_rtty100(Fl_Widget *w, void *arg)
{
	progdefaults.rtty_baud = 5;
	progdefaults.rtty_bits = 0;
	progdefaults.rtty_shift = 3;
	set_rtty_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_rttyCustom(Fl_Widget *w, void *arg)
{
	open_config(TAB_RTTY);

	cb_init_mode(w, arg);
}

void set_fsq_tab_widgets()
{
	btn_fsqbaud[0]->value(0);
	btn_fsqbaud[1]->value(0);
	btn_fsqbaud[2]->value(0);
	btn_fsqbaud[3]->value(0);
	btn_fsqbaud[4]->value(0);
	if (progdefaults.fsqbaud == 1.5) btn_fsqbaud[0]->value(1);
	else if (progdefaults.fsqbaud == 2.0) btn_fsqbaud[1]->value(1);
	else if (progdefaults.fsqbaud == 3.0) btn_fsqbaud[2]->value(1);
	else if (progdefaults.fsqbaud == 4.5) btn_fsqbaud[3]->value(1);
	else btn_fsqbaud[4]->value(1);
}

void cb_fsq1p5(Fl_Widget *w, void *arg)
{
	progdefaults.fsqbaud = 1.5;
	set_fsq_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_fsq2(Fl_Widget *w, void *arg)
{
	progdefaults.fsqbaud = 2.0;
	set_fsq_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_fsq3(Fl_Widget *w, void *arg)
{
	progdefaults.fsqbaud = 3.0;
	set_fsq_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_fsq4p5(Fl_Widget *w, void *arg)
{
	progdefaults.fsqbaud = 4.5;
	set_fsq_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_fsq6(Fl_Widget *w, void *arg)
{
	progdefaults.fsqbaud = 6.0;
	set_fsq_tab_widgets();
	cb_init_mode(w, arg);
}

void set_ifkp_tab_widgets()
{
	btn_ifkpbaud[0]->value(0);
	btn_ifkpbaud[1]->value(0);
	btn_ifkpbaud[2]->value(0);
	if (progdefaults.ifkp_baud == 0) {
		btn_ifkpbaud[0]->value(1);
		put_MODEstatus("IFKP 0.5");
	} else if (progdefaults.ifkp_baud == 1) {
		btn_ifkpbaud[1]->value(1);
		put_MODEstatus("IFKP 1.0");
	}
	else {
		btn_ifkpbaud[2]->value(1);
		put_MODEstatus("IFKP 2.0");
	}
}

void cb_ifkp0p5 (Fl_Widget *w, void *arg)
{
	progdefaults.ifkp_baud = 0;
	set_ifkp_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_ifkp0p5a (Fl_Widget *w, void *arg)
{
	progdefaults.ifkp_baud = 0;
	set_ifkp_tab_widgets();
}

void cb_ifkp1p0 (Fl_Widget *w, void *arg)
{
	progdefaults.ifkp_baud = 1;
	set_ifkp_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_ifkp1p0a (Fl_Widget *w, void *arg)
{
	progdefaults.ifkp_baud = 1;
	set_ifkp_tab_widgets();
}

void cb_ifkp2p0 (Fl_Widget *w, void *arg)
{
	progdefaults.ifkp_baud = 2;
	set_ifkp_tab_widgets();
	cb_init_mode(w, arg);
}

void cb_ifkp2p0a (Fl_Widget *w, void *arg)
{
	progdefaults.ifkp_baud = 2;
	set_ifkp_tab_widgets();
}

void set_dominoex_tab_widgets()
{
	chkDominoEX_FEC->value(progdefaults.DOMINOEX_FEC);
}

//void cb_pkt1200(Fl_Widget *w, void *arg)
//{
//    progdefaults.PKT_BAUD_SELECT = 0;
//    selPacket_Baud->value(progdefaults.PKT_BAUD_SELECT);
//    cb_init_mode(w, arg);
//}

//void cb_pkt300(Fl_Widget *w, void *arg)
//{
//    progdefaults.PKT_BAUD_SELECT = 1;
//    selPacket_Baud->value(progdefaults.PKT_BAUD_SELECT);
//    cb_init_mode(w, arg);
//}

//void cb_pkt2400(Fl_Widget *w, void *arg)
//{
//    progdefaults.PKT_BAUD_SELECT = 2;
//    selPacket_Baud->value(progdefaults.PKT_BAUD_SELECT);
//    cb_init_mode(w, arg);
//}

void set_mode_controls(trx_mode id)
{
	if (id == MODE_CW) {
		cntCW_WPM->show();
		btnCW_Default->show();
		Status1->hide();
		if (mvsquelch) {
			mvsquelch->value(progStatus.VIEWER_cwsquelch);
			mvsquelch->range(0, 40.0);
			mvsquelch->redraw();
		}
		if (sldrViewerSquelch) {
			sldrViewerSquelch->value(progStatus.VIEWER_cwsquelch);
			sldrViewerSquelch->range(0, 40.0);
			sldrViewerSquelch->redraw();
		}
	} else {
		cntCW_WPM->hide();
		btnCW_Default->hide();
		Status1->show();
	}

	if (id == MODE_RTTY) {
		if (mvsquelch) {
			mvsquelch->value(progStatus.VIEWER_rttysquelch);
			mvsquelch->range(-6.0, 34.0);
		}
		if (sldrViewerSquelch) {
			sldrViewerSquelch->value(progStatus.VIEWER_rttysquelch);
			sldrViewerSquelch->range(-12.0, 6.0);
		}
	}

	if (id >= MODE_PSK_FIRST && id <= MODE_PSK_LAST) {
		if (mvsquelch) {
			mvsquelch->value(progStatus.VIEWER_psksquelch);
			mvsquelch->range(-3.0, 6.0);
		}
		if (sldrViewerSquelch) {
			sldrViewerSquelch->value(progStatus.VIEWER_psksquelch);
			sldrViewerSquelch->range(-3.0, 6.0);
		}
	}

	if (!bWF_only) {
		if (id >= MODE_WEFAX_FIRST && id <= MODE_WEFAX_LAST) {
			text_group->hide();
			fsq_group->hide();
			ifkp_group->hide();
			fmt_group->hide();
			wefax_group->show();
			center_group->redraw();
		} else if (id == MODE_FSQ) {
			text_group->hide();
			wefax_group->hide();
			ifkp_group->hide();
			fmt_group->hide();
			fsq_group->show();
			center_group->redraw();
		} else if (id == MODE_IFKP) {
			text_group->hide();
			wefax_group->hide();
			fsq_group->hide();
			fmt_group->hide();
			ifkp_group->show();
			center_group->redraw();
		} else if (id == MODE_FMT) {
			text_group->hide();
			wefax_group->hide();
			fsq_group->hide();
			ifkp_group->hide();
			fmt_group->show();
			center_group->redraw();
		} else {
			text_group->show();
			wefax_group->hide();
			fsq_group->hide();
			ifkp_group->hide();
			fmt_group->hide();
			if (id >= MODE_HELL_FIRST && id <= MODE_HELL_LAST) {
				ReceiveText->hide();
				FHdisp->show();
			} else {
				FHdisp->hide();
				ReceiveText->show();
			}
			center_group->redraw();
		}
		ifkp_avatar->hide();
		thor_avatar->hide();
		string call = inpCall->value();
		if (id == MODE_IFKP) {
			NFtabs->resize(
				NFtabs->x(), NFtabs->y(),
				fl_digi_main->w() - NFtabs->x() - 59 - pad, NFtabs->h());
			ifkp_avatar->resize(fl_digi_main->w() - 59 - pad, NFtabs->y(), 59, 74);
			ifkp_avatar->show();
			if (!call.empty())
				ifkp_load_avatar(inpCall->value());
			else
				ifkp_load_avatar();
		} else if ( ((id >= MODE_THOR11) && (id <= MODE_THOR22))) {
			NFtabs->resize(
				NFtabs->x(), NFtabs->y(),
				fl_digi_main->w() - NFtabs->x() - 59 - pad, NFtabs->h());
			thor_avatar->resize(fl_digi_main->w() - 59 - pad, NFtabs->y(), 59, 74);
			thor_avatar->show();
			if (!call.empty())
				thor_load_avatar(inpCall->value());
			else
				thor_load_avatar();
		}
		else {
			NFtabs->resize(
				NFtabs->x(), NFtabs->y(),
				fl_digi_main->w() - NFtabs->x() - 2*pad, NFtabs->h());
		}
		ifkp_avatar->redraw();
		thor_avatar->redraw();
		NFtabs->init_sizes();
		NFtabs->redraw();

	}

}

void startup_modem(modem* m, int f)
{
	trx_start_modem(m, f);
#if BENCHMARK_MODE
	return;
#endif

	restoreFocus(1);

	trx_mode id = m->get_mode();

	set_mode_controls(id);

	if (id >= MODE_PSK_FIRST && id <= MODE_PSK_LAST) {
		m->set_sigsearch(SIGSEARCH);
	}

	if (m->get_cap() & modem::CAP_AFC) {
		btnAFC->value(progStatus.afconoff);
		btnAFC->activate();
	}
	else {
		btnAFC->value(0);
		btnAFC->deactivate();
	}

	if (m->get_cap() & modem::CAP_REV) {
		wf->btnRev->value(wf->Reverse());
		wf->btnRev->activate();
	}
	else {
		wf->btnRev->value(0);
		wf->btnRev->deactivate();
	}
}

void cb_mnuOpenMacro(Fl_Menu_*, void*) {
	if (macros.changed) {
		switch (fl_choice2(_("Save changed macros?"), _("Cancel"), _("Save"), _("Don't save"))) {
		case 0:
			return;
		case 1:
			macros.saveMacroFile();
			// fall through
		case 2:
			break;
		}
	}
	macros.openMacroFile();
	macros.changed = false;
	restoreFocus(2);
}

void cb_mnuSaveMacro(Fl_Menu_*, void*) {
	macros.saveMacroFile();
	restoreFocus(3);
}

void remove_windows()
{
	std::string titles[] = {
		"scope view", "record loader", "cluster viewer", "dxcc window",
		"viewer", "logbook", "lotw review",
		"export", "cabrillo",
		"config", "notify",
		"mfsk rxwin", "mfsk txwin",
		"thor rxwin", "thor txwin",
		"fsq monitor", "fsq rxwin", "fsq txwin",
		"ifkp rxwin", "ifkp txwin",
		"macro editor", "test signals", "rx audio",
		"wefax tx dialog"
	};
	Fl_Double_Window *w[] = {
		scopeview, dlgRecordLoader,
		dxcluster_viewer, dxcc_window,
		dlgViewer, dlgLogbook, lotw_review_dialog,  
		wExport, wCabrillo,
		dlgConfig, notify_window,
		picRxWin, picTxWin,
		thorpicRxWin, thorpicTxWin,
		fsqMonitor, fsqpicRxWin, fsqpicTxWin,
		ifkppicRxWin, ifkppicTxWin,
		MacroEditDialog,
		test_signal_window,
		rxaudio_dialog, wefax_pic_tx_win };
		std::string sdeleting = "\nDeleting dialogs / Stopping debug session";
	for (size_t n = 0; n < sizeof(w) / sizeof(*w); n++) {
		if (w[n]) {
		sdeleting.append("\n   ").append(titles[n]);
			w[n]->hide();
			delete w[n];
			w[n] = 0;
		}
	}
	if (font_browser) {
		sdeleting.append("\n   font browser");
		font_browser->hide();
		delete font_browser;
		font_browser = 0;
	}
	LOG_INFO("%s", sdeleting.c_str());
	MilliSleep(50);
	debug::stop();
}

// callback executed from Escape / Window decoration 'X' or OS X cmd-Q

void cb_wMain(Fl_Widget*, void*)
{
	if (!clean_exit(true)) return;
	remove_windows();
	LOG_INFO("Hiding main window");
	fl_digi_main->hide();
}

// callback executed from menu item File/Exit
void cb_E(Fl_Menu_*, void*) {
	if (!clean_exit(true))
		return;
	remove_windows();
	LOG_INFO("Hiding main window");
// this will make Fl::run return
	fl_digi_main->hide();
}

static int squelch_val;
void rsid_squelch_timer(void*)
{
	progStatus.sqlonoff = squelch_val;
	if (progStatus.sqlonoff)
		btnSQL->value(1);
}

void init_modem_squelch(trx_mode mode, int freq)
{
	squelch_val = progStatus.sqlonoff;
	progStatus.sqlonoff = 0;
	btnSQL->value(0);
	Fl::add_timeout(progdefaults.rsid_squelch, rsid_squelch_timer);
	init_modem(mode, freq);
}

extern bool valid_kiss_modem(std::string modem_name);

void init_modem(trx_mode mode, int freq)
{
	ENSURE_THREAD(FLMAIN_TID);

	if (bWF_only)
		if (mode == MODE_FSQ ||
			mode == MODE_IFKP ||
			mode == MODE_FELDHELL ||
			mode == MODE_SLOWHELL ||
			mode == MODE_HELLX5 ||
			mode == MODE_HELLX9 ||
			mode == MODE_FSKH245 ||
			mode == MODE_FSKH105 ||
			mode == MODE_HELL80 ||
			mode == MODE_WEFAX_576 ||
			mode == MODE_WEFAX_288 ||
			mode == MODE_NAVTEX ||
			mode == MODE_SITORB )
		mode = MODE_PSK31;

	stopMacroTimer();

	if (data_io_enabled == KISS_IO) {
		trx_mode current_mode = active_modem->get_mode();
		if(!bcast_rsid_kiss_frame(freq, mode, (int) active_modem->get_txfreq(), current_mode,
								  progdefaults.rsid_notify_only ? RSID_KISS_NOTIFY : RSID_KISS_ACTIVE)) {

			LOG_INFO("Invaild Modem for KISS I/O (%s)",  mode_info[mode].sname);

			int _yes = false;
			if(!progdefaults.kiss_io_modem_change_inhibit)
				_yes = fl_choice2(_("Switch to ARQ I/O"), _("No"), _("Yes"), NULL);

			if(_yes) {
				enable_arq();
			} else {
				std::string modem_name;
				modem_name.assign(mode_info[current_mode].sname);
				bool valid = valid_kiss_modem(modem_name);
				if(!valid)
					current_mode = MODE_PSK250;
				mode = current_mode;
			}
		}
	}

	//LOG_INFO("mode: %d, freq: %d", (int)mode, freq);

#if !BENCHMARK_MODE
	   quick_change = 0;
//	   modem_config_tab = tabsModems->child(0);
#endif

	switch (mode) {
	case MODE_NEXT:
		if ((mode = active_modem->get_mode() + 1) == NUM_MODES)
			mode = 0;
		return init_modem(mode, freq);
	case MODE_PREV:
		if ((mode = active_modem->get_mode() - 1) < 0)
			mode = NUM_MODES - 1;
		return init_modem(mode, freq);

	case MODE_NULL:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new NULLMODEM, freq);
		break;

	case MODE_CW:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new cw, freq);
//		modem_config_tab = tabCW;
		break;

	case MODE_THORMICRO: case MODE_THOR4: case MODE_THOR5: case MODE_THOR8:
	case MODE_THOR11:case MODE_THOR16: case MODE_THOR22:
	case MODE_THOR25x4: case MODE_THOR50x1: case MODE_THOR50x2: case MODE_THOR100:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new thor(mode), freq);
		quick_change = quick_change_thor;
//		modem_config_tab = tabTHOR;
		break;

	case MODE_DOMINOEXMICRO: case MODE_DOMINOEX4: case MODE_DOMINOEX5: case MODE_DOMINOEX8:
	case MODE_DOMINOEX11: case MODE_DOMINOEX16: case MODE_DOMINOEX22:
	case MODE_DOMINOEX44: case MODE_DOMINOEX88:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new dominoex(mode), freq);
		quick_change = quick_change_domino;
//		modem_config_tab = tabDomEX;
		break;

	case MODE_FELDHELL:
	case MODE_SLOWHELL:
	case MODE_HELLX5:
	case MODE_HELLX9:
	case MODE_FSKH245:
	case MODE_FSKH105:
	case MODE_HELL80:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new feld(mode), freq);
		quick_change = quick_change_feld;
//		modem_config_tab = tabFeld;
		break;

	case MODE_MFSK4:
	case MODE_MFSK11:
	case MODE_MFSK22:
	case MODE_MFSK31:
	case MODE_MFSK64:
	case MODE_MFSK8:
	case MODE_MFSK16:
	case MODE_MFSK32:
	case MODE_MFSK128:
	case MODE_MFSK64L:
	case MODE_MFSK128L:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new mfsk(mode), freq);
		quick_change = quick_change_mfsk;
		break;

	case MODE_WEFAX_576:
	case MODE_WEFAX_288:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new wefax(mode), freq);
		quick_change = quick_change_wefax;
//		modem_config_tab = tabWefax;
		break;

	case MODE_NAVTEX:
	case MODE_SITORB:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new navtex(mode), freq);
		quick_change = quick_change_navtex;
//		modem_config_tab = tabNavtex;
		break;

	case MODE_MT63_500S: case MODE_MT63_1000S: case MODE_MT63_2000S :
	case MODE_MT63_500L: case MODE_MT63_1000L: case MODE_MT63_2000L :
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new mt63(mode), freq);
		quick_change = quick_change_mt63;
//		modem_config_tab = tabMT63;
		break;

	case MODE_PSK31: case MODE_PSK63: case MODE_PSK63F:
	case MODE_PSK125: case MODE_PSK250: case MODE_PSK500:
	case MODE_PSK1000:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new psk(mode), freq);
		quick_change = quick_change_psk;
//		modem_config_tab = tabPSK;
		break;

	case MODE_QPSK31: case MODE_QPSK63: case MODE_QPSK125: case MODE_QPSK250: case MODE_QPSK500:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new psk(mode), freq);
		quick_change = quick_change_qpsk;
//		modem_config_tab = tabPSK;
		break;
	case MODE_8PSK125:
	case MODE_8PSK250:
	case MODE_8PSK500:
	case MODE_8PSK1000:
	case MODE_8PSK125FL:
	case MODE_8PSK125F:
	case MODE_8PSK250FL:
	case MODE_8PSK250F:
	case MODE_8PSK500F:
	case MODE_8PSK1000F:
	case MODE_8PSK1200F:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new psk(mode), freq);
		quick_change = quick_change_8psk;
//		modem_config_tab = tabPSK;
		break;
		
		
	case MODE_OFDM_500F:
	case MODE_OFDM_750F:
	case MODE_OFDM_2000F:
	case MODE_OFDM_2000:
	case MODE_OFDM_3500:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
		*mode_info[mode].modem = new psk(mode), freq);
		quick_change = quick_change_ofdm;
		//		modem_config_tab = tabPSK;
		break;
		
		
	case MODE_PSK125R: case MODE_PSK250R: case MODE_PSK500R:
	case MODE_PSK1000R:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new psk(mode), freq);
		quick_change = quick_change_pskr;
//		modem_config_tab = tabPSK;
		break;

	case MODE_12X_PSK125 :
	case MODE_6X_PSK250 :
	case MODE_2X_PSK500 :
	case MODE_4X_PSK500 :
	case MODE_2X_PSK800 :
	case MODE_2X_PSK1000 :
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new psk(mode), freq);
		quick_change = quick_change_psk_multi;
//		modem_config_tab = tabPSK;
		break;

	case MODE_4X_PSK63R :
	case MODE_5X_PSK63R :
	case MODE_10X_PSK63R :
	case MODE_20X_PSK63R :
	case MODE_32X_PSK63R :

	case MODE_4X_PSK125R :
	case MODE_5X_PSK125R :
	case MODE_10X_PSK125R :
	case MODE_12X_PSK125R :
	case MODE_16X_PSK125R :

	case MODE_2X_PSK250R :
	case MODE_3X_PSK250R :
	case MODE_5X_PSK250R :
	case MODE_6X_PSK250R :
	case MODE_7X_PSK250R :

	case MODE_2X_PSK500R :
	case MODE_3X_PSK500R :
	case MODE_4X_PSK500R :

	case MODE_2X_PSK800R :
	case MODE_2X_PSK1000R :
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new psk(mode), freq);
		quick_change = quick_change_psk_multiR;
//		modem_config_tab = tabPSK;
		break;

	case MODE_OLIVIA:
	case MODE_OLIVIA_4_125:
	case MODE_OLIVIA_4_250:
	case MODE_OLIVIA_4_500:
	case MODE_OLIVIA_4_1000:
	case MODE_OLIVIA_4_2000:
	case MODE_OLIVIA_8_125:
	case MODE_OLIVIA_8_250:
	case MODE_OLIVIA_8_500:
	case MODE_OLIVIA_8_1000:
	case MODE_OLIVIA_8_2000:
	case MODE_OLIVIA_16_500:
	case MODE_OLIVIA_16_1000:
	case MODE_OLIVIA_16_2000:
	case MODE_OLIVIA_32_1000:
	case MODE_OLIVIA_32_2000:
	case MODE_OLIVIA_64_500:
	case MODE_OLIVIA_64_1000:
	case MODE_OLIVIA_64_2000:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new olivia(mode), freq);
//		modem_config_tab = tabOlivia;
		quick_change = quick_change_olivia;
		break;

	case MODE_CONTESTIA:
	case MODE_CONTESTIA_4_125:  case MODE_CONTESTIA_4_250:
	case MODE_CONTESTIA_4_500:  case MODE_CONTESTIA_4_1000:   case MODE_CONTESTIA_4_2000:
	case MODE_CONTESTIA_8_125:  case MODE_CONTESTIA_8_250:
	case MODE_CONTESTIA_8_500:  case MODE_CONTESTIA_8_1000:   case MODE_CONTESTIA_8_2000:
	case MODE_CONTESTIA_16_250:  case MODE_CONTESTIA_16_500:
	case MODE_CONTESTIA_16_1000: case MODE_CONTESTIA_16_2000:
	case MODE_CONTESTIA_32_1000: case MODE_CONTESTIA_32_2000:
	case MODE_CONTESTIA_64_500:  case MODE_CONTESTIA_64_1000: case MODE_CONTESTIA_64_2000:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new contestia(mode), freq);
//		modem_config_tab = tabContestia;
		quick_change = quick_change_contestia;
		break;

	case MODE_FSQ:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new fsq(mode), freq);
//		modem_config_tab = tabFSQ;
		quick_change = quick_change_fsq;
		break;

	case MODE_IFKP:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new ifkp(mode), freq);
//		modem_config_tab = tabIFKP;
		quick_change = quick_change_ifkp;
		break;

	case MODE_RTTY:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new rtty(mode), freq);
//		modem_config_tab = tabRTTY;

		if (progStatus.nanoFSK_online || progStatus.Nav_online)
			quick_change = 0;
		else
			quick_change = quick_change_rtty;
		break;

	case MODE_THROB1: case MODE_THROB2: case MODE_THROB4:
	case MODE_THROBX1: case MODE_THROBX2: case MODE_THROBX4:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new throb(mode), freq);
		quick_change = quick_change_throb;
		break;

//	case MODE_PACKET:
//		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
//			      *mode_info[mode].modem = new pkt(mode), freq);
//		modem_config_tab = tabNavtex;
//		quick_change = quick_change_pkt;
//		break;

	case MODE_WWV:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new wwv, freq);
		break;

	case MODE_ANALYSIS:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new anal, freq);
		break;

	case MODE_FMT:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new fmt, freq);
		break;

	case MODE_SSB:
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new ssb, freq);
		break;

	default:
		LOG_ERROR("Unknown mode: %d", (int)mode);
		mode = MODE_PSK31;
		startup_modem(*mode_info[mode].modem ? *mode_info[mode].modem :
				  *mode_info[mode].modem = new psk(mode), freq);
		quick_change = quick_change_psk;
//		modem_config_tab = tabPSK;
		break;
	}

#if BENCHMARK_MODE
	return;
#endif

	clear_StatusMessages();
	progStatus.lastmode = mode;

	if (wf->xmtlock->value() == 1 && !mailserver) {
		if(!progdefaults.retain_freq_lock) {
			wf->xmtlock->value(0);
			wf->xmtlock->damage();
			active_modem->set_freqlock(false);
		}
	}

//	if (FD_logged_on) FD_mode_check();
}

void init_modem_sync(trx_mode m, int f)
{
	ENSURE_THREAD(FLMAIN_TID);

	int count = 2000;
	if (trx_state != STATE_RX) {
		LOG_INFO("Waiting for %s", mode_info[active_modem->get_mode()].name);
		abort_tx();
		while (trx_state != STATE_RX && count) {
			LOG_DEBUG("%0.2f secs remaining", count / 100.0);
			Fl::awake();
			MilliSleep(10);
			count--;
		}
		if (count == 0) {
			LOG_ERROR("%s", "TIMED OUT!!");
			return;  // abort modem selection
		}
	}

	init_modem(m, f);

	count = 500;
	if (trx_state != STATE_RX) {
		while (trx_state != STATE_RX && count) {
			Fl::awake();
			MilliSleep(10);
			count--;
		}
		if (count == 0)
			LOG_ERROR("%s", "Wait for STATE_RX timed out");
	}

	REQ_FLUSH(TRX_TID);
}

void cb_init_mode(Fl_Widget *, void *mode)
{
	init_modem(reinterpret_cast<trx_mode>(mode));
}

// character set selection menu

void set_charset_listbox(int rxtx_charset)
{
	int tiniconv_id = charset_list[rxtx_charset].tiniconv_id;

	// order all converters to switch to the new encoding
	rx_chd.set_input_encoding(tiniconv_id);
	echo_chd.set_input_encoding(tiniconv_id);
	tx_encoder.set_output_encoding(tiniconv_id);

	if (mainViewer)
		mainViewer->set_input_encoding(tiniconv_id);
	if (brwsViewer)
		brwsViewer->set_input_encoding(tiniconv_id);

	// update the button
	progdefaults.charset_name = charset_list[rxtx_charset].name;
	listbox_charset_status->value(progdefaults.charset_name.c_str());
	restoreFocus(4);
}

void cb_listbox_charset(Fl_Widget *w, void *)
{
	Fl_ListBox * lbox = (Fl_ListBox *)w;
	set_charset_listbox(lbox->index());
}

void populate_charset_listbox(void)
{
	for (unsigned int i = 0; i < number_of_charsets; i++)
		listbox_charset_status->add( charset_list[i].name );
	listbox_charset_status->value(progdefaults.charset_name.c_str());
}

// find the position of the default charset in charset_list[] and trigger the callback
void set_default_charset(void)
{
	for (unsigned int i = 0; i < number_of_charsets; i++) {
		if (strcmp(charset_list[i].name, progdefaults.charset_name.c_str()) == 0) {
			set_charset_listbox(i);
			return;
		}
	}
}

// if w is not NULL, give focus to TransmitText only if the last event was an Enter keypress
void restoreFocus(int n)
{
	if (Fl::focus() == NULL) return;
	if (!active_modem) {
		TransmitText->take_focus();
		return;
	}
	if (active_modem->get_mode() == MODE_FSQ && fsq_tx_text)
		fsq_tx_text->take_focus();
	else if (active_modem->get_mode() == MODE_IFKP && ifkp_tx_text)
		ifkp_tx_text->take_focus();
	else if (TransmitText)
		TransmitText->take_focus();
}

void macro_cb(Fl_Widget *w, void *v)
{
//	if (active_modem->get_mode() == MODE_FSQ)
//		return;

	int b = (int)(reinterpret_cast<long long> (v));

	if (b & 0x80) { // 4 bar docked macros
		b &= 0x7F;
	} else {
		if (progdefaults.mbar_scheme > MACRO_SINGLE_BAR_MAX) {
			if (b >= NUMMACKEYS) b += (altMacros - 1) * NUMMACKEYS;
		} else {
			b += altMacros * NUMMACKEYS;
		}
	}

	int mouse = Fl::event_button();
	if (mouse == FL_LEFT_MOUSE && !macros.text[b].empty()) {
		if (progStatus.timer) return;
		stopMacroTimer();
		progStatus.skip_sked_macro = false;
		macros.execute(b);
	}
	else if (mouse == FL_RIGHT_MOUSE)
		editMacro(b);
	if (Fl::focus() != qsoFreqDisp)
		restoreFocus(5);
}

void colorize_48macros(int i)
{
	if (progdefaults.useGroupColors == true) {
		int k = i / 4;
		if (k == 0 || k == 3 || k == 6 || k == 9)
			btnDockMacro[i]->color(fl_rgb_color(
				progdefaults.btnGroup1.R,
				progdefaults.btnGroup1.G,
				progdefaults.btnGroup1.B));
		else if (k == 1 || k == 4 || k == 7 || k == 10)
			btnDockMacro[i]->color(fl_rgb_color(
				progdefaults.btnGroup2.R,
				progdefaults.btnGroup2.G,
				progdefaults.btnGroup2.B));
		else
			btnDockMacro[i]->color(fl_rgb_color(
				progdefaults.btnGroup3.R,
				progdefaults.btnGroup3.G,
				progdefaults.btnGroup3.B));
		btnDockMacro[i]->labelcolor(
			fl_rgb_color(
				progdefaults.btnFkeyTextColor.R,
				progdefaults.btnFkeyTextColor.G,
				progdefaults.btnFkeyTextColor.B ));
		btnDockMacro[i]->labelcolor(progdefaults.MacroBtnFontcolor);
		btnDockMacro[i]->labelfont(progdefaults.MacroBtnFontnbr);
		btnDockMacro[i]->labelsize(progdefaults.MacroBtnFontsize);
	} else {
		btnDockMacro[i]->color(FL_BACKGROUND_COLOR);
		btnDockMacro[i]->labelcolor(FL_FOREGROUND_COLOR);
		btnDockMacro[i]->labelfont(progdefaults.MacroBtnFontnbr);
		btnDockMacro[i]->labelsize(progdefaults.MacroBtnFontsize);
	}
}

void colorize_macro(int i)
{
	int j = i % NUMMACKEYS;
	if (progdefaults.useGroupColors == true) {
		if (j < 4) {
			btnMacro[i]->color(fl_rgb_color(
				progdefaults.btnGroup1.R,
				progdefaults.btnGroup1.G,
				progdefaults.btnGroup1.B));
		} else if (j < 8) {
			btnMacro[i]->color(fl_rgb_color(
				progdefaults.btnGroup2.R,
				progdefaults.btnGroup2.G,
				progdefaults.btnGroup2.B));
		} else {
			btnMacro[i]->color(fl_rgb_color(
				progdefaults.btnGroup3.R,
				progdefaults.btnGroup3.G,
				progdefaults.btnGroup3.B));
		}
		btnMacro[i]->labelcolor(
			fl_rgb_color(
				progdefaults.btnFkeyTextColor.R,
				progdefaults.btnFkeyTextColor.G,
				progdefaults.btnFkeyTextColor.B ));
		btnMacro[i]->labelcolor(progdefaults.MacroBtnFontcolor);
		btnMacro[i]->labelfont(progdefaults.MacroBtnFontnbr);
		btnMacro[i]->labelsize(progdefaults.MacroBtnFontsize);
	} else {
		btnMacro[i]->color(FL_BACKGROUND_COLOR);
		btnMacro[i]->labelcolor(FL_FOREGROUND_COLOR);
		btnMacro[i]->labelfont(progdefaults.MacroBtnFontnbr);
		btnMacro[i]->labelsize(progdefaults.MacroBtnFontsize);
	}
	btnMacro[i]->redraw_label();
}

void colorize_macros()
{
	for (int i = 0; i < NUMMACKEYS * NUMKEYROWS; i++) colorize_macro(i);
	for (int i = 0; i < 48; i++) colorize_48macros(i);
	btnAltMacros1->labelsize(progdefaults.MacroBtnFontsize);
	btnAltMacros1->redraw_label();
	btnAltMacros2->labelsize(progdefaults.MacroBtnFontsize);
	btnAltMacros2->redraw_label();
}

void altmacro_cb(Fl_Widget *w, void *v)
{
	static char alt_text[2] = "1";

	intptr_t arg = reinterpret_cast<intptr_t>(v);
	if (arg)
		altMacros += arg;
	else
		altMacros = altMacros + (Fl::event_button() == FL_RIGHT_MOUSE ? -1 : 1);

	if (progdefaults.mbar_scheme > MACRO_SINGLE_BAR_MAX) { // alternate set
		altMacros = WCLAMP(altMacros, 1, 3);
		alt_text[0] = '1' + altMacros;
		for (int i = 0; i < NUMMACKEYS; i++) {
			btnMacro[i + NUMMACKEYS]->label(macros.name[i + (altMacros * NUMMACKEYS)].c_str());
			btnMacro[i + NUMMACKEYS]->redraw_label();
		}
		btnAltMacros2->label(alt_text);
		btnAltMacros2->redraw_label();
	} else { // primary set
		altMacros = WCLAMP(altMacros, 0, 3);
		alt_text[0] = '1' + altMacros;
		for (int i = 0; i < NUMMACKEYS; i++) {
			btnMacro[i]->label(macros.name[i + (altMacros * NUMMACKEYS)].c_str());
			btnMacro[i]->redraw_label();
		}
		btnAltMacros1->label(alt_text);
		btnAltMacros1->redraw_label();
	}
	restoreFocus(6);
}

void cb_mnuConfigNotify(Fl_Menu_*, void*)
{
	notify_show();
}

void cb_mnuTestSignals(Fl_Menu_*, void*)
{
	show_testdialog();
}

void cb_mnuConfigModems(Fl_Menu_*, void*) {
	switch (active_modem->get_mode()) {
		case MODE_CW:
			open_config(TAB_CW);
			break;
		case MODE_THORMICRO: case MODE_THOR4: case MODE_THOR5: case MODE_THOR8:
		case MODE_THOR11:case MODE_THOR16: case MODE_THOR22:
		case MODE_THOR25x4: case MODE_THOR50x1: case MODE_THOR50x2: case MODE_THOR100:
			open_config(TAB_THOR);
			break;
		case MODE_DOMINOEXMICRO: case MODE_DOMINOEX4: case MODE_DOMINOEX5: case MODE_DOMINOEX8:
		case MODE_DOMINOEX11: case MODE_DOMINOEX16: case MODE_DOMINOEX22:
		case MODE_DOMINOEX44: case MODE_DOMINOEX88:
			open_config(TAB_DOMINOEX);
			break;
		case MODE_FELDHELL: case MODE_SLOWHELL: case MODE_HELLX5: case MODE_HELLX9:
		case MODE_FSKH245: case MODE_FSKH105:case MODE_HELL80:
			open_config(TAB_FELDHELL);
			break;
		case MODE_WEFAX_576: case MODE_WEFAX_288:
			open_config(TAB_WEFAX);
			break;
		case MODE_NAVTEX: case MODE_SITORB:
			open_config(TAB_NAVTEX);
			break;
		case MODE_MT63_500S: case MODE_MT63_1000S: case MODE_MT63_2000S :
		case MODE_MT63_500L: case MODE_MT63_1000L: case MODE_MT63_2000L :
			quick_change = quick_change_mt63;
			open_config(TAB_MT63);
			break;
		case MODE_OLIVIA:
		case MODE_OLIVIA_4_125:   case MODE_OLIVIA_4_250:   case MODE_OLIVIA_4_500: 
		case MODE_OLIVIA_4_1000:  case MODE_OLIVIA_4_2000:
		case MODE_OLIVIA_8_125:   case MODE_OLIVIA_8_250:   case MODE_OLIVIA_8_500: 
		case MODE_OLIVIA_8_1000:  case MODE_OLIVIA_8_2000:
		case MODE_OLIVIA_16_500:  case MODE_OLIVIA_16_1000: case MODE_OLIVIA_16_2000:
		case MODE_OLIVIA_32_1000: case MODE_OLIVIA_32_2000:
		case MODE_OLIVIA_64_500:  case MODE_OLIVIA_64_1000: case MODE_OLIVIA_64_2000:
			open_config(TAB_OLIVIA);
			break;
		case MODE_CONTESTIA:
		case MODE_CONTESTIA_4_125:   case MODE_CONTESTIA_4_250:   case MODE_CONTESTIA_4_500: 
		case MODE_CONTESTIA_4_1000:  case MODE_CONTESTIA_4_2000:
		case MODE_CONTESTIA_8_125:   case MODE_CONTESTIA_8_250:   case MODE_CONTESTIA_8_500: 
		case MODE_CONTESTIA_8_1000:  case MODE_CONTESTIA_8_2000:
		case MODE_CONTESTIA_16_250:  case MODE_CONTESTIA_16_500:
		case MODE_CONTESTIA_16_1000: case MODE_CONTESTIA_16_2000:
		case MODE_CONTESTIA_32_1000: case MODE_CONTESTIA_32_2000:
		case MODE_CONTESTIA_64_500:  case MODE_CONTESTIA_64_1000: case MODE_CONTESTIA_64_2000:
			open_config(TAB_CONTESTIA);
			break;
		case MODE_FSQ:
			open_config(TAB_FSQ);
			break;
		case MODE_IFKP:
			open_config(TAB_IFKP);
			break;
		case MODE_RTTY:
			open_config(TAB_RTTY);
			break;
		default:
			open_config(TAB_PSK);
			break;
	}
}

/*
void cb_mnuConfigWinkeyer(Fl_Menu_*, void*) {
	open_config(TAB_CW);
}

void cb_mnuConfigWFcontrols(Fl_Menu_ *, void*) {
	open_config(TAB_UI_WATERFALL);
}

void cb_n3fjp_logs(Fl_Menu_ *, void*) {
	open_config(TAB_UI_N3FJP);
}

void cb_maclogger(Fl_Menu_ *, void*) {
	open_config(TAB_UI_MACLOGGER);
}
*/

void cb_mnuConfigLoTW(Fl_Menu_ *, void *) {
	open_config(TAB_LOG_LOTW);
}

void cb_logfile(Fl_Widget* w, void*)
{
	progStatus.LOGenabled = reinterpret_cast<Fl_Menu_*>(w)->mvalue()->value();
	if (progStatus.LOGenabled == true) {
		Date tdy;
		string lfname = HomeDir;
		lfname.append("fldigi");
		lfname.append(tdy.szDate(2));
		lfname.append(".log");
	   	logfile = new cLogfile(lfname);
		logfile->log_to_file_start();
	} else {
		logfile->log_to_file_stop();
		delete logfile;
		logfile = 0;
	}
}


// LOGBOOK server connect
void cb_log_server(Fl_Widget* w, void*)
{
	progdefaults.xml_logbook = reinterpret_cast<Fl_Menu_*>(w)->mvalue()->value();
	close_logbook();
	connect_to_log_server();
}

void cb_fd_viewer(Fl_Widget* w, void*)
{
	if (field_day_viewer->visible())
		field_day_viewer->hide();
	else
		field_day_viewer->show();
}

void cb_dxc_viewer(Fl_Widget* w, void*)
{
	if (dxcluster_viewer->visible())
		dxcluster_viewer->hide();
	else
		dxcluster_viewer->show();
}

void set_server_label(bool val)
{
	Fl_Menu_Item *m = getMenuItem(LOG_CONNECT_SERVER);
	if (val) m->set();
	else m->clear();
}

static int save_mvx = 0;

void cb_view_hide_channels(Fl_Menu_ *w, void *d)
{
	int mvgw = mvgroup->w();

	progStatus.show_channels = !(mvgw > mvgroup->x());

	if (!progStatus.show_channels) {
		save_mvx = mvgw;
		progStatus.tile_x = mvgroup->x();
	} else {
		progStatus.tile_x = save_mvx;
	}

	if (progdefaults.rxtx_swap) {
		progStatus.tile_y = TransmitText->h();
		progStatus.tile_y_ratio = 1.0 * TransmitText->h() / text_panel->h();
	} else {
		progStatus.tile_y = ReceiveText->h();
		progStatus.tile_y_ratio = 1.0 * ReceiveText->h() / text_panel->h();
	}
	UI_select();
	return;
}


static bool capval = false;
static bool genval = false;
static bool playval = false;

void cb_mnuCapture(Fl_Widget *w, void *d)
{
	if (!RXscard) return;
	Fl_Menu_Item *m = getMenuItem(((Fl_Menu_*)w)->mvalue()->label()); //eek
	if (playval || genval) {
		m->clear();
		return;
	}
	capval = m->value();

	if (!m->value()) {
		RXscard->stopCapture();
		return;
	}

	std::string fname;
	int format;
	SND_SUPPORT::get_file_params("capture", fname, format, true);
	if (fname.empty()) {
		m->clear();
		capval = 0;
		return;
	}

	if(!RXscard->startCapture(fname, format)) {
		m->clear();
		capval = false;
	}
}

void cb_mnuGenerate(Fl_Widget *w, void *d)
{
	Fl_Menu_Item *m = getMenuItem(((Fl_Menu_*)w)->mvalue()->label());
	if (capval || playval) {
		m->clear();
		return;
	}
	if (!TXscard) return;
	genval = m->value();

	if (!genval) {
		TXscard->stopGenerate();
		return;
	}

	std::string fname;
	int format;
	SND_SUPPORT::get_file_params("generate", fname, format, true);
	if (fname.empty()) {
		m->clear();
		genval = 0;
		return;
	}

	if (!TXscard->startGenerate(fname, format)) {
		m->clear();
		genval = false;
	}
}

Fl_Menu_Item *Playback_menu_item = (Fl_Menu_Item *)0;
void reset_mnuPlayback()
{
	if (Playback_menu_item == 0) return;
	Playback_menu_item->clear();
	playval = false;
}

void cb_mnuPlayback(Fl_Widget *w, void *d)
{
	if (!RXscard) return;
	Fl_Menu_Item *m = getMenuItem(((Fl_Menu_*)w)->mvalue()->label());
	Playback_menu_item = m;
	if (capval || genval) {
		m->clear();
		bHighSpeed = false;
		return;
	}
	playval = m->value();
	if (!playval) {
		bHighSpeed = false;
		RXscard->stopPlayback();
		return;
	}

	std::string fname;
	int format;
	SND_SUPPORT::get_file_params("playback", fname, format, false);

	if (fname.empty()) {
		m->clear();
		playval = 0;
		return;
	}

	progdefaults.loop_playback = fl_choice2(_("Playback continuous loop?"), _("No"), _("Yes"), NULL);

	int err = RXscard->startPlayback(fname, format);

	if(err) {
		fl_alert2(_("Unsupported audio format"));
		m->clear();
		playval = false;
		bHighSpeed = false;
		progdefaults.loop_playback = false;
	}
	else if (btnAutoSpot->value()) {
		put_status(_("Spotting disabled"), 3.0);
		btnAutoSpot->value(0);
		btnAutoSpot->do_callback();
	}
}

bool first_tab_select = true;

void cb_mnu_config_dialog(Fl_Menu_*, void*)
{
	if (first_tab_select) {
		select_tab_tree(TAB_STATION);
		first_tab_select = false;
	}
	dlgConfig->show();
}

void cb_mnuSaveConfig(Fl_Menu_ *, void *) {
	progdefaults.saveDefaults();
	restoreFocus(7);
}

// This function may be called by the QRZ thread
void cb_mnuVisitURL(Fl_Widget*, void* arg)
{
	const char* url = reinterpret_cast<const char *>(arg);
#ifndef __WOE32__
	const char* browsers[] = {
#  ifdef __APPLE__
		getenv("FLDIGI_BROWSER"), // valid for any OS - set by user
		"open"                    // OS X
#  else
		"fl-xdg-open",            // Puppy Linux
		"xdg-open",               // other Unix-Linux distros
		getenv("FLDIGI_BROWSER"), // force use of spec'd browser
		getenv("BROWSER"),        // most Linux distributions
		"sensible-browser",
		"firefox",
		"mozilla"                 // must be something out there!
#  endif
	};
	switch (fork()) {
	case 0:
#  ifndef NDEBUG
		unsetenv("MALLOC_CHECK_");
		unsetenv("MALLOC_PERTURB_");
#  endif
		for (size_t i = 0; i < sizeof(browsers)/sizeof(browsers[0]); i++)
			if (browsers[i])
				execlp(browsers[i], browsers[i], url, (char*)0);
		exit(EXIT_FAILURE);
	case -1:
		fl_alert2(_("Could not run a web browser:\n%s\n\n"
			 "Open this URL manually:\n%s"),
			 strerror(errno), url);
	}
#else
	// gurgle... gurgle... HOWL
	// "The return value is cast as an HINSTANCE for backward
	// compatibility with 16-bit Windows applications. It is
	// not a true HINSTANCE, however. The only thing that can
	// be done with the returned HINSTANCE is to cast it to an
	// int and compare it with the value 32 or one of the error
	// codes below." (Error codes omitted to preserve sanity).
	if ((INT_PTR)ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL) <= 32)
		fl_alert2(_("Could not open url:\n%s\n"), url);
#endif
}

void open_recv_folder(const char *folder)
{
	cb_mnuVisitURL(0, (void*)folder);
}

void cb_mnuVisitPSKRep(Fl_Widget*, void*)
{
	cb_mnuVisitURL(0, (void*)string("http://pskreporter.info/pskmap?").append(progdefaults.myCall).c_str());
}

void html_help( const string &Html)
{
	if (!help_dialog)
		help_dialog = new Fl_Help_Dialog;
	help_dialog->value(Html.c_str());
	help_dialog->show();
}

void cb_mnuBeginnersURL(Fl_Widget*, void*)
{
	string deffname = HelpDir;
	deffname.append("beginners.html");
	ofstream f(deffname.c_str());
	if (!f)
		return;
	f << szBeginner;
	f.close();
#ifndef __WOE32__
	cb_mnuVisitURL(NULL, (void *)deffname.insert(0, "file://").c_str());
#else
	cb_mnuVisitURL(NULL, (void *)deffname.c_str());
#endif
}

void cb_mnuOnLineDOCS(Fl_Widget *, void *)
{
	string helpfile = HelpDir;
	helpfile.append("fldigi-help/index.html");
	ifstream f(helpfile.c_str());
	if (!f) {
		cb_mnuVisitURL(0, (void *)PACKAGE_DOCS);
	} else {
		f.close();
		cb_mnuVisitURL(0, (void *)helpfile.c_str());
	}
}

inline int version_check(string v1, string v2) {

	long v1a, v1b, v1c;
	long v2a, v2b, v2c;
	size_t p;

	v1a = atol(v1.c_str()); p = v1.find("."); v1.erase(0, p + 1);
	v1b = atol(v1.c_str()); p = v1.find("."); v1.erase(0, p + 1);
	v1c = atol(v1.c_str()); p = v1.find("."); v1.erase(0, p + 1);

	v2a = atol(v2.c_str()); p = v2.find("."); v2.erase(0, p + 1);
	v2b = atol(v2.c_str()); p = v2.find("."); v2.erase(0, p + 1);
	v2c = atol(v2.c_str()); p = v2.find("."); v2.erase(0, p + 1);

	long l1, l2;
	l1 = v1a * 10000 + v1b * 100 + v1c;
	l2 = v2a * 10000 + v2b * 100 + v2c;

	if (l1 < l2) return -1;
	if (l1 > l2) return 1;
	if (v1 == v2) return 0;
	return 1;
}

static notify_dialog *latest_dialog = 0;
void cb_mnuCheckUpdate(Fl_Widget *, void *)
{
	const char *url = "http://www.w1hkj.com/files/fldigi/";

	string version_str;
	string reply;

	put_status(_("Checking for updates..."));

	int ret = get_http(url, reply, 20.0);
	if (!ret) {
		put_status(_("Update site not available"), 10);
		return;
	}

	size_t p = reply.find("_setup.exe");
	size_t p2 = reply.rfind("fldigi", p);
	p2 += 7;
	version_str = reply.substr(p2, p - p2);
	int is_ok = version_check(string(PACKAGE_VERSION), version_str);

	if (!latest_dialog) latest_dialog = new notify_dialog;
	if (is_ok == 0) {
		latest_dialog->notify(_("You are running the latest version"), 5.0);
		REQ(show_notifier, latest_dialog);
	} else if (is_ok > 0) {
		std::string probable;
		probable.assign(_("You are probably running an alpha version "));
		probable.append( PACKAGE_VERSION ).append(_("\nPosted version: "));
		probable.append(version_str);
		latest_dialog->notify(probable.c_str(), 5.0);
		REQ(show_notifier, latest_dialog);
	} else
		fl_message2(_("Version %s is available at Source Forge"),
				  version_str.c_str());

	put_status("");
}

void cb_mnuAboutURL(Fl_Widget*, void*)
{
	if (!help_dialog)
		help_dialog = new Fl_Help_Dialog;
	help_dialog->value(szAbout);
	help_dialog->resize(help_dialog->x(), help_dialog->y(), help_dialog->w(), 440);
	help_dialog->show();
}

void fldigi_help(const string& theHelp)
{
	string htmlHelp =
"<HTML>"
"<HEAD>"
"<TITLE>" PACKAGE " Help</TITLE>"
"</HEAD>"
"<BODY>"
"<FONT FACE=fixed>"
"<P><TT>";

	for (size_t i = 0; i < theHelp.length(); i++) {
		if (theHelp[i] == '\n') {
			if (theHelp[i+1] == '\n') {
				htmlHelp += "</TT></P><P><TT>";
				i++;
			}
			else
				htmlHelp += "<BR>";
		} else if (theHelp[i] == ' ' && theHelp[i+1] == ' ') {
			htmlHelp += "&nbsp;&nbsp;";
			i++;
		} else
			htmlHelp += theHelp[i];
	}
	htmlHelp +=
"</TT></P>"
"</BODY>"
"</HTML>";
	html_help(htmlHelp);
}

void cb_mnuCmdLineHelp(Fl_Widget*, void*)
{
	extern string option_help;
	fldigi_help(option_help);
	restoreFocus(8);
}

void cb_mnuBuildInfo(Fl_Widget*, void*)
{
	extern string build_text;
	fldigi_help(build_text);
	restoreFocus(9);
}

void cb_mnuDebug(Fl_Widget*, void*)
{
	debug::show();
}

#ifndef NDEBUG
void cb_mnuFun(Fl_Widget*, void*)
{
		fl_message2(_("Sunspot creation underway!"));
}
#endif

void cb_mnuAudioInfo(Fl_Widget*, void*)
{
		if (progdefaults.btnAudioIOis != SND_IDX_PORT) {
				fl_alert2(_("Audio device information is only available for the PortAudio backend"));
				return;
		}

#if USE_PORTAUDIO
	size_t ndev;
		string devtext[2], headers[2];
	SoundPort::devices_info(devtext[0], devtext[1]);
	if (devtext[0] != devtext[1]) {
		headers[0] = _("Capture device");
		headers[1] = _("Playback device");
		ndev = 2;
	}
	else {
		headers[0] = _("Capture and playback devices");
		ndev = 1;
	}

	string audio_info;
	for (size_t i = 0; i < ndev; i++) {
		audio_info.append("<center><h4>").append(headers[i]).append("</h4>\n<table border=\"1\">\n");

		string::size_type j, n = 0;
		while ((j = devtext[i].find(": ", n)) != string::npos) {
			audio_info.append("<tr>")
				  .append("<td align=\"center\">")
				  .append(devtext[i].substr(n, j-n))
				  .append("</td>");

			if ((n = devtext[i].find('\n', j)) == string::npos) {
				devtext[i] += '\n';
				n = devtext[i].length() - 1;
			}

			audio_info.append("<td align=\"center\">")
				  .append(devtext[i].substr(j+2, n-j-2))
				  .append("</td>")
				  .append("</tr>\n");
		}
		audio_info.append("</table></center><br>\n");
	}

	fldigi_help(audio_info);
#endif
}

void cb_ShowConfig(Fl_Widget*, void*)
{
	cb_mnuVisitURL(0, (void*)HomeDir.c_str());
}

static void cb_ShowDATA(Fl_Widget*, void*)
{
	/// Must be already created by createRecordLoader()
	dlgRecordLoader->show();
}

bool ask_dir_creation( const std::string & dir )
{
	if ( 0 == directory_is_created(dir.c_str())) {
		int ans = fl_choice2(_("%s: Do not exist, create?"), _("No"), _("Yes"), 0, dir.c_str() );
		if (!ans) return false ;
		return true ;
	}
	return false ;
}

void cb_ShowNBEMS(Fl_Widget*, void*)
{
	if ( ask_dir_creation(NBEMS_dir)) {
		check_nbems_dirs();
	}
	cb_mnuVisitURL(0, (void*)NBEMS_dir.c_str());
}

void cb_ShowFLMSG(Fl_Widget*, void*)
{
	if ( ask_dir_creation(FLMSG_dir)) {
		check_nbems_dirs();
	}
	cb_mnuVisitURL(0, (void*)FLMSG_dir.c_str());
}

void cb_ShowWEFAX_images(Fl_Widget*, void*)
{
	if (progdefaults.wefax_save_dir.empty())
		cb_mnuVisitURL(0, (void*)PicsDir.c_str());
	else
		cb_mnuVisitURL(0, (void*)progdefaults.wefax_save_dir.c_str());
}

void cbTune(Fl_Widget *w, void *) {
	Fl_Button *b = (Fl_Button *)w;
	if (!(active_modem->get_cap() & modem::CAP_TX)) {
		b->value(0);
		return;
	}
	if (b->value() == 1) {
		b->labelcolor(FL_RED);
		trx_tune();
	} else {
		b->labelcolor(FL_FOREGROUND_COLOR);
		trx_receive();
	}
	restoreFocus(10);
}

void cb_quick_rsid (Fl_Widget *w, void *)
{
	progdefaults.rsidWideSearch = !progdefaults.rsidWideSearch;
	if (progdefaults.rsidWideSearch) chkRSidWideSearch->set();
	else chkRSidWideSearch->clear();
}

static Fl_Menu_Item quick_change_rsid[] = {
	{ "Passband", 0, cb_quick_rsid, 0, FL_MENU_TOGGLE },
	{0,0,0,0,0,0,0,0,0}
};

void cbRSID(Fl_Widget *w, void *)
{
	if (Fl::focus() != btnRSID) {
		progdefaults.rsid = btnRSID->value();
		btnRSID->redraw();
		return;
	}

	switch (Fl::event_button()) {
	case FL_LEFT_MOUSE:
		progdefaults.rsid = btnRSID->value();
		progdefaults.changed = true;
		break;
	case FL_RIGHT_MOUSE:
	{
		btnRSID->value(progdefaults.rsid);
		btnRSID->redraw();
		if (progdefaults.rsidWideSearch) quick_change_rsid[0].set();
		else quick_change_rsid[0].clear();
		const Fl_Menu_Item *m =
			quick_change_rsid->popup(
				btnRSID->x(),
				btnRSID->y() + btnRSID->h());
		if (m && m->callback()) m->do_callback(0);
		break;
	}
	default:
		break;
	}

	Fl_Color clr = progdefaults.rsidWideSearch ? progdefaults.RxIDwideColor : progdefaults.RxIDColor;
	btnRSID->selection_color(clr);
	btnRSID->redraw();
	restoreFocus(11);
}

void cbTxRSID(Fl_Widget *w, void*)
{
	progdefaults.TransmitRSid = btnTxRSID->value();
	if (Fl::focus() != btnTxRSID) {
		btnTxRSID->redraw();
		return;
	}
	progdefaults.changed = true;
	restoreFocus(12);
}

void cbAutoSpot(Fl_Widget* w, void*)
{
	progStatus.spot_recv = static_cast<Fl_Light_Button*>(w)->value();
}

void toggleRSID()
{
	progdefaults.rsid = !progdefaults.rsid;
	cbRSID(NULL, NULL);
}

static notify_dialog *rx_monitor_alert = 0;
void cb_mnuRxAudioDialog(Fl_Menu_ *w, void *d) {
	if (!progdefaults.enable_audio_alerts) {
		if (!rx_monitor_alert) rx_monitor_alert = new notify_dialog;
		rx_monitor_alert->notify("Audio-Alert / Rx-Monitor device NOT enabled", 10.0);
		show_notifier(rx_monitor_alert);
		return;
	}
	if (rxaudio_dialog)
		rxaudio_dialog->show();
}

void cb_mnuDigiscope(Fl_Menu_ *w, void *d) {
	if (scopeview)
		scopeview->show();
}

void cb_mnuViewer(Fl_Menu_ *, void *) {
	openViewer();
}

void cb_mnuSpectrum (Fl_Menu_ *, void *) {
	open_spectrum_viewer();
}

void cb_mnuShowCountries(Fl_Menu_ *, void *)
{
	notify_dxcc_show();
}

void set_macroLabels()
{
	if (bWF_only) return;
	if (progdefaults.mbar_scheme > MACRO_SINGLE_BAR_MAX) {
		altMacros = 1;
		for (int i = 0; i < NUMMACKEYS; i++) {
			btnMacro[i]->label(macros.name[i].c_str());
			btnMacro[i]->redraw_label();
			btnMacro[NUMMACKEYS + i]->label(
				macros.name[(altMacros * NUMMACKEYS) + i].c_str());
			btnMacro[NUMMACKEYS + i]->redraw_label();
		}
		btnAltMacros1->label("1");
		btnAltMacros1->redraw_label();
		btnAltMacros2->label("2");
		btnAltMacros2->redraw_label();
	} else {
		altMacros = 0;
		btnAltMacros1->label("1");
		btnAltMacros1->redraw_label();
		for (int i = 0; i < NUMMACKEYS; i++) {
			btnMacro[i]->label(macros.name[i].c_str());
			btnMacro[i]->redraw_label();
		}
	}
	for (int i = 0; i < 48; i++) {
		btnDockMacro[i]->label(macros.name[i].c_str());
		btnDockMacro[i]->redraw_label();
	}
}

void cb_mnuPicViewer(Fl_Menu_ *, void *) {
	if (picRxWin) {
		picRx->redraw();
		picRxWin->show();
	}
}

void cb_mnuThorViewRaw(Fl_Menu_ *, void *) {
	thor_load_raw_video();
}

void cb_mnuIfkpViewRaw(Fl_Menu_ *, void *) {
	ifkp_load_raw_video();
}

void cb_sldrSquelch(Fl_Slider* o, void*) {

	if (progdefaults.show_psm_btn && progStatus.kpsql_enabled) {
		progStatus.sldrPwrSquelchValue = o->value();
	} else {
		progStatus.sldrSquelchValue = o->value();
	}

	restoreFocus(13);
}

bool oktoclear = true;

void updateOutSerNo()
{
	Fl_Input2* outsn[] = {
		outSQSO_serno1, outSQSO_serno2,
		outSerNo1, outSerNo2, outSerNo3, outSerNo4,
		outSerNo5, outSerNo6, outSerNo7, outSerNo8,
		out_IARI_SerNo1, out_IARI_SerNo2,
//		outSerNo_WAE1, outSerNo_WAE2,
		outSerNo_WPX1, outSerNo_WPX2
		};

	size_t num_fields = sizeof(outsn)/sizeof(*outsn);
	for (size_t i = 0; i < num_fields; i++) {
		outsn[i]->value("");
		outsn[i]->redraw();
	}

	int nr = contest_count.count;

	if (!n3fjp_serno.empty()) {
		sscanf(n3fjp_serno.c_str(), "%d", &nr);
	}
	char szcnt[10] = "";
	contest_count.Format(progdefaults.ContestDigits, progdefaults.UseLeadingZeros);
	snprintf(szcnt, sizeof(szcnt), contest_count.fmt.c_str(), nr);

	for (size_t i = 0; i < num_fields; i++) {
		outsn[i]->value(szcnt);
		outsn[i]->redraw();
	}
}

static string old_call;
static string new_call;

void set599()
{
	if (bWF_only)
		return;

	Fl_Input2* rstin[] = {
		inpRstIn1, inpRstIn2, inpRstIn3, inpRstIn4, inpRstIn_AICW2,
		inpRstIn_SQSO2,
		inp_IARI_RSTin2,
//		inpRstIn_WAE2,
		inpRstIn_WPX2};
	Fl_Input2* rstout[] = {
		inpRstOut1, inpRstOut2, inpRstOut3, inpRstOut4, inpRstIn_AICW2,
		inpRstOut_SQSO2,
		inp_IARI_RSTout2,
//		inpRstOut_WAE2,
		inpRstOut_WPX2};
	if (!active_modem) return;
	string defrst = (active_modem->get_mode() == MODE_SSB) ? "59" : "599";
	if (progdefaults.RSTin_default) {
		size_t num_fields = sizeof(rstin)/sizeof(*rstin);
		for (size_t i = 0; i < num_fields; i++)
			rstin[i]->value(defrst.c_str());
	}
	if (progdefaults.RSTdefault) {
		size_t num_fields = sizeof(rstout)/sizeof(*rstout);
		for (size_t i = 0; i < num_fields; i++)
			rstout[i]->value(defrst.c_str());
	}
	if (progdefaults.logging > 0 && progdefaults.fixed599) {
		size_t num_fields = sizeof(rstout)/sizeof(*rstout);
		for (size_t i = 0; i < num_fields; i++)
			rstout[i]->value(defrst.c_str());
	}
}

void init_country_fields()
{
	Fl_ComboBox *country_fields[] = {
		cboCountryQSO,
		cboCountryAICW2,
		cboCountryAIDX2,
		cboCountryCQ2,
		cboCountryCQDX2,
		cboCountryIARI2,
		cboCountryRTU2//,
//		cboCountryWAE2
	};
	for (size_t i = 0; i < sizeof(country_fields)/sizeof(*country_fields); i++) {
		country_fields[i]->add(cbolist.c_str());
	}

	cboCountyQSO->add(counties().c_str());

}

void set_log_colors()
{
Fl_Input2* qso_fields[] = {
	inpCall1, inpCall2, inpCall3, inpCall4,
	inpName1, inpName2,
	inpTimeOn1, inpTimeOn2, inpTimeOn3, inpTimeOn4, inpTimeOn5,
	inpRstIn1, inpRstIn2,
	inpRstOut1, inpRstOut2,
	inpQth, inpLoc1, inpAZ, inpVEprov,
	inpState1,
	inpSerNo1, inpSerNo2, inpSerNo3, inpSerNo4,
	outSerNo1, outSerNo2, outSerNo3, outSerNo4,
	outSerNo5, outSerNo6, outSerNo7, outSerNo8,
	inpXchgIn1, inpXchgIn2,
	inp_FD_class1, inp_FD_section1, inp_FD_class2, inp_FD_section2,
	inp_KD_name2, inp_KD_age1, inp_KD_age2,
	inp_KD_state1, inp_KD_state2,
	inp_KD_VEprov1, inp_KD_VEprov2,
	inp_KD_XchgIn1, inp_KD_XchgIn2,
	inp_SS_Check1, inp_SS_Precedence1, inp_SS_Section1, inp_SS_SerialNoR1,
	inp_SS_Check2, inp_SS_Precedence2, inp_SS_Section2, inp_SS_SerialNoR2,
	inp_CQ_RSTin2, inp_CQDX_RSTin2,
	inp_CQ_RSTout2, inp_CQDX_RSTout2,
	inp_CQstate1, inp_CQstate2,
	inp_CQzone1, inp_CQzone2,
	inp_CQDXzone1, inp_CQDXzone2,
	inp_1010_XchgIn1, inp_1010_XchgIn2, inp_1010_name2, inp_1010_nr1, inp_1010_nr2,
	inp_ARR_Name2, inp_ARR_XchgIn1, inp_ARR_XchgIn2, inp_ARR_check1, inp_ARR_check2,
	inp_ASCR_RSTin2, inp_ASCR_RSTout2, inp_ASCR_XchgIn1, inp_ASCR_XchgIn2,
	inp_ASCR_class1, inp_ASCR_class2, inp_ASCR_name2,
	inp_vhf_Loc1, inp_vhf_Loc2,
	inp_vhf_RSTin1, inp_vhf_RSTin2,
	inp_vhf_RSTout1, inp_vhf_RSTout2,
	inpSPCnum_NAQP1, inpSPCnum_NAQP2,
	inpNAQPname2, inp_name_NAS2,
	inp_ser_NAS1, inpSPCnum_NAS1,
	inp_ser_NAS2, inpSPCnum_NAS2,
	inpRTU_stpr1, inpRTU_stpr2,
	inpRTU_RSTin2, inpRTU_RSTout2,
	inpRTU_serno1, inpRTU_serno2,
	inp_IARI_PR1, inp_IARI_PR2,
	inp_IARI_RSTin2, inp_IARI_RSTout2,
	out_IARI_SerNo1, inp_IARI_SerNo1,
	out_IARI_SerNo2, inp_IARI_SerNo2,
	inpRstIn3, inpRstOut3,
	inpRstIn4, inpRstOut4,
	inp_JOTA_scout1, inp_JOTA_scout2,
	inp_JOTA_troop1, inp_JOTA_troop2,
	inp_JOTA_spc1, inp_JOTA_spc2,
	inpRstIn_AICW2, inpRstOut_AICW2,
	inpSPCnum_AICW1, inpSPCnum_AICW2,
	inpSQSO_state1, inpSQSO_state2,
	inpSQSO_county1, inpSQSO_county2,
	inpSQSO_serno1, inpSQSO_serno2,
	outSQSO_serno1, outSQSO_serno2,
	inpRstIn_SQSO2, inpRstOut_SQSO2,
	inpSQSO_name2,
	inpSQSO_category1, inpSQSO_category2,
	inpSerNo_WPX1, inpSerNo_WPX2,
	inpRstIn_WPX2, inpRstOut_WPX2,
	outSerNo_WPX1, outSerNo_WPX2,
	inpSerNo_WAE1, inpSerNo_WAE2,
	outSerNo_WAE1, outSerNo_WAE2,
	inpRstIn_WAE2, inpRstOut_WAE2,
	inpNotes };

	if (!bWF_only) {
		Fl_ComboBox *country_fields[] = {
			cboCountyQSO, cboCountryQSO,
			cboCountryAICW2,
			cboCountryAIDX2,
			cboCountryCQ2,
			cboCountryCQDX2,
			cboCountryIARI2,
			cboCountryRTU2 //,
//			cboCountryWAE2
		};
		for (size_t i = 0; i < sizeof(country_fields)/sizeof(*country_fields); i++) {
			country_fields[i]->redraw();
			combo_color_font(country_fields[i]);
		}

		size_t num_fields = sizeof(qso_fields)/sizeof(*qso_fields);
		for (size_t i = 0; i < num_fields; i++) {
			qso_fields[i]->textsize(progdefaults.LOGGINGtextsize);
			qso_fields[i]->textfont(progdefaults.LOGGINGtextfont);
			qso_fields[i]->textcolor(progdefaults.LOGGINGtextcolor);
			qso_fields[i]->color(progdefaults.LOGGINGcolor);
			qso_fields[i]->labelfont(progdefaults.LOGGINGtextfont);
			qso_fields[i]->redraw_label();
		}

		if (!progdefaults.SQSOlogstate) {
			inpSQSO_state1->hide();
			inpSQSO_state2->hide();
		}

		if (!progdefaults.SQSOlogcounty) {
			inpSQSO_county1->hide();
			inpSQSO_county2->hide();
		}

		if (!progdefaults.SQSOlogserno) {
			inpSQSO_serno1->hide();
			inpSQSO_serno2->hide();
			outSQSO_serno1->hide();
			outSQSO_serno2->hide();
		}

		if (!progdefaults.SQSOlogrst) {
			inpRstIn_SQSO2->hide();
			inpRstOut_SQSO2->hide();
		}

		if (!progdefaults.SQSOlogname) {
			inpSQSO_name2->hide();
		}

		if (!progdefaults.SQSOlogcat) {
			inpSQSO_category1->hide();
			inpSQSO_category2->hide();
		}
		for (size_t i = 0; i < num_fields; i++) {
			qso_fields[i]->redraw();
		}
	}

}

void clear_time_on()
{
Fl_Input2* log_fields[] = {
	inpTimeOn1, inpTimeOn2, inpTimeOn3, inpTimeOn4, inpTimeOn5 };

	size_t num_fields = sizeof(log_fields)/sizeof(*log_fields);
	for (size_t i = 0; i < num_fields; i++) {
		log_fields[i]->value("");
		log_fields[i]->textsize(progdefaults.LOGGINGtextsize);
		log_fields[i]->textfont(progdefaults.LOGGINGtextfont);
		log_fields[i]->textcolor(progdefaults.LOGGINGtextcolor);
		log_fields[i]->color(progdefaults.LOGGINGcolor);
		log_fields[i]->labelfont(progdefaults.LOGGINGtextfont);
		log_fields[i]->show();
		log_fields[i]->redraw_label();
		log_fields[i]->redraw();
	}
}

void clear_log_fields()
{
Fl_Input2* log_fields[] = {
	inpName1, inpName2,
//	inpTimeOn1, inpTimeOn2, inpTimeOn3, inpTimeOn4, inpTimeOn5,
	inpRstIn1, inpRstIn2,
	inpRstOut1, inpRstOut2,
	inpQth, inpLoc1, inpAZ, inpVEprov,
	inpState1,
	inpSerNo1, inpSerNo2,
	outSerNo1, outSerNo2,
	outSerNo3, outSerNo4,
	inpXchgIn1, inpXchgIn2,
	inp_FD_class1, inp_FD_section1, inp_FD_class2, inp_FD_section2,
	inp_KD_name2, inp_KD_age1, inp_KD_age2,
	inp_KD_state1, inp_KD_state2,
	inp_KD_VEprov1, inp_KD_VEprov2,
	inp_KD_XchgIn1, inp_KD_XchgIn2,
	inp_SS_Check1, inp_SS_Precedence1, inp_SS_Section1, inp_SS_SerialNoR1,
	inp_SS_Check2, inp_SS_Precedence2, inp_SS_Section2, inp_SS_SerialNoR2,
	inp_CQ_RSTin2, inp_CQDX_RSTin2,
	inp_CQ_RSTout2, inp_CQDX_RSTout2,
	inp_CQstate1, inp_CQstate2,
	inp_CQzone1, inp_CQzone2,
	inp_CQDXzone1, inp_CQDXzone2,
	inp_1010_XchgIn1, inp_1010_XchgIn2, inp_1010_name2, inp_1010_nr1, inp_1010_nr2,
	inp_ARR_Name2, inp_ARR_XchgIn1, inp_ARR_XchgIn2, inp_ARR_check1, inp_ARR_check2,
	inp_ASCR_RSTin2, inp_ASCR_RSTout2, inp_ASCR_XchgIn1, inp_ASCR_XchgIn2,
	inp_ASCR_class1, inp_ASCR_class2, inp_ASCR_name2,
	inp_vhf_Loc1, inp_vhf_Loc2,
	inp_vhf_RSTin1, inp_vhf_RSTin2,
	inp_vhf_RSTout1, inp_vhf_RSTout2,
	inpSPCnum_NAQP1, inpSPCnum_NAQP2,
	inpNAQPname2, inp_name_NAS2,
	outSerNo4, inp_ser_NAS1, inpSPCnum_NAS1,
	outSerNo5, inp_ser_NAS2, inpSPCnum_NAS2,
	inpRTU_stpr1, inpRTU_stpr2,
	inpRTU_RSTin2, inpRTU_RSTout2,
	inpRTU_serno1, inpRTU_serno2,
	inp_IARI_RSTin2, inp_IARI_RSTout2,
	out_IARI_SerNo1, inp_IARI_SerNo1,
	out_IARI_SerNo2, inp_IARI_SerNo2,
	inp_IARI_PR1, inp_IARI_PR2,
	inpSerNo3, inpSerNo4,
	outSerNo7, outSerNo8,
	inpRstIn3, inpRstOut3,
	inpRstIn4, inpRstOut4,
	inp_JOTA_scout1, inp_JOTA_scout2,
	inp_JOTA_troop1, inp_JOTA_troop2,
	inp_JOTA_spc1, inp_JOTA_spc2,
	inpRstIn_AICW2, inpRstOut_AICW2,
	inpSPCnum_AICW1, inpSPCnum_AICW2,
	inpSerNo_WPX1, inpSerNo_WPX2,
	inpRstIn_WPX2, inpRstOut_WPX2,
	inpSerNo_WAE1, inpSerNo_WAE2,
	outSerNo_WAE1, outSerNo_WAE2,
	inpRstIn_WAE2, inpRstOut_WAE2,
	inpSQSO_category1, inpSQSO_category2,
	inpSQSO_county1, inpSQSO_county2,
	inpSQSO_name2,
	inpSQSO_serno1, inpSQSO_serno2,
	inpSQSO_state1, inpSQSO_state2,
	inpNotes };

	size_t num_fields = sizeof(log_fields)/sizeof(*log_fields);
	for (size_t i = 0; i < num_fields; i++) {
		log_fields[i]->value("");
		log_fields[i]->textsize(progdefaults.LOGGINGtextsize);
		log_fields[i]->textfont(progdefaults.LOGGINGtextfont);
		log_fields[i]->textcolor(progdefaults.LOGGINGtextcolor);
		log_fields[i]->color(progdefaults.LOGGINGcolor);
		log_fields[i]->labelfont(progdefaults.LOGGINGtextfont);
		log_fields[i]->show();
		log_fields[i]->redraw_label();
		log_fields[i]->redraw();
	}

	Fl_ComboBox *country_fields[] = {
		cboCountyQSO, cboCountryQSO,
		cboCountryAICW2,
		cboCountryAIDX2,
		cboCountryCQ2,
		cboCountryCQDX2,
		cboCountryIARI2,
		cboCountryRTU2 //,
//		cboCountryWAE2
	};
	for (size_t i = 0; i < sizeof(country_fields)/sizeof(*country_fields); i++) {
		country_fields[i]->value("");
		country_fields[i]->redraw();
		combo_color_font(country_fields[i]);
	}

	if (!progdefaults.SQSOlogstate) {
		inpSQSO_state1->hide();
		inpSQSO_state2->hide();
	}

	if (!progdefaults.SQSOlogcounty) {
		inpSQSO_county1->hide();
		inpSQSO_county2->hide();
	}

	if (!progdefaults.SQSOlogserno) {
		inpSQSO_serno1->hide();
		inpSQSO_serno2->hide();
		outSQSO_serno1->hide();
		outSQSO_serno2->hide();
	}

	if (!progdefaults.SQSOlogrst) {
		inpRstIn_SQSO2->hide();
		inpRstOut_SQSO2->hide();
	}

	if (!progdefaults.SQSOlogname) {
		inpSQSO_name2->hide();
	}

	if (!progdefaults.SQSOlogcat) {
		inpSQSO_category1->hide();
		inpSQSO_category2->hide();
	}

	if (progdefaults.logging == LOG_SQSO) {
		std::string tmp = QSOparties.qso_parties[progdefaults.SQSOcontest].state;
		if (!progdefaults.SQSOinstate && (tmp != "7QP") && (tmp != "6NE"))
			inpState->value(tmp.c_str());
	}

	set599();
	updateOutSerNo();

}

void clearQSO()
{
if (bWF_only) return;

	Fl_Input2* call_fields[] = { inpCall1, inpCall2, inpCall3, inpCall4 };
	size_t num_fields = sizeof(call_fields)/sizeof(*call_fields);
	for (size_t i = 0; i < num_fields; i++) {
		call_fields[i]->value("");
		call_fields[i]->textsize(progdefaults.LOGGINGtextsize);
		call_fields[i]->textfont(progdefaults.LOGGINGtextfont);
		call_fields[i]->textcolor(progdefaults.LOGGINGtextcolor);
		call_fields[i]->color(progdefaults.LOGGINGcolor);
		call_fields[i]->labelfont(progdefaults.LOGGINGtextfont);
		call_fields[i]->show();
		call_fields[i]->redraw_label();
		call_fields[i]->redraw();
	}

	clear_log_fields();
	clear_time_on();

	if (inpSearchString)
		inpSearchString->value ("");
	old_call.clear();
	new_call.clear();
	qso_time.clear();
	qso_exchange.clear();
	oktoclear = true;

	inpCall->take_focus();

	if (n3fjp_connected)
		n3fjp_clear_record();

	set599();
	updateOutSerNo();

}

void cb_ResetSerNbr()
{
	contest_count.count = progdefaults.ContestStart;
	updateOutSerNo();
}

void cb_btnTimeOn(Fl_Widget* w, void*)
{
	inpTimeOn->value(inpTimeOff->value(), inpTimeOff->size());
	inpTimeOn1->value(inpTimeOff->value(), inpTimeOff->size());
	inpTimeOn2->value(inpTimeOff->value(), inpTimeOff->size());
	inpTimeOn3->value(inpTimeOff->value(), inpTimeOff->size());
	inpTimeOn4->value(inpTimeOff->value(), inpTimeOff->size());
	inpTimeOn5->value(inpTimeOff->value(), inpTimeOff->size());
	sTime_on = sTime_off = ztime();
	sDate_on = sDate_off = zdate();
	restoreFocus(14);
}

void cb_loc(Fl_Widget* w, void*)
{
	Fl_Input2 *inp = (Fl_Input2 *) w;

	inpLoc1->value(inp->value());
	inp_vhf_Loc1->value(inp->value());
	inp_vhf_Loc2->value(inp->value());

	std::string s;
	s = inp->value();

	if (s.length() < 3) return;

	double lon[2], lat[2], distance, azimuth;
	size_t len = s.length();
	if (len > MAX_LOC) {
		s.erase(MAX_LOC);
		len = MAX_LOC;
	}
	bool ok = true;
	for (size_t i = 0; i < len; i++) {
		if (ok)
		switch (i) {
			case 0 :
			case 1 :
			case 4 :
			case 5 :
				ok = isalpha(s[i]);
				break;
			case 2 :
			case 3 :
			case 6 :
			case 7 :
				ok = (s[i] >= '0' && s[i] <= '9');
		}
	}
	if ( !ok) {
		inpLoc1->value("");
		inp_vhf_Loc1->value("");
		inp_vhf_Loc2->value("");
		return;
	}

	if (QRB::locator2longlat(&lon[0], &lat[0], progdefaults.myLocator.c_str()) == QRB::QRB_OK &&
		QRB::locator2longlat(&lon[1], &lat[1], s.c_str()) == QRB::QRB_OK &&
		QRB::qrb(lon[0], lat[0], lon[1], lat[1], &distance, &azimuth) == QRB::QRB_OK) {
		char az[4];
		snprintf(az, sizeof(az), "%3.0f", azimuth);
		inpAZ->value(az);
	} else
		inpAZ->value("");

	if (Fl::event() == FL_KEYBOARD) {
		int k = Fl::event_key();
		if (k == FL_Enter || k == FL_KP_Enter)
			restoreFocus(15);
	}
}

void cb_call(Fl_Widget* w, void*)
{
if (bWF_only) return;

	qsodb.isdirty(1);

	if (progdefaults.calluppercase) {
		int pos = inpCall->position();
		char* uc = new char[inpCall->size()];
		transform(inpCall->value(), inpCall->value() + inpCall->size(), uc,
			  static_cast<int (*)(int)>(std::toupper));
		inpCall->value(uc, inpCall->size());
		inpCall->position(pos);
		delete [] uc;
	}

	new_call = inpCall->value();

	if (new_call.length() > MAX_CALL) {
		new_call.erase(MAX_CALL);
	}

	if (new_call != old_call) clear_log_fields();

	inpCall1->value(new_call.c_str());
	inpCall2->value(new_call.c_str());
	inpCall3->value(new_call.c_str());
	inpCall4->value(new_call.c_str());

	sDate_on = sDate_off = zdate();
	sTime_on = sTime_off = ztime();

	inpTimeOn->value(inpTimeOff->value());
	inpTimeOn1->value(inpTimeOff->value());
	inpTimeOn2->value(inpTimeOff->value());
	inpTimeOn3->value(inpTimeOff->value());
	inpTimeOn4->value(inpTimeOff->value());
	inpTimeOn5->value(inpTimeOff->value());

	if (progStatus.timer && (Fl::event() != FL_HIDE))
		stopMacroTimer();

	if (Fl::event() == FL_KEYBOARD) {
		int k = Fl::event_key();
		if (k == FL_Enter || k == FL_KP_Enter) {
			restoreFocus(16);
			n3fjp_calltab = true;
		}
		if (k == FL_Tab) {
			n3fjp_calltab = true;
		}
	}

	if (old_call == new_call) {
		if (n3fjp_calltab && n3fjp_connected)
			SearchLastQSO(inpCall->value());
		return;
	}

	if (new_call.empty()) {
		if (n3fjp_connected) n3fjp_clear_record();
		ifkp_load_avatar();
		thor_load_avatar();
		updateOutSerNo();
		oktoclear = true;
		return;
	}

	old_call = new_call;
	oktoclear = false;

	SearchLastQSO(inpCall->value());

	if (active_modem->get_mode() == MODE_IFKP)
		ifkp_load_avatar(inpCall->value());

	if (active_modem->get_mode() >= MODE_THOR11 &&
		active_modem->get_mode() <= MODE_THOR22)
		thor_load_avatar(inpCall->value());

	const struct dxcc* e = dxcc_lookup(inpCall->value());
	if (e) {
		if (progdefaults.autofill_qso_fields || progdefaults.logging != LOG_QSO) {
			double lon, lat, distance, azimuth;
			if (QRB::locator2longlat(&lon, &lat, progdefaults.myLocator.c_str()) == QRB::QRB_OK &&
				QRB::qrb(lon, lat, -e->longitude, e->latitude, &distance, &azimuth) == QRB::QRB_OK) {
				char az[4];
				snprintf(az, sizeof(az), "%3.0f", azimuth);
				inpAZ->value(az, sizeof(az) - 1);
			}
		}
		std::string cntry = e->country;
		std::ostringstream zone;
		zone << e->cq_zone;
		if (cntry.find("United States") != std::string::npos)
			cntry = "USA";
		cboCountry->value(cntry.c_str());
		inp_CQzone->value(zone.str().c_str());
	}

	if (progdefaults.EnableDupCheck || FD_logged_on) {
		DupCheck();
	}

	updateOutSerNo();

	if (w != inpCall)
		restoreFocus(17);
}

void cb_country(Fl_Widget *w, void*)
{
	Fl_ComboBox * inp = (Fl_ComboBox *) w;
	std::string str = inp->value();

	Fl_ComboBox *country_fields[] = {
		cboCountryQSO,
		cboCountryAICW2,
		cboCountryAIDX2,
		cboCountryCQ2,
		cboCountryCQDX2,
		cboCountryIARI2,
		cboCountryRTU2 //,
//		cboCountryWAE2
	};
	for (size_t i = 0; i < sizeof(country_fields)/sizeof(*country_fields); i++) {
		country_fields[i]->value(str.c_str());
		country_fields[i]->position(0);
		country_fields[i]->redraw();
	}

	if (progdefaults.EnableDupCheck || FD_logged_on) {
		DupCheck();
	}

	if (Fl::event() == FL_KEYBOARD) {
		int k = Fl::event_key();
		if (k == FL_Enter || k == FL_KP_Enter)
			restoreFocus(18);
	}
}

void cb_log(Fl_Widget* w, void*)
{
	Fl_Input2 *inp = (Fl_Input2 *) w;

	if (inp == inpName1 || inp == inpName2 ||
		inp == inp_KD_name2 ||
		inp == inp_1010_name2 ||
		inp == inp_ARR_Name2 ||
		inp == inpNAQPname2 ) {
		int p = inp->position();
		std::string val = inp->value();
		inpName1->value(val.c_str());
		inpName2->value(val.c_str());
		inp_KD_name2->value(val.c_str());
		inp_1010_name2->value(val.c_str());
		inp_ARR_Name2->value(val.c_str());
		inpNAQPname2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_KD_name2 || inp == inp_1010_name2 ||
		inp == inp_ARR_Name2 || inp == inpNAQPname2) {
		int p = inp->position();
		std::string val = inp->value();
		inpName1->value(val.c_str());
		inpName2->value(val.c_str());
		inp_KD_name2->value(val.c_str());
		inp_1010_name2->value(val.c_str());
		inp_ARR_Name2->value(val.c_str());
		inpNAQPname2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inpRstIn1 || inp == inpRstIn2 ||
			 inp == inpRstIn3 || inp == inpRstIn4 || inp == inpRstIn_AICW2 ||
			 inp == inpRTU_RSTin2 || inp == inp_CQ_RSTin2 ||
			 inp == inp_vhf_RSTin1 || inp == inp_vhf_RSTin2 ) {
		int p = inp->position();
		std::string val = inp->value();
		inpRstIn1->value(val.c_str());
		inpRstIn2->value(val.c_str());
		inpRstIn3->value(val.c_str());
		inpRstIn4->value(val.c_str());
		inpRstIn_AICW2->value(val.c_str());
		inpRTU_RSTin2->value(val.c_str());
		inp_vhf_RSTin1->value(val.c_str());
		inp_vhf_RSTin2->value(val.c_str());
		inp_CQ_RSTin2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inpRstOut1 || inp == inpRstOut2 ||
		inp == inp_CQ_RSTout2 ||
		inp == inpRTU_RSTout2 ||
		inp == inpRstOut3 || inp == inpRstOut4 || inp == inpRstOut_AICW2 ||
		inp == inp_vhf_RSTout1 || inp == inp_vhf_RSTout2 ) {
		int p = inp->position();
		std::string val = inp->value();
		inpRstOut1->value(val.c_str());
		inpRstOut2->value(val.c_str());
		inpRstOut3->value(val.c_str());
		inpRstOut4->value(val.c_str());
		inpRTU_RSTout2->value(val.c_str());
		inpRstOut_AICW2->value(val.c_str());
		inp_CQ_RSTout2->value(val.c_str());
		inp_vhf_RSTout1->value(val.c_str());
		inp_vhf_RSTout2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inpTimeOn1 || inp == inpTimeOn2 || inp == inpTimeOn3 ||
			 inp == inpTimeOn4 || inp == inpTimeOn5) {
		int p = inp->position();
		std::string val = inp->value();
		inpTimeOn1->value(val.c_str());
		inpTimeOn2->value(val.c_str());
		inpTimeOn3->value(val.c_str());
		inpTimeOn4->value(val.c_str());
		inpTimeOn5->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inpTimeOff1 || inp == inpTimeOff2 || inp == inpTimeOff3 ||
			 inp == inpTimeOff4 || inp == inpTimeOff5) {
		int p = inp->position();
		std::string val = inp->value();
		inpTimeOff1->value(val.c_str());
		inpTimeOff2->value(val.c_str());
		inpTimeOff3->value(val.c_str());
		inpTimeOff4->value(val.c_str());
		inpTimeOff5->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inpXchgIn1 || inp == inpXchgIn2 ) {
		int p = inp->position();
		std::string val = inp->value();
		inpXchgIn1->value(val.c_str());
		inpXchgIn2->value(val.c_str());
		inp_KD_XchgIn1->value(val.c_str());
		inp_KD_XchgIn2->value(val.c_str());
		inp_1010_XchgIn1->value(val.c_str());
		inp_1010_XchgIn2->value(val.c_str());
		inp_ARR_XchgIn1->value(val.c_str());
		inp_ARR_XchgIn2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_KD_XchgIn1 || inp == inp_KD_XchgIn2) {
		int p = inp->position();
		std::string val = inp->value();
		inpXchgIn1->value(val.c_str());
		inpXchgIn2->value(val.c_str());
		inp_KD_XchgIn1->value(val.c_str());
		inp_KD_XchgIn2->value(val.c_str());
		inp_1010_XchgIn1->value(val.c_str());
		inp_1010_XchgIn2->value(val.c_str());
		inp_ARR_XchgIn1->value(val.c_str());
		inp_ARR_XchgIn2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_1010_XchgIn1 || inp == inp_1010_XchgIn2) {
		int p = inp->position();
		std::string val = inp->value();
		inpXchgIn1->value(val.c_str());
		inpXchgIn2->value(val.c_str());
		inp_KD_XchgIn1->value(val.c_str());
		inp_KD_XchgIn2->value(val.c_str());
		inp_1010_XchgIn1->value(val.c_str());
		inp_1010_XchgIn2->value(val.c_str());
		inp_ARR_XchgIn1->value(val.c_str());
		inp_ARR_XchgIn2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_ARR_XchgIn1 || inp == inp_ARR_XchgIn2) {
		int p = inp->position();
		std::string val = inp->value();
		inpXchgIn1->value(val.c_str());
		inpXchgIn2->value(val.c_str());
		inp_KD_XchgIn1->value(val.c_str());
		inp_KD_XchgIn2->value(val.c_str());
		inp_1010_XchgIn1->value(val.c_str());
		inp_1010_XchgIn2->value(val.c_str());
		inp_ARR_XchgIn1->value(val.c_str());
		inp_ARR_XchgIn2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inpSerNo1 || inp == inpSerNo2 ||
		inp == inpRTU_serno1 || inp == inpRTU_serno2 ||
		inp == inp_SS_SerialNoR1 || inp == inp_SS_SerialNoR2 ) {
		int p = inp->position();
		std::string val = inp->value();
		inpSerNo1->value(val.c_str());
		inpSerNo2->value(val.c_str());
		inp_SS_SerialNoR1->value(val.c_str());
		inp_SS_SerialNoR2->value(val.c_str());
		inpRTU_serno1->value(val.c_str());
		inpRTU_serno2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_SS_Precedence1 || inp == inp_SS_Precedence2) {
		int p = inp->position();
		std::string val = inp->value();
		inp_SS_Precedence1->value(val.c_str());
		inp_SS_Precedence2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_SS_Check1 || inp == inp_SS_Check2) {
		int p = inp->position();
		std::string val = inp->value();
		inp_SS_Check1->value(val.c_str());
		inp_SS_Check2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_SS_Section1 || inp == inp_SS_Section2) {
		int p = inp->position();
		std::string val = inp->value();
		inp_SS_Section1->value(val.c_str());
		inp_SS_Section2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inpSPCnum_NAQP1 || inp == inpSPCnum_NAQP2) {
		int p = inp->position();
		std::string val = inp->value();
		inpSPCnum_NAQP1->value(val.c_str());
		inpSPCnum_NAQP2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inpSPCnum_AICW1 || inp == inpSPCnum_AICW2) { // Rx power
		int p = inp->position();
		std::string val = inp->value();
		inpSPCnum_AICW1->value(val.c_str());
		inpSPCnum_AICW2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_ARR_check1 || inp == inp_ARR_check2) {
		int p = inp->position();
		std::string val = inp->value();
		inp_ARR_check1->value(val.c_str());
		inp_ARR_check2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_1010_nr1 || inp == inp_1010_nr2) {
		int p = inp->position();
		std::string val = inp->value();
		inp_1010_nr1->value(val.c_str());
		inp_1010_nr2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_FD_class1 || inp == inp_FD_class2) {
		int p = inp->position();
		std::string str = ucasestr(inp->value());
		inp_FD_class1->value(str.c_str());
		inp_FD_class2->value(str.c_str());
		inp->position(p);
	}

	else if (inp == inp_ASCR_class1 || inp == inp_ASCR_class2) {
		int p = inp->position();
		std::string str = ucasestr(inp->value());
		inp_ASCR_class1->value(str.c_str());
		inp_ASCR_class2->value(str.c_str());
		inp->position(p);
	}

	else if (inp == inp_FD_section1 || inp == inp_FD_section2) {
		int p = inp->position();
		std::string str = ucasestr(inp->value());
		inp_FD_section1->value(str.c_str());
		inp_FD_section2->value(str.c_str());
		inp->position(p);
	}

	else if (inp == inp_KD_age1 || inp == inp_KD_age2) {
		int p = inp->position();
		std::string val = inp->value();
		inp_KD_age1->value(val.c_str());
		inp_KD_age2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_KD_state1 || inp == inp_KD_state2) {
		int p = inp->position();
		std::string str = ucasestr(inp->value());
		inp_KD_state1->value(str.c_str());
		inp_KD_state2->value(str.c_str());
		inp->position(p);
	}

	else if (inp == inp_KD_VEprov1 || inp == inp_KD_VEprov2) {
		int p = inp->position();
		std::string str = ucasestr(inp->value());
		inp_KD_VEprov1->value(str.c_str());
		inp_KD_VEprov2->value(str.c_str());
		inp->position(p);
	}

	else if (inp == inp_ser_NAS1 || inp == inp_ser_NAS2) {
		int p = inp->position();
		std::string val = inp->value();
		inp_ser_NAS1->value(val.c_str());
		inp_ser_NAS2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_JOTA_scout1 || inp == inp_JOTA_scout2) {
		int p = inp->position();
		std::string val = inp->value();
		inp_JOTA_scout1->value(val.c_str());
		inp_JOTA_scout2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_JOTA_troop1 || inp == inp_JOTA_troop2) {
		int p = inp->position();
		std::string val = inp->value();
		inp_JOTA_troop1->value(val.c_str());
		inp_JOTA_troop2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inp_JOTA_spc1 || inp == inp_JOTA_spc2) {
		int p = inp->position();
		std::string val = inp->value();
		inp_JOTA_spc1->value(val.c_str());
		inp_JOTA_spc2->value(val.c_str());
		inp->position(p);
	}

	else if (inp == inpState) {
		Cstates st;
		if (inpCounty->value()[0])
			cboCountyQSO->value(
				string(st.state_short(inpState->value())).append(" ").
				append(st.county(inpState->value(), inpCounty->value())).c_str());
		else
			cboCountyQSO->clear_entry();
		cboCountyQSO->redraw();
	}

	else if (inp == inpCounty) {
		Cstates st;
		if (inpState->value()[0])
			cboCountyQSO->value(
				string(st.state_short(inpState->value())).append(" ").
				append(st.county(inpState->value(), inpCounty->value())).c_str());
		else
			cboCountyQSO->clear_entry();
		cboCountyQSO->redraw();
	}

	if (progdefaults.EnableDupCheck || FD_logged_on) {
		DupCheck();
	}

	if (Fl::event() == FL_KEYBOARD) {
		int k = Fl::event_key();
		if (k == FL_Enter || k == FL_KP_Enter)
			restoreFocus(18);
	}
}

void cbClearCall(Fl_Widget *b, void *)
{
	clearQSO();
}

void qsoClear_cb(Fl_Widget *b, void *)
{
	bool CLEARLOG = true;
	if (progdefaults.NagMe && !oktoclear)
		CLEARLOG = (fl_choice2(_("Clear log fields?"), _("Cancel"), _("OK"), NULL) == 1);
	if (CLEARLOG) {
		clearQSO();
	}
	clear_Lookup();
	if (active_modem->get_mode() == MODE_IFKP)
		ifkp_clear_avatar();
	if (active_modem->get_mode() >= MODE_THOR11 &&
		active_modem->get_mode() <= MODE_THOR22)
		thor_clear_avatar();
	qsodb.isdirty(0);
}

extern cQsoDb	qsodb;

void qso_save_now()
{
//	if (!qsodb.isdirty()) return;
	string havecall = inpCall->value();
	string timeon   = inpTimeOn->value();

	while (!havecall.empty() && havecall[0] <= ' ') havecall.erase(0,1);
	while (!havecall.empty() && havecall[havecall.length() - 1] <= ' ') havecall.erase(havecall.length()-1, 1);

	if (havecall.empty())
		return;

	sDate_off = zdate();
	sTime_off = ztime();

	if (!timeon.empty())
		sTime_on = timeon.c_str();
	else
		sTime_on = sTime_off;

	submit_log();
	if (progdefaults.ClearOnSave)
		clearQSO();
}

void qsoSave_cb(Fl_Widget *b, void *)
{
	qso_save_now();
	ReceiveText->mark(FTextBase::XMIT);
	restoreFocus(20);
}

void cb_QRZ(Fl_Widget *b, void *)
{
	if (!*inpCall->value())
		return restoreFocus(21);

	switch (Fl::event_button()) {
	case FL_LEFT_MOUSE:
		CALLSIGNquery();
		oktoclear = false;
		break;
	case FL_RIGHT_MOUSE:
		if (quick_choice(string("Spot \"").append(inpCall->value()).append("\"?").c_str(),
				 2, _("Confirm"), _("Cancel"), NULL) == 1)
			spot_manual(inpCall->value(), inpLoc->value());
		break;
	default:
		break;
	}
	restoreFocus(22);
}

void status_cb(Fl_Widget *b, void *arg)
{
	if (Fl::event_button() == FL_RIGHT_MOUSE) {
		trx_mode md = active_modem->get_mode();
		if (md == MODE_FMT) open_config(TAB_FMT);
		else if (md == MODE_CW) open_config(TAB_CW);
		else if (md == MODE_IFKP) open_config(TAB_IFKP);
		else if (md == MODE_FSQ) open_config(TAB_FSQ);
		else if (md == MODE_RTTY) open_config(TAB_RTTY);
		else if (md >= MODE_THOR_FIRST && md <= MODE_THOR_LAST) open_config(TAB_THOR);
		else if (md >= MODE_OLIVIA_FIRST && md <= MODE_OLIVIA_LAST) open_config(TAB_OLIVIA);
		else if (md >= MODE_PSK_FIRST && md <= MODE_PSK_LAST) open_config(TAB_PSK);
		else if (md >= MODE_QPSK_FIRST && md <= MODE_QPSK_LAST) open_config(TAB_PSK);
		else if (md >= MODE_8PSK_FIRST && md <= MODE_8PSK_LAST) open_config(TAB_PSK);
		else if (md >= MODE_MT63_FIRST && md <= MODE_MT63_LAST) open_config(TAB_MT63);
		else if (md >= MODE_NAVTEX_FIRST && md <= MODE_NAVTEX_LAST) open_config(TAB_NAVTEX);
		else if (md >= MODE_WEFAX_FIRST && md <= MODE_WEFAX_LAST) open_config(TAB_WEFAX);
		else if (md >= MODE_HELL_FIRST && md <= MODE_HELL_LAST) open_config(TAB_FELDHELL);
		else if (md >= MODE_DOMINOEX_FIRST && md <= MODE_DOMINOEX_LAST) open_config(TAB_DOMINOEX);
		else if (md >= MODE_CONTESTIA_FIRST && md <= MODE_CONTESTIA_LAST) open_config(TAB_CONTESTIA);
	}
	else {
		if (!quick_change)
			return;
		const Fl_Menu_Item *m = quick_change->popup(Fl::event_x(), Fl::event_y());
		if (m && m->callback())
		m->do_callback(0);
	}
	static_cast<Fl_Button*>(b)->clear();
	restoreFocus(23);
}

void cbAFC(Fl_Widget *w, void *vi)
{
	Fl_Button *b = (Fl_Button *)w;
	int v = b->value();
	progStatus.afconoff = v;
}

void cbSQL(Fl_Widget *w, void *vi)
{
	Fl_Button *b = (Fl_Button *)w;
	int v = b->value();
	progStatus.sqlonoff = v ? true : false;
}

extern void set_wf_mode(void);

void cbPwrSQL(Fl_Widget *w, void *vi)
{
	Fl_Button *b = (Fl_Button *)w;
	int v = b->value();
	if(!v) {
		sldrSquelch->value(progStatus.sldrSquelchValue);
		progStatus.kpsql_enabled = false;
		progdefaults.kpsql_enabled = false;
		b->clear();
	} else {
		sldrSquelch->value(progStatus.sldrPwrSquelchValue);
		progStatus.kpsql_enabled = true;
		progdefaults.kpsql_enabled = true;
		set_wf_mode();
		b->set();
	}
}

void startMacroTimer()
{
	ENSURE_THREAD(FLMAIN_TID);

	btnMacroTimer->color(fl_rgb_color(240, 240, 0));
	btnMacroTimer->clear_output();
	Fl::add_timeout(0.0, macro_timer);
}

void stopMacroTimer()
{
	ENSURE_THREAD(FLMAIN_TID);

	progStatus.timer = 0;
	progStatus.repeatMacro = -1;
	local_timed_exec = false;
	Fl::remove_timeout(macro_timer);
	Fl::remove_timeout(macro_timed_execute);

	btnMacroTimer->label(0);
	btnMacroTimer->color(FL_BACKGROUND_COLOR);
	btnMacroTimer->set_output();
}

void macro_timer(void*)
{
	char buf[8];
	snprintf(buf, sizeof(buf), "%d", progStatus.timer);
	btnMacroTimer->copy_label(buf);

	if (progStatus.timer-- == 0) {
		stopMacroTimer();
		if (active_modem->get_mode() == MODE_IFKP) {
			ifkp_tx_text->clear();
		} else {
			TransmitText->clear();
		}
		macros.execute(progStatus.timerMacro);
	}
	else
		Fl::repeat_timeout(1.0, macro_timer);
}

static long mt_xdt, mt_xtm;

// called by main loop...ok to write to widget
void show_clock(bool yes)
{
	if (!yes) {
		StatusBar->label("");
		StatusBar->redraw();
		return;
	}
	static char s_clk_time[40];
	time_t sked_time = time(NULL);
	tm sked_tm;
	gmtime_r(&sked_time, &sked_tm);
	int hrs = sked_tm.tm_hour;
	int mins = sked_tm.tm_min;
	int secs = sked_tm.tm_sec;
	snprintf(s_clk_time, sizeof(s_clk_time), "%02d:%02d:%02d",
		hrs, mins, secs);
	StatusBar->label(s_clk_time);
	StatusBar->redraw();
}

static int timed_ptt = -1;
void macro_timed_execute(void *)
{
	long dt, tm;
	dt = atol(local_timed_exec ? ldate() : zdate());
	tm = atol(local_timed_exec ? ltime() : ztime());
	if (dt >= mt_xdt && tm >= mt_xtm) {
		show_clock(false);
		if (timed_ptt != 1) {
			push2talk->set(true);
			timed_ptt = 1;
		}
		macros.timed_execute();
		btnMacroTimer->label(0);
		btnMacroTimer->color(FL_BACKGROUND_COLOR);
		btnMacroTimer->set_output();
		mt_xdt = mt_xtm = 0;
	} else {
		show_clock(true);
		if (timed_ptt != 0) {
			push2talk->set(false);
			timed_ptt = 0;
		}
		Fl::repeat_timeout(1.0, macro_timed_execute);
	}
}

void startTimedExecute(std::string &title)
{
	ENSURE_THREAD(FLMAIN_TID);
	string txt = "Macro '";
	txt.append(title).
		append("' scheduled on ").
		append(exec_date.substr(0,4)).append("/").
		append(exec_date.substr(4,2)).append("/").
		append(exec_date.substr(6,2)).
		append(" at ").
		append(exec_time.substr(0,2)).append(":").
		append(exec_time.substr(2,2)).append(":").
		append(exec_time.substr(4,2)).
		append(local_timed_exec ? " Local" : " Zulu").
		append("\n");

	btnMacroTimer->label("SKED");
	btnMacroTimer->color(fl_rgb_color(240, 240, 0));
	btnMacroTimer->redraw_label();
	ReceiveText->clear();
	ReceiveText->addstr(txt, FTextBase::CTRL);
	mt_xdt = atol(exec_date.c_str());
	mt_xtm = atol(exec_time.c_str());
	Fl::add_timeout(0.0, macro_timed_execute);
}

void cbMacroTimerButton(Fl_Widget*, void*)
{
	stopMacroTimer();
	restoreFocus(24);
}

void cb_mvsquelch(Fl_Widget *w, void *d)
{
	progStatus.squelch_value = mvsquelch->value();

	if (active_modem->get_mode() == MODE_CW)
		progStatus.VIEWER_cwsquelch = progStatus.squelch_value;
	else if (active_modem->get_mode() == MODE_RTTY)
		progStatus.VIEWER_rttysquelch = progStatus.squelch_value;
	else
		progStatus.VIEWER_psksquelch = progStatus.squelch_value;

	if (sldrViewerSquelch)
		sldrViewerSquelch->value(progStatus.squelch_value);

}

void cb_btnClearMViewer(Fl_Widget *w, void *d)
{
	if (brwsViewer)
		brwsViewer->clear();
	mainViewer->clear();
	active_modem->clear_viewer();
}

int default_handler(int event)
{
	if (bWF_only) {
		if (Fl::event_key() == FL_Escape)
			return 1;
		return 0;
	}

	if (event != FL_SHORTCUT)
		return 0;

	if (RigViewerFrame && Fl::event_key() == FL_Escape &&
		RigViewerFrame->visible() && Fl::event_inside(RigViewerFrame)) {
		CloseQsoView();
		return 1;
	}

	Fl_Widget* w = Fl::focus();

	int key = Fl::event_key();
	if ((key == FL_F + 4) && Fl::event_alt()) clean_exit(true);

	if (fl_digi_main->contains(w)) {
		if (key == FL_Escape || (key >= FL_F && key <= FL_F_Last) ||
			((key == '1' || key == '2' || key == '3' || key == '4') && Fl::event_alt())) {
			TransmitText->take_focus();
			TransmitText->handle(FL_KEYBOARD);
			return 1;
		}
#ifdef __APPLE__
		if ((key == '=') && (Fl::event_state() == FL_COMMAND))
#else
		if (key == '=' && Fl::event_alt())
#endif
		{
			progdefaults.txlevel += 0.1;
			if (progdefaults.txlevel > 0) progdefaults.txlevel = 0;
			cntTxLevel->value(progdefaults.txlevel);
			return 1;
		}
#ifdef __APPLE__
		if ((key == '-') && (Fl::event_state() == FL_COMMAND))
#else
		if (key == '-' && Fl::event_alt())
#endif
		{
			progdefaults.txlevel -= 0.1;
			if (progdefaults.txlevel < -30) progdefaults.txlevel = -30;
			cntTxLevel->value(progdefaults.txlevel);
			return 1;
		}
	}
	else if (dlgLogbook->contains(w))
		return log_search_handler(event);

	else if (Fl::event_key() == FL_Escape)
		return 1;

	else if ( (fl_digi_main->contains(w) || dlgLogbook->contains(w)) &&
				Fl::event_ctrl() )
			return w->handle(FL_KEYBOARD);

	return 0;
}

int wo_default_handler(int event)
{
	if (event != FL_SHORTCUT)
		return 0;

	if (RigViewerFrame && Fl::event_key() == FL_Escape &&
		RigViewerFrame->visible() && Fl::event_inside(RigViewerFrame)) {
		CloseQsoView();
		return 1;
	}

	Fl_Widget* w = Fl::focus();
	int key = Fl::event_key();

	if ((key == FL_F + 4) && Fl::event_alt()) clean_exit(true);

	if (fl_digi_main->contains(w)) {
		if (key == FL_Escape || (key >= FL_F && key <= FL_F_Last) ||
			((key == '1' || key == '2' || key == '3' || key == '4') && Fl::event_alt())) {
			return 1;
		}
#ifdef __APPLE__
		if ((key == '=') && (Fl::event_state() == FL_COMMAND))
#else
		if (key == '=' && Fl::event_alt())
#endif
		{
			progdefaults.txlevel += 0.1;
			if (progdefaults.txlevel > 0) progdefaults.txlevel = 0;
			cntTxLevel->value(progdefaults.txlevel);
			return 1;
		}
#ifdef __APPLE__
		if ((key == '-') && (Fl::event_state() == FL_COMMAND))
#else
		if (key == '-' && Fl::event_alt())
#endif
		{
			progdefaults.txlevel -= 0.1;
			if (progdefaults.txlevel < -30) progdefaults.txlevel = -30;
			cntTxLevel->value(progdefaults.txlevel);
			return 1;
		}
	}

	else if (Fl::event_ctrl()) return w->handle(FL_KEYBOARD);

	return 0;
}

void save_on_exit() {
	if (progdefaults.changed && progdefaults.SaveConfig) {
		switch (fl_choice2(_("Save changed configuration?"), NULL, _("Yes"), _("No"))) {
			case 1:
				progdefaults.saveDefaults();
			default:
				break;
		}
	}
	if (macros.changed && progdefaults.SaveMacros) {
		switch (fl_choice2(_("Save changed macros?"), NULL, _("Yes"), _("No"))) {
			case 1:
				macros.writeMacroFile();
			default:
				break;
		}
	}
	if (!oktoclear && progdefaults.NagMe) {
		switch (fl_choice2(_("Save log entry?"), NULL, _("Yes"), _("No"))) {
			case 1:
				qsoSave_cb(0, 0);
			default:
				break;
		}
	}
	return;
}

bool first_use = false;

bool bEXITING = false;

bool clean_exit(bool ask) {
	if (ask && first_use) {
		switch(fl_choice2(_("Confirm Quit"), NULL, _("Yes"), _("No"))) {
			case 2:
				return false;
			default:
				break;
		}
		progdefaults.saveDefaults();
		macros.writeMacroFile();
		if (!oktoclear) {
			switch (fl_choice2(_("Save log entry?"), NULL, _("Yes"), _("No"))) {
				case 1:
					qsoSave_cb(0, 0);
				default:
					break;
			}
		}
	} else {
		if (ask &&
			progdefaults.confirmExit &&
			(!(progdefaults.changed && progdefaults.SaveConfig) ||
			 !(macros.changed && progdefaults.SaveMacros) ||
			 !(!oktoclear && progdefaults.NagMe))) {
			switch (fl_choice2(_("Confirm quit?"), NULL, _("Yes"), _("No"))) {
				case 1:
					break;
				default:
					return false;
			}
		}
		if (ask)
			save_on_exit();
	}

	bEXITING = true;

	if (Maillogfile)
		Maillogfile->log_to_file_stop();
	if (logfile)
		logfile->log_to_file_stop();

	saveFreqList();

	progStatus.saveLastState();

	if (scopeview) scopeview->hide();
	if (dlgViewer) dlgViewer->hide();
	if (dlgLogbook) dlgLogbook->hide();

	if (trx_state != STATE_RX) {
LOG_INFO("Disable TUNE");
		btnTune->labelcolor(FL_FOREGROUND_COLOR);
		Fl::flush();
		push2talk->set(0);
		set_flrig_ptt(0);
		trx_receive();
		MilliSleep(200);
	}

LOG_INFO("Disable PTT");
	delete push2talk;
#if USE_HAMLIB
LOG_INFO("Close hamlib");
	hamlib_close();
#endif
LOG_INFO("Close rigCAT");
	rigCAT_close();

LOG_INFO("Close ADIF i/o");
	ADIF_RW_close();

LOG_INFO("Close T/R processing");
	trx_close();

#if USE_HAMLIB
	if (xcvr) delete xcvr;
#endif

LOG_INFO("Close logbook");
	close_logbook();
	MilliSleep(50);

LOG_INFO("Stop flrig i/o");
	stop_flrig_thread();

LOG_INFO("Stop N3FJP logging");
	n3fjp_close();

LOG_INFO("Stop FMT process");
	FMT_thread_close();

LOG_INFO("Close WinKeyer i/o");
	WK_exit();

LOG_INFO("Stop TOD clock");
	TOD_close();

LOG_INFO("Delete audio_alert");
	delete audio_alert;

LOG_INFO("Exit_process");
	exit_process();


	if (field_day_viewer)
		if (field_day_viewer->visible())
			field_day_viewer->hide();

	if (dxcluster_viewer)
		if (dxcluster_viewer->visible())
			dxcluster_viewer->hide();

	save_counties();

	return true;
}

bool first_check = true;

void UI_check_swap()
{
	int mv_x = text_panel->x();
	int mv_y = text_panel->y();
	int mv_w = mvgroup->w() > 1 ? mvgroup->w() : text_panel->w() / 2;
	int mv_h = text_panel->h();

	int tx_y = 0, tx_h = 0, tx_x = 0, tx_w = 0;
	int rx_y = 0, rx_h = 0, rx_x = 0, rx_w = 0;

	if (progdefaults.rxtx_swap && (ReceiveText->y() <= TransmitText->y())) {
		tx_y = ReceiveText->y();
		tx_h = first_check ? progStatus.tile_y : TransmitText->h();
		tx_x = mv_x + mv_w;
		tx_w = text_panel->w() - mv_w;

		rx_y = tx_y + tx_h;
		rx_h = mv_h - tx_h;
		rx_x = tx_x;
		rx_w = tx_w;

		text_panel->remove(minbox);
		text_panel->remove(TransmitText);
		text_panel->remove(FHdisp);
		text_panel->remove(ReceiveText);
		text_panel->remove(mvgroup);

		mvgroup->resize(mv_x, mv_y, mv_w, mv_h);
		TransmitText->resize(tx_x, tx_y, tx_w, tx_h);
		ReceiveText->resize(rx_x, rx_y, rx_w, rx_h);
		FHdisp->resize(rx_x, rx_y, rx_w, rx_h);
		minbox->resize(
				text_panel->x(),
				text_panel->y() + minhtext,
				text_panel->w() - 100,
				text_panel->h() - 2*minhtext);

		text_panel->add(mvgroup);
		text_panel->add(TransmitText);
		text_panel->add(ReceiveText);
		text_panel->add(FHdisp);
		text_panel->add(minbox);
		text_panel->resizable(minbox);
		progStatus.tile_y = TransmitText->h();
		progStatus.tile_y_ratio = 1.0 * TransmitText->h() / text_panel->h();
	}
	else if (!progdefaults.rxtx_swap && ReceiveText->y() > TransmitText->y()) {
		rx_y = TransmitText->y();
		rx_h = first_check ? progStatus.tile_y : ReceiveText->h();
		rx_x = mv_x + mv_w;
		rx_w = text_panel->w() - mv_w;

		tx_y = rx_y + rx_h;
		tx_h = mv_h - rx_h;
		tx_x = rx_x;
		tx_w = rx_w;

		text_panel->remove(minbox);
		text_panel->remove(TransmitText);
		text_panel->remove(FHdisp);
		text_panel->remove(ReceiveText);
		text_panel->remove(mvgroup);

		mvgroup->resize(mv_x, mv_y, mv_w, mv_h);
		TransmitText->resize(tx_x, tx_y, tx_w, tx_h);
		ReceiveText->resize(rx_x, rx_y, rx_w, rx_h);
		FHdisp->resize(rx_x, rx_y, rx_w, rx_h);
		minbox->resize(
			text_panel->x(),
			text_panel->y() + minhtext,
			text_panel->w() - 100,
			text_panel->h() - 2*minhtext);

		text_panel->add(mvgroup);
		text_panel->add(ReceiveText);
		text_panel->add(FHdisp);
		text_panel->add(TransmitText);
		text_panel->add(minbox);
		text_panel->resizable(minbox);
		progStatus.tile_y = ReceiveText->h();
		progStatus.tile_y_ratio = 1.0 * ReceiveText->h() / text_panel->h();
	}

// resize fsq UI
	int fsq_rx_h = text_panel->h() * progStatus.fsq_ratio;
	if (fsq_rx_h < minhtext) fsq_rx_h = minhtext;
	int fsq_tx_h = text_panel->h() - fsq_rx_h;
	if (fsq_tx_h < minhtext) {
		fsq_tx_h = minhtext;
		fsq_rx_h = text_panel->h() - fsq_tx_h;
	}

	fsq_left->remove(fsq_minbox);
	fsq_left->remove(fsq_rx_text);
	fsq_left->remove(fsq_tx_text);

	fsq_rx_text->resize(fsq_left->x(), fsq_left->y(), fsq_left->w(), fsq_rx_h);
	fsq_tx_text->resize(fsq_left->x(), fsq_left->y() + fsq_rx_text->h(), fsq_left->w(), fsq_tx_h);
	fsq_minbox->resize(
			text_panel->x(),
			text_panel->y() + minhtext,
			text_panel->w() - 100,
			text_panel->h() - 2*minhtext);

	fsq_left->add(fsq_rx_text);
	fsq_left->add(fsq_tx_text);
	fsq_left->add(fsq_minbox);
	fsq_left->resizable(fsq_minbox);

// resize IFKP UI
	int ifkp_rx_h = text_panel->h() * progStatus.ifkp_ratio;
	if (ifkp_rx_h < minhtext) ifkp_rx_h = minhtext;
	int ifkp_tx_h = text_panel->h() - ifkp_rx_h;
	if (ifkp_tx_h < minhtext) {
		ifkp_tx_h = minhtext;
		ifkp_rx_h = text_panel->h() - ifkp_tx_h;
	}

	ifkp_left->remove(ifkp_minbox);
	ifkp_left->remove(ifkp_rx_text);
	ifkp_left->remove(ifkp_tx_text);

	ifkp_rx_text->resize(
		ifkp_left->x(), ifkp_left->y(),
		ifkp_left->w(), ifkp_rx_h);
	ifkp_tx_text->resize(
		ifkp_left->x(), ifkp_left->y() + ifkp_rx_text->h(),
		ifkp_left->w(), ifkp_tx_h);
	ifkp_minbox->resize(
		text_panel->x(),
		text_panel->y() + minhtext,
		text_panel->w() - 100,
		text_panel->h() - 2*minhtext);

	ifkp_left->add(ifkp_rx_text);
	ifkp_left->add(ifkp_tx_text);
	ifkp_left->add(ifkp_minbox);
	ifkp_left->resizable(ifkp_minbox);

	first_check = false;
}

static bool restore_minimize = false;

void UI_select_central_frame(int y, int ht)
{
	text_panel->resize(0, y, fl_digi_main->w(), ht);
	center_group->init_sizes();
}

void resize_macroframe_1(int x, int y, int w, int h)
{
	macroFrame1->resize(x, y, w, h);
	macroFrame1->init_sizes();
	macroFrame1->redraw();
}

void resize_macroframe_2(int x, int y, int w, int h)
{
	macroFrame2->resize(x, y, w, h);
	macroFrame2->init_sizes();
	macroFrame2->redraw();
}

void UI_position_macros(int x, int y1, int w, int HTh)
{
	int mh = progdefaults.macro_height;

	if (progdefaults.display_48macros) {
		macroFrame2->hide();
		macroFrame1->hide();
		if (!progdefaults.four_bar_position) {
			tbar->resize(x, y1, w, 4 * TB_HEIGHT);
			tbar->show();
			y1 += tbar->h();
			HTh -= tbar->h();
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);
			y1 += HTh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			status_group->position(x, y1);
		} else {
			int htbar = 4 * TB_HEIGHT;

			HTh -= htbar;

			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);

			y1 += HTh;

			tbar->resize(x, y1, w, htbar);
			tbar->show();

			y1 += htbar;

			wf_group->position(x, y1);
			y1 += wf_group->h();
			status_group->position(x, y1);
		}
		fl_digi_main->init_sizes();
		return;
	}

	tbar->hide();
	switch (progdefaults.mbar_scheme) { // 0, 1, 2 one bar schema
		case 0:
			resize_macroframe_2(x,y1,w,mh);
			macroFrame2->hide();
			btnAltMacros2->deactivate();
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->activate();
			y1 += mh;
			HTh -= mh;
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);
			y1 += HTh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			status_group->position(x, y1);
			break;
		default:
		case 1:
			resize_macroframe_2(x,y1,w,mh);
			macroFrame2->hide();
			btnAltMacros2->deactivate();
			HTh -= mh;
			center_group->resize(x, y1, w, HTh);
//			text_panel->resize(x, y1, w, HTh);
//			wefax_group->resize(x, y1, w, HTh);
//			fsq_group->resize(x, y1, w, HTh);
//			ifkp_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);
			y1 += HTh;
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->activate();
			y1 += mh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			status_group->position(x, y1);
			break;
		case 2:
			resize_macroframe_2(x,y1,w,mh);
			macroFrame2->hide();
			btnAltMacros2->deactivate();
			HTh -= mh;
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);
			y1 += HTh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->activate();
			y1 += mh;
			status_group->position(x, y1);
			break;
		case 3:
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->deactivate();
			y1 += mh;
			HTh -= mh;
			resize_macroframe_2(x, y1, w, mh);
			macroFrame2->show();
			btnAltMacros2->activate();
			y1 += mh;
			HTh -= mh;
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);
			y1 += HTh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			status_group->position(x, y1);
			break;
		case 4:
			resize_macroframe_2(x, y1, w, mh);
			macroFrame2->show();
			btnAltMacros2->activate();
			y1 += mh;
			HTh -= mh;
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->deactivate();
			y1 += mh;
			HTh -= mh;
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);
			y1 += HTh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			status_group->position(x, y1);
			break;
		case 5:
			HTh -= 2*mh;
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);
			y1 += HTh;
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->deactivate();
			y1 += mh;
			resize_macroframe_2(x, y1, w, mh);
			macroFrame2->show();
			btnAltMacros2->activate();
			y1 += mh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			status_group->position(x, y1);
			break;
		case 6:
			HTh -= 2*mh;
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			y1 += HTh;
			resize_macroframe_2(x, y1, w, mh);
			macroFrame2->show();
			btnAltMacros2->activate();
			y1 += mh;
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->deactivate();
			y1 += mh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			status_group->position(x, y1);
			break;
		case 7:
			HTh -= 2*mh;
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);
			y1 += HTh;
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->deactivate();
			y1 += mh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			resize_macroframe_2(x, y1, w, mh);
			macroFrame2->show();
			y1 += mh;
			status_group->position(x, y1);
			break;
		case 8:
			HTh -= 2*mh;
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			y1 += HTh;
			resize_macroframe_2(x, y1, w, mh);
			macroFrame2->show();
			y1 += mh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->deactivate();
			y1 += mh;
			status_group->position(x, y1);
			break;
		case 9:
			HTh -= 2*mh;
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);
			y1 += HTh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->deactivate();
			y1 += mh;
			resize_macroframe_2(x, y1, w, mh);
			macroFrame2->show();
			btnAltMacros2->activate();
			y1 += mh;
			status_group->position(x, y1);
			break;
		case 10:
			HTh -= 2*mh;
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);
			y1 += HTh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			resize_macroframe_2(x, y1, w, mh);
			macroFrame2->show();
			btnAltMacros2->activate();
			y1 += mh;
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->deactivate();
			y1 += mh;
			status_group->position(x, y1);
			break;
		case 11:
			resize_macroframe_2(x, y1, w, mh);
			macroFrame2->show();
			btnAltMacros2->activate();
			y1 += mh;
			HTh -= 2*mh;
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);
			y1 += HTh;
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->deactivate();
			y1 += mh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			status_group->position(x, y1);
			break;
		case 12:
			resize_macroframe_1(x, y1, w, mh);
			macroFrame1->show();
			btnAltMacros1->deactivate();
			y1 += mh;
			HTh -= 2*mh;
			center_group->resize(x, y1, w, HTh);
			text_panel->resize(x, y1, w, HTh);
			wefax_group->resize(x, y1, w, HTh);
			fsq_group->resize(x, y1, w, HTh);
			ifkp_group->resize(x, y1, w, HTh);
			fmt_group->resize(x, y1, w, HTh);
			UI_select_central_frame(y1, HTh);
			y1 += HTh;
			resize_macroframe_2(x, y1, w, mh);
			macroFrame2->show();
			btnAltMacros2->activate();
			y1 += mh;
			wf_group->position(x, y1);
			y1 += wf_group->h();
			status_group->position(x, y1);
			break;
	}
	fl_digi_main->init_sizes();
	return;
}

bool UI_first = true;
void UI_select()
{
	if (bWF_only) {
		int Y = cntTxLevel->y();
		int psm_width = progdefaults.show_psm_btn ? bwSqlOnOff : 0;
		int X = rightof(Status2);
		int W = fl_digi_main->w() - X - bwTxLevel - Wwarn - bwAfcOnOff -
			bwSqlOnOff - psm_width;

		StatusBar->resize( X, Y, W, StatusBar->h());
		VuMeter->resize( X, Y, W, VuMeter->h());

		cntTxLevel->position(rightof(VuMeter), Y);

		WARNstatus->position(rightof(cntTxLevel), Y);
		btnAFC->position(rightof(WARNstatus), Y);
		btnSQL->position(rightof(btnAFC), Y);
		btnPSQL->resize(rightof(btnSQL), Y, psm_width, btnPSQL->h());

		if (progdefaults.show_psm_btn)
			btnPSQL->show();
		else
			btnPSQL->hide();

		cntTxLevel->redraw();
		WARNstatus->redraw();
		btnAFC->redraw();
		btnSQL->redraw();
		btnPSQL->redraw();
		StatusBar->redraw();

		status_group->init_sizes();
		status_group->redraw();

		fl_digi_main->init_sizes();
		fl_digi_main->redraw();
		return;
	}

	int x =   0;
	int y1 =  Hmenu;
	int w =   fl_digi_main->w();
	int HTh = fl_digi_main->h() - y1;

	if (cnt_macro_height) {
		cnt_macro_height->minimum(MACROBAR_MIN);
		cnt_macro_height->maximum(MACROBAR_MAX);
		cnt_macro_height->step(1);
		if (progdefaults.macro_height < MACROBAR_MIN) progdefaults.macro_height = MACROBAR_MIN;
		if (progdefaults.macro_height > MACROBAR_MAX) progdefaults.macro_height = MACROBAR_MAX;
		cnt_macro_height->value(progdefaults.macro_height);
	}

	HTh -= wf_group->h();
	HTh -= status_group->h();

	if (progStatus.NO_RIGLOG && !restore_minimize) {
		TopFrame1->hide();
		TopFrame2->hide();
		TopFrame3->hide();
		Status2->hide();
		inpCall4->show();
		inpCall = inpCall4;
		UI_position_macros(x, y1, w, HTh);
		goto UI_return;
	}

	if (!progStatus.Rig_Log_UI || restore_minimize) {
		TopFrame1->resize( x, y1, w, Hqsoframe );
		y1 += (TopFrame1->h());
		HTh -= (TopFrame1->h());
		UI_position_macros(x, y1, w, HTh);
		TopFrame2->hide();
		TopFrame3->hide();
		TopFrame1->show();
		inpFreq = inpFreq1;
		inpCall = inpCall1;
		inpTimeOn = inpTimeOn1;
		inpTimeOff = inpTimeOff1;
		inpName = inpName1;
		inpRstIn = inpRstIn1;
		inpRstOut = inpRstOut1;
		inpSerNo = inpSerNo1;
		outSerNo = outSerNo1;
		inpXchgIn = inpXchgIn1;
		inpState = inpState1;
		inpLoc = inpLoc1;
		inpQTH = inpQth;
		inp_JOTA_scout = inp_JOTA_scout1;
		inp_JOTA_troop = inp_JOTA_troop1;
		cboCountry = cboCountryQSO;

		gGEN_QSO_1->hide();
		gGEN_CONTEST->hide();
		gCQWW_RTTY->hide();
		gCQWW_DX->hide();
		gFD->hide();
		gCWSS->hide();
		gKD_1->hide();
		gARR->hide();
		g1010->hide();
		gVHF->hide();
		gASCR->hide();
		gNAQP->hide();
		gARRL_RTTY->hide();
		gIARI->hide();
		gNAS->hide();
		gAIDX->hide();
		gJOTA->hide();
		gAICW->hide();
		gSQSO->hide();
		gCQWPX->hide();
		gWAE->hide();

		switch (progdefaults.logging) {
			case LOG_FD:
				inpClass = inp_FD_class1;
				inpSection = inp_FD_section1;
				gFD->show();
				break;
			case LOG_WFD:
				inpClass = inp_FD_class1;
				inpSection = inp_FD_section1;
				gFD->show();
				break;
			case LOG_KD:
				inp_KD_age = inp_KD_age1;
				inpState = inp_KD_state1;
				inpVEprov = inp_KD_VEprov1;
				inpXchgIn = inp_KD_XchgIn1;
				gKD_1->show();
				break;
			case LOG_ARR:
				inpCall = inpCall1;
				inpName = inpName;
				inp_ARR_check = inp_ARR_check1;
				inpXchgIn = inp_ARR_XchgIn1;
				gARR->show();
				break;
			case LOG_1010:
				inp_1010_nr = inp_1010_nr1;
				inpXchgIn = inp_1010_XchgIn1;
				g1010->show();
				break;
			case LOG_VHF:
				inpRstIn = inp_vhf_RSTin1;
				inpRstOut = inp_vhf_RSTout1;
				inpLoc = inp_vhf_Loc1;
				inp_vhf_Loc1->show();
				inp_vhf_RSTin1->show();
				inp_vhf_RSTout1->show();
				gVHF->show();
				break;
			case LOG_CQ_WPX:
				inpSerNo = inpSerNo_WPX1;
				outSerNo = outSerNo_WPX1;
				inpSerNo_WPX1->show();
				outSerNo_WPX1->show();
				gCQWPX->show();
				break;
			case LOG_CQWW_DX:
				cboCountry = cboCountryQSO;
				inp_CQzone = inp_CQDXzone1;
				inp_CQDXzone1->show();
				gCQWW_DX->show();
				break;
			case LOG_CQWW_RTTY:
				inpState = inp_CQstate = inp_CQstate1;
				cboCountry = cboCountryQSO;
				inp_CQzone = inp_CQzone1;
				gCQWW_RTTY->show();
				break;
			case LOG_CWSS:
				outSerNo = outSerNo3;
				inpSerNo = inp_SS_SerialNoR1;
				inp_SS_SerialNoR = inp_SS_SerialNoR1;
				inp_SS_Check = inp_SS_Check1;
				inp_SS_Precedence = inp_SS_Precedence1;
				inp_SS_Section = inp_SS_Section1;
				gCWSS->show();
				break;
			case LOG_ASCR:
				inpClass = inp_ASCR_class1;
				inpXchgIn = inp_ASCR_XchgIn1;
				inp_ASCR_class1->show();
				inp_ASCR_XchgIn1->show();
				gASCR->show();
				break;
			case LOG_IARI:
				inpXchgIn = inp_IARI_PR1;
				cboCountry = cboCountryQSO;
				inp_IARI_PR1->show();
				gIARI->show();
				break;
			case LOG_NAQP:
				inpSPCnum = inpSPCnum_NAQP1;
				inpSPCnum_NAQP1->show();
				gNAQP->show();
				break;
			case LOG_RTTY:
				inpState = inpRTU_stpr1;
				inpSerNo = inpRTU_serno1;
				cboCountry = cboCountryQSO;
				gARRL_RTTY->show();
				break;
			case LOG_NAS:
				inpSerNo = inp_ser_NAS1;
				inpXchgIn = inpSPCnum_NAS1;
				outSerNo5->show();
				inp_ser_NAS1->show();
				inpSPCnum_NAS1->show();
				gNAS->show();
				break;
			case LOG_AIDX:
				outSerNo = outSerNo7;
				inpSerNo = inpSerNo3;
				cboCountry = cboCountryAIDX = cboCountryQSO;
				outSerNo7->show();
				inpSerNo3->show();
				gAIDX->show();
				break;
			case LOG_JOTA:
				inp_JOTA_scout = inp_JOTA_scout1;
				inp_JOTA_troop = inp_JOTA_troop1;
				inpXchgIn = inp_JOTA_spc1;
				inp_JOTA_scout1->show();
				inp_JOTA_spc1->show();
				inp_JOTA_troop1->show();
				gJOTA->show();
				break;
			case LOG_AICW:
				inpSPCnum = inpSPCnum_AICW1;
				cboCountry = cboCountryQSO;//cboCountryAICW1;
				inpSPCnum_AICW1->show();
				gAICW->show();
				break;
			case LOG_SQSO:
				inpRstIn = inpRstIn1;
				inpRstOut = inpRstOut1;
				inpCounty = inpSQSO_county1;
				inpSQSO_county1->show();
				outSerNo = outSQSO_serno1;
				outSQSO_serno1->show();
				inpSerNo = inpSQSO_serno1;
				inpSQSO_serno1->show();
				inpState = inpSQSO_state1;
				inpSQSO_state1->show();
				if (progdefaults.SQSOlogcat) {
					inpSQSO_category1->show();
					inpSQSO_category = inpSQSO_category1;
				} else {
					inpSQSO_category1->hide();
				}
				gSQSO->show();
				break;
//			case LOG_WAE:
//				inpSerNo = inpSerNo_WAE1;
//				inpSerNo_WAE1->show();
//				outSerNo = outSerNo_WAE1;
//				outSerNo_WAE1->show();
//				cboCountry = cboCountryWAE1;
//				cboCountryWAE1->show();
//				gWAE->show();
//				break;
			case LOG_BART:
			case LOG_GENERIC:
				gGEN_CONTEST->show();
				break;
		default: // no contest
				gGEN_QSO_1->show();
		}

		gGEN_QSO_1->redraw();
		gGEN_CONTEST->redraw();
		gCQWW_RTTY->redraw();
		gCQWW_DX->redraw();
		gFD->redraw();
		gCWSS->redraw();
		gKD_1->redraw();
		gARR->redraw();
		g1010->redraw();
		gVHF->redraw();
		gIARI->redraw();
		gAICW->redraw();
		gSQSO->redraw();
		gCQWPX->redraw();
		gWAE->redraw();

		qsoFreqDisp = qsoFreqDisp1;
		TopFrame1->init_sizes();

		goto UI_return;
	}
	else {
		if (progdefaults.logging == LOG_QSO) { // no contest
			TopFrame2->resize( x, y1, w, Hentry + 2 * pad);
			y1 += TopFrame2->h();
			HTh -= TopFrame2->h();
			UI_position_macros(x, y1, w, HTh);
			TopFrame1->hide();
			TopFrame3->hide();
			TopFrame2->show();
			inpCall = inpCall2;
			inpTimeOn = inpTimeOn2;
			inpTimeOff = inpTimeOff2;
			inpName = inpName2;
			inpSerNo = inpSerNo1;
			outSerNo = outSerNo1;
			inpRstIn = inpRstIn2;
			inpRstOut = inpRstOut2;
			inpState = inpState1;
			inpLoc = inpLoc1;
			inpQTH = inpQth;
			qsoFreqDisp = qsoFreqDisp2;
			inpCall4->hide();
			Status2->show();
			goto UI_return;
		}

		TopFrame3->resize( x, y1, w, Hentry + 2 * pad);
		y1 += TopFrame3->h();
		HTh -= TopFrame3->h();
		UI_position_macros(x, y1, w, HTh);
		TopFrame1->hide();
		TopFrame2->hide();
		TopFrame3->show();
		inpCall = inpCall3;
		inpTimeOn = inpTimeOn3;
		inpTimeOff = inpTimeOff3;

		inpSerNo = inpSerNo2;
		outSerNo = outSerNo2;
		inpXchgIn = inpXchgIn2;

//		inpSQSO_category2->hide();

		log_generic_frame->hide();
		log_fd_frame->hide();
		log_kd_frame->hide();
		log_1010_frame->hide();
		log_arr_frame->hide();
		log_vhf_frame->hide();
		log_cqww_frame->hide();
		log_cqww_rtty_frame->hide();
		log_cqss_frame->hide();
		log_cqwpx_frame->hide();
		log_ascr_frame->hide();
		log_naqp_frame->hide();
		log_rtty_frame->hide();
		log_iari_frame->hide();
		log_nas_frame->hide();
		log_aidx_frame->hide();
		log_jota_frame->hide();
		log_aicw_frame->hide();
		log_sqso_frame->hide();
		log_wae_frame->hide();

		switch (progdefaults.logging) {
			case LOG_QSO:
				log_generic_frame->show();
				break;
			case LOG_FD:
				log_fd_frame->show();
				inpClass = inp_FD_class2;
				inpSection = inp_FD_section2;
				break;
			case LOG_WFD:
				log_fd_frame->show();
				inpClass = inp_FD_class2;
				inpSection = inp_FD_section2;
				break;
			case LOG_KD:
				log_kd_frame->show();
				inpName = inp_KD_name2;
				inp_KD_age = inp_KD_age2;
				inpState = inp_KD_state2;
				inpVEprov = inp_KD_VEprov2;
				inpXchgIn = inp_KD_XchgIn2;
				break;
			case LOG_ARR:
				log_arr_frame->show();
				inpCall = inpCall3;
				inpName = inp_ARR_Name2;
				inp_ARR_check = inp_ARR_check2;
				inpXchgIn = inp_ARR_XchgIn2;
				break;
			case LOG_1010:
				log_1010_frame->show();
				inpName = inp_1010_name2;
				inp_1010_nr = inp_1010_nr1;
				inpXchgIn = inp_1010_XchgIn2;
				break;
			case LOG_VHF:
				log_vhf_frame->show();
				inpRstIn = inp_vhf_RSTin2;
				inpRstOut = inp_vhf_RSTout2;
				inpLoc = inp_vhf_Loc2;
				break;
			case LOG_CQWW_DX:
				log_cqww_frame->show();
				inpRstIn = inp_CQDX_RSTin2;
				inpRstOut = inp_CQDX_RSTout2;
				inp_CQzone = inp_CQDXzone2;
				cboCountry = cboCountryCQDX2;
				break;
			case LOG_CQWW_RTTY:
				log_cqww_rtty_frame->show();
				inpRstIn = inp_CQ_RSTin2;
				inpRstOut = inp_CQ_RSTout2;
				inpState = inp_CQstate = inp_CQstate2;
				inp_CQzone = inp_CQzone2;
				cboCountry = cboCountryCQ2;
				break;
			case LOG_CWSS:
				log_cqss_frame->show();
				outSerNo = outSerNo4;
				inpSerNo = inp_SS_SerialNoR2;
				inpTimeOff = inpTimeOff3;
				inpTimeOn = inpTimeOn3;
				inp_SS_Check = inp_SS_Check2;
				inp_SS_Precedence = inp_SS_Precedence2;
				inp_SS_Section = inp_SS_Section2;
				inp_SS_SerialNoR = inp_SS_SerialNoR2;
				break;
			case LOG_ASCR:
				log_ascr_frame->show();
				inpName = inp_ASCR_name2;
				inpRstIn = inp_ASCR_RSTin2;
				inpRstOut = inp_ASCR_RSTout2;
				inpClass = inp_ASCR_class2;
				inpXchgIn = inp_ASCR_XchgIn2;
				break;
			case LOG_IARI:
				log_iari_frame->show();
				inpRstIn = inp_IARI_RSTin2;
				inpRstOut = inp_IARI_RSTout2;
				inpSerNo = inp_IARI_SerNo2;
				outSerNo = out_IARI_SerNo2;
				inpXchgIn = inp_IARI_PR2;
				cboCountry = cboCountryIARI2;
				break;
			case LOG_NAQP:
				log_naqp_frame->show();
				inpName = inpNAQPname2;
				inpSPCnum = inpSPCnum_NAQP2;
				break;
			case LOG_RTTY:
				log_rtty_frame->show();
				inpState = inpRTU_stpr2;
				inpRstIn = inpRTU_RSTin2;
				inpRstOut = inpRTU_RSTout2;
				inpSerNo = inpRTU_serno2;
				cboCountry = cboCountryRTU2;
				break;
			case LOG_AIDX:
				log_aidx_frame->show();
				inpRstIn = inpRstIn3;
				inpRstOut = inpRstOut3;
				outSerNo = outSerNo8;
				inpSerNo = inpSerNo4;
				cboCountry = cboCountryAIDX = cboCountryAIDX2;
				break;
			case LOG_JOTA:
				log_jota_frame->show();
				inpRstIn = inpRstIn4;
				inpRstOut = inpRstOut4;
				inp_JOTA_scout = inp_JOTA_scout2;
				inp_JOTA_troop = inp_JOTA_troop2;
				inpXchgIn = inp_JOTA_spc2;
				break;
			case LOG_AICW:
				log_aicw_frame->show();
				inpRstIn = inpRstIn_AICW2;
				inpRstOut = inpRstOut_AICW2;
				cboCountry = cboCountryAICW2;
				inpSPCnum = inpSPCnum_AICW2;
				break;
			case LOG_SQSO:
				inpSQSO_category2->hide();
				inpSQSO_name2->hide();
				inpSQSO_serno2->hide();
				inpRstOut_SQSO2->hide();
				inpRstIn_SQSO2->hide();

				inpState = inpSQSO_state2;
				inpSQSO_state2->show();
				inpCounty = inpSQSO_county2;
				inpSQSO_county2->show();
				if (progdefaults.SQSOlogcat) {
					inpSQSO_category2->show();
					inpSQSO_category2->redraw();
					inpSQSO_category = inpSQSO_category2;
				}
				if (progdefaults.SQSOlogrst) {
					inpRstIn = inpRstIn_SQSO2;
					inpRstIn_SQSO2->show();
					inpRstOut = inpRstOut_SQSO2;
					inpRstOut_SQSO2->show();
				}
				if (progdefaults.SQSOlogserno) {
					inpSerNo = inpSQSO_serno2;
					inpSQSO_serno2->show();
					outSerNo = outSQSO_serno2;
					outSQSO_serno2->show();
				}
				if (progdefaults.SQSOlogname) {
					inpName = inpSQSO_name2;
					inpSQSO_name2->show();
				}
				inpSQSO_category2->redraw();
				inpSQSO_name2->redraw();
				inpSQSO_serno2->redraw();
				inpRstOut_SQSO2->redraw();
				inpRstIn_SQSO2->redraw();
				inpSQSO_state2->redraw();
				inpSQSO_county2->redraw();

				log_sqso_frame->show();
				break;
			case LOG_NAS:
				inpSerNo = inp_ser_NAS2;
				inpXchgIn = inpSPCnum_NAS2;
				inpName = inp_name_NAS2;
				log_nas_frame->show();
				break;
			case LOG_CQ_WPX:
				log_cqwpx_frame->show();
				inpSerNo = inpSerNo_WPX2;
				outSerNo = outSerNo_WPX2;
				inpRstIn = inpRstIn_WPX2;
				inpRstOut = inpRstOut_WPX2;
				break;
//			case LOG_WAE:
//				log_wae_frame->show();
//				inpSerNo = inpSerNo_WAE2;
//				outSerNo = outSerNo_WAE2;
//				inpRstIn = inpRstIn_WAE2;
//				inpRstOut = inpRstOut_WAE2;
//				cboCountry = cboCountryWAE2;
//				break;
			case LOG_BART:
			case LOG_GENERIC:
			default:
				log_generic_frame->show();
				inpTimeOn = inpTimeOn3;
				inpTimeOff = inpTimeOff3;
				inpSerNo = inpSerNo2;
				inpXchgIn = inpXchgIn2;
				break;
		}

		qsoFreqDisp = qsoFreqDisp3;
		TopFrame3->redraw();
		inpCall4->hide();
		Status2->show();
		goto UI_return;
	}

UI_return:
	UI_check_swap();

	if (UI_first) {
		UI_first = false;
	 }
	else {
		int orgx = text_panel->orgx();
		int orgy = text_panel->orgy();
		int nux = text_panel->x() + progStatus.tile_x;
		int nuy = text_panel->y() + progStatus.tile_y_ratio * text_panel->h();

		text_panel->position(orgx, orgy, nux, nuy);
	}

	{
		int Y = status_group->y();
		int psm_width = progdefaults.show_psm_btn ? bwSqlOnOff : 0;

		int vuw = 
			fl_digi_main->w() - Status2->x() - Status2->w() - 2 -
			bwTxLevel -  // tx level control
			Wwarn -      // Warn indicator
			bwAfcOnOff - // afc button
			bwSqlOnOff - // sql button
			psm_width -  // psm button, bwSqlOnOff / 0
			corner_box->w();

		StatusBar->resize( 
			Status2->x() + Status2->w() + 2, Y, vuw, StatusBar->h());

		VuMeter->resize(
			Status2->x() + Status2->w() + 2, Y, vuw, VuMeter->h());

		cntTxLevel->position(rightof(VuMeter), Y);

		WARNstatus->position(rightof(cntTxLevel), Y);
		btnAFC->position(rightof(WARNstatus), Y);
		btnSQL->position(rightof(btnAFC), Y);

		btnPSQL->resize(rightof(btnSQL), Y, psm_width, btnPSQL->h());
		if (progdefaults.show_psm_btn)
			btnPSQL->show();
		else
			btnPSQL->hide();

		status_group->init_sizes();
		status_group->redraw();

	}

	RigControlFrame->init_sizes();
	RigControlFrame->redraw();
	smeter->redraw();
	pwrmeter->redraw();

	center_group->redraw();
	text_panel->redraw();
	wefax_group->redraw();
	fsq_group->redraw();
	ifkp_group->redraw();
	fmt_group->redraw();
	macroFrame1->redraw();
	macroFrame2->redraw();
	viewer_redraw();

	fl_digi_main->init_sizes();
	update_main_title();
	LOGBOOK_colors_font();
	fl_digi_main->redraw();

Fl::flush();
}


void cb_mnu_wf_all(Fl_Menu_* w, void *d)
{
	wf->UI_select(progStatus.WF_UI = w->mvalue()->value());
}

void cb_mnu_riglog_all(Fl_Menu_* w, void *d)
{
	getMenuItem(w->mvalue()->label())->setonly();
	progStatus.Rig_Log_UI = false;
	progStatus.NO_RIGLOG = false;

	UI_select();
}

void cb_mnu_riglog_partial(Fl_Menu_* w, void *d)
{
	getMenuItem(w->mvalue()->label())->setonly();
	progStatus.Rig_Log_UI = true;
	progStatus.NO_RIGLOG = false;

	UI_select();
}

void cb_mnu_riglog_none(Fl_Menu_* w, void *d)
{
	getMenuItem(w->mvalue()->label())->setonly();
	progStatus.NO_RIGLOG = true;
	progStatus.Rig_Log_UI = false;

	UI_select();
}

void cb_mnuDockedscope(Fl_Menu_ *w, void *d)
{
	wf->show_scope(progStatus.DOCKEDSCOPE = w->mvalue()->value());
}

void WF_UI()
{
	wf->UI_select(progStatus.WF_UI);
}

void toggle_smeter()
{
	if (progStatus.meters && !smeter->visible()) {
		pwrmeter->hide();
		smeter->show();
		qso_combos->hide();
	} else if (!progStatus.meters && smeter->visible()) {
		pwrmeter->hide();
		smeter->hide();
		qso_combos->show();
	}
	RigControlFrame->redraw();
}

void cb_toggle_smeter(Fl_Widget *w, void *v)
{
	progStatus.meters = !progStatus.meters;
	toggle_smeter();
}

extern void cb_scripts(bool);

void cb_menu_scripts(Fl_Widget*, void*)
{
	cb_scripts(false);
}

extern void cb_create_default_script(void);

void cb_menu_make_default_scripts(Fl_Widget*, void*)
{
	cb_create_default_script();
}

void cb_48macros(Fl_Widget*, void*)
{
	progdefaults.display_48macros = !progdefaults.display_48macros;
	UI_select();
}

static void cb_opmode_show(Fl_Widget* w, void*);

static Fl_Menu_Item menu_[] = {
{_("&File"), 0,  0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},

{ icons::make_icon_label(_("Folders")), 0, 0, 0, FL_SUBMENU, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Fldigi config..."), folder_open_icon), 0, cb_ShowConfig, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("FLMSG files..."), folder_open_icon), 0, cb_ShowFLMSG, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("NBEMS files..."), folder_open_icon), 0, cb_ShowNBEMS, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("WEFAX images..."), folder_open_icon), 0, cb_ShowWEFAX_images, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Data files..."), folder_open_icon), 0, cb_ShowDATA, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ icons::make_icon_label(_("Macros")), 0, 0, 0, FL_MENU_DIVIDER | FL_SUBMENU, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Open ..."), file_open_icon), 0,  (Fl_Callback*)cb_mnuOpenMacro, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Save ..."), save_as_icon), 0,  (Fl_Callback*)cb_mnuSaveMacro, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ icons::make_icon_label(_("Config Scripts")), 0, 0, 0, FL_MENU_DIVIDER | FL_SUBMENU, _FL_MULTI_LABEL, 0, 14, 0},
{ _("Execute"),  0, (Fl_Callback*)cb_menu_scripts,  0, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ _("Generate"), 0, (Fl_Callback*)cb_menu_make_default_scripts,  0, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ 0,0,0,0,0,0,0,0,0},

{ icons::make_icon_label(_("Text Capture")), 0, 0, 0, FL_MENU_DIVIDER | FL_SUBMENU, _FL_MULTI_LABEL, 0, 14, 0},
{ LOG_TO_FILE_MLABEL, 0, cb_logfile, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ icons::make_icon_label(_("Audio")), 0, 0, 0, FL_MENU_DIVIDER | FL_SUBMENU, _FL_MULTI_LABEL, 0, 14, 0},
{_("RX capture"),  0, (Fl_Callback*)cb_mnuCapture,  0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
{_("TX generate"), 0, (Fl_Callback*)cb_mnuGenerate, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
{_("Playback"),    0, (Fl_Callback*)cb_mnuPlayback, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ icons::make_icon_label(_("Exit"), log_out_icon), 'x',  (Fl_Callback*)cb_E, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ OPMODES_MLABEL, 0,  0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_CW].name, 0, cb_init_mode, (void *)MODE_CW, 0, FL_NORMAL_LABEL, 0, 14, 0},

{"Contestia", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_4_125].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_4_250].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_4_500].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_4_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_4_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_CONTESTIA_8_125].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_8_250].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_8_500].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_8_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_8_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_CONTESTIA_16_250].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_16_250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_16_500].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_16_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_16_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_16_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_16_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_16_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_CONTESTIA_32_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_32_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_32_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_32_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_CONTESTIA_64_500].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_64_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_64_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_64_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_64_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_64_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ _("Custom..."), 0, cb_contestiaCustom, (void *)MODE_CONTESTIA, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"DominoEX", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEXMICRO].name, 0, cb_init_mode, (void *)MODE_DOMINOEXMICRO, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX4].name, 0, cb_init_mode, (void *)MODE_DOMINOEX4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX5].name, 0, cb_init_mode, (void *)MODE_DOMINOEX5, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX8].name, 0, cb_init_mode, (void *)MODE_DOMINOEX8, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX11].name, 0, cb_init_mode, (void *)MODE_DOMINOEX11, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX16].name, 0, cb_init_mode, (void *)MODE_DOMINOEX16, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX22].name, 0, cb_init_mode, (void *)MODE_DOMINOEX22, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX44].name, 0,  cb_init_mode, (void *)MODE_DOMINOEX44, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX88].name, 0,  cb_init_mode, (void *)MODE_DOMINOEX88, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ "FSQ", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ "FSQ-6", 0, cb_fsq6, (void *)MODE_FSQ, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "FSQ-4.5", 0, cb_fsq4p5, (void *)MODE_FSQ, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "FSQ-3", 0, cb_fsq3, (void *)MODE_FSQ, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "FSQ-2", 0, cb_fsq2, (void *)MODE_FSQ, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "FSQ-1.5", 0, cb_fsq1p5, (void *)MODE_FSQ, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"Hell", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_FELDHELL].name, 0, cb_init_mode, (void *)MODE_FELDHELL, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_SLOWHELL].name, 0,  cb_init_mode, (void *)MODE_SLOWHELL, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_HELLX5].name, 0,  cb_init_mode, (void *)MODE_HELLX5, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_HELLX9].name, 0,  cb_init_mode, (void *)MODE_HELLX9, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_FSKH245].name, 0, cb_init_mode, (void *)MODE_FSKH245, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_FSKH105].name, 0, cb_init_mode, (void *)MODE_FSKH105, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_HELL80].name, 0, cb_init_mode, (void *)MODE_HELL80, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ "IFKP", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ "IFKP 0.5", 0, cb_ifkp0p5, (void *)MODE_IFKP, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "IFKP 1.0", 0, cb_ifkp1p0, (void *)MODE_IFKP, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "IFKP 2.0", 0, cb_ifkp2p0, (void *)MODE_IFKP, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"MFSK", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK4].name, 0,  cb_init_mode, (void *)MODE_MFSK4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK8].name, 0,  cb_init_mode, (void *)MODE_MFSK8, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK11].name, 0,  cb_init_mode, (void *)MODE_MFSK11, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK16].name, 0,  cb_init_mode, (void *)MODE_MFSK16, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK22].name, 0,  cb_init_mode, (void *)MODE_MFSK22, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK31].name, 0,  cb_init_mode, (void *)MODE_MFSK31, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK32].name, 0,  cb_init_mode, (void *)MODE_MFSK32, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK64].name, 0,  cb_init_mode, (void *)MODE_MFSK64, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK128].name, 0,  cb_init_mode, (void *)MODE_MFSK128, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK64L].name, 0,  cb_init_mode, (void *)MODE_MFSK64L, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK128L].name, 0,  cb_init_mode, (void *)MODE_MFSK128L, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"MT63", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_500S].name, 0,  cb_init_mode, (void *)MODE_MT63_500S, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_500L].name, 0,  cb_init_mode, (void *)MODE_MT63_500L, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_1000S].name, 0,  cb_init_mode, (void *)MODE_MT63_1000S, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_1000L].name, 0,  cb_init_mode, (void *)MODE_MT63_1000L, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_2000S].name, 0,  cb_init_mode, (void *)MODE_MT63_2000S, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_2000L].name, 0,  cb_init_mode, (void *)MODE_MT63_2000L, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"OFDM", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OFDM_500F].name, 0,  cb_init_mode, (void *)MODE_OFDM_500F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OFDM_750F].name, 0,  cb_init_mode, (void *)MODE_OFDM_750F, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
//{ mode_info[MODE_OFDM_2000F].name, 0,  cb_init_mode, (void *)MODE_OFDM_2000F, 0, FL_NORMAL_LABEL, 0, 14, 0},
//{ mode_info[MODE_OFDM_2000].name, 0,  cb_init_mode, (void *)MODE_OFDM_2000, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OFDM_3500].name, 0,  cb_init_mode, (void *)MODE_OFDM_3500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ OLIVIA_MLABEL, 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_4_125].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_4_250].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_4_500].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_4_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_4_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_OLIVIA_8_125].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_8_250].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_8_500].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_8_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_8_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_OLIVIA_16_500].name, 0, cb_init_mode, (void *)MODE_OLIVIA_16_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_16_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_16_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_16_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_16_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_OLIVIA_32_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_32_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_32_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_32_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_OLIVIA_64_500].name, 0, cb_init_mode, (void *)MODE_OLIVIA_64_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_64_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_64_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_64_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_64_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ _("Custom..."), 0, cb_oliviaCustom, (void *)MODE_OLIVIA, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"PSK", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK31].name, 0, cb_init_mode, (void *)MODE_PSK31, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK63].name, 0, cb_init_mode, (void *)MODE_PSK63, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK63F].name, 0, cb_init_mode, (void *)MODE_PSK63F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK125].name, 0, cb_init_mode, (void *)MODE_PSK125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK250].name, 0, cb_init_mode, (void *)MODE_PSK250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK500].name, 0, cb_init_mode, (void *)MODE_PSK500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK1000].name, 0, cb_init_mode, (void *)MODE_PSK1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"MultiCarrier", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_12X_PSK125].name, 0, cb_init_mode, (void *)MODE_12X_PSK125, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_6X_PSK250].name, 0, cb_init_mode, (void *)MODE_6X_PSK250, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK500].name, 0, cb_init_mode, (void *)MODE_2X_PSK500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_4X_PSK500].name, 0, cb_init_mode, (void *)MODE_4X_PSK500, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK800].name, 0, cb_init_mode, (void *)MODE_2X_PSK800, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK1000].name, 0, cb_init_mode, (void *)MODE_2X_PSK1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0},

{"QPSK", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK31].name, 0, cb_init_mode, (void *)MODE_QPSK31, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK63].name, 0, cb_init_mode, (void *)MODE_QPSK63, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK125].name, 0, cb_init_mode, (void *)MODE_QPSK125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK250].name, 0, cb_init_mode, (void *)MODE_QPSK250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK500].name, 0, cb_init_mode, (void *)MODE_QPSK500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"8PSK", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK125].name, 0, cb_init_mode, (void *)MODE_8PSK125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK250].name, 0, cb_init_mode, (void *)MODE_8PSK250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK500].name, 0, cb_init_mode, (void *)MODE_8PSK500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK1000].name, 0, cb_init_mode, (void *)MODE_8PSK1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK125FL].name, 0, cb_init_mode, (void *)MODE_8PSK125FL, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK125F].name, 0, cb_init_mode, (void *)MODE_8PSK125F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK250FL].name, 0, cb_init_mode, (void *)MODE_8PSK250FL, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK250F].name, 0, cb_init_mode, (void *)MODE_8PSK250F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK500F].name, 0, cb_init_mode, (void *)MODE_8PSK500F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK1000F].name, 0, cb_init_mode, (void *)MODE_8PSK1000F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK1200F].name, 0, cb_init_mode, (void *)MODE_8PSK1200F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"PSKR", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK125R].name, 0, cb_init_mode, (void *)MODE_PSK125R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK250R].name, 0, cb_init_mode, (void *)MODE_PSK250R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK500R].name, 0, cb_init_mode, (void *)MODE_PSK500R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK1000R].name, 0, cb_init_mode, (void *)MODE_PSK1000R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"MultiCarrier", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_4X_PSK63R].name, 0, cb_init_mode, (void *)MODE_4X_PSK63R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_5X_PSK63R].name, 0, cb_init_mode, (void *)MODE_5X_PSK63R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_10X_PSK63R].name, 0, cb_init_mode, (void *)MODE_10X_PSK63R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_20X_PSK63R].name, 0, cb_init_mode, (void *)MODE_20X_PSK63R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_32X_PSK63R].name, 0, cb_init_mode, (void *)MODE_32X_PSK63R, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_4X_PSK125R].name, 0, cb_init_mode, (void *)MODE_4X_PSK125R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_5X_PSK125R].name, 0, cb_init_mode, (void *)MODE_5X_PSK125R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_10X_PSK125R].name, 0, cb_init_mode, (void *)MODE_10X_PSK125R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_12X_PSK125R].name, 0, cb_init_mode, (void *)MODE_12X_PSK125R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_16X_PSK125R].name, 0, cb_init_mode, (void *)MODE_16X_PSK125R, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK250R].name, 0, cb_init_mode, (void *)MODE_2X_PSK250R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_3X_PSK250R].name, 0, cb_init_mode, (void *)MODE_3X_PSK250R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_5X_PSK250R].name, 0, cb_init_mode, (void *)MODE_5X_PSK250R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_6X_PSK250R].name, 0, cb_init_mode, (void *)MODE_6X_PSK250R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_7X_PSK250R].name, 0, cb_init_mode, (void *)MODE_7X_PSK250R, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK500R].name, 0, cb_init_mode, (void *)MODE_2X_PSK500R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_3X_PSK500R].name, 0, cb_init_mode, (void *)MODE_3X_PSK500R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_4X_PSK500R].name, 0, cb_init_mode, (void *)MODE_4X_PSK500R, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK800R].name, 0, cb_init_mode, (void *)MODE_2X_PSK800R, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK1000R].name, 0, cb_init_mode, (void *)MODE_2X_PSK1000R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0},

{ RTTY_MLABEL, 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ "RTTY-45", 0, cb_rtty45, (void *)MODE_RTTY, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "RTTY-50", 0, cb_rtty50, (void *)MODE_RTTY, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "RTTY-75N", 0, cb_rtty75N, (void *)MODE_RTTY, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "RTTY-75W", 0, cb_rtty75W, (void *)MODE_RTTY, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ _("Custom..."), 0, cb_rttyCustom, (void *)MODE_RTTY, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"THOR", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THORMICRO].name, 0, cb_init_mode, (void *)MODE_THORMICRO, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR4].name, 0, cb_init_mode, (void *)MODE_THOR4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR5].name, 0, cb_init_mode, (void *)MODE_THOR5, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR8].name, 0, cb_init_mode, (void *)MODE_THOR8, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR11].name, 0, cb_init_mode, (void *)MODE_THOR11, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR16].name, 0, cb_init_mode, (void *)MODE_THOR16, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR22].name, 0, cb_init_mode, (void *)MODE_THOR22, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR25x4].name, 0,  cb_init_mode, (void *)MODE_THOR25x4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR50x1].name, 0,  cb_init_mode, (void *)MODE_THOR50x1, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR50x2].name, 0,  cb_init_mode, (void *)MODE_THOR50x2, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR100].name, 0,  cb_init_mode, (void *)MODE_THOR100, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"Throb", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROB1].name, 0, cb_init_mode, (void *)MODE_THROB1, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROB2].name, 0, cb_init_mode, (void *)MODE_THROB2, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROB4].name, 0, cb_init_mode, (void *)MODE_THROB4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROBX1].name, 0, cb_init_mode, (void *)MODE_THROBX1, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROBX2].name, 0, cb_init_mode, (void *)MODE_THROBX2, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROBX4].name, 0, cb_init_mode, (void *)MODE_THROBX4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

//{ "Packet", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
//{ " 300 baud", 0, cb_pkt300, (void *)MODE_PACKET, 0, FL_NORMAL_LABEL, 0, 14, 0},
//{ "1200 baud", 0, cb_pkt1200, (void *)MODE_PACKET, 0, FL_NORMAL_LABEL, 0, 14, 0},
//{ "2400 baud", 0, cb_pkt2400, (void *)MODE_PACKET, 0, FL_NORMAL_LABEL, 0, 14, 0},
//{0,0,0,0,0,0,0,0,0},

{"WEFAX", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_WEFAX_576].name, 0,  cb_init_mode, (void *)MODE_WEFAX_576, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_WEFAX_288].name, 0,  cb_init_mode, (void *)MODE_WEFAX_288, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"Navtex/SitorB", 0, 0, 0, FL_SUBMENU | FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_NAVTEX].name, 0,  cb_init_mode, (void *)MODE_NAVTEX, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_SITORB].name, 0,  cb_init_mode, (void *)MODE_SITORB, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ mode_info[MODE_WWV].name, 0, cb_init_mode, (void *)MODE_WWV, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_ANALYSIS].name, 0, cb_init_mode, (void *)MODE_ANALYSIS, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_FMT].name, 0, cb_init_mode, (void *)MODE_FMT, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_NULL].name, 0, cb_init_mode, (void *)MODE_NULL, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_SSB].name, 0, cb_init_mode, (void *)MODE_SSB, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ OPMODES_FEWER, 0, cb_opmode_show, 0, FL_MENU_INVISIBLE, FL_NORMAL_LABEL, FL_HELVETICA_ITALIC, 14, 0 },
{0,0,0,0,0,0,0,0,0},

{_("&Configure"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},

{ icons::make_icon_label(_("Config Dialog")), 0, (Fl_Callback*)cb_mnu_config_dialog, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Save Config"), save_icon), 0, (Fl_Callback*)cb_mnuSaveConfig, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Notifications")), 0,  (Fl_Callback*)cb_mnuConfigNotify, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Test Signals")), 0, (Fl_Callback*)cb_mnuTestSignals, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ VIEW_MLABEL, 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},

{ icons::make_icon_label(_("Rx Audio Dialog")), 'a', (Fl_Callback*)cb_mnuRxAudioDialog, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},

{ icons::make_icon_label(_("View/Hide Channels")), 'c', (Fl_Callback*)cb_view_hide_channels, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Signal browser")), 'b', (Fl_Callback*)cb_mnuViewer, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},

{ icons::make_icon_label(_("View/Hide 48 macros")), 'm', (Fl_Callback*)cb_48macros, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},

{ icons::make_icon_label(_("DX Cluster")), 'd', (Fl_Callback*)cb_dxc_viewer, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},

{ icons::make_icon_label(_("Floating scope"), utilities_system_monitor_icon), 'f', (Fl_Callback*)cb_mnuDigiscope, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Spectrum scope"), utilities_system_monitor_icon), 's', (Fl_Callback*)cb_mnuSpectrum, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},

{ icons::make_icon_label(MFSK_IMAGE_MLABEL, image_icon), 0, (Fl_Callback*)cb_mnuPicViewer, 0, FL_MENU_INACTIVE, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(THOR_IMAGE_MLABEL, image_icon), 0, (Fl_Callback*)cb_mnuThorViewRaw,0, FL_MENU_INACTIVE, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(IFKP_IMAGE_MLABEL, image_icon), 0, (Fl_Callback*)cb_mnuIfkpViewRaw,0, FL_MENU_INACTIVE | FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(WEFAX_TX_IMAGE_MLABEL, image_icon), 0, (Fl_Callback*)wefax_pic::cb_mnu_pic_viewer_tx,0, FL_MENU_INACTIVE | FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},

{ icons::make_icon_label(COUNTRIES_MLABEL), 0, (Fl_Callback*)cb_mnuShowCountries, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},

{ icons::make_icon_label(_("Rig/Log Controls")), 0, 0, 0, FL_SUBMENU, _FL_MULTI_LABEL, 0, 14, 0},
{ RIGLOG_FULL_MLABEL, 0, (Fl_Callback*)cb_mnu_riglog_all, 0, FL_MENU_RADIO, FL_NORMAL_LABEL, 0, 14, 0},
{ RIGLOG_PARTIAL_MLABEL, 0, (Fl_Callback*)cb_mnu_riglog_partial, 0, FL_MENU_RADIO, FL_NORMAL_LABEL, 0, 14, 0},
{ RIGLOG_NONE_MLABEL, 0, (Fl_Callback*)cb_mnu_riglog_none, 0, FL_MENU_RADIO, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ icons::make_icon_label(_("Waterfall")), 0, 0, 0, FL_SUBMENU, _FL_MULTI_LABEL, 0, 14, 0},
{ DOCKEDSCOPE_MLABEL, 0, (Fl_Callback*)cb_mnuDockedscope, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
{ WF_MLABEL, 0, (Fl_Callback*)cb_mnu_wf_all, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{0,0,0,0,0,0,0,0,0},

{ _("&Logbook"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("View")), 'l', (Fl_Callback*)cb_mnuShowLogbook, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},

{ icons::make_icon_label(_("Files")), 0, 0, 0, FL_SUBMENU | FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Open...")), 0, (Fl_Callback*)cb_mnuOpenLogbook, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Save")), 0, (Fl_Callback*)cb_mnuSaveLogbook, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("New")), 0, (Fl_Callback*)cb_mnuNewLogbook, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ icons::make_icon_label(_("ADIF")), 0, 0, 0, FL_SUBMENU | FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Merge...")), 0, (Fl_Callback*)cb_mnuMergeADIF_log, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Export...")), 0, (Fl_Callback*)cb_mnuExportADIF_log, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ icons::make_icon_label(_("LoTW")), 0, (Fl_Callback*)cb_mnuConfigLoTW,  0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},

{ icons::make_icon_label(_("Reports")), 0, 0, 0, FL_SUBMENU | FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Text...")), 0, (Fl_Callback*)cb_mnuExportTEXT_log, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("CSV...")), 0, (Fl_Callback*)cb_mnuExportCSV_log, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Cabrillo...")), 0, (Fl_Callback*)cb_Export_Cabrillo, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ LOG_CONNECT_SERVER, 0, (Fl_Callback*)cb_log_server, 0, FL_MENU_TOGGLE | FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},

{ icons::make_icon_label(_("Field Day Logging")), 0, (Fl_Callback*)cb_fd_viewer, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},

{0,0,0,0,0,0,0,0,0},

{"     ", 0, 0, 0, FL_MENU_INACTIVE, FL_NORMAL_LABEL, 0, 14, 0},
{_("&Help"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
#ifndef NDEBUG
// settle the gmfsk vs fldigi argument once and for all
{ icons::make_icon_label(_("Create sunspots"), weather_clear_icon), 0, cb_mnuFun, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
#endif
{ icons::make_icon_label(_("Beginners' Guide"), start_here_icon), 0, cb_mnuBeginnersURL, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Online documentation..."), help_browser_icon), 0, cb_mnuOnLineDOCS, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Fldigi web site..."), net_icon), 0, cb_mnuVisitURL, (void *)PACKAGE_HOME, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Reception reports..."), pskr_icon), 0, cb_mnuVisitPSKRep, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Command line options"), utilities_terminal_icon), 0, cb_mnuCmdLineHelp, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Audio device info"), audio_card_icon), 0, cb_mnuAudioInfo, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Build info"), executable_icon), 0, cb_mnuBuildInfo, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Event log"), dialog_information_icon), 0, cb_mnuDebug, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Check for updates..."), system_software_update_icon), 0, cb_mnuCheckUpdate, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("&About"), help_about_icon), 'a', cb_mnuAboutURL, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"  ", 0, 0, 0, FL_MENU_INACTIVE, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},
};

static int count_visible_items(Fl_Menu_Item* menu)
{
	int n = 0;
	if (menu->flags & FL_SUBMENU)
		menu++;
	while (menu->label()) {
		if (!(menu->flags & FL_SUBMENU) && menu->visible())
			n++;
		menu++;
	}
	return n;
}

static bool modes_hidden;
static void cb_opmode_show(Fl_Widget* w, void*)
{
	Fl_Menu_* m = (Fl_Menu_*)w;
	const char* label = m->mvalue()->label();

	Fl_Menu_Item *item = 0, *first = 0, *last = 0;
	item = first = getMenuItem(OPMODES_MLABEL) + 1;

	if (!strcmp(label, OPMODES_ALL)) {
		last = getMenuItem(OPMODES_ALL);
		while (item != last) {
			if (item->label())
				item->show();
			item++;
		}
		menu_[m->value()].label(OPMODES_FEWER);
		modes_hidden = false;
	}
	else {
		last = getMenuItem(OPMODES_FEWER);
		while (item != last) {
			if (item->label() && item->callback() == cb_init_mode) {
				intptr_t mode = (intptr_t)item->user_data();
				if (mode < NUM_MODES) {
					if (progdefaults.visible_modes.test(mode))
						item->show();
					else
						item->hide();
				}
			}
			item++;
		}
		item = first;
		while (item != last) {
			if (item->flags & FL_SUBMENU) {
				if (count_visible_items(item))
					item->show();
				else
					item->hide();
			}
			item++;
		}
		if (progdefaults.visible_modes.test(MODE_OLIVIA))
			getMenuItem("Olivia")->show();
		else
			getMenuItem("Olivia")->hide();

		if (progdefaults.visible_modes.test(MODE_CONTESTIA))
			getMenuItem("Contestia")->show();
		else
			getMenuItem("Contestia")->hide();

		if (progdefaults.visible_modes.test(MODE_RTTY))
			getMenuItem("RTTY")->show();
		else
			getMenuItem("RTTY")->hide();

		if (progdefaults.visible_modes.test(MODE_IFKP))
			getMenuItem("IFKP")->show();
		else
			getMenuItem("IFKP")->hide();

		if (progdefaults.visible_modes.test(MODE_FSQ))
			getMenuItem("FSQ")->show();
		else
			getMenuItem("FSQ")->hide();

		menu_[m->value()].label(OPMODES_ALL);
		modes_hidden = true;
	}
	m->redraw();
}

void toggle_visible_modes(Fl_Widget*, void*)
{
	Fl_Menu_Item* show_modes = modes_hidden ? getMenuItem(OPMODES_ALL) : getMenuItem(OPMODES_FEWER);

	if (!(~progdefaults.visible_modes).none()) { // some modes disabled
		show_modes->label(OPMODES_FEWER);
		show_modes->show();
		(show_modes - 1)->flags |= FL_MENU_DIVIDER;
		mnu->value(show_modes);
		show_modes->do_callback(mnu, (void*)0);
	}
	else {
		mnu->value(show_modes);
		show_modes->do_callback(mnu, (void*)0);
		show_modes->hide();
		(show_modes - 1)->flags &= ~FL_MENU_DIVIDER;
	}
}

Fl_Menu_Item *getMenuItem(const char *caption, Fl_Menu_Item* submenu)
{
	if (submenu == 0 || !(submenu->flags & FL_SUBMENU)) {
		if ( menu_->size() != sizeof(menu_)/sizeof(*menu_) ) {
			LOG_ERROR("FIXME: the menu_ table is corrupt!");
			abort();
		}
 		submenu = menu_;
	}

	int size = submenu->size() - 1;
	Fl_Menu_Item *item = 0;
	const char* label;
	for (int i = 0; i < size; i++) {
		label = (submenu[i].labeltype() == _FL_MULTI_LABEL) ?
			icons::get_icon_label_text(&submenu[i]) : submenu[i].text;
		if (label && !strcmp(label, caption)) {
			item = submenu + i;
			break;
		}
	}
	if (!item) {
		LOG_ERROR("FIXME: could not find menu item \"%s\"", caption);
		abort();
	}
	return item;
}

void activate_wefax_image_item(bool b)
{
	/// Maybe do not do anything if the new modem has activated this menu item.
	/// This is necessary because of trx_start_modem_loop which deletes
	/// the current modem after the new one is created..
	if( ( b == false )
	 && ( active_modem->get_cap() & modem::CAP_IMG )
	 && ( active_modem->get_mode() >= MODE_WEFAX_FIRST )
	 && ( active_modem->get_mode() <= MODE_WEFAX_LAST )
	 )
	{
		return ;
	}

	Fl_Menu_Item *wefax_tx_item = getMenuItem(WEFAX_TX_IMAGE_MLABEL);
	if (wefax_tx_item)
		icons::set_active(wefax_tx_item, b);
}


void activate_menu_item(const char *caption, bool val)
{
	Fl_Menu_Item *m = getMenuItem(caption);
	icons::set_active(m, val);
}

void activate_mfsk_image_item(bool b)
{
	Fl_Menu_Item *mfsk_item = getMenuItem(MFSK_IMAGE_MLABEL);
	if (mfsk_item)
		icons::set_active(mfsk_item, b);
}

void activate_thor_image_item(bool b)
{
	Fl_Menu_Item *menu_item = getMenuItem(THOR_IMAGE_MLABEL);
	if (menu_item)
		icons::set_active(menu_item, b);
}

void activate_ifkp_image_item(bool b)
{
	Fl_Menu_Item *menu_item = getMenuItem(IFKP_IMAGE_MLABEL);
	if (menu_item)
		icons::set_active(menu_item, b);
}

inline int rightof(Fl_Widget* w)
{
	return w->x() + w->w();
}

inline int leftof(Fl_Widget* w)
{
	unsigned int a = w->align();
	if (a == FL_ALIGN_CENTER || a & FL_ALIGN_INSIDE)
		return w->x();

	fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
	int lw = static_cast<int>(ceil(fl_width(w->label())));

	if (a & (FL_ALIGN_TOP | FL_ALIGN_BOTTOM)) {
		if (a & FL_ALIGN_LEFT)
			return w->x();
		else if (a & FL_ALIGN_RIGHT)
			return w->x() - (lw > w->w() ? lw - w->w() : 0);
		else
			return w->x() - (lw > w->w() ? (lw - w->w())/2 : 0);
	}
	else {
		if (a & FL_ALIGN_LEFT)
			return w->x() - lw;
		else
			return w->x();
	}
}

inline int above(Fl_Widget* w)
{
	unsigned int a = w->align();
	if (a == FL_ALIGN_CENTER || a & FL_ALIGN_INSIDE)
		return w->y();

	return (a & FL_ALIGN_TOP) ? w->y() + FL_NORMAL_SIZE : w->y();
}

inline int below(Fl_Widget* w)
{
	unsigned int a = w->align();
	if (a == FL_ALIGN_CENTER || a & FL_ALIGN_INSIDE)
		return w->y() + w->h();

	return (a & FL_ALIGN_BOTTOM) ? w->y() + w->h() + FL_NORMAL_SIZE : w->y() + w->h();
}

string argv_window_title;
string main_window_title;
string xcvr_title;

void update_main_title()
{
	string buf = argv_window_title;
	buf.append(" ver").append(PACKAGE_VERSION);
	if (!xcvr_title.empty()) {
		buf.append(" / ").append(xcvr_title);
	}
	buf.append(" - ");
	if (bWF_only)
		buf.append(_("waterfall-only mode"));
	else {
		buf.append(progdefaults.myCall.empty() ? _("NO CALLSIGN SET") : progdefaults.myCall.c_str());
		if (progdefaults.logging > LOG_QSO && progdefaults.logging < LOG_SQSO)
			buf.append(" :  ").append(contests[progdefaults.logging].name);
		if (progdefaults.logging == LOG_SQSO) {
			buf.append(" : ").append(QSOparties.qso_parties[progdefaults.SQSOcontest].contest);
		}
		if (progdefaults.logging == 0 && n3fjp_connected) {
			buf.append(" : N3FJP Amateur Contact Log");
		}
	}
	if (fl_digi_main) {
		fl_digi_main->copy_label(buf.c_str());
		fl_digi_main->redraw();
	}
}

void showOpBrowserView(Fl_Widget *, void *)
{
	if (RigViewerFrame->visible())
		return CloseQsoView();

	Logging_frame->hide();
	RigViewerFrame->show();
	qso_opPICK->box(FL_DOWN_BOX);
	qso_opBrowser->take_focus();
	qso_opPICK->tooltip(_("Close List"));
}

void CloseQsoView()
{
	RigViewerFrame->hide();
	Logging_frame->show();
	qso_opPICK->box(FL_UP_BOX);
	qso_opPICK->tooltip(_("Open List"));
	if (restore_minimize) {
		restore_minimize = false;

		UI_select();
	}
}

void showOpBrowserView2(Fl_Widget *w, void *)
{
	restore_minimize = true;

	UI_select();
	showOpBrowserView(w, NULL);
}

void cb_qso_btnSelFreq(Fl_Widget *, void *)
{
	qso_selectFreq();
}

void cb_qso_btnDelFreq(Fl_Widget *, void *)
{
	qso_delFreq();
}

void cb_qso_btnAddFreq(Fl_Widget *, void *)
{
	qso_addFreq();
}

void cb_qso_btnClearList(Fl_Widget *, void *)
{
	if (quick_choice(_("Clear list?"), 2, _("Confirm"), _("Cancel"), NULL) == 1)
		clearList();
}

//======================================================================
// PSK reporter interface
// use separate thread to accomodate very slow responses from
// remote server
//======================================================================
#define PSKREP_MENU_MAX 8

static pthread_t PSKREP_thread;
static string pskrep_data, pskrep_url, popup_title;
static string pskrep_str[PSKREP_MENU_MAX];
static string::size_type pskrep_i;
static Fl_Menu_Item pskrep_menu[PSKREP_MENU_MAX + 1];
static bool pskrep_working = false;

void do_pskreporter_popup()
{
	int j;
	int sel = 0;
	int t = Fl_Tooltip::enabled();
	const Fl_Menu_Item* p = (Fl_Menu_Item *)0;

	put_status(""); 

	Fl_Tooltip::disable();
	p = pskrep_menu->popup(
		qso_inpAct->x() + qso_inpAct->w(), qso_inpAct->y() + qso_inpAct->h(),
		popup_title.c_str(), pskrep_menu + sel);

	j = p ? p - pskrep_menu + 1 : 0;
	if (j)
		qsy(strtoll(pskrep_str[j - 1].erase(pskrep_str[j - 1].find(' ')).c_str(), NULL, 10));

	Fl_Tooltip::enable(t);
}

void *do_pskreporter_lookup(void *)  // thread action
{
	pskrep_working = true;
	pskrep_data.clear();
	pskrep_url.assign("https://pskreporter.info/cgi-bin/psk-freq.pl");
	pskrep_url.append("?mode=").append(mode_info[active_modem->get_mode()].adif_name);
	if (qso_inpAct->size())
		pskrep_url.append("&?grid=").append(qso_inpAct->value());
	else if (progdefaults.myLocator.length() > 2)
		pskrep_url.append("&?grid=").append(progdefaults.myLocator, 0, 2);

	if (get_http(pskrep_url, pskrep_data, 20.0) != MBEDTLS_EXIT_SUCCESS) {
		LOG_ERROR("Error while fetching \"%s\": %s", pskrep_url.c_str(), pskrep_data.c_str());
		pskrep_working = false;
		return NULL;
	}

	if (pskrep_data.find("IP has made too many requests") != std::string::npos) {
		popup_title.assign(progdefaults.myName).append("\nSlow down the requests\nLAST data");
		REQ(do_pskreporter_popup);
		pskrep_working = false;
		return NULL;
	}

	pskrep_i = pskrep_data.rfind("\r\n\r\n");
	if (pskrep_i == string::npos) {
		LOG_ERROR("Pskreporter return invalid: %s", pskrep_data.c_str());
		pskrep_working = false;
		return NULL;
	}
	pskrep_i += 4;
	pskrep_i = pskrep_data.find("\r\n", pskrep_i);
	pskrep_i += 2;
	re_t re("([[:digit:]]{6,}) [[:digit:]]+ ([[:digit:]]+)[[:space:]]+", REG_EXTENDED);

	size_t j = 0;
	memset(pskrep_menu, 0, sizeof(pskrep_menu));
	string title;
	while (re.match(pskrep_data.substr(pskrep_i).c_str()) && j < PSKREP_MENU_MAX) {
		pskrep_i = pskrep_data.find("\r\n", pskrep_i + 1);
		if (pskrep_i == string::npos) break;
		pskrep_i += 2;
		pskrep_str[j].assign(re.submatch(1)).append(" (").append(re.submatch(2)).
			append(" ").append(atoi(re.submatch(2).c_str()) == 1 ? _("report") : _("reports")).append(")");
		pskrep_menu[j].label(pskrep_str[j].c_str());
		pskrep_menu[++j].label(NULL);
	}
	if ((pskrep_i = pskrep_data.rfind(" grid ")) != string::npos) {
		popup_title.assign(_("Recent activity for grid ")).
					append(pskrep_data.substr(pskrep_i + 5, 3));
	} else {
		popup_title = " (?) Check network event log!\n";
#ifdef __WIN32__
		popup_title.append("fldigi.files\\debug\\network_debug.txt");
#else
		popup_title.append("~/.fldigi/debug/network_debug.txt");
#endif
	}
	REQ(do_pskreporter_popup);
	pskrep_working = false;
	return NULL;
}

void cb_qso_inpAct(Fl_Widget*, void*)
{
	if (pskrep_working) {
		return;
	}
	if (pthread_create(&PSKREP_thread, NULL, do_pskreporter_lookup, NULL) < 0) {
		LOG_ERROR("%s", "pthread_create failed");
		return;
	}
	put_status("Fetching PSK Reporter data", 15); 
}
//======================================================================

static int i_opUsage;
static string s_opEntry;
static string s_opUsageEntry;
static string s_outEntry;

void cb_opUsageEnter(Fl_Button *, void*)
{
	s_opUsageEntry = opUsage->value();
	s_opEntry.append(s_opUsageEntry);
	qso_opBrowser->text(i_opUsage, s_opEntry.c_str());
	qso_updateEntry(i_opUsage, s_opUsageEntry);
	opUsageFrame->hide();
	qso_opBrowser->show();
}

void qso_opBrowser_amend(int i)
{
	size_t pos;
	s_opEntry = qso_opBrowser->text(i);
	pos = s_opEntry.rfind('|');
	s_opUsageEntry = s_opEntry.substr(pos+1);
	s_opEntry.erase(pos + 1);

	s_outEntry = s_opEntry.substr(0, pos);
	while ((pos = s_outEntry.find('|')) != string::npos)
		s_outEntry.replace(pos, 1, "  ");

	opOutUsage->value(s_outEntry.c_str());

	opUsage->value(s_opUsageEntry.c_str());

	i_opUsage = i;
	qso_opBrowser->hide();
	opUsageFrame->show();
}

void cb_qso_opBrowser(Fl_Browser*, void*)
{
	int i = qso_opBrowser->value();
	if (!i)
		return;

	// This makes the multi browser behave more like a hold browser,
	// but with the ability to invoke the callback via space/return.
	qso_opBrowser->deselect();
	qso_opBrowser->select(i);

	int ikey = Fl::event_key();
	switch (ikey) {
	case FL_Enter: case FL_KP_Enter: case FL_Button + FL_LEFT_MOUSE:
		if (ikey == FL_Button + FL_LEFT_MOUSE && !Fl::event_clicks())
			break;
		qso_selectFreq();
		CloseQsoView();
		break;
	case ' ': case FL_Button + FL_RIGHT_MOUSE:
		if ((Fl::event_state() & FL_SHIFT) == FL_SHIFT) {
			qso_opBrowser_amend(i);
		} else
			qso_setFreq();
		break;
	case FL_Button + FL_MIDDLE_MOUSE:
		i = qso_opBrowser->value();
		qso_delFreq();
		qso_addFreq();
		qso_opBrowser->select(i);
		break;
	}
}

void _show_frequency(long long freq)
{
	if (!qsoFreqDisp1 || !qsoFreqDisp2 || !qsoFreqDisp3)
		return;
	qsoFreqDisp1->value(freq);
	qsoFreqDisp2->value(freq);
	qsoFreqDisp3->value(freq);
//	if (FD_logged_on) FD_band_check();
}

void show_frequency(long long freq)
{
	REQ(_show_frequency, freq);
}

void show_mode(const string sMode)
{
	REQ(&Fl_ListBox::put_value, qso_opMODE, sMode.c_str());
}

void show_bw(const string sWidth)
{
	REQ(&Fl_ListBox::put_value, qso_opBW, sWidth.c_str());
}

void show_bw1(const string sVal)
{
	REQ(&Fl_ListBox::put_value, qso_opBW1, sVal.c_str());
}

void show_bw2(const string sVal)
{
	REQ(&Fl_ListBox::put_value, qso_opBW2, sVal.c_str());
}

void show_spot(bool v)
{
//if (bWF_only) return;
	static bool oldval = false;
	if (v) {
		mnu->size(btnAutoSpot->x(), mnu->h());
		if (oldval)
			progStatus.spot_recv = true;
		btnAutoSpot->value(progStatus.spot_recv);
		btnAutoSpot->activate();
	}
	else {
		btnAutoSpot->deactivate();
		oldval = btnAutoSpot->value();
		btnAutoSpot->value(v);
		btnAutoSpot->do_callback();
		mnu->size(btnRSID->x(), mnu->h());
	}
	mnu->redraw();
}

void setTabColors()
{
	if (dlgConfig->visible()) dlgConfig->redraw();

	if (dxcluster_viewer) {
		cluster_tabs->selection_color(progdefaults.TabsColor);
		if (dxcluster_viewer->visible()) dxcluster_viewer->redraw();
	}
}

void showMacroSet() {
	set_macroLabels();
}

void showDTMF(const string s) {
	string dtmfstr = "\n<DTMF> ";
	dtmfstr.append(s);
	ReceiveText->addstr(dtmfstr);
}

void setwfrange() {
	wf->opmode();
}

void sync_cw_parameters()
{
	active_modem->sync_parameters();
	active_modem->update_Status();
}

void cb_cntCW_WPM(Fl_Widget * w, void *v)
{
	Fl_Counter2 *cnt = (Fl_Counter2 *) w;
	if (progStatus.WK_online && progStatus.WK_use_pot) {
		cnt->value(progStatus.WK_speed_wpm);
		return;
	}

	if (progStatus.WK_online && cnt->value() > 55) cnt->value(55);
	if (use_nanoIO && cnt->value() > 60) cnt->value(60);
	if (use_nanoIO && cnt->value() < 5) cnt->value(5);

	progdefaults.CWspeed = (int)cnt->value();
LOG_INFO("%f WPM", progdefaults.CWspeed);
	sldrCWxmtWPM->value(progdefaults.CWspeed);
	cntr_nanoCW_WPM->value(progdefaults.CWspeed);

	progdefaults.changed = true;
	sync_cw_parameters();

	if (progStatus.WK_online) WK_set_wpm();
	flrig_set_wpm();

	restoreFocus(25);
}

void cb_btnCW_Default(Fl_Widget *w, void *v)
{
	active_modem->toggleWPM();
	restoreFocus(26);
}

static void cb_mainViewer_Seek(Fl_Input *, void *)
{
	static const Fl_Color seek_color[2] = { FL_FOREGROUND_COLOR,
					  adjust_color(FL_RED, FL_BACKGROUND2_COLOR) }; // invalid RE
	seek_re.recompile(*txtInpSeek->value() ? txtInpSeek->value() : "[invalid");
	if (txtInpSeek->textcolor() != seek_color[!seek_re]) {
		txtInpSeek->textcolor(seek_color[!seek_re]);
		txtInpSeek->redraw();
	}
	progStatus.browser_search = txtInpSeek->value();
	if (viewer_inp_seek)
		viewer_inp_seek->value(progStatus.browser_search.c_str());
}

static void cb_cntTxLevel(Fl_Counter2* o, void*) {
  progdefaults.txlevel = o->value();
}

static void cb_mainViewer(Fl_Hold_Browser*, void*) {
	int sel = mainViewer->value();
	if (sel == 0 || sel > progdefaults.VIEWERchannels)
		return;

	switch (Fl::event_button()) {
	case FL_LEFT_MOUSE:
		if (mainViewer->freq(sel) != NULLFREQ) {
			if (progdefaults.VIEWERhistory){
				ReceiveText->addchr('\n', FTextBase::RECV);
				bHistory = true;
			} else {
				ReceiveText->addchr('\n', FTextBase::ALTR);
				ReceiveText->addstr(mainViewer->line(sel), FTextBase::ALTR);
			}
			active_modem->set_freq(mainViewer->freq(sel));
			recenter_spectrum_viewer();
			active_modem->set_sigsearch(SIGSEARCH);
			if (brwsViewer) brwsViewer->select(sel);
		} else
			mainViewer->deselect();
		break;
	case FL_MIDDLE_MOUSE: // copy from modem
//		set_freq(sel, active_modem->get_freq());
		break;
	case FL_RIGHT_MOUSE: // reset
		{
		int ch = progdefaults.VIEWERascend ? progdefaults.VIEWERchannels - sel : sel - 1;
		active_modem->clear_ch(ch);
		mainViewer->deselect();
		if (brwsViewer) brwsViewer->deselect();
		break;
		}
	default:
		break;
	}
}

void widget_color_font(Fl_Widget *widget)
{
	widget->labelsize(progdefaults.LOGGINGtextsize);
	widget->labelfont(progdefaults.LOGGINGtextfont);
	widget->labelcolor(progdefaults.LOGGINGtextcolor);
	widget->color(progdefaults.LOGGINGcolor);
	widget->redraw_label();
	widget->redraw();
}

void input_color_font(Fl_Input *input)
{
	input->textsize(progdefaults.LOGGINGtextsize);
	input->textfont(progdefaults.LOGGINGtextfont);
	input->textcolor(progdefaults.LOGGINGtextcolor);
	input->color(progdefaults.LOGGINGcolor);
	input->redraw();
}

void counter_color_font(Fl_Counter2 * cntr)
{
	cntr->textsize(progdefaults.LOGGINGtextsize);
	cntr->textfont(progdefaults.LOGGINGtextfont);
	cntr->textcolor(progdefaults.LOGGINGtextcolor);
	cntr->textbkcolor(progdefaults.LOGGINGcolor);
	cntr->redraw();
}

void combo_color_font(Fl_ComboBox *cbo)
{
	cbo->color(progdefaults.LOGGINGcolor);
	cbo->selection_color(progdefaults.LOGGINGcolor);
	cbo->textfont(progdefaults.LOGGINGtextfont);
	cbo->textsize(progdefaults.LOGGINGtextsize);
	cbo->textcolor(progdefaults.LOGGINGtextcolor);
	cbo->redraw();
	cbo->redraw_label();
}

void LOGGING_colors_font()
{
	Fl_Input* in[] = {
		inpFreq1,
		inpCall1, inpCall2, inpCall3, inpCall4,
		inpName1, inpName2,
		inpTimeOn1, inpTimeOn2, inpTimeOn3, inpTimeOn4, inpTimeOn5,
		inpTimeOff1, inpTimeOff2, inpTimeOff3, inpTimeOff4, inpTimeOff5,
		inpRstIn1, inpRstIn2,
		inpRstOut1, inpRstOut2,
		inpQth, inpLoc, inpAZ, inpVEprov,
		inpState1,
		inpLoc1,
		inpSerNo1, inpSerNo2,
		outSerNo1, outSerNo2,
		outSerNo3, outSerNo4,
		inp_SS_Check1, inp_SS_Precedence1,
		inp_SS_Section1, inp_SS_SerialNoR1,
		inp_SS_Check2, inp_SS_Precedence2,
		inp_SS_Section2, inp_SS_SerialNoR2,
		inpXchgIn1, inpXchgIn2,
		inp_FD_class1, inp_FD_class2,
		inp_FD_section1, inp_FD_section2,
		inp_KD_age1, inp_KD_age2,
		inp_KD_state1, inp_KD_state2,
		inp_KD_VEprov1, inp_KD_VEprov2,
		inp_KD_XchgIn1, inp_KD_XchgIn2,
		inp_CQ_RSTin2, inp_CQ_RSTout2, inp_CQzone1, inp_CQzone2,
		inp_CQstate1, inp_CQstate2,
		inp_CQDX_RSTin2, inp_CQDX_RSTout2,
		inp_CQDXzone1, inp_CQDXzone2,
		inp_ASCR_RSTin2, inp_ASCR_RSTout2,
		inp_ASCR_XchgIn1, inp_ASCR_XchgIn2,
		inp_ASCR_class1, inp_ASCR_class2,
		inp_ASCR_name2,
		inpNAQPname2, inp_name_NAS2,
		inpSPCnum_NAQP1, inpSPCnum_NAQP2,
		inpRTU_stpr1, inpRTU_stpr2, inpRTU_RSTin2, inpRTU_RSTout2,
		inpRTU_serno1, inpRTU_serno2,
		outSerNo4, inp_ser_NAS1, inpSPCnum_NAS1,
		outSerNo5, inp_ser_NAS2, inpSPCnum_NAS2,
		inpSerNo3, inpSerNo4,
		outSerNo7, outSerNo8,
		inpRstIn3, inpRstOut3,
		inp_JOTA_scout1, inp_JOTA_scout2,
		inp_JOTA_troop1, inp_JOTA_troop2,
		inp_JOTA_spc1, inp_JOTA_spc2,
		inpRstIn_AICW2, inpRstOut_AICW2,
		inpSPCnum_AICW1, inpSPCnum_AICW2
	};
	for (size_t i = 0; i < sizeof(in)/sizeof(*in); i++) {
		input_color_font(in[i]);
	}
	input_color_font(inpNotes);

// buttons, boxes
	Fl_Widget *wid[] = {
		MODEstatus, Status1, Status2, StatusBar, WARNstatus };
	for (size_t i = 0; i < sizeof(wid)/sizeof(*wid); i++)
		widget_color_font(wid[i]);

// counters
	counter_color_font(cntCW_WPM);
	counter_color_font(cntTxLevel);
	counter_color_font(wf->wfRefLevel);
	counter_color_font(wf->wfAmpSpan);
	counter_color_font(wf->wfcarrier);

// combo boxes
	Fl_ComboBox *cbo_widgets[] = {
		qso_opMODE, qso_opBW, qso_opBW1, qso_opBW2,
		cboCountyQSO, cboCountryQSO,
		cboCountryAICW2,
		cboCountryAIDX2,
		cboCountryCQ2,
		cboCountryCQDX2,
		cboCountryIARI2,
		cboCountryRTU2 //,
//		cboCountryWAE2
	};
	for (size_t i = 0; i < sizeof(cbo_widgets)/sizeof(*cbo_widgets); i++) {
		combo_color_font(cbo_widgets[i]);
	}

	fl_digi_main->redraw();

}

inline void inp_font_pos(Fl_Input2* inp, int x, int y, int w, int h)
{
	inp->textsize(progdefaults.LOGBOOKtextsize);
	inp->textfont(progdefaults.LOGBOOKtextfont);
	inp->textcolor(progdefaults.LOGBOOKtextcolor);
	inp->color(progdefaults.LOGBOOKcolor);
	inp->labelfont(progdefaults.LOGBOOKtextfont);
	int ls = progdefaults.LOGBOOKtextsize - 1;
	ls = ls < 12 ? 12 : (ls > 14 ? 14 : ls);
	inp->labelsize(ls);
	inp->redraw_label();
	inp->resize(x, y, w, h);
	inp->redraw();
}

inline void date_font_pos(Fl_DateInput* inp, int x, int y, int w, int h)
{
	inp->textsize(progdefaults.LOGBOOKtextsize);
	inp->textfont(progdefaults.LOGBOOKtextfont);
	inp->textcolor(progdefaults.LOGBOOKtextcolor);
	inp->color(progdefaults.LOGBOOKcolor);
	inp->labelfont(progdefaults.LOGBOOKtextfont);
	int ls = progdefaults.LOGBOOKtextsize - 1;
	ls = ls < 10 ? 10 : (ls > 14 ? 14 : ls);
	inp->labelsize(ls);
	inp->redraw_label();
	inp->resize(x, y, w, h);
}

inline void btn_font_pos(Fl_Widget* btn, int x, int y, int w, int h)
{
	btn->labelfont(progdefaults.LOGBOOKtextfont);
	int ls = progdefaults.LOGBOOKtextsize - 1;
	ls = ls < 10 ? 10 : (ls > 14 ? 14 : ls);
	btn->labelsize(ls);
	btn->redraw_label();
	btn->resize(x, y, w, h);
	btn->redraw();
}

inline void tab_font_pos(Fl_Widget* tab, int x, int y, int w, int h, int ls)
{
	tab->labelfont(progdefaults.LOGBOOKtextfont);
	tab->labelsize(ls);
	tab->redraw_label();
	tab->resize(x, y, w, h);
	tab->redraw();
}

inline void chc_font_pos(Fl_Choice* chc, int x, int y, int w, int h)
{
	chc->labelfont(progdefaults.LOGBOOKtextfont);
	int ls = progdefaults.LOGBOOKtextsize - 1;
	ls = ls < 10 ? 10 : (ls > 14 ? 14 : ls);
	chc->labelsize(ls);
	chc->redraw_label();
	chc->resize(x, y, w, h);
	chc->redraw();
}

void LOGBOOK_colors_font()
{
	if (!dlgLogbook) return;

	int ls = progdefaults.LOGBOOKtextsize;

// input / output / date / text fields
	fl_font(progdefaults.LOGBOOKtextfont, ls);
	int wh = fl_height() + 4;// + 8;
	int width_date = fl_width("888888888") + wh;
	int width_time = fl_width("23:59:599");
	int width_freq = fl_width("WW/WW8WWW/WW.");//fl_width("99.9999999");
	int width_rst  = fl_width("5999");
	int width_pwr  = fl_width("0000");
	int width_loc  = fl_width("XX88XXX");
	int width_mode = fl_width("CONTESTIA");
	int width_state = fl_width("WWWW");
	int width_province = fl_width("WWW.");
	int width_country = fl_width("WWWWWWWWWWWWWWWWWWWW");

	int dlg_width =	2 +
					width_date + 2 +
					width_time + 2 +
					width_freq + 2 +
					width_mode + 2 +
					width_pwr + 2 +
					width_rst + 2 +
					width_loc + 2;

	int newheight = 4*(wh + 20) + 3*wh + 2 + wh + 2 + wBrowser->h() + 2; //+ 24;

	if (dlg_width > progStatus.logbook_w)
		progStatus.logbook_w = dlg_width;
	else
		dlg_width = progStatus.logbook_w;
	if (newheight > progStatus.logbook_h)
		progStatus.logbook_h = newheight;
	else
		newheight = progStatus.logbook_h;

	dlgLogbook->resize( dlgLogbook->x(), dlgLogbook->y(), progStatus.logbook_w, progStatus.logbook_h);

// row1
	int ypos = 24;

// date on
	int xpos = 2;
	date_font_pos(inpDate_log, xpos, ypos, width_date, wh);
// timeon
	xpos += width_date + 2;
	inp_font_pos(inpTimeOn_log, xpos, ypos, width_time, wh);
// call
	xpos += width_time + 2;
	inp_font_pos(inpCall_log, xpos, ypos, width_freq, wh);
// name
	xpos += width_freq + 2;
	int wname = dlg_width - xpos - width_rst - width_loc - 6;
	inp_font_pos(inpName_log, xpos, ypos, wname, wh);
// rcvd RST
	xpos += wname + 2;
	inp_font_pos(inpRstR_log, xpos, ypos, width_rst, wh);
// nbr records
	xpos += width_rst + 2;
	inp_font_pos(txtNbrRecs_log, xpos, ypos, width_loc, wh);

// row2
	ypos += wh + 20;
//date off
	xpos = 2;
	date_font_pos(inpDateOff_log, xpos, ypos, width_date, wh);
//time off
	xpos += width_date + 2;
	inp_font_pos(inpTimeOff_log, xpos, ypos, width_time, wh);
//frequency
	xpos += width_time + 2;
	inp_font_pos(inpFreq_log, xpos, ypos, width_freq, wh);
//mode
	xpos += width_freq + 2;
	int wmode = dlg_width - xpos - width_rst - width_pwr - width_loc - 8;
	inp_font_pos(inpMode_log, xpos, ypos, wmode, wh);
//power
	xpos += wmode + 2;
	inp_font_pos(inpTX_pwr_log, xpos, ypos, width_pwr, wh);
//sent RST
	xpos += width_pwr + 2;
	inp_font_pos(inpRstS_log, xpos, ypos, width_rst, wh);
// locator
	xpos += width_rst + 2;
	inp_font_pos(inpLoc_log, xpos, ypos, width_loc, wh);

// row 3
// QTH
	ypos += wh + 20;
	xpos = 2;
	int wqth = dlg_width - 4 - width_state - 2 - width_province - 2 - width_country - 2;
	inp_font_pos(inpQth_log, xpos, ypos, wqth, wh);
// state
	xpos += wqth + 2;
	inp_font_pos(inpState_log, xpos, ypos, width_state, wh);
// province
	xpos += width_state + 2;
	inp_font_pos(inpVE_Prov_log, xpos, ypos, width_province, wh);
// country
	xpos += width_province + 2;
	inp_font_pos(inpCountry_log, xpos, ypos, width_country, wh);

	ypos += wh + 2;

	grpTabsSearch->position(0, ypos);
	Tabs->position(2, grpTabsSearch->y() + 2);

	inp_font_pos(inpSearchString, Tabs->x() + Tabs->w() + 4, Tabs->y() + 30,
		dlgLogbook->w() - Tabs->w() - 8, wh);

	int tab_h = wh * 14 / progdefaults.LOGBOOKtextsize;
	int tab_grp_h = 4 * wh + 4;
//	Tabs->resize(2, ypos, dlg_width - 6 - inpSearchString->w(), tab_grp_h + tab_h);
	Tabs->selection_color(progdefaults.TabsColor);

	tab_font_pos(tab_log_qsl, 2, ypos + tab_h, Tabs->w(), tab_grp_h, 14);
	tab_font_pos(tab_log_contest, 2, ypos + tab_h, Tabs->w(), tab_grp_h, 14);
	tab_font_pos(tab_log_other, 2, ypos + tab_h, Tabs->w(), tab_grp_h, 14);
	tab_font_pos(tab_log_notes, 2, ypos + tab_h, Tabs->w(), tab_grp_h, 14);

	Fl_Input2* qso_fields[] = {
		inpTimeOn_log, inpCall_log, inpName_log, inpRstR_log, inpTimeOff_log,
		inpFreq_log, inpMode_log, inpTX_pwr_log, inpRstS_log, inpQth_log,
		inpState_log, inpVE_Prov_log, inpLoc_log, inpCountry_log, inpQSL_VIA_log,
		inpCNTY_log, inpIOTA_log, inpCQZ_log, inpCONT_log, inpITUZ_log,
		inpDXCC_log, inpNotes_log, inp_log_sta_call, inp_log_op_call, inp_log_sta_qth,
		inp_log_sta_loc, inpSerNoOut_log, inpMyXchg_log, inpSerNoIn_log, inpXchgIn_log,
		inpClass_log, inpSection_log, inp_age_log, inp_1010_log, inpBand_log,
		inp_check_log, inp_log_cwss_serno, inp_log_cwss_sec, inp_log_cwss_prec, inp_log_cwss_chk,
		inp_log_troop_s, inp_log_troop_r, inp_log_scout_s, inp_log_scout_r, inpSearchString,
		txtNbrRecs_log
	};

	Fl_DateInput* dti[] = {
		inp_export_start_date, inp_export_stop_date, inpDate_log, inpDateOff_log,
		inpQSLrcvddate_log, inpEQSLrcvddate_log, inpLOTWrcvddate_log, inpQSLsentdate_log,
		inpEQSLsentdate_log, inpLOTWsentdate_log
	};
	for (size_t i = 0; i < sizeof(qso_fields) / sizeof(*qso_fields); i++)
		inp_font_pos(
			qso_fields[i], qso_fields[i]->x(),
			qso_fields[i]->y(), qso_fields[i]->w(), wh);

	for (size_t i = 0; i < sizeof(dti) / sizeof(*dti); i++)
		date_font_pos(	dti[i], dti[i]->x(), dti[i]->y(), dti[i]->w(), wh);

	inpNotes_log->resize(
		tab_log_notes->x() + 2,
		tab_log_notes->y() + 4,
		tab_log_notes->w() - 4,
		tab_log_notes->h() - 6);

	ypos += grpTabsSearch->h() + 2;

	grpFileButtons->resize(0, ypos, dlgLogbook->w(), grpFileButtons->h());
	grpFileButtons->redraw();

	txtLogFile->textsize(ls);
	txtLogFile->textfont(progdefaults.LOGBOOKtextfont);
	txtLogFile->textcolor(progdefaults.LOGBOOKtextcolor);
	txtLogFile->color(progdefaults.LOGBOOKcolor);

	ypos += grpFileButtons->h() + 2;

	wBrowser->font(progdefaults.LOGBOOKtextfont);
	wBrowser->fontsize(progdefaults.LOGBOOKtextsize);
	wBrowser->color(progdefaults.LOGBOOKcolor);
	wBrowser->selection_color(FL_SELECTION_COLOR);

	int datewidth = wBrowser->columnWidth(0);
	int timewidth = wBrowser->columnWidth(1);
	int callwidth = wBrowser->columnWidth(2);
	int namewidth = wBrowser->columnWidth(3);
	int freqwidth = wBrowser->columnWidth(4);
	int modewidth = wBrowser->columnWidth(5);
	int totalwidth = datewidth + timewidth + callwidth + namewidth + freqwidth + modewidth;

	int nuwidth = dlgLogbook->w() - 2*wBrowser->x();

	wBrowser->resize(wBrowser->x(), ypos, nuwidth, dlgLogbook->h() - 2 - ypos);

	nuwidth -= wBrowser->vScrollWidth();

	datewidth *= (1.0 * nuwidth / totalwidth);
	timewidth *= (1.0 * nuwidth / totalwidth);
	callwidth *= (1.0 * nuwidth / totalwidth);
	freqwidth *= (1.0 * nuwidth / totalwidth);
	modewidth *= (1.0 * nuwidth / totalwidth);

	namewidth = nuwidth - datewidth - timewidth - callwidth - freqwidth - modewidth;

	wBrowser->columnWidth (0, datewidth); // Date column
	wBrowser->columnWidth (1, timewidth); // Time column
	wBrowser->columnWidth (2, callwidth); // Callsign column
	wBrowser->columnWidth (3, namewidth); // Name column
	wBrowser->columnWidth (4, freqwidth); // Frequency column
	wBrowser->columnWidth (5, modewidth); // Mode column

	dlgLogbook->init_sizes();
	dlgLogbook->damage();
	dlgLogbook->redraw();

}

void set_smeter_colors()
{
	Fl_Color clr = fl_rgb_color(
					progdefaults.Smeter_bg_color.R,
					progdefaults.Smeter_bg_color.G,
					progdefaults.Smeter_bg_color.B);
	smeter->set_background(clr);

	clr = fl_rgb_color(
			progdefaults.Smeter_meter_color.R,
			progdefaults.Smeter_meter_color.G,
			progdefaults.Smeter_meter_color.B);
	smeter->set_metercolor(clr);

	clr = fl_rgb_color(
			progdefaults.Smeter_scale_color.R,
			progdefaults.Smeter_scale_color.G,
			progdefaults.Smeter_scale_color.B);
	smeter->set_scalecolor(clr);

	smeter->redraw();

	clr = fl_rgb_color(
			progdefaults.PWRmeter_bg_color.R,
			progdefaults.PWRmeter_bg_color.G,
			progdefaults.PWRmeter_bg_color.B);
	pwrmeter->set_background(clr);

	clr = fl_rgb_color(
			progdefaults.PWRmeter_meter_color.R,
			progdefaults.PWRmeter_meter_color.G,
			progdefaults.PWRmeter_meter_color.B);
	pwrmeter->set_metercolor(clr);

	clr = fl_rgb_color(
			progdefaults.PWRmeter_scale_color.R,
			progdefaults.PWRmeter_scale_color.G,
			progdefaults.PWRmeter_scale_color.B);
	pwrmeter->set_scalecolor(clr);

	pwrmeter->select(progdefaults.PWRselect);

	pwrmeter->redraw();
}

void FREQ_callback(Fl_Input2 *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_FREQ) s.erase(MAX_FREQ);
	w->value(s.c_str());
}

void TIME_callback(Fl_Input2 *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_TIME) s.erase(MAX_TIME);
	w->value(s.c_str());
}

void RST_callback(Fl_Input2 *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_RST) s.erase(MAX_RST);
	w->value(s.c_str());
}

void CALL_callback(Fl_Input2 *w) {
	cb_call(w, NULL);
}

void NAME_callback(Fl_Input2 *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_NAME) s.erase(MAX_NAME);
	w->value(s.c_str());
}

void AZ_callback(Fl_Input2 *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_AZ) s.erase(MAX_AZ);
	w->value(s.c_str());
}

void QTH_callback(Fl_Input2 *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_QTH) s.erase(MAX_QTH);
	w->value(s.c_str());
}

void STATE_callback(Fl_Input2 *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_STATE) s.erase(MAX_STATE);
	w->value(s.c_str());
}

void VEPROV_callback(Fl_Input2 *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_STATE) s.erase(MAX_STATE);
	w->value(s.c_str());
}

void LOC_callback(Fl_Input2 *w) {
	cb_loc(w, NULL);
}

void SERNO_callback(Fl_Input2 *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_SERNO) s.erase(MAX_SERNO);
	w->value(s.c_str());
}

void XCHG_IN_callback(Fl_Input2 *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_XCHG_IN) s.erase(MAX_XCHG_IN);
	w->value(s.c_str());
}

void COUNTRY_callback(Fl_ComboBox *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_COUNTRY) s.erase(MAX_COUNTRY);
	if (country_test(s))
		w->value(country_match.c_str());
	else
		w->value(s.c_str());
}

void COUNTY_callback(Fl_Input2 *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_COUNTY) s.erase(MAX_COUNTY);
	w->value(s.c_str());
}

void NOTES_callback(Fl_Input2 *w) {
	std::string s;
	s = w->value();
	if (s.length() > MAX_NOTES) s.erase(MAX_NOTES);
	w->value(s.c_str());
}

void log_callback(Fl_Widget *w) {
	if (w == inpCall) {
		n3fjp_calltab = true;
		CALL_callback((Fl_Input2 *)w);
		DupCheck();
		return;
	}
	if (w == inpName) {
		NAME_callback((Fl_Input2 *)w);
		return;
	}
	if (w == cboCountry) {
		COUNTRY_callback((Fl_ComboBox *)w);
		return;
	}
	if (w == inpCounty) {
		COUNTY_callback((Fl_Input2 *)w);
		return;
	}
	if (w == inpNotes) {
		NOTES_callback((Fl_Input2 *)w);
		return;
	}
	if (w == inpLoc) {
		LOC_callback((Fl_Input2 *)w);
		return;
	}
	if (w == inpXchgIn) {
		XCHG_IN_callback((Fl_Input2 *)w);
		return;
	}
	if (w == inpState) {
		STATE_callback((Fl_Input2 *)w);
		return;
	}
	if (w == inpVEprov) {
		VEPROV_callback((Fl_Input2 *)w);
		return;
	}
	if (w == inpQTH) {
		QTH_callback((Fl_Input2 *)w);
		return;
	}
	if (w == inpAZ) {
		AZ_callback((Fl_Input2 *)w);
		return;
	}
	if (w == inpSerNo) {
		SERNO_callback((Fl_Input2 *)w);
		return;
	}
	if (w == inpRstIn) {
		RST_callback((Fl_Input2 *)w);
		return;
	}
	if (w == inpTimeOff || w == inpTimeOn) {
		TIME_callback((Fl_Input2 *)w);
		return;
	}
//	LOG_ERROR("unknown widget %p", w);
}

void cb_CountyQSO(Fl_Widget *)
{
	string sc = cboCountyQSO->value();
	if (sc.empty()) return;
	Cstates st;
	string ST = sc.substr(0,2);
	string CNTY = st.cnty_short(sc.substr(0,2), sc.substr(3));
	if (inpState) inpState->value(ST.c_str());
	if (inpState1) inpState1->value(ST.c_str());
	if (inp_CQstate1) inp_CQstate1->value(ST.c_str());
	if (inp_CQstate2) inp_CQstate2->value(ST.c_str());
	if (inpSQSO_state1) inpSQSO_state1->value(ST.c_str());
	if (inpSQSO_state2) inpSQSO_state2->value(ST.c_str());
	if (inpSQSO_county1) inpSQSO_county1->value(CNTY.c_str());
	if (inpSQSO_county2) inpSQSO_county2->value(CNTY.c_str());
}

void cb_meters(Fl_Widget *)
{
	if (!rigCAT_active()) return;
	pwrlevel_grp->show();
}

void cb_set_pwr_level(void *)
{
	rigCAT_set_pwrlevel((int)pwr_level->value());
}

void cb_exit_pwr_level(void*)
{
	pwrlevel_grp->hide();
}

#include "fl_digi_main.cxx"

void cb_mnuAltDockedscope(Fl_Menu_ *w, void *d);

static Fl_Menu_Item alt_menu_[] = {
{_("&File"), 0,  0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Exit"), log_out_icon), 'x',  (Fl_Callback*)cb_E, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{_("Op &Mode"), 0,  0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_CW].name, 0, cb_init_mode, (void *)MODE_CW, 0, FL_NORMAL_LABEL, 0, 14, 0},

{"Contestia", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_4_125].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_4_250].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_4_500].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_4_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_4_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_4_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_CONTESTIA_8_125].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_8_250].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_8_500].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_8_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_8_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_8_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_CONTESTIA_16_250].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_16_250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_16_500].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_16_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_16_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_16_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_16_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_16_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_CONTESTIA_32_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_32_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_32_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_32_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_CONTESTIA_64_500].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_64_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_64_1000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_64_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_CONTESTIA_64_2000].name, 0, cb_init_mode, (void *)MODE_CONTESTIA_64_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ _("Custom..."), 0, cb_contestiaCustom, (void *)MODE_CONTESTIA, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"DominoEX", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEXMICRO].name, 0, cb_init_mode, (void *)MODE_DOMINOEXMICRO, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX4].name, 0, cb_init_mode, (void *)MODE_DOMINOEX4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX5].name, 0, cb_init_mode, (void *)MODE_DOMINOEX5, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX8].name, 0, cb_init_mode, (void *)MODE_DOMINOEX8, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX11].name, 0, cb_init_mode, (void *)MODE_DOMINOEX11, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX16].name, 0, cb_init_mode, (void *)MODE_DOMINOEX16, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX22].name, 0, cb_init_mode, (void *)MODE_DOMINOEX22, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX44].name, 0,  cb_init_mode, (void *)MODE_DOMINOEX44, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_DOMINOEX88].name, 0,  cb_init_mode, (void *)MODE_DOMINOEX88, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"MFSK", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK4].name, 0,  cb_init_mode, (void *)MODE_MFSK4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK8].name, 0,  cb_init_mode, (void *)MODE_MFSK8, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK11].name, 0,  cb_init_mode, (void *)MODE_MFSK11, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK16].name, 0,  cb_init_mode, (void *)MODE_MFSK16, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK22].name, 0,  cb_init_mode, (void *)MODE_MFSK22, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK31].name, 0,  cb_init_mode, (void *)MODE_MFSK31, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK32].name, 0,  cb_init_mode, (void *)MODE_MFSK32, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK64].name, 0,  cb_init_mode, (void *)MODE_MFSK64, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK128].name, 0,  cb_init_mode, (void *)MODE_MFSK128, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK64L].name, 0,  cb_init_mode, (void *)MODE_MFSK64L, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MFSK128L].name, 0,  cb_init_mode, (void *)MODE_MFSK128L, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"MT63", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_500S].name, 0,  cb_init_mode, (void *)MODE_MT63_500S, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_500L].name, 0,  cb_init_mode, (void *)MODE_MT63_500L, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_1000S].name, 0,  cb_init_mode, (void *)MODE_MT63_1000S, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_1000L].name, 0,  cb_init_mode, (void *)MODE_MT63_1000L, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_2000S].name, 0,  cb_init_mode, (void *)MODE_MT63_2000S, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_MT63_2000L].name, 0,  cb_init_mode, (void *)MODE_MT63_2000L, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"OFDM", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OFDM_500F].name, 0,  cb_init_mode, (void *)MODE_OFDM_500F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OFDM_750F].name, 0,  cb_init_mode, (void *)MODE_OFDM_750F, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
//{ mode_info[MODE_OFDM_2000F].name, 0,  cb_init_mode, (void *)MODE_OFDM_2000F, 0, FL_NORMAL_LABEL, 0, 14, 0},
//{ mode_info[MODE_OFDM_2000].name, 0,  cb_init_mode, (void *)MODE_OFDM_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OFDM_3500].name, 0,  cb_init_mode, (void *)MODE_OFDM_3500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"Olivia", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_4_125].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_4_250].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_4_500].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_4_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_4_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_4_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_OLIVIA_8_125].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_8_250].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_8_500].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_8_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_8_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_8_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_OLIVIA_16_500].name, 0, cb_init_mode, (void *)MODE_OLIVIA_16_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_16_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_16_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_16_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_16_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_OLIVIA_32_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_32_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_32_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_32_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},

{ mode_info[MODE_OLIVIA_64_500].name, 0, cb_init_mode, (void *)MODE_OLIVIA_64_500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_64_1000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_64_1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_OLIVIA_64_2000].name, 0, cb_init_mode, (void *)MODE_OLIVIA_64_2000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ _("Custom..."), 0, cb_oliviaCustom, (void *)MODE_OLIVIA, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"PSK", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK31].name, 0, cb_init_mode, (void *)MODE_PSK31, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK63].name, 0, cb_init_mode, (void *)MODE_PSK63, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK63F].name, 0, cb_init_mode, (void *)MODE_PSK63F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK125].name, 0, cb_init_mode, (void *)MODE_PSK125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK250].name, 0, cb_init_mode, (void *)MODE_PSK250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK500].name, 0, cb_init_mode, (void *)MODE_PSK500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK1000].name, 0, cb_init_mode, (void *)MODE_PSK1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"MultiCarrier", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_12X_PSK125].name, 0, cb_init_mode, (void *)MODE_12X_PSK125, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_6X_PSK250].name, 0, cb_init_mode, (void *)MODE_6X_PSK250, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK500].name, 0, cb_init_mode, (void *)MODE_2X_PSK500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_4X_PSK500].name, 0, cb_init_mode, (void *)MODE_4X_PSK500, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK800].name, 0, cb_init_mode, (void *)MODE_2X_PSK800, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK1000].name, 0, cb_init_mode, (void *)MODE_2X_PSK1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0},

{"QPSK", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK31].name, 0, cb_init_mode, (void *)MODE_QPSK31, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK63].name, 0, cb_init_mode, (void *)MODE_QPSK63, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK125].name, 0, cb_init_mode, (void *)MODE_QPSK125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK250].name, 0, cb_init_mode, (void *)MODE_QPSK250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_QPSK500].name, 0, cb_init_mode, (void *)MODE_QPSK500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"8PSK", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK125].name, 0, cb_init_mode, (void *)MODE_8PSK125, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK250].name, 0, cb_init_mode, (void *)MODE_8PSK250, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK500].name, 0, cb_init_mode, (void *)MODE_8PSK500, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK1000].name, 0, cb_init_mode, (void *)MODE_8PSK1000, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK125FL].name, 0, cb_init_mode, (void *)MODE_8PSK125FL, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK125F].name, 0, cb_init_mode, (void *)MODE_8PSK125F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK250FL].name, 0, cb_init_mode, (void *)MODE_8PSK250FL, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK250F].name, 0, cb_init_mode, (void *)MODE_8PSK250F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK500F].name, 0, cb_init_mode, (void *)MODE_8PSK500F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK1000F].name, 0, cb_init_mode, (void *)MODE_8PSK1000F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_8PSK1200F].name, 0, cb_init_mode, (void *)MODE_8PSK1200F, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"PSKR", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK125R].name, 0, cb_init_mode, (void *)MODE_PSK125R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK250R].name, 0, cb_init_mode, (void *)MODE_PSK250R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK500R].name, 0, cb_init_mode, (void *)MODE_PSK500R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_PSK1000R].name, 0, cb_init_mode, (void *)MODE_PSK1000R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{"MultiCarrier", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_4X_PSK63R].name, 0, cb_init_mode, (void *)MODE_4X_PSK63R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_5X_PSK63R].name, 0, cb_init_mode, (void *)MODE_5X_PSK63R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_10X_PSK63R].name, 0, cb_init_mode, (void *)MODE_10X_PSK63R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_20X_PSK63R].name, 0, cb_init_mode, (void *)MODE_20X_PSK63R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_32X_PSK63R].name, 0, cb_init_mode, (void *)MODE_32X_PSK63R, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_4X_PSK125R].name, 0, cb_init_mode, (void *)MODE_4X_PSK125R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_5X_PSK125R].name, 0, cb_init_mode, (void *)MODE_5X_PSK125R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_10X_PSK125R].name, 0, cb_init_mode, (void *)MODE_10X_PSK125R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_12X_PSK125R].name, 0, cb_init_mode, (void *)MODE_12X_PSK125R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_16X_PSK125R].name, 0, cb_init_mode, (void *)MODE_16X_PSK125R, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK250R].name, 0, cb_init_mode, (void *)MODE_2X_PSK250R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_3X_PSK250R].name, 0, cb_init_mode, (void *)MODE_3X_PSK250R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_5X_PSK250R].name, 0, cb_init_mode, (void *)MODE_5X_PSK250R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_6X_PSK250R].name, 0, cb_init_mode, (void *)MODE_6X_PSK250R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_7X_PSK250R].name, 0, cb_init_mode, (void *)MODE_7X_PSK250R, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK500R].name, 0, cb_init_mode, (void *)MODE_2X_PSK500R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_3X_PSK500R].name, 0, cb_init_mode, (void *)MODE_3X_PSK500R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_4X_PSK500R].name, 0, cb_init_mode, (void *)MODE_4X_PSK500R, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK800R].name, 0, cb_init_mode, (void *)MODE_2X_PSK800R, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_2X_PSK1000R].name, 0, cb_init_mode, (void *)MODE_2X_PSK1000R, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0,0},

{ RTTY_MLABEL, 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ "RTTY-45", 0, cb_rtty45, (void *)MODE_RTTY, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "RTTY-50", 0, cb_rtty50, (void *)MODE_RTTY, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "RTTY-75N", 0, cb_rtty75N, (void *)MODE_RTTY, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ "RTTY-75W", 0, cb_rtty75W, (void *)MODE_RTTY, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ _("Custom..."), 0, cb_rttyCustom, (void *)MODE_RTTY, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"THOR", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THORMICRO].name, 0, cb_init_mode, (void *)MODE_THORMICRO, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR4].name, 0, cb_init_mode, (void *)MODE_THOR4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR5].name, 0, cb_init_mode, (void *)MODE_THOR5, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR8].name, 0, cb_init_mode, (void *)MODE_THOR8, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR11].name, 0, cb_init_mode, (void *)MODE_THOR11, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR16].name, 0, cb_init_mode, (void *)MODE_THOR16, FL_MENU_DIVIDER, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR22].name, 0, cb_init_mode, (void *)MODE_THOR22, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR25x4].name, 0,  cb_init_mode, (void *)MODE_THOR25x4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR50x1].name, 0,  cb_init_mode, (void *)MODE_THOR50x1, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR50x2].name, 0,  cb_init_mode, (void *)MODE_THOR50x2, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THOR100].name, 0,  cb_init_mode, (void *)MODE_THOR100, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{"Throb", 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROB1].name, 0, cb_init_mode, (void *)MODE_THROB1, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROB2].name, 0, cb_init_mode, (void *)MODE_THROB2, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROB4].name, 0, cb_init_mode, (void *)MODE_THROB4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROBX1].name, 0, cb_init_mode, (void *)MODE_THROBX1, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROBX2].name, 0, cb_init_mode, (void *)MODE_THROBX2, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_THROBX4].name, 0, cb_init_mode, (void *)MODE_THROBX4, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ mode_info[MODE_WWV].name, 0, cb_init_mode, (void *)MODE_WWV, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_NULL].name, 0, cb_init_mode, (void *)MODE_NULL, 0, FL_NORMAL_LABEL, 0, 14, 0},
{ mode_info[MODE_SSB].name, 0, cb_init_mode, (void *)MODE_SSB, 0, FL_NORMAL_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{_("&Configure"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Config Dialog")), 0, (Fl_Callback*)cb_mnu_config_dialog, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Test Signals")), 0, (Fl_Callback*)cb_mnuTestSignals, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Notifications")), 0,  (Fl_Callback*)cb_mnuConfigNotify, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Save Config"), save_icon), 0, (Fl_Callback*)cb_mnuSaveConfig, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{ VIEW_MLABEL, 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Rx Audio Dialog")), 'a', (Fl_Callback*)cb_mnuRxAudioDialog, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Signal Browser")), 0, (Fl_Callback*)cb_mnuViewer, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Spectrum scope")), 0, (Fl_Callback*)cb_mnuSpectrum, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Floating scope"), utilities_system_monitor_icon), 'f', (Fl_Callback*)cb_mnuDigiscope, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ DOCKEDSCOPE_MLABEL, 0, (Fl_Callback*)cb_mnuAltDockedscope, 0, FL_MENU_TOGGLE, FL_NORMAL_LABEL, 0, 14, 0},

{ icons::make_icon_label(MFSK_IMAGE_MLABEL, image_icon), 0, (Fl_Callback*)cb_mnuPicViewer, 0, FL_MENU_INACTIVE, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(THOR_IMAGE_MLABEL, image_icon), 0, (Fl_Callback*)cb_mnuThorViewRaw,0, FL_MENU_INACTIVE, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(IFKP_IMAGE_MLABEL, image_icon), 0, (Fl_Callback*)cb_mnuIfkpViewRaw,0, FL_MENU_INACTIVE | FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(WEFAX_TX_IMAGE_MLABEL, image_icon), 0, (Fl_Callback*)wefax_pic::cb_mnu_pic_viewer_tx,0, FL_MENU_INACTIVE | FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{_("&Help"), 0, 0, 0, FL_SUBMENU, FL_NORMAL_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Online documentation..."), help_browser_icon), 0, cb_mnuOnLineDOCS, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Event log"), dialog_information_icon), 0, cb_mnuDebug, 0, FL_MENU_DIVIDER, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("Check for updates..."), system_software_update_icon), 0, cb_mnuCheckUpdate, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{ icons::make_icon_label(_("&About"), help_about_icon), 'a', cb_mnuAboutURL, 0, 0, _FL_MULTI_LABEL, 0, 14, 0},
{0,0,0,0,0,0,0,0,0},

{0,0,0,0,0,0,0,0,0},
};

void cb_mnuAltDockedscope(Fl_Menu_ *w, void *d) {
	Fl_Menu_Item *m = getMenuItem(((Fl_Menu_*)w)->mvalue()->label(), alt_menu_);
	progStatus.DOCKEDSCOPE = m->value();
	wf->show_scope(progStatus.DOCKEDSCOPE);
}


#define defwidget 0, 0, 10, 10, ""

void noop_controls() // create and then hide all controls not being used
{
	Fl_Double_Window *dummywindow = new Fl_Double_Window(0,0,100,100,"");

	btnMacroTimer = new Fl_Button(defwidget); btnMacroTimer->hide();

	ReceiveText = new FTextRX(0,0,100,100); ReceiveText->hide();
	TransmitText = new FTextTX(0,0,100,100); TransmitText->hide();
	FHdisp = new Raster(0,0,10,100); FHdisp->hide();

	for (int i = 0; i < NUMMACKEYS * NUMKEYROWS; i++) {
		btnMacro[i] = new Fl_Button(defwidget); btnMacro[i]->hide();
	}
	for (int i = 0; i < 48; i++) {
		btnDockMacro[i] = new Fl_Button(defwidget); btnDockMacro[i]->hide();
	}

	inpQth = new Fl_Input2(defwidget); inpQth->hide();
	inpLoc1 = new Fl_Input2(defwidget); inpLoc1->hide();
	inpState1 = new Fl_Input2(defwidget); inpState1->hide();
	cboCountryQSO = new Fl_ComboBox(defwidget); cboCountryQSO->end();
	cboCountryQSO->hide();
	cboCountyQSO = new Fl_ComboBox(defwidget); cboCountyQSO->hide();
	cboCountyQSO->hide();
	inpSerNo = new Fl_Input2(defwidget); inpSerNo->hide();
	outSerNo = new Fl_Input2(defwidget); outSerNo->hide();
	inpXchgIn = new Fl_Input2(defwidget); inpXchgIn->hide();
	inpVEprov = new Fl_Input2(defwidget); inpVEprov->hide();
	inpNotes = new Fl_Input2(defwidget); inpNotes->hide();
	inpAZ = new Fl_Input2(defwidget); inpAZ->hide();

	qsoTime = new Fl_Button(defwidget); qsoTime->hide();
	btnQRZ = new Fl_Button(defwidget); btnQRZ->hide();
	qsoClear = new Fl_Button(defwidget); qsoClear->hide();
	qsoSave = new Fl_Button(defwidget); qsoSave->hide();

	qsoFreqDisp = new cFreqControl(0,0,80,20,""); qsoFreqDisp->hide();
	qso_opMODE = new Fl_ListBox(defwidget); qso_opMODE->hide();
	qso_opBW = new Fl_ListBox(defwidget); qso_opBW->hide();
	qso_opPICK = new Fl_Button(defwidget); qso_opPICK->hide();
	qso_opGROUP = new Fl_Group(defwidget); qso_opGROUP->hide();

	inpFreq = new Fl_Input2(defwidget); inpFreq->hide();
	inpTimeOff = new Fl_Input2(defwidget); inpTimeOff->hide();
	inpTimeOn = new Fl_Input2(defwidget); inpTimeOn->hide();
	btnTimeOn = new Fl_Button(defwidget); btnTimeOn->hide();
	inpCall = new Fl_Input2(defwidget); inpCall->hide();
	inpName = new Fl_Input2(defwidget); inpName->hide();
	inpRstIn = new Fl_Input2(defwidget); inpRstIn->hide();
	inpRstOut = new Fl_Input2(defwidget); inpRstOut->hide();

	inpFreq1 = new Fl_Input2(defwidget); inpFreq1->hide();
	inpTimeOff1 = new Fl_Input2(defwidget); inpTimeOff1->hide();
	inpTimeOn1 = new Fl_Input2(defwidget); inpTimeOn1->hide();
	btnTimeOn1 = new Fl_Button(defwidget); btnTimeOn1->hide();
	inpCall1 = new Fl_Input2(defwidget); inpCall1->hide();
	inpName1 = new Fl_Input2(defwidget); inpName1->hide();
	inpRstIn1 = new Fl_Input2(defwidget); inpRstIn1->hide();
	inpRstOut1 = new Fl_Input2(defwidget); inpRstOut1->hide();
	inpXchgIn1 = new Fl_Input2(defwidget); inpXchgIn1->hide();
	outSerNo1 = new Fl_Input2(defwidget); outSerNo1->hide();
	inpSerNo1 = new Fl_Input2(defwidget); inpSerNo1->hide();
	qsoFreqDisp1 = new cFreqControl(0,0,80,20,""); qsoFreqDisp1->hide();

	inp_FD_class1 = new Fl_Input2(defwidget); inp_FD_class1->hide();
	inp_FD_class2 = new Fl_Input2(defwidget); inp_FD_class2->hide();
	inp_FD_section1 = new Fl_Input2(defwidget); inp_FD_section1->hide();
	inp_FD_section2 = new Fl_Input2(defwidget); inp_FD_section2->hide();

	inp_KD_name2 = new Fl_Input2(defwidget); inp_KD_name2->hide();
	inp_KD_age1 = new  Fl_Input2(defwidget); inp_KD_age1->hide();
	inp_KD_age2 = new  Fl_Input2(defwidget); inp_KD_age2->hide();
	inp_KD_state1 = new  Fl_Input2(defwidget); inp_KD_state1->hide();
	inp_KD_state2 = new  Fl_Input2(defwidget); inp_KD_state2->hide();
	inp_KD_VEprov1 = new Fl_Input2(defwidget); inp_KD_VEprov1->hide();
	inp_KD_VEprov2 = new Fl_Input2(defwidget); inp_KD_VEprov2->hide();
	inp_KD_XchgIn1 = new  Fl_Input2(defwidget); inp_KD_XchgIn1->hide();
	inp_KD_XchgIn2 = new  Fl_Input2(defwidget); inp_KD_XchgIn2->hide();

	inp_1010_XchgIn1 = new Fl_Input2(defwidget); inp_1010_XchgIn1->hide();
	inp_1010_XchgIn2 = new Fl_Input2(defwidget); inp_1010_XchgIn2->hide();
	inp_1010_nr1 = new Fl_Input2(defwidget); inp_1010_nr1->hide();
	inp_1010_nr2 = new Fl_Input2(defwidget); inp_1010_nr2->hide();
	inp_1010_name2 = new Fl_Input2(defwidget); inp_1010_name2->hide();

	inp_ARR_Name2 = new Fl_Input2(defwidget); inp_ARR_Name2->hide();
	inp_ARR_XchgIn1 = new Fl_Input2(defwidget); inp_ARR_XchgIn1->hide();
	inp_ARR_XchgIn2 = new Fl_Input2(defwidget); inp_ARR_XchgIn2->hide();
	inp_ARR_check1 = new Fl_Input2(defwidget); inp_ARR_check1->hide();
	inp_ARR_check2 = new Fl_Input2(defwidget); inp_ARR_check2->hide();

	inp_vhf_Loc1 = new Fl_Input2(defwidget); inp_vhf_Loc1->hide();
	inp_vhf_Loc2 = new Fl_Input2(defwidget); inp_vhf_Loc2->hide();
	inp_vhf_RSTin1 = new Fl_Input2(defwidget); inp_vhf_RSTin1->hide();
	inp_vhf_RSTin2 = new Fl_Input2(defwidget); inp_vhf_RSTin2->hide();
	inp_vhf_RSTout1 = new Fl_Input2(defwidget); inp_vhf_RSTout1->hide();
	inp_vhf_RSTout2 = new Fl_Input2(defwidget); inp_vhf_RSTout2->hide();

	inp_CQ_RSTin2 = new Fl_Input2(defwidget); inp_CQ_RSTin2->hide();
	inp_CQ_RSTout2 = new Fl_Input2(defwidget); inp_CQ_RSTout2->hide();
	inp_CQstate1 = new Fl_Input2(defwidget); inp_CQstate1->hide();
	inp_CQstate2 = new Fl_Input2(defwidget); inp_CQstate2->hide();
	inp_CQzone1 = new Fl_Input2(defwidget); inp_CQzone1->hide();
	inp_CQzone2 = new Fl_Input2(defwidget); inp_CQzone2->hide();
	cboCountryCQ2 = new Fl_ComboBox(defwidget); cboCountryCQ2->end();
	cboCountryCQ2->hide();
	inp_CQDX_RSTin2 = new Fl_Input2(defwidget); inp_CQDX_RSTin2->hide();
	inp_CQDX_RSTout2 = new Fl_Input2(defwidget); inp_CQDX_RSTout2->hide();
	cboCountryCQDX2 = new Fl_ComboBox(defwidget); cboCountryCQDX2->end();
	cboCountryCQDX2->hide();
	inp_CQDXzone1 = new Fl_Input2(defwidget); inp_CQDXzone1->hide();
	inp_CQDXzone2 = new Fl_Input2(defwidget); inp_CQDXzone2->hide();

	inpTimeOff2 = new Fl_Input2(defwidget); inpTimeOff2->hide();
	inpTimeOn2 = new Fl_Input2(defwidget); inpTimeOn2->hide();
	btnTimeOn2 = new Fl_Button(defwidget); btnTimeOn2->hide();
	inpCall2 = new Fl_Input2(defwidget); inpCall2->hide();
	inpName2 = new Fl_Input2(defwidget); inpName2->hide();
	inpRstIn2 = new Fl_Input2(defwidget); inpRstIn2->hide();
	inpRstOut2 = new Fl_Input2(defwidget); inpRstOut2->hide();
	qsoFreqDisp2 = new cFreqControl(0,0,80,20,""); qsoFreqDisp2->hide();

	qso_opPICK2 = new Fl_Button(defwidget); qso_opPICK2->hide();
	qsoClear2 = new Fl_Button(defwidget); qsoClear2->hide();
	qsoSave2 = new Fl_Button(defwidget); qsoSave2->hide();
	btnQRZ2 = new Fl_Button(defwidget); btnQRZ2->hide();

	inpTimeOff3 = new Fl_Input2(defwidget); inpTimeOff3->hide();
	inpTimeOn3 = new Fl_Input2(defwidget); inpTimeOn3->hide();
	btnTimeOn3 = new Fl_Button(defwidget); btnTimeOn3->hide();
	inpCall3 = new Fl_Input2(defwidget); inpCall3->hide();
	outSerNo2 = new Fl_Input2(defwidget); outSerNo2->hide();
	inpSerNo2 = new Fl_Input2(defwidget); inpSerNo2->hide();
	inpXchgIn2 = new Fl_Input2(defwidget); inpXchgIn2->hide();
	qsoFreqDisp3 = new cFreqControl(0,0,80,20,""); qsoFreqDisp3->hide();

	inpTimeOff4 = new Fl_Input2(defwidget); inpTimeOff4->hide();
	inpTimeOn4 = new Fl_Input2(defwidget); inpTimeOn4->hide();
	btnTimeOn4 = new Fl_Button(defwidget); btnTimeOn4->hide();
	inpTimeOff5 = new Fl_Input2(defwidget); inpTimeOff5->hide();
	inpTimeOn5 = new Fl_Input2(defwidget); inpTimeOn5->hide();
	btnTimeOn5 = new Fl_Button(defwidget); btnTimeOn5->hide();

	outSerNo3 = new Fl_Input2(defwidget); outSerNo3->hide();
	inp_SS_SerialNoR1 = new Fl_Input2(defwidget); inp_SS_SerialNoR1->hide();
	inp_SS_Precedence1 = new Fl_Input2(defwidget); inp_SS_Precedence1->hide();
	inp_SS_Check1 = new Fl_Input2(defwidget); inp_SS_Check1->hide();
	inp_SS_Section1 = new Fl_Input2(defwidget); inp_SS_Section1->hide();
	outSerNo4 = new Fl_Input2(defwidget); outSerNo4->hide();
	inp_SS_SerialNoR2 = new Fl_Input2(defwidget); inp_SS_SerialNoR2->hide();
	inp_SS_Precedence2 = new Fl_Input2(defwidget); inp_SS_Precedence2->hide();
	inp_SS_Check2 = new Fl_Input2(defwidget); inp_SS_Check2->hide();
	inp_SS_Section2 = new Fl_Input2(defwidget); inp_SS_Section2->hide();

	inp_ASCR_class1 = new Fl_Input2(defwidget); inp_ASCR_class1->hide();
	inp_ASCR_XchgIn1 = new Fl_Input2(defwidget); inp_ASCR_XchgIn1->hide();
	inp_ASCR_name2 = new Fl_Input2(defwidget); inp_ASCR_name2->hide();
	inp_ASCR_class2 = new Fl_Input2(defwidget); inp_ASCR_class2->hide();
	inp_ASCR_XchgIn2 = new Fl_Input2(defwidget); inp_ASCR_XchgIn2->hide();
	inp_ASCR_RSTin2 = new Fl_Input2(defwidget); inp_ASCR_RSTin2->hide();
	inp_ASCR_RSTout2 = new Fl_Input2(defwidget); inp_ASCR_RSTout2->hide();

	inpNAQPname2 = new Fl_Input2(defwidget); inpNAQPname2->hide();
	inpSPCnum_NAQP1 =  new Fl_Input2(defwidget); inpSPCnum_NAQP1->hide();
	inpSPCnum_NAQP2 =  new Fl_Input2(defwidget); inpSPCnum_NAQP2->hide();

	inpRTU_stpr1 = new Fl_Input2(defwidget); inpRTU_stpr1->hide();
	inpRTU_serno1 = new Fl_Input2(defwidget); inpRTU_serno1->hide();
	inpRTU_RSTin2 = new Fl_Input2(defwidget); inpRTU_RSTin2->hide();
	inpRTU_RSTout2 = new Fl_Input2(defwidget); inpRTU_RSTout2->hide();
	inpRTU_stpr2 = new Fl_Input2(defwidget); inpRTU_stpr2->hide();
	inpRTU_serno2 = new Fl_Input2(defwidget); inpRTU_serno2->hide();
	cboCountryRTU2 = new Fl_ComboBox(defwidget); cboCountryRTU2->end();
	cboCountryRTU2->hide();

	inp_IARI_PR1 = new Fl_Input2(defwidget); inp_IARI_PR1->hide();
	inp_IARI_RSTin2 = new Fl_Input2(defwidget); inp_IARI_RSTin2->hide();
	inp_IARI_RSTout2 = new Fl_Input2(defwidget); inp_IARI_RSTout2->hide();
	out_IARI_SerNo1 = new Fl_Input2(defwidget); out_IARI_SerNo1->hide();
	inp_IARI_SerNo1 = new Fl_Input2(defwidget); inp_IARI_SerNo1->hide();
	out_IARI_SerNo2 = new Fl_Input2(defwidget); out_IARI_SerNo2->hide();
	inp_IARI_SerNo2 = new Fl_Input2(defwidget); inp_IARI_SerNo2->hide();
	inp_IARI_PR2 = new Fl_Input2(defwidget); inp_IARI_PR2->hide();
	cboCountryIARI2 = new Fl_ComboBox(defwidget); cboCountryIARI2->end();
	cboCountryIARI2->hide();

	outSerNo5 = new Fl_Input2(defwidget); outSerNo4->hide();
	inp_ser_NAS1 = new Fl_Input2(defwidget); inp_ser_NAS1->hide();
	inpSPCnum_NAS1 = new Fl_Input2(defwidget); inpSPCnum_NAS1->hide();
	outSerNo6 = new Fl_Input2(defwidget); outSerNo5->hide();
	inp_ser_NAS2 = new Fl_Input2(defwidget); inp_ser_NAS2->hide();
	inpSPCnum_NAS2 = new Fl_Input2(defwidget); inpSPCnum_NAS2->hide();
	inp_name_NAS2 = new Fl_Input2(defwidget); inp_name_NAS2->hide();

	outSerNo7 = new Fl_Input2(defwidget); outSerNo7->hide();
	inpSerNo3 = new Fl_Input2(defwidget); inpSerNo3->hide();
	cboCountryAIDX2 = new Fl_ComboBox(defwidget); cboCountryAIDX2->end();
	cboCountryAIDX2->hide();

	inpRstIn3 = new Fl_Input2(defwidget); inpRstIn3->hide();
	inpRstOut3 = new Fl_Input2(defwidget); inpRstOut3->hide();
	outSerNo8 = new Fl_Input2(defwidget); outSerNo8->hide();
	inpSerNo4 = new Fl_Input2(defwidget); inpSerNo4->hide();

	inp_JOTA_troop1 = new Fl_Input2(defwidget); inp_JOTA_troop1->hide();
	inp_JOTA_troop2 = new Fl_Input2(defwidget); inp_JOTA_troop2->hide();
	inp_JOTA_scout1 = new Fl_Input2(defwidget); inp_JOTA_scout1->hide();
	inp_JOTA_scout2 = new Fl_Input2(defwidget); inp_JOTA_scout2->hide();
	inp_JOTA_spc1 = new Fl_Input2(defwidget); inp_JOTA_spc1->hide();
	inp_JOTA_spc2 = new Fl_Input2(defwidget); inp_JOTA_spc2->hide();

	inpSPCnum_AICW1 = new Fl_Input2(defwidget); inpSPCnum_AICW1->hide();
	inpSPCnum_AICW2 = new Fl_Input2(defwidget); inpSPCnum_AICW2->hide();
	inpRstIn_AICW2 = new Fl_Input2(defwidget); inpRstIn_AICW2->hide();
	inpRstOut_AICW2 = new Fl_Input2(defwidget); inpRstOut_AICW2->hide();

	inpSQSO_state1 = new Fl_Input2(defwidget); inpSQSO_state1->hide();
	inpSQSO_state2 = new Fl_Input2(defwidget); inpSQSO_state2->hide();
	inpSQSO_county1 = new Fl_Input2(defwidget); inpSQSO_county1->hide();
	inpSQSO_county2 = new Fl_Input2(defwidget); inpSQSO_county2->hide();
	inpSQSO_serno1 = new Fl_Input2(defwidget); inpSQSO_serno1->hide();
	inpSQSO_serno2 = new Fl_Input2(defwidget); inpSQSO_serno2->hide();
	outSQSO_serno1 = new Fl_Input2(defwidget); outSQSO_serno1->hide();
	outSQSO_serno2 = new Fl_Input2(defwidget); outSQSO_serno2->hide();
	inpSQSO_name2 = new Fl_Input2(defwidget); inpSQSO_name2->hide();
	inpRstIn_SQSO2 = new Fl_Input2(defwidget); inpRstIn_SQSO2->hide();
	inpRstOut_SQSO2 = new Fl_Input2(defwidget); inpRstOut_SQSO2->hide();
	inpSQSO_category1 = new Fl_Input2(defwidget); inpSQSO_category1->hide();
	inpSQSO_category2 = new Fl_Input2(defwidget); inpSQSO_category2->hide();

	inpSerNo_WPX1 = new Fl_Input2(defwidget); inpSerNo_WPX1->hide();
	outSerNo_WPX1 = new Fl_Input2(defwidget); outSerNo_WPX1->hide();
	inpSerNo_WPX2 = new Fl_Input2(defwidget); inpSerNo_WPX2->hide();
	outSerNo_WPX2 = new Fl_Input2(defwidget); outSerNo_WPX2->hide();
	inpRstIn_WPX2 = new Fl_Input2(defwidget); inpRstIn_WPX2->hide();
	inpRstOut_WPX2 = new Fl_Input2(defwidget); inpRstOut_WPX2->hide();

	inpSerNo_WAE1 = new Fl_Input2(defwidget); inpSerNo_WAE1->hide();
	inpSerNo_WAE2 = new Fl_Input2(defwidget); inpSerNo_WAE2->hide();
	outSerNo_WAE1 = new Fl_Input2(defwidget); outSerNo_WAE1->hide();
	outSerNo_WAE2 = new Fl_Input2(defwidget); outSerNo_WAE2->hide();
	inpRstIn_WAE2 = new Fl_Input2(defwidget); inpRstIn_WAE2->hide();
	inpRstOut_WAE2 = new Fl_Input2(defwidget); inpRstOut_WAE2->hide();
	cboCountryWAE2 = new Fl_ComboBox(defwidget); cboCountryWAE2->end();
	cboCountryWAE2->hide();

	qso_opPICK3 = new Fl_Button(defwidget); qso_opPICK3->hide();
	qsoClear3 = new Fl_Button(defwidget); qsoClear3->hide();
	qsoSave3 = new Fl_Button(defwidget); qsoSave3->hide();

	inpCall4 = new Fl_Input2(defwidget); inpCall4->hide();

	qso_opBrowser = new Fl_Browser(defwidget); qso_opBrowser->hide();
	qso_btnAddFreq = new Fl_Button(defwidget); qso_btnAddFreq->hide();
	qso_btnSelFreq = new Fl_Button(defwidget); qso_btnSelFreq->hide();
	qso_btnDelFreq = new Fl_Button(defwidget); qso_btnDelFreq->hide();
	qso_btnClearList = new Fl_Button(defwidget); qso_btnClearList->hide();
	qso_btnAct = new Fl_Button(defwidget); qso_btnAct->hide();
	qso_inpAct = new Fl_Input2(defwidget); qso_inpAct->hide();

	pwrmeter = new PWRmeter(defwidget); pwrmeter->hide();
	smeter = new Smeter(defwidget); smeter->hide();
	pwr_level = new Fl_Value_Slider2(defwidget); pwr_level->hide();
	set_pwr_level = new Fl_Button(defwidget); set_pwr_level->hide();

	dummywindow->end();
	dummywindow->hide();

}

void make_scopeviewer()
{
	scopeview = new Fl_Double_Window(0,0,140,140, _("Scope"));
	scopeview->xclass(PACKAGE_NAME);
	digiscope = new Digiscope (0, 0, 140, 140);
	scopeview->resizable(digiscope);
	scopeview->size_range(SCOPEWIN_MIN_WIDTH, SCOPEWIN_MIN_HEIGHT);
	scopeview->end();
	scopeview->hide();
}

static int WF_only_height = 0;

void create_fl_digi_main_WF_only() {

	int fnt = fl_font();
	int fsize = fl_size();
	int freqheight = Hentry + 2 * pad;
	int Y = 0;
	int W = progStatus.mainW;

	fl_font(fnt, freqheight);
	fl_font(fnt, fsize);


	IMAGE_WIDTH = 4000;
	Hwfall = progdefaults.wfheight;
	Wwfall = W - 2 * DEFAULT_SW - 2 * pad;
	WF_only_height = Hmenu + Hwfall + Hstatus + 4 * pad;

	fl_digi_main = new Fl_Double_Window(W, WF_only_height);

		mnuFrame = new Fl_Group(0, 0, W, Hmenu);

			mnu = new Fl_Menu_Bar(0, 0, W - 275 - pad, Hmenu);
// do some more work on the menu
			for (size_t i = 0; i < sizeof(alt_menu_)/sizeof(alt_menu_[0]); i++) {
// FL_NORMAL_SIZE may have changed; update the menu items
				if (alt_menu_[i].text) {
					alt_menu_[i].labelsize_ = FL_NORMAL_SIZE;
				}
// set the icon label for items with the multi label type
				if (alt_menu_[i].labeltype() == _FL_MULTI_LABEL)
					icons::set_icon_label(&alt_menu_[i]);
			}
			mnu->menu(alt_menu_);

			tx_timer = new Fl_Box(W - 275 - pad, 0, 75 - pad, Hmenu, "");
			tx_timer->box(FL_UP_BOX);
			tx_timer->color(FL_BACKGROUND_COLOR);
			tx_timer->labelcolor(FL_BACKGROUND_COLOR);

			btnAutoSpot = new Fl_Light_Button(W - 200 - pad, 0, 50, Hmenu, "Spot");
			btnAutoSpot->selection_color(progdefaults.SpotColor);
			btnAutoSpot->callback(cbAutoSpot, 0);
			btnAutoSpot->deactivate();

			btnRSID = new Fl_Light_Button(W - 150 - pad, 0, 50, Hmenu, "RxID");
			btnRSID->tooltip("Receive RSID");
			btnRSID->value(progdefaults.rsid);
			btnRSID->callback(cbRSID, 0);

			btnTxRSID = new Fl_Light_Button(W - 100 - pad, 0, 50, Hmenu, "TxID");
			btnTxRSID->selection_color(progdefaults.TxIDColor);
			btnTxRSID->tooltip("Transmit RSID");
			btnTxRSID->callback(cbTxRSID, 0);

			btnTune = new Fl_Light_Button(W - 50 - pad, 0, 50, Hmenu, "TUNE");
			btnTune->selection_color(progdefaults.TuneColor);
			btnTune->callback(cbTune, 0);

		mnuFrame->resizable(mnu);
		mnuFrame->end();

		Y = Hmenu + pad;

		wf_group = new Fl_Group(0, Y, W, Hwfall);

			wf = new waterfall(0, Y, Wwfall, Hwfall);
			wf->end();

			pgrsSquelch = new Progress(
				rightof(wf), Y + pad,
				DEFAULT_SW, Hwfall - 2 * pad,
				"");
			pgrsSquelch->color(FL_BACKGROUND2_COLOR, FL_DARK_GREEN);
			pgrsSquelch->type(Progress::VERTICAL);
			pgrsSquelch->tooltip(_("Detected signal level"));

			sldrSquelch = new Fl_Slider2(
				rightof(pgrsSquelch), Y + pad,
				DEFAULT_SW, Hwfall - 2 * pad,
				"");
			sldrSquelch->minimum(100);
			sldrSquelch->maximum(0);
			sldrSquelch->step(1);
			sldrSquelch->value(progStatus.sldrSquelchValue);
			sldrSquelch->callback((Fl_Callback*)cb_sldrSquelch);
			sldrSquelch->color(FL_INACTIVE_COLOR);
			sldrSquelch->tooltip(_("Squelch level"));
			Fl_Group::current()->resizable(wf);
		wf_group->end();

		Y += (Hwfall + pad);

		status_group = new Fl_Group(0, Y, W, Hstatus);

			MODEstatus = new Fl_Button(
				0, Y,
				Wmode, Hstatus, "");
			MODEstatus->box(FL_DOWN_BOX);
			MODEstatus->color(FL_BACKGROUND2_COLOR);
			MODEstatus->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			MODEstatus->callback(status_cb, (void *)0);
			MODEstatus->when(FL_WHEN_CHANGED);
			MODEstatus->tooltip(_("Left click: change mode\nRight click: configure"));

			cntCW_WPM = new Fl_Counter2(
				rightof(MODEstatus), Y,
				Ws2n - Hstatus, Hstatus, "");
			cntCW_WPM->callback(cb_cntCW_WPM);
			cntCW_WPM->minimum(progdefaults.CWlowerlimit);
			cntCW_WPM->maximum(progdefaults.CWupperlimit);
			cntCW_WPM->value(progdefaults.CWspeed);
			cntCW_WPM->tooltip(_("CW transmit WPM"));
			cntCW_WPM->type(1);
			cntCW_WPM->step(1);
			cntCW_WPM->hide();

			btnCW_Default = new Fl_Button(
				rightof(cntCW_WPM), Y,
				Hstatus, Hstatus, "*");
			btnCW_Default->callback(cb_btnCW_Default);
			btnCW_Default->tooltip(_("Default WPM"));
			btnCW_Default->hide();

			Status1 = new Fl_Box(
				rightof(MODEstatus), Y,
				Ws2n, Hstatus, "");
			Status1->box(FL_DOWN_BOX);
			Status1->color(FL_BACKGROUND2_COLOR);
			Status1->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			Status2 = new Fl_Box(
				rightof(Status1), Y,
				Wimd, Hstatus, "");
			Status2->box(FL_DOWN_BOX);
			Status2->color(FL_BACKGROUND2_COLOR);
			Status2->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

			StatusBar = new status_box(
				rightof(Status2), Y,
				W - rightof(Status2)
				- bwAfcOnOff - bwSqlOnOff
				- Wwarn - bwTxLevel
				- bwSqlOnOff
				- cbwidth,
				Hstatus, "");
			StatusBar->box(FL_DOWN_BOX);
			StatusBar->color(FL_BACKGROUND2_COLOR);
			StatusBar->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
			StatusBar->callback((Fl_Callback *)StatusBar_cb);
			StatusBar->when(FL_WHEN_RELEASE_ALWAYS);
			StatusBar->tooltip(_("Left click to toggle VuMeter"));
			StatusBar->show();

			VuMeter = new vumeter(StatusBar->x(), StatusBar->y(), StatusBar->w(), StatusBar->h(), "" );
			VuMeter->align(Fl_Align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE));
			VuMeter->when(FL_WHEN_RELEASE_ALWAYS);
			VuMeter->tooltip(_("Left click to toggle Status Bar"));
			VuMeter->callback((Fl_Callback *)VuMeter_cb);

			cntTxLevel = new Fl_Counter2(
				rightof(StatusBar), Y,
				bwTxLevel, Hstatus, "");
			cntTxLevel->minimum(-30);
			cntTxLevel->maximum(0);
			cntTxLevel->value(-6);
			cntTxLevel->callback((Fl_Callback*)cb_cntTxLevel);
			cntTxLevel->value(progdefaults.txlevel);
			cntTxLevel->lstep(1.0);
			cntTxLevel->tooltip(_("Tx level attenuator (dB)"));

			WARNstatus = new Fl_Box(
				rightof(cntTxLevel), Y,
				Wwarn, Hstatus, "");
			WARNstatus->box(FL_DIAMOND_DOWN_BOX);
			WARNstatus->color(FL_BACKGROUND_COLOR);
			WARNstatus->labelcolor(FL_RED);
			WARNstatus->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

			btnAFC = new Fl_Light_Button(
				rightof(WARNstatus), Y,
				bwAfcOnOff, Hstatus, "AFC");
			btnAFC->selection_color(progdefaults.AfcColor);

			btnSQL = new Fl_Light_Button(
				rightof(btnAFC), Y,
				bwSqlOnOff, Hstatus, "SQL");

// btnPSQL will be resized later depending on the state of the
// configuration parameter to show that widget

			btnPSQL = new Fl_Light_Button(
				rightof(btnSQL), Y,
				bwSqlOnOff, Hstatus, "PSM");

			btnAFC->callback(cbAFC, 0);
			btnAFC->value(1);
			btnAFC->tooltip(_("Automatic Frequency Control"));

			btnSQL->callback(cbSQL, 0);
			btnSQL->selection_color(progdefaults.Sql1Color);
			btnSQL->value(1);
			btnSQL->tooltip(_("Squelch"));

			btnPSQL->selection_color(progdefaults.Sql1Color);
			btnPSQL->callback(cbPwrSQL, 0);
			btnPSQL->value(progdefaults.kpsql_enabled);
			btnPSQL->tooltip(_("Power Signal Monitor"));

			Fl_Group::current()->resizable(VuMeter);

		status_group->end();

	Fl::add_handler(wo_default_handler);

	fl_digi_main->end();
	fl_digi_main->callback(cb_wMain);
	fl_digi_main->resizable(wf);

	const struct {
		bool var; const char* label;
	} toggles[] = {
		{ progStatus.DOCKEDSCOPE, DOCKEDSCOPE_MLABEL }
	};
	Fl_Menu_Item* toggle;
	for (size_t i = 0; i < sizeof(toggles)/sizeof(*toggles); i++) {
		if (toggles[i].var && (toggle = getMenuItem(toggles[i].label, alt_menu_))) {
			toggle->set();
			if (toggle->callback()) {
				mnu->value(toggle);
				toggle->do_callback(reinterpret_cast<Fl_Widget*>(mnu));
			}
		}
	}

	make_scopeviewer();
	noop_controls();

	progdefaults.WF_UIwfcarrier =
	progdefaults.WF_UIwfreflevel =
	progdefaults.WF_UIwfampspan =
	progdefaults.WF_UIwfmode =
	progdefaults.WF_UIx1 =
	progdefaults.WF_UIwfshift =
	progdefaults.WF_UIrev =
	progdefaults.WF_UIwfstore =
	progdefaults.WF_UIxmtlock =
	progdefaults.WF_UIwfdrop = true;
	progdefaults.WF_UIqsy = false;
	wf->UI_select(true);

	load_counties();

	createConfig();

	createRecordLoader();

	if (rx_only) {
		btnTune->deactivate();
		wf->xmtrcv->deactivate();
	}

	UI_select();

}


void create_fl_digi_main(int argc, char** argv)
{
	if (bWF_only)
		create_fl_digi_main_WF_only();
	else
		create_fl_digi_main_primary();

#if defined(__WOE32__)
#  ifndef IDI_ICON
#    define IDI_ICON 101
#  endif
	fl_digi_main->icon((char*)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON)));
#elif !defined(__APPLE__) && USE_X
	make_pixmap(&fldigi_icon_pixmap, fldigi_icon, argc, argv);
	fl_digi_main->icon((char *)fldigi_icon_pixmap);
#endif

	fl_digi_main->xclass(PACKAGE_NAME);

	if (bWF_only)
		fl_digi_main->size_range(WMIN, WF_only_height, 0, WF_only_height);
	else
		fl_digi_main->size_range(WMIN, main_hmin, 0, 0);

	set_colors();

}

void put_freq(double frequency)
{
	wf->carrier((int)floor(frequency + 0.5));
}

void put_Bandwidth(int bandwidth)
{
	wf->Bandwidth ((int)bandwidth);
}

void callback_set_metric(double metric)
{
	pgrsSquelch->value(metric);

	if (active_modem->get_mode() == MODE_FSQ)
		ind_fsq_s2n->value(metric);

	if (active_modem->get_mode() == MODE_IFKP)
		ifkp_s2n_progress->value(metric);

	if (progdefaults.show_psm_btn && progStatus.kpsql_enabled) {
		if ((metric >= progStatus.sldrPwrSquelchValue) || inhibit_tx_seconds)
			btnPSQL->selection_color(progdefaults.Sql2Color);
		else
			btnPSQL->selection_color(progdefaults.Sql1Color);

		btnPSQL->redraw_label();

	} else if(progStatus.sqlonoff) {
		if (metric < progStatus.sldrSquelchValue)
			btnSQL->selection_color(progdefaults.Sql1Color);
		else
			btnSQL->selection_color(progdefaults.Sql2Color);
		btnSQL->redraw_label();
	}
}

void put_cwRcvWPM(double wpm)
{
	int U = progdefaults.CWupperlimit;
	int L = progdefaults.CWlowerlimit;
	double dWPM = 100.0*(wpm - L)/(U - L);
	REQ_DROP(static_cast<void (Fl_Progress::*)(float)>(&Fl_Progress::value), prgsCWrcvWPM, dWPM);
	REQ_DROP(static_cast<int (Fl_Value_Output::*)(double)>(&Fl_Value_Output::value), valCWrcvWPM, (int)wpm);
}

void set_scope_mode(Digiscope::scope_mode md)
{
	if (digiscope) {
		digiscope->mode(md);
		REQ(&Fl_Window::size_range, scopeview, SCOPEWIN_MIN_WIDTH, SCOPEWIN_MIN_HEIGHT,
			0, 0, 0, 0, (md == Digiscope::PHASE || md == Digiscope::XHAIRS));
	}
	wf->wfscope->mode(md);
	if (md == Digiscope::SCOPE) set_scope_clear_axis();
}

void set_scope(double *data, int len, bool autoscale)
{
	if (digiscope)
		digiscope->data(data, len, autoscale);
	wf->wfscope->data(data, len, autoscale);
}

void set_phase(double phase, double quality, bool highlight)
{
	if (digiscope)
		digiscope->phase(phase, quality, highlight);
	wf->wfscope->phase(phase, quality, highlight);
}

void set_rtty(double flo, double fhi, double amp)
{
	if (digiscope)
		digiscope->rtty(flo, fhi, amp);
	wf->wfscope->rtty(flo, fhi, amp);
}

void set_video(double *data, int len, bool dir)
{
	if (digiscope)
		digiscope->video(data, len, dir);
	wf->wfscope->video(data, len, dir);
}

void set_zdata(cmplx *zarray, int len)
{
	if (digiscope)
		digiscope->zdata(zarray, len);
	wf->wfscope->zdata(zarray, len);
}

void set_scope_xaxis_1(double y1)
{
	if (digiscope)
		digiscope->xaxis_1(y1);
	wf->wfscope->xaxis_1(y1);
}

void set_scope_xaxis_2(double y2)
{
	if (digiscope)
		digiscope->xaxis_2(y2);
	wf->wfscope->xaxis_2(y2);
}

void set_scope_yaxis_1(double x1)
{
	if (digiscope)
		digiscope->yaxis_1(x1);
	wf->wfscope->yaxis_1(x1);
}

void set_scope_yaxis_2(double x2)
{
	if (digiscope)
		digiscope->yaxis_2(x2);
	wf->wfscope->yaxis_2(x2);
}

void set_scope_clear_axis()
{
	if (digiscope) {
		digiscope->xaxis_1(0);
		digiscope->xaxis_2(0);
		digiscope->yaxis_1(0);
		digiscope->yaxis_2(0);
	}
	wf->wfscope->xaxis_1(0);
	wf->wfscope->xaxis_2(0);
	wf->wfscope->yaxis_1(0);
	wf->wfscope->yaxis_2(0);
}

// raw buffer functions can ONLY be called by FLMAIN_TID

//======================================================================
#define RAW_BUFF_LEN 4096

static char rxtx_raw_chars[RAW_BUFF_LEN+1] = "";
static char rxtx_raw_buff[RAW_BUFF_LEN+1] = "";
static int  rxtx_raw_len = 0;

char *get_rxtx_data()
{
	ENSURE_THREAD(FLMAIN_TID);
	memset(rxtx_raw_chars, 0, RAW_BUFF_LEN+1);
	strcpy(rxtx_raw_chars, rxtx_raw_buff);
	memset(rxtx_raw_buff, 0, RAW_BUFF_LEN+1);
	rxtx_raw_len = 0;
	return rxtx_raw_chars;
}

void add_rxtx_char(int data)
{
	ENSURE_THREAD(FLMAIN_TID);
	if (rxtx_raw_len == RAW_BUFF_LEN) {
		memset(rxtx_raw_buff, 0, RAW_BUFF_LEN+1);
		rxtx_raw_len = 0;
	}
	rxtx_raw_buff[rxtx_raw_len++] = (unsigned char)data;
}

//======================================================================
static char rx_raw_chars[RAW_BUFF_LEN+1] = "";
static char rx_raw_buff[RAW_BUFF_LEN+1] = "";
static int  rx_raw_len = 0;

static pthread_mutex_t rx_data_mutex = PTHREAD_MUTEX_INITIALIZER;

char *get_rx_data()
{
//	ENSURE_THREAD(FLMAIN_TID);
	
	guard_lock datalock(&rx_data_mutex);
	memset(rx_raw_chars, 0, RAW_BUFF_LEN+1);
	strcpy(rx_raw_chars, rx_raw_buff);
	memset(rx_raw_buff, 0, RAW_BUFF_LEN+1);
	rx_raw_len = 0;
	return rx_raw_chars;
}

void add_rx_char(int data)
{
//	ENSURE_THREAD(FLMAIN_TID);

	guard_lock datalock(&rx_data_mutex);
	add_rxtx_char(data);
	if (rx_raw_len == RAW_BUFF_LEN) {
		memset(rx_raw_buff, 0, RAW_BUFF_LEN+1);
		rx_raw_len = 0;
	}
	rx_raw_buff[rx_raw_len++] = (unsigned char)data;
}

//======================================================================
static char tx_raw_chars[RAW_BUFF_LEN+1] = "";
static char tx_raw_buff[RAW_BUFF_LEN+1] = "";
static int  tx_raw_len = 0;

char *get_tx_data()
{
	ENSURE_THREAD(FLMAIN_TID);
	memset(tx_raw_chars, 0, RAW_BUFF_LEN+1);
	strcpy(tx_raw_chars, tx_raw_buff);
	memset(tx_raw_buff, 0, RAW_BUFF_LEN+1);
	tx_raw_len = 0;
	return tx_raw_chars;
}

void add_tx_char(int data)
{
	ENSURE_THREAD(FLMAIN_TID);
	add_rxtx_char(data);
	if (tx_raw_len == RAW_BUFF_LEN) {
		memset(tx_raw_buff, 0, RAW_BUFF_LEN+1);
		tx_raw_len = 0;
	}
	tx_raw_buff[tx_raw_len++] = (unsigned char)data;
}

//======================================================================
static void TTY_bell()
{
	if (progdefaults.audibleBELL)
		audio_alert->alert(progdefaults.BELL_RING);
}

static void display_rx_data(const unsigned char data, int style)
{
	if (data != '\r') {
		if (active_modem->get_mode() == MODE_FSQ)
			fsq_rx_text->add(data,style);
		else if (active_modem->get_mode() == MODE_IFKP)
			ifkp_rx_text->add(data,style);
		else
			ReceiveText->add(data, style);
	}

	if (bWF_only) return;

	speak(data);

	if (Maillogfile)
		Maillogfile->log_to_file(cLogfile::LOG_RX, string(1, (const char)data));

	if (progStatus.LOGenabled)
		logfile->log_to_file(cLogfile::LOG_RX, string(1, (const char)data));
}

static void rx_parser(const unsigned char data, int style)
{
	// assign a style to the incoming data
	if (extract_wrap || extract_flamp)
		style = FTextBase::RECV;
	if ((data < ' ') && iscntrl(data))
		style = FTextBase::CTRL;
	if (wf->tmp_carrier())
		style = FTextBase::ALTR;

	// Collapse the "\r\n" sequence into "\n".
	//
	// The 'data' variable possibly contains only a part of a multi-byte
	// UTF-8 character. This is not a problem. All data has passed
	// through a distiller before we got here, so we can be sure that
	// the input is valid UTF-8. All bytes of a multi-byte character
	// will therefore have the eight bit set and can not match either
	// '\r' or '\n'.

	static unsigned int lastdata = 0;
	if (data == '\n' && lastdata == '\r');
	else if (data == '\r') {
		display_rx_data('\n', style);
	} else {
		display_rx_data(data, style);
	}
	lastdata = data;

	if (!(data < ' ' && iscntrl(data)) && progStatus.spot_recv)
		spot_recv(data);
}

static void put_rx_char_flmain(unsigned int data, int style)
{
	ENSURE_THREAD(FLMAIN_TID);

	// possible destinations for the data
	enum dest_type {
		DEST_RECV,	// ordinary received text
		DEST_ALTR	// alternate received text
	};
	static enum dest_type destination = DEST_RECV;
	static enum dest_type prev_destination = DEST_RECV;

	// Determine the destination of the incoming data. If the destination had
	// changed, clear the contents of the distiller.
	destination = (wf->tmp_carrier() ? DEST_ALTR : DEST_RECV);

	if (destination != prev_destination) {
		rx_chd.reset();
		rx_chd.clear();
	}

	// select a byte translation table
	trx_mode mode = active_modem->get_mode();

	add_rx_char(data & 0xFF);

	if (mailclient || mailserver)
		rx_chd.rx((unsigned char *)ascii2[data & 0xFF]);

	else if (progdefaults.show_all_codes && iscntrl(data & 0xFF))
		rx_chd.rx((unsigned char *)ascii3[data & 0xFF]);

	else if (mode == MODE_RTTY)
		if (data == '\a') {
			if (progdefaults.visibleBELL)
				rx_chd.rx((unsigned char *)ascii2[7]);
			REQ(TTY_bell);
		} else
			rx_chd.rx((unsigned char *)ascii[data & 0xFF]);
	else
		rx_chd.rx(data & 0xFF);

	// feed the decoded data into the RX parser
	if (rx_chd.data_length() > 0) {
		const char *ptr = rx_chd.data().data();
		const char *end = ptr + rx_chd.data_length();
		while (ptr < end)
			rx_parser((const unsigned char)*ptr++, style);
		rx_chd.clear();
	}
}

static std::string rx_process_buf = "";
static std::string tx_process_buf = "";
static pthread_mutex_t rx_proc_mutex = PTHREAD_MUTEX_INITIALIZER;

void put_rx_processed_char(unsigned int data, int style)
{
	guard_lock rx_proc_lock(&rx_proc_mutex);

	if(style == FTextBase::XMIT) {
		tx_process_buf += (char) (data & 0xff);
	} else if(style == FTextBase::RECV) {
		rx_process_buf += (char) (data & 0xff);
	}
}

void disp_rx_processed_char(void)
{
	guard_lock rx_proc_lock(&rx_proc_mutex);
	unsigned int index = 0;

	if(!rx_process_buf.empty()) {
		unsigned int count = rx_process_buf.size();
		for(index = 0; index < count; index++)
			REQ(put_rx_char_flmain, rx_process_buf[index], FTextBase::RECV);
		rx_process_buf.clear();
	}

	if(!tx_process_buf.empty()) {
		unsigned int count = tx_process_buf.size();
		for(index = 0; index < count; index++)
			REQ(put_rx_char_flmain, tx_process_buf[index], FTextBase::XMIT);
		tx_process_buf.clear();
	}
}

void put_rx_char(unsigned int data, int style)
{
#if BENCHMARK_MODE
	if (!benchmark.output.empty()) {
		if (unlikely(benchmark.buffer.length() + 16 > benchmark.buffer.capacity()))
			benchmark.buffer.reserve(benchmark.buffer.capacity() + BUFSIZ);
		benchmark.buffer += (char)data;
	}
#else
	if (progdefaults.autoextract == true)
		rx_extract_add(data);

	if (active_modem->get_mode() == MODE_FSQ) {
		REQ(put_rx_char_flmain, data, style);
		return;
	}

	switch(data_io_enabled) {
		case ARQ_IO:
			WriteARQ(data);
			break;
		case KISS_IO:
			WriteKISS(data);
			break;
	}

	if(progdefaults.ax25_decode_enabled && data_io_enabled == KISS_IO)
		disp_rx_processed_char();
	else
		REQ(put_rx_char_flmain, data, style);
#endif
}

static string strSecText = "";

static void put_sec_char_flmain(char chr)
{
	ENSURE_THREAD(FLMAIN_TID);

	fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
	char s[2] = "W";
	int lc = (int)ceil(fl_width(s));
	int w = StatusBar->w();
	int lw = (int)ceil(fl_width(StatusBar->label()));
	int over = 2 * lc + lw - w;

	if (chr >= ' ' && chr <= 'z') {
		if ( over > 0 )
			strSecText.erase(0, (int)(1.0 * over / lc + 0.5));
		strSecText.append(1, chr);
		StatusBar->label(strSecText.c_str());
		WARNstatus->damage();
	}
}

void put_sec_char(char chr)
{
	REQ(put_sec_char_flmain, chr);
}

static void clear_status_cb(void* arg)
{
	reinterpret_cast<Fl_Box*>(arg)->label("");
}
static void dim_status_cb(void* arg)
{
	reinterpret_cast<Fl_Box*>(arg)->deactivate();
}

static void (*const timeout_action[STATUS_NUM])(void*) = { clear_status_cb, dim_status_cb };

struct PSM_STRUCT {
	Fl_Widget *w;
	double timeout;
	status_timeout action;
	std::string msg;
};

void put_status_msg(void *d)
{
	PSM_STRUCT *psm = (PSM_STRUCT *)d;

	psm->w->activate();
	psm->w->label(psm->msg.c_str());
	if (psm->timeout > 0.0) {
		Fl::remove_timeout(timeout_action[psm->action], psm->w);
		Fl::add_timeout(psm->timeout, timeout_action[psm->action], psm->w);
	}
}

void put_status(const char *msg, double timeout, status_timeout action)
{
	static PSM_STRUCT ps;
	ps.msg.clear();
	ps.msg.assign(msg);
	ps.timeout = timeout;
	ps.action = action;
	ps.w = StatusBar;
	Fl::awake(put_status_msg, (void *)&ps);
}

void put_Status2(const char *msg, double timeout, status_timeout action)
{
	static PSM_STRUCT ps;
	ps.msg.clear();
	ps.msg.assign(msg);
	ps.timeout = timeout;
	ps.action = action;
	ps.w = Status2;

	info2msg = msg;

	Fl::awake(put_status_msg, (void *)&ps);
}

void put_Status1(const char *msg, double timeout, status_timeout action)
{
	static PSM_STRUCT ps;
	ps.msg.clear();
	ps.msg.assign(msg);
	ps.timeout = timeout;
	ps.action = action;
	ps.w = Status1;

	info1msg = msg;

	if (!active_modem) return;
	if (progStatus.NO_RIGLOG && active_modem->get_mode() != MODE_FSQ) return;

	Fl::awake(put_status_msg, (void *)&ps);

}

void put_WARNstatus(double v)
{
	sig_vumeter->value(v);
	sig_vumeter2->value(v);
	VuMeter->value(v);

	double val = 20 * log10(v == 0 ? 1e-9 : v);
	if (val < progdefaults.normal_signal_level)
		WARNstatus->color(progdefaults.LowSignal);
	else if (val < progdefaults.high_signal_level )
		WARNstatus->color(progdefaults.NormSignal);
	else if (val < progdefaults.over_signal_level)
		WARNstatus->color(progdefaults.HighSignal);
	else WARNstatus->color(progdefaults.OverSignal);
	WARNstatus->redraw();
}


void set_CWwpm()
{
	if (sldrCWxmtWPM)
		sldrCWxmtWPM->value(progdefaults.CWspeed);
	if (cntCW_WPM)
		cntCW_WPM->value(progdefaults.CWspeed);
	if (use_nanoIO) set_nanoWPM(progdefaults.CWspeed);
}

void clear_StatusMessages()
{
	StatusBar->label("");
	Status1->label("");
	Status2->label("");
	info1msg = "";
	info2msg = "";
}

void put_MODEstatus(const char* fmt, ...)
{
	static char s[32];
	va_list args;
	va_start(args, fmt);
	vsnprintf(s, sizeof(s), fmt, args);
	va_end(args);

	REQ(static_cast<void (Fl_Button::*)(const char *)>(&Fl_Button::label), MODEstatus, s);
	REQ(static_cast<void (Fl_Button::*)()>(&Fl_Button::redraw_label), MODEstatus);
}

void put_MODEstatus(trx_mode mode)
{
	put_MODEstatus("%s", mode_info[mode].sname);
}


void put_rx_data(int *data, int len)
{
 	FHdisp->data(data, len);
}

bool idling = false;

void get_tx_char_idle(void *)
{
	idling = false;
	progStatus.repeatIdleTime = 0;
}

int Qwait_time = 0;
int Qidle_time = 0;

static int que_timeout = 0;
bool que_ok = true;
bool tx_queue_done = true;
bool que_waiting = true;

void post_queue_execute(void*)
{
	if (!que_timeout) {
		LOG_ERROR("%s", "timed out");
		return;
	}
	while (!que_ok && trx_state != STATE_RX) {
		que_timeout--;
		Fl::repeat_timeout(0.05, post_queue_execute);
		Fl::awake();
	}
	trx_transmit();
}

void queue_execute_after_rx(void*)
{
	que_waiting = false;
	if (!que_timeout) {
		LOG_ERROR("%s", "timed out");
		return;
	}
	while (trx_state == STATE_TX) {
		que_timeout--;
		Fl::repeat_timeout(0.05, queue_execute_after_rx);
		Fl::awake();
		return;
	}
	que_timeout = 100; // 5 seconds
	Fl::add_timeout(0.05, post_queue_execute);
	que_ok = false;
	tx_queue_done = false;
	Tx_queue_execute();
}

void do_que_execute(void *)
{
	tx_queue_done = false;
	que_ok = false;
	Tx_queue_execute();
	que_waiting = false;
}

char szTestChar[] = "E|I|S|T|M|O|A|V";

//string bools = "------";
//char testbools[7];

extern int get_fsq_tx_char();
bool disable_lowercase = false;

int get_tx_char(void)
{
	enum { STATE_CHAR, STATE_CTRL };
	static int state = STATE_CHAR;

	if (idling || csma_idling ) { return GET_TX_CHAR_NODATA; } // Keep this a the top of the list (CSMA TX delay).

	if (active_modem->get_mode() == MODE_FSQ) return get_fsq_tx_char();

	if (!que_ok) { return GET_TX_CHAR_NODATA; }
	if (Qwait_time) { return GET_TX_CHAR_NODATA; }
	if (Qidle_time) { return GET_TX_CHAR_NODATA; }
	if (macro_idle_on) { return GET_TX_CHAR_NODATA; }

	if ((progStatus.repeatMacro > -1) && text2repeat.length()) {
		string repeat_content;
		int utf8size = fl_utf8len1(text2repeat[repeatchar]);
		for (int i = 0; i < utf8size; i++)
			repeat_content += text2repeat[repeatchar + i];
		repeatchar += utf8size;
		tx_encoder.push(repeat_content);

		if (repeatchar >= text2repeat.length()) {
			text2repeat.clear();
			macros.repeat(progStatus.repeatMacro);
		}
		goto transmit;
	}

	int c;

	if (!macrochar.empty()) {
		c = macrochar[0];
		macrochar.erase(0,1);
		start_deadman();
		return c;
	}

	if (data_io_enabled == ARQ_IO && arq_text_available) {
		start_deadman();
		c = arq_get_char();
		return c;
	} else if (data_io_enabled == KISS_IO && kiss_text_available) {
		start_deadman();
		c = kiss_get_char();
		return c;
	}

	if (active_modem == cw_modem && progdefaults.QSKadjust) {
		start_deadman();
		c = szTestChar[2 * progdefaults.TestChar];
		return c;
	}

	if ( (progStatus.repeatMacro > -1) && progStatus.repeatIdleTime > 0 &&
		 !idling ) {
		Fl::add_timeout(progStatus.repeatIdleTime, get_tx_char_idle);
		idling = true;
		return GET_TX_CHAR_NODATA;
	}

	if ((c = tx_encoder.pop()) != -1) {
		start_deadman();
		return(c);
	}

	disable_lowercase = false;
	if (xmltest_char_available) {
		num_cps_chars++;
		start_deadman();
		c = xmltest_char();
		disable_lowercase = true;
	}
	else if (active_modem->get_mode() == MODE_IFKP) {
		c = ifkp_tx_text->nextChar();
	}
	else if ((c = next_buffered_macro_char()) == 0) { // preference given to buffered macro chars
		c = TransmitText->nextChar();
	}

	if (c == GET_TX_CHAR_ETX) {
		active_modem->set_CW_EOT();
		return c;
	}

	if (c == '^' && state == STATE_CHAR) {
		state = STATE_CTRL;
		if (active_modem->get_mode() == MODE_IFKP)
			c = ifkp_tx_text->nextChar();
		else
			if ((c = next_buffered_macro_char()) == 0) // preference given to buffered macro chars
				c = TransmitText->nextChar();
	}

	if (c == -1) {
		return(GET_TX_CHAR_NODATA);
	}

	if (state == STATE_CTRL) {
		state = STATE_CHAR;

		switch (c) {
		case 'a': case 'A':
			if (active_modem->get_mode() == MODE_IFKP)
				active_modem->m_ifkp_send_avatar();
			else if (active_modem->get_mode() >= MODE_THOR_FIRST &&
					 active_modem->get_mode() <= MODE_THOR_LAST)
				active_modem->m_thor_send_avatar();
			return(GET_TX_CHAR_NODATA);

		case 'i': case 'I':
			{
				string fname;
				if (active_modem->get_mode() == MODE_IFKP)
					c = ifkp_tx_text->nextChar();
				else
					if ((c = next_buffered_macro_char()) == 0) // preference given to buffered macro chars
						c = TransmitText->nextChar();
				if (c == '[') {
					if (active_modem->get_mode() == MODE_IFKP)
						c = ifkp_tx_text->nextChar();
					else
						if ((c = next_buffered_macro_char()) == 0) // preference given to buffered macro chars
							c = TransmitText->nextChar();
					while (c != ']' && c != -1) {
						fname += c;
						if (active_modem->get_mode() == MODE_IFKP)
							c = ifkp_tx_text->nextChar();
						else
							if ((c = next_buffered_macro_char()) == 0) // preference given to buffered macro chars
								c = TransmitText->nextChar();
					}
					if (c == -1) return (GET_TX_CHAR_NODATA);
					if (active_modem->get_mode() == MODE_IFKP) {
						ifkp_load_scaled_image(fname);
						return (GET_TX_CHAR_NODATA);
					}
					if (active_modem->get_mode() >= MODE_THOR_FIRST &&
					 active_modem->get_mode() <= MODE_THOR_LAST) {
						thor_load_scaled_image(fname);
						return (GET_TX_CHAR_NODATA);
					}
					active_modem->send_color_image(fname);
				}
			}
			return(GET_TX_CHAR_NODATA);

		case 'p': case 'P':
			TransmitText->pause();
			break;
		case 'r':
			local_timed_exec = false;
			active_modem->set_CW_EOT();
			if (active_modem->get_mode() == MODE_IFKP)
				REQ(&FTextTX::clear, ifkp_tx_text);
			else
				REQ(&FTextTX::clear, TransmitText);
			REQ(Rx_queue_execute);
			return(GET_TX_CHAR_ETX);
			break;
		case 'R':
			local_timed_exec = false;
			active_modem->set_CW_EOT();
			if (active_modem->get_mode() == MODE_IFKP) {
				if (ifkp_tx_text->eot()) {
					REQ(&FTextTX::clear, ifkp_tx_text);
					REQ(Rx_queue_execute);
					return(GET_TX_CHAR_ETX);
				} else
					return(GET_TX_CHAR_NODATA);
			} else {
				REQ(&FTextTX::clear, TransmitText);
				REQ(Rx_queue_execute);
				return(GET_TX_CHAR_ETX);
			}
			break;
		case 'L':
			REQ(qso_save_now);
			return(GET_TX_CHAR_NODATA);
			break;
		case 'C':
			REQ(clearQSO);
			return(GET_TX_CHAR_NODATA);
			break;
		case '!':
			if (queue_must_rx()) {
				que_timeout = 400; // 20 seconds
				REQ(queue_execute_after_rx, (void *)0);
				while(que_waiting) { MilliSleep(10); Fl::awake(); }
				return(GET_TX_CHAR_ETX);
			} else {
				if (active_modem->get_stopflag()) {
					return (GET_TX_CHAR_NODATA);
				}
				REQ(do_que_execute, (void*)0);
				MilliSleep(10);
				while (do_tune_on) return (GET_TX_CHAR_NODATA);
				while (que_waiting) { MilliSleep(10); Fl::awake(); }
				return (GET_TX_CHAR_NODATA);
			}
			break;
		default:
			char utf8_char[6];
			int utf8_len = fl_utf8encode(c, utf8_char);
			tx_encoder.push("^" + string(utf8_char, utf8_len));
		}
	}
	else if (c == '\n') {
		tx_encoder.push("\r\n");
	}
	else {
		char utf8_char[6];
		int utf8_len = fl_utf8encode(c, utf8_char);
		tx_encoder.push(string(utf8_char, utf8_len));
	}

	transmit:

	c = tx_encoder.pop();
	if (c == -1) {
		LOG_ERROR("TX encoding conversion error: pushed content, but got nothing back");
		return(GET_TX_CHAR_NODATA);
	}

	if (progdefaults.tx_lowercase && !disable_lowercase)
		c = fl_tolower(c);

	start_deadman();

	return(c);
}

void put_echo_char(unsigned int data, int style)
{
// suppress print to rx widget when making timing tests
	if (PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST) return;

	if(progdefaults.ax25_decode_enabled && data_io_enabled == KISS_IO) {
		disp_rx_processed_char();
		return;
	}

	trx_mode mode = active_modem->get_mode();

	if (mode == MODE_CW && progdefaults.QSKadjust)
		return;

	REQ(&add_tx_char, data & 0xFF);

	// select a byte translation table
	const char **asc = NULL;//ascii;
	if (mailclient || mailserver) {
		asc = ascii2;
		style = FTextBase::CTRL;
	} else if ((progdefaults.show_all_codes && iscntrl(data & 0xFF)) ||
				PERFORM_CPS_TEST || active_modem->XMLRPC_CPS_TEST)
		asc = ascii3;

	// receive and convert the data
	static unsigned int lastdata = 0;

	if (data == '\r' && lastdata == '\r') // reject multiple CRs
		return;

	if (mode == MODE_RTTY && data == '\a') {
		if (progdefaults.visibleBELL)
			echo_chd.rx((unsigned char *)ascii2[7]);
		REQ(TTY_bell);
	} else if (asc == NULL)
		echo_chd.rx(data & 0xFF);
	else
		echo_chd.rx((unsigned char *)asc[data & 0xFF]);

	lastdata = data;

	if (Maillogfile) {
		string s = iscntrl(data & 0x7F) ? ascii2[data & 0x7F] : string(1, data);
		Maillogfile->log_to_file(cLogfile::LOG_TX, s);
	}


	if (echo_chd.data_length() > 0)
	{
		if (active_modem->get_mode() == MODE_FSQ)
			REQ(&FTextRX::addstr, fsq_rx_text, echo_chd.data(), style);
		else if (active_modem->get_mode() == MODE_IFKP)
			REQ(&FTextRX::addstr, ifkp_rx_text, echo_chd.data(), style);
		else
			REQ(&FTextRX::addstr, ReceiveText, echo_chd.data(), style);
		if (progStatus.LOGenabled)
			logfile->log_to_file(cLogfile::LOG_TX, echo_chd.data());

		echo_chd.clear();
	}
}

void resetRTTY() {
	if (active_modem->get_mode() == MODE_RTTY)
		trx_start_modem(active_modem);
}

void resetOLIVIA() {
	trx_mode md = active_modem->get_mode();
	if (md >= MODE_OLIVIA && md <= MODE_OLIVIA_64_2000)
		trx_start_modem(active_modem);
}

void resetCONTESTIA() {
	if (active_modem->get_mode() == MODE_CONTESTIA)
		trx_start_modem(active_modem);
}

//void updatePACKET() {
//    if (active_modem->get_mode() == MODE_PACKET)
//	trx_start_modem(active_modem);
//}

void resetTHOR() {
	trx_mode md = active_modem->get_mode();
	if (md == MODE_THORMICRO || md == MODE_THOR4 || md == MODE_THOR5 || md == MODE_THOR8 ||
		md == MODE_THOR11 ||
		md == MODE_THOR16 || md == MODE_THOR22 ||
		md == MODE_THOR25x4 || md == MODE_THOR50x1 ||
		md == MODE_THOR50x2 || md == MODE_THOR100 )
		trx_start_modem(active_modem);
}

void resetDOMEX() {
	trx_mode md = active_modem->get_mode();
	if (md == MODE_DOMINOEXMICRO || md == MODE_DOMINOEX4 || md == MODE_DOMINOEX5 ||
		md == MODE_DOMINOEX8 || md == MODE_DOMINOEX11 ||
		md == MODE_DOMINOEX16 || md == MODE_DOMINOEX22 ||
		md == MODE_DOMINOEX44 || md == MODE_DOMINOEX88 )
		trx_start_modem(active_modem);
}

void resetSoundCard()
{
	trx_reset();
}

void setReverse(int rev) {
	active_modem->set_reverse(rev);
}

void start_tx()
{
	if (!(active_modem->get_cap() & modem::CAP_TX))
		return;
	trx_transmit();
}

void abort_tx()
{
	if (trx_state == STATE_TUNE) {
		btnTune->value(0);
		btnTune->do_callback();
		return;
	}
	if (trx_state == STATE_TX) {
		queue_reset();
		trx_start_modem(active_modem);
	}
}

void set_rx_tx()
{
	abort_tx();
	rx_only = false;
	btnTune->activate();
	wf->xmtrcv->activate();
}

void set_rx_only()
{
	abort_tx();
	rx_only = true;
	btnTune->deactivate();
	wf->xmtrcv->deactivate();
}

void qsy(long long rfc, int fmid)
{
	if (rfc <= 0LL) {
		rfc = wf->rfcarrier();
	}

	if (fmid > 0) {
		if (active_modem->freqlocked())
			active_modem->set_freqlock(false);
		else
			active_modem->set_freq(fmid);
		// required for modems that will not change their freq (e.g. mt63)
		int adj = active_modem->get_freq() - fmid;
		if (adj)
			rfc += (wf->USB() ? adj : -adj);
	}

	if (connected_to_flrig)
		REQ(xmlrpc_rig_set_qsy, rfc);
	else if (progdefaults.chkUSERIGCATis)
		REQ(rigCAT_set_qsy, rfc);
#if USE_HAMLIB
	else if (progdefaults.chkUSEHAMLIBis)
		REQ(hamlib_set_qsy, rfc);
#endif
	else
		qso_selectFreq((long int) rfc, fmid);

	string testmode = qso_opMODE->value();
	bool xcvr_useFSK = (testmode.find("RTTY") != string::npos);
	if (xcvr_useFSK) {
		int fmid = progdefaults.xcvr_FSK_MARK + rtty::SHIFT[progdefaults.rtty_shift]/2;
		wf->carrier(fmid);
	}
}

map<string, qrg_mode_t> qrg_marks;
qrg_mode_t last_marked_qrg;

void note_qrg(bool no_dup, const char* prefix, const char* suffix, trx_mode mode, long long rfc, int afreq)
{
	qrg_mode_t m;
	m.rfcarrier = (rfc ? rfc : wf->rfcarrier());
	m.carrier = (afreq ? afreq : active_modem->get_freq());
	m.mode = (mode < NUM_MODES ? mode : active_modem->get_mode());
	if (no_dup && last_marked_qrg == m)
		return;
	last_marked_qrg = m;

	char buf[64];

	time_t t = time(NULL);
	struct tm tm;
	gmtime_r(&t, &tm);
	size_t r1;
	if ((r1 = strftime(buf, sizeof(buf), "<<%Y-%m-%dT%H:%MZ ", &tm)) == 0)
		return;

	size_t r2;
	if (m.rfcarrier)
		r2 = snprintf(buf+r1, sizeof(buf)-r1, "%s @ %lld%c%04d>>",
				 mode_info[m.mode].name, m.rfcarrier, (wf->USB() ? '+' : '-'), m.carrier);
	else
		r2 = snprintf(buf+r1, sizeof(buf)-r1, "%s @ %04d>>", mode_info[m.mode].name, m.carrier);
	if (r2 >= sizeof(buf)-r1)
		return;

	qrg_marks[buf] = m;
	if (prefix && *prefix)
		ReceiveText->addstr(prefix);
	ReceiveText->addstr(buf, FTextBase::QSY);
	ReceiveText->mark();
	if (suffix && *suffix)
		ReceiveText->addstr(suffix);
}


// To be called from the main thread.
void * set_xmtrcv_button_true(void)
{
	wf->xmtrcv->value(true);
	wf->xmtrcv->redraw();
	return (void *)0;
}

// To be called from the main thread.
void * set_xmtrcv_button_false(void)
{
	wf->xmtrcv->value(false);
	wf->xmtrcv->redraw();
	return (void *)0;
}

// To be called from the main thread.
void * set_xmtrcv_selection_color_transmitting(void)
{
	wf->xmtrcv_selection_color(progdefaults.XmtColor);
	return (void *)0;
}

// To be called from the main thread.
void * set_xmtrcv_selection_color_pending(void)
{
	wf->xmtrcv_selection_color(FL_YELLOW);
	return (void *)0;
}

void xmtrcv_selection_color(Fl_Color clr)
{
	wf->xmtrcv_selection_color(clr);
}

void xmtrcv_selection_color()
{
	wf->xmtrcv_selection_color(progdefaults.XmtColor);
}

void rev_selection_color()
{
	wf->reverse_selection_color(progdefaults.RevColor);
}

void xmtlock_selection_color()
{
	wf->xmtlock_selection_color(progdefaults.LkColor);
}

void sql_selection_color()
{
	if (!btnSQL) return;
	btnSQL->selection_color(progdefaults.Sql1Color);
	btnSQL->redraw();
}

void afc_selection_color()
{
	if (!btnAFC) return;
	btnAFC->selection_color(progdefaults.AfcColor);
	btnAFC->redraw();
}

void rxid_selection_color()
{
	cbRSID(NULL, NULL);
}

void txid_selection_color()
{
	if (!btnTxRSID) return;
	btnTxRSID->selection_color(progdefaults.TxIDColor);
	btnTxRSID->redraw();
}

void tune_selection_color()
{
	if (!btnTune) return;
	btnTune->selection_color(progdefaults.TuneColor);
	btnTune->redraw();
}

void spot_selection_color()
{
	if (!btnAutoSpot) return;
	btnAutoSpot->selection_color(progdefaults.SpotColor);
	btnAutoSpot->redraw();
}

void set_default_btn_color()
{
	Fl_Light_Button *buttons[] = {
		btn_FSQCALL, btn_SELCAL, btn_MONITOR, btnPSQL,
		btnDupCheckOn, btn_WKCW_connect, btn_WKFSK_connect,
		btn_nanoCW_connect, btn_nanoIO_connect,
		btn_enable_auditlog, btn_enable_fsq_heard_log,
		btn_enable_ifkp_audit_log, btn_enable_ifkp_audit_log,
		btn_Nav_connect, btn_Nav_config, btn_fmt_record,
		btnConnectTalker,
		btn_unk_enable, btn_ref_enable };

	size_t nbtns = sizeof(buttons)/sizeof(*buttons);

	for (size_t i = 0; i < nbtns; i++) {
		if (buttons[i] != NULL) {
			buttons[i]->selection_color(progdefaults.default_btn_color);
			buttons[i]->redraw();
		}
	}
	trx_mode md = active_modem->get_mode();
	if ((md > MODE_WEFAX_FIRST) && (md <= MODE_WEFAX_LAST)) {
		wefax_round_rx_noise_removal->selection_color(progdefaults.default_btn_color);
		wefax_round_rx_binary->selection_color(progdefaults.default_btn_color);
		wefax_round_rx_non_stop->selection_color(progdefaults.default_btn_color);
	}

}

void set_colors()
{
	spot_selection_color();
	tune_selection_color();
	txid_selection_color();
	rxid_selection_color();
	sql_selection_color();
	afc_selection_color();
	xmtlock_selection_color();
	tune_selection_color();
	set_default_btn_color();
	set_log_colors();
}

// Olivia
void set_olivia_bw(int bw)
{
	int i;
	if (bw == 125)
		i = 0;
	else if (bw == 250)
		i = 1;
	else if (bw == 500)
		i = 2;
	else if (bw == 1000)
		i = 3;
	else
		i = 4;
	bool changed = progdefaults.changed;
	i_listbox_olivia_bandwidth->index(i);
	i_listbox_olivia_bandwidth->do_callback();
	progdefaults.changed = changed;
}

void set_olivia_tones(int tones)
{
	unsigned i = -1;
	while (tones >>= 1)
		i++;
	bool changed = progdefaults.changed;
	i_listbox_olivia_tones->index(i);//+1);
	i_listbox_olivia_tones->do_callback();
	progdefaults.changed = changed;
}

//Contestia
void set_contestia_bw(int bw)
{
	int i;
	if (bw == 125)
		i = 0;
	else if (bw == 250)
		i = 1;
	else if (bw == 500)
		i = 2;
	else if (bw == 1000)
		i = 3;
	else
		i = 4;
	bool changed = progdefaults.changed;
	i_listbox_contestia_bandwidth->index(i);
	i_listbox_contestia_bandwidth->do_callback();
	progdefaults.changed = changed;
}

void set_contestia_tones(int tones)
{
	unsigned i = -1;
	while (tones >>= 1)
		i++;
	bool changed = progdefaults.changed;
	i_listbox_contestia_tones->index(i);
	i_listbox_contestia_tones->do_callback();
	progdefaults.changed = changed;
}


void set_rtty_shift(int shift)
{
	if (shift < selCustomShift->minimum() || shift > selCustomShift->maximum())
		return;

	// Static const array otherwise will be built at each call.
	static const int shifts[] = { 23, 85, 160, 170, 182, 200, 240, 350, 425, 850 };
	size_t i;
	for (i = 0; i < sizeof(shifts)/sizeof(*shifts); i++)
		if (shifts[i] == shift)
			break;
	selShift->index(i);
	selShift->do_callback();
	if (i == sizeof(shifts)/sizeof(*shifts)) {
		selCustomShift->value(shift);
		selCustomShift->do_callback();
	}
}

void set_rtty_baud(float baud)
{
	// Static const array otherwise will be rebuilt at each call.
	static const float bauds[] = {
		45.0f, 45.45f, 50.0f, 56.0f, 75.0f,
		100.0f, 110.0f, 150.0f, 200.0f, 300.0f
	};
	for (size_t i = 0; i < sizeof(bauds)/sizeof(*bauds); i++) {
		if (bauds[i] == baud) {
			selBaud->index(i);
			selBaud->do_callback();
			break;
		}
	}
}

void set_rtty_bits(int bits)
{
	// Static const array otherwise will be built at each call.
	static const int bits_[] = { 5, 7, 8 };
	for (size_t i = 0; i < sizeof(bits_)/sizeof(*bits_); i++) {
		if (bits_[i] == bits) {
			selBits->index(i);
			selBits->do_callback();
			break;
		}
	}
}

void set_rtty_bw(float bw)
{
}

int notch_frequency = 0;
void notch_on(int freq)
{
	notch_frequency = freq;
	if (progdefaults.fldigi_client_to_flrig)
		set_flrig_notch();
	else
		rigCAT_set_notch(notch_frequency);
}

void notch_off()
{
	notch_frequency = 0;
	if (progdefaults.fldigi_client_to_flrig)
		set_flrig_notch();
	else
		rigCAT_set_notch(notch_frequency);
}

void enable_kiss(void)
{
	if(btnEnable_arq->value()) {
		btnEnable_arq->value(false);
	}

	progdefaults.changed = true;
	progdefaults.data_io_enabled = KISS_IO;
	progStatus.data_io_enabled = KISS_IO;
	data_io_enabled = KISS_IO;

	btnEnable_kiss->value(true);

	enable_disable_kpsql();
}

void enable_arq(void)
{
	if(btnEnable_kiss->value()) {
		btnEnable_kiss->value(false);
	}

	progdefaults.changed = true;
	progdefaults.data_io_enabled = ARQ_IO;
	progStatus.data_io_enabled = ARQ_IO;
	data_io_enabled = ARQ_IO;

	btnEnable_arq->value(true);

	enable_disable_kpsql();
}

void enable_disable_kpsql(void)
{
	if (progdefaults.data_io_enabled == KISS_IO) {
		check_kiss_modem();
		//btnPSQL->activate();
		//if(progStatus.kpsql_enabled || progdefaults.kpsql_enabled) {
		//    btnPSQL->value(true);
		//    btnPSQL->do_callback();
		//}
	} else {
		sldrSquelch->value(progStatus.sldrSquelchValue);
		//btnPSQL->value(false);
		//btnPSQL->deactivate();
	}

	progStatus.data_io_enabled = progdefaults.data_io_enabled;
}

void disable_config_p2p_io_widgets(void)
{
	btnEnable_arq->deactivate();
	btnEnable_kiss->deactivate();
	btnEnable_ax25_decode->deactivate();
	//btnEnable_csma->deactivate();

	txtKiss_ip_address->deactivate();
	txtKiss_ip_io_port_no->deactivate();
	txtKiss_ip_out_port_no->deactivate();
	btnEnable_dual_port->deactivate();
	//btnEnableBusyChannel->deactivate();
	//cntKPSQLAttenuation->deactivate();
	//cntBusyChannelSeconds->deactivate();
	btnDefault_kiss_ip->deactivate();
	btn_restart_kiss->deactivate();
	btnEnable_7bit_modem_inhibit->deactivate();
	btnEnable_auto_connect->deactivate();
	btnKissTCPIO->deactivate();
	btnKissUDPIO->deactivate();
	btnKissTCPListen->deactivate();
	btn_connect_kiss_io->deactivate();

	txtArq_ip_address->deactivate();
	txtArq_ip_port_no->deactivate();
	btnDefault_arq_ip->deactivate();
	btn_restart_arq->deactivate();

	txtXmlrpc_ip_address->deactivate();
	txtXmlrpc_ip_port_no->deactivate();
	btnDefault_xmlrpc_ip->deactivate();
	btn_restart_xml->deactivate();

	txt_flrig_ip_address->deactivate();
	txt_flrig_ip_port->deactivate();
	btnDefault_flrig_ip->deactivate();
	btn_reconnect_flrig_server->deactivate();

	txt_fllog_ip_address->deactivate();
	txt_fllog_ip_port->deactivate();
	btnDefault_fllog_ip->deactivate();
	btn_reconnect_log_server->deactivate();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void enable_config_p2p_io_widgets(void)
{
	btnEnable_arq->activate();
	btnEnable_kiss->activate();
	btnEnable_ax25_decode->activate();
	//btnEnable_csma->activate();

	txtKiss_ip_address->activate();
	txtKiss_ip_io_port_no->activate();
	txtKiss_ip_out_port_no->activate();
	btnEnable_dual_port->activate();
	//btnEnableBusyChannel->activate();
	//cntKPSQLAttenuation->activate();
	//cntBusyChannelSeconds->activate();
	btnDefault_kiss_ip->activate();
	btn_restart_kiss->activate();
	btnEnable_7bit_modem_inhibit->activate();
	btnEnable_auto_connect->activate();
	btnKissTCPIO->activate();
	btnKissUDPIO->activate();
	btnKissTCPListen->activate();
	btn_connect_kiss_io->activate();

	txtArq_ip_address->activate();
	txtArq_ip_port_no->activate();
	btnDefault_arq_ip->activate();
	btn_restart_arq->activate();

	txtXmlrpc_ip_address->activate();
	txtXmlrpc_ip_port_no->activate();
	btnDefault_xmlrpc_ip->activate();
	btn_restart_xml->activate();

	txt_flrig_ip_address->activate();
	txt_flrig_ip_port->activate();
	btnDefault_flrig_ip->activate();
	btn_reconnect_flrig_server->activate();

	txt_fllog_ip_address->activate();
	txt_fllog_ip_port->activate();
	btnDefault_fllog_ip->activate();
	btn_reconnect_log_server->activate();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void set_ip_to_default(int which_io)
{

	switch(which_io) {
		case ARQ_IO:
			txtArq_ip_address->value(DEFAULT_ARQ_IP_ADDRESS);
			txtArq_ip_port_no->value(DEFAULT_ARQ_IP_PORT);
			txtArq_ip_address->do_callback();
			txtArq_ip_port_no->do_callback();
			break;

		case KISS_IO:
			txtKiss_ip_address->value(DEFAULT_KISS_IP_ADDRESS);
			txtKiss_ip_io_port_no->value(DEFAULT_KISS_IP_IO_PORT);
			txtKiss_ip_out_port_no->value(DEFAULT_KISS_IP_OUT_PORT);
			btnEnable_dual_port->value(false);
			txtKiss_ip_address->do_callback();
			txtKiss_ip_io_port_no->do_callback();
			txtKiss_ip_out_port_no->do_callback();
			btnEnable_dual_port->do_callback();
			break;

		case XMLRPC_IO:
			txtXmlrpc_ip_address->value(DEFAULT_XMLPRC_IP_ADDRESS);
			txtXmlrpc_ip_port_no->value(DEFAULT_XMLRPC_IP_PORT);
			txtXmlrpc_ip_address->do_callback();
			txtXmlrpc_ip_port_no->do_callback();
			break;

		case FLRIG_IO:
			txt_flrig_ip_address->value(DEFAULT_FLRIG_IP_ADDRESS);
			txt_flrig_ip_port->value(DEFAULT_FLRIG_IP_PORT);
			txt_flrig_ip_address->do_callback();
			txt_flrig_ip_port->do_callback();
			break;

		case FLLOG_IO:
			txt_fllog_ip_address->value(DEFAULT_FLLOG_IP_ADDRESS);
			txt_fllog_ip_port->value(DEFAULT_FLLOG_IP_PORT);
			txt_fllog_ip_address->do_callback();
			txt_fllog_ip_port->do_callback();
			break;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void kiss_io_set_button_state(void *ptr)
{

	if(progStatus.kiss_tcp_io) {
		btn_connect_kiss_io->activate();

		btn_connect_kiss_io->redraw();
		btnKissTCPIO->activate();
		btnKissTCPIO->value(true);
		btnKissTCPListen->activate();

		btnKissUDPIO->value(false);
		btnKissUDPIO->activate();
		btnEnable_dual_port->deactivate();

	} else {
		btn_connect_kiss_io->activate();

		btnKissTCPIO->value(false);
		btnKissTCPIO->activate();
		btnKissTCPListen->activate();

		btnKissUDPIO->value(true);
		btnKissUDPIO->activate();
		btnEnable_dual_port->activate();
	}

	char *label = (char *)0;
	if(ptr)
		label = (char *)ptr;

	if(label) {
		btn_connect_kiss_io->label(label);
		btn_connect_kiss_io->redraw();
	}

	if(progStatus.ip_lock) {
		btn_connect_kiss_io->deactivate();
		btnKissTCPIO->deactivate();
		btnKissUDPIO->deactivate();
		btnKissTCPListen->deactivate();
		btnEnable_dual_port->deactivate();
	}

}

//-----------------------------------------------------------------------------
// Update CSMA Display Widgets in the IO Configuration Panel
//-----------------------------------------------------------------------------
void update_csma_io_config(int which)
{
   char buf[32];

   if(which & CSMA_PERSISTANCE) {
	  cntPersistance->value(progStatus.csma_persistance);
	  if(progStatus.csma_persistance >= 0) {
		 float results = ((progStatus.csma_persistance + 1) / 256.0) * 100.0;
		 memset(buf, 0, sizeof(buf));
		 snprintf(buf, sizeof(buf) - 1, "%f", results);
		 OutputPersistancePercent->value(buf);
	  }
   }

   if(which & CSMA_SLOT_TIME) {
	  cntSlotTime->value(progStatus.csma_slot_time);
	  int results = progStatus.csma_slot_time * 10;
	  memset(buf, 0, sizeof(buf));
	  snprintf(buf, sizeof(buf) - 1, "%d", results);
	  OutputSlotTimeMS->value(buf);
   }

   if(which & CSMA_TX_DELAY) {
	  cntTransmitDelay->value(progStatus.csma_transmit_delay);
	  int results = progStatus.csma_transmit_delay * 10;
	  memset(buf, 0, sizeof(buf));
	  snprintf(buf, sizeof(buf) - 1, "%d", results);
	  OutputTransmitDelayMS->value(buf);
   }
}

//-----------------------------------------------------------------------------
// Set PSM configuration panel defaults values.
//-----------------------------------------------------------------------------
void psm_set_defaults(void)
{
	progdefaults.csma_persistance               = progStatus.csma_persistance               = 63;
	progdefaults.csma_slot_time                 = progStatus.csma_slot_time                 = 10;
	progdefaults.csma_transmit_delay            = progStatus.csma_transmit_delay            = 50;
	progdefaults.psm_flush_buffer_timeout       = progStatus.psm_flush_buffer_timeout       = 15;
	progdefaults.psm_minimum_bandwidth_margin   = progStatus.psm_minimum_bandwidth_margin   = 10;
	progdefaults.psm_histogram_offset_threshold = progStatus.psm_histogram_offset_threshold = 3;
	progdefaults.psm_hit_time_window            = progStatus.psm_hit_time_window            = 15;
	progdefaults.kpsql_attenuation              = progStatus.kpsql_attenuation              = 2;
	progdefaults.busyChannelSeconds             = progStatus.busyChannelSeconds             = 3;

	cntPersistance->value(progStatus.csma_persistance);
	cntSlotTime->value(progStatus.csma_slot_time);
	cntTransmitDelay->value(progStatus.csma_transmit_delay);
	cntPSMTXBufferFlushTimer->value(progStatus.psm_flush_buffer_timeout);
	cntPSMBandwidthMargins->value(progStatus.psm_minimum_bandwidth_margin);
	cntPSMThreshold->value(progStatus.psm_histogram_offset_threshold);
	cntPSMValidSamplePeriod->value(progStatus.psm_hit_time_window);
	cntKPSQLAttenuation->value(progdefaults.kpsql_attenuation);
	cntBusyChannelSeconds->value(progStatus.busyChannelSeconds);

	update_csma_io_config(CSMA_ALL);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void set_CSV(int start)
{
	if (active_modem->get_mode() == MODE_ANALYSIS) {
		if (active_modem->get_mode() != MODE_ANALYSIS) return;
		if (start == 1)
			active_modem->start_csv();
		else if (start == 0)
			active_modem->stop_csv();
		else if (active_modem->write_to_csv == true)
			active_modem->stop_csv();
		else
			active_modem->start_csv();
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void set_freq_control_lsd()
{
	qsoFreqDisp1->set_lsd(progdefaults.sel_lsd);
	qsoFreqDisp2->set_lsd(progdefaults.sel_lsd);
	qsoFreqDisp3->set_lsd(progdefaults.sel_lsd);
}

//-----------------------------------------------------------------------------
// FSQ mode control interface functions
//-----------------------------------------------------------------------------
std::string fsq_selected_call = "allcall";
static int heard_picked;

void clear_heard_list();

void cb_heard_delete(Fl_Widget *w, void *)
{
	int sel = fl_choice2(_("Delete entry?"), _("All"), _("No"), _("Yes"));
	if (sel == 2) {
		fsq_heard->remove(heard_picked);
		fsq_heard->redraw();
	}
	if (sel == 0) clear_heard_list();
}

void cb_heard_copy(Fl_Widget *w, void *)
{
// copy to clipboard
	Fl::copy(fsq_selected_call.c_str(), fsq_selected_call.length(), 1);
}

void cb_heard_copy_to_log(Fl_Widget *w, void *)
{
	inpCall->value(fsq_selected_call.c_str());
	cb_call(inpCall, (void *)0);
}

void cb_heard_copy_all(Fl_Widget *w, void *)
{
	if (fsq_heard->size() < 2) return;
	fsq_selected_call.clear();
	for (int i = 2; i <= fsq_heard->size(); i++) {
		fsq_selected_call.append(fsq_heard->text(i));
		size_t p = fsq_selected_call.find(',');
		if (p != std::string::npos) fsq_selected_call.erase(p);
		fsq_selected_call.append(" ");
	}
	Fl::copy(fsq_selected_call.c_str(), fsq_selected_call.length(), 1);
}

void cb_heard_query_snr(Fl_Widget *w, void *)
{
	std::string s = fsq_selected_call.c_str();
	s.append("?");
	fsq_xmt(s);
}

void cb_heard_query_heard(Fl_Widget *w, void *)
{
	std::string s = fsq_selected_call.c_str();
	s.append("$");
	fsq_xmt(s);
}

void cb_heard_query_at(Fl_Widget *w, void *)
{
	std::string s = fsq_selected_call.c_str();
	s.append("@");
	fsq_xmt(s);
}

void cb_heard_query_carat(Fl_Widget *w, void *)
{
	std::string s = fsq_selected_call.c_str();
	s.append("^^");
	fsq_xmt(s);
}

void cb_heard_query_amp(Fl_Widget *w, void *)
{
	std::string s = fsq_selected_call.c_str();
	s.append("&");
	fsq_xmt(s);
}

void cb_heard_send_file(Fl_Widget *w, void *)
{
	std::string deffilename = TempDir;
	deffilename.append("fsq.txt");

	const char* p = FSEL::select( "Select send file", "*.txt", deffilename.c_str());
	if (!p) return;
	if (!*p) return;

	std::string fname = fl_filename_name(p);
	ifstream txfile(p);
	if (!txfile) return;
	stringstream text;
	char ch = txfile.get();
	while (!txfile.eof()) {
		text << ch;
		ch = txfile.get();
	}
	txfile.close();
	std::string s = fsq_selected_call.c_str();
	s.append("#[");
	s.append(fname.c_str());
	s.append("]\n");
	s.append(text.str().c_str());
	fsq_xmt(s);

}

void cb_heard_read_file(Fl_Widget *w, void*)
{
	const char *p = fl_input2("File name");
	if (p == NULL) return;
	string fname = p;
	if (fname.empty()) return;

	std::string s = fsq_selected_call.c_str();
	s.append("+[");
	s.append(fname.c_str());
	s.append("]^r");
	fsq_xmt(s);
}

void cb_heard_query_plus(Fl_Widget *w, void *)
{
	std::string s = fsq_selected_call.c_str();
	s.append("+");
	fsq_xmt(s);
}

void cb_heard_send_msg(Fl_Widget *w, void*)
{
	const char *p = fl_input2("Send message");
	if (p == NULL) return;
	string msg = p;
	if (msg.empty()) return;

	std::string s = fsq_selected_call.c_str();
	s.append("#[");
	s.append(active_modem->fsq_mycall());
	s.append("]");
	s.append(msg.c_str());
	fsq_xmt(s);
}

void cb_heard_send_image(Fl_Widget *w, void *)
{
	fsq_showTxViewer('L');
}

static const Fl_Menu_Item *heard_popup;

static const Fl_Menu_Item all_popup[] = {
	{ "Copy", 0, cb_heard_copy, 0 },
	{ "Copy All", 0, cb_heard_copy_all, 0 , FL_MENU_DIVIDER },
	{ "Send File To... (#)", 0, cb_heard_send_file, 0, FL_MENU_DIVIDER },
	{ "Send Image To... (%)", 0, cb_heard_send_image, 0 },
	{ 0, 0, 0, 0 }
};

static const Fl_Menu_Item directed_popup[] = {
	{ "Copy", 0, cb_heard_copy, 0 },
	{ "Log call", 0, cb_heard_copy_to_log, 0 },
	{ "Copy All", 0, cb_heard_copy_all, 0 },
	{ "Delete", 0, cb_heard_delete, 0, FL_MENU_DIVIDER },
	{ "Query SNR (?)", 0, cb_heard_query_snr, 0 },
	{ "Query Heard ($)", 0, cb_heard_query_heard, 0 },
	{ "Query Location (@@)", 0, cb_heard_query_at, 0 },
	{ "Query Station Msg (&&)", 0, cb_heard_query_amp, 0 },
	{ "Query program version (^)", 0, cb_heard_query_carat, 0, FL_MENU_DIVIDER },
	{ "Send Message To... (#)", 0, cb_heard_send_msg, 0 },
	{ "Read Messages From  (+)", 0, cb_heard_query_plus, 0,  FL_MENU_DIVIDER },
	{ "Send File To... (#)", 0, cb_heard_send_file, 0 },
	{ "Read File From... (+)", 0, cb_heard_read_file, 0, FL_MENU_DIVIDER },
	{ "Send Image To... (%)", 0, cb_heard_send_image, 0 },
	{ 0, 0, 0, 0 }
};

std::string heard_list()
{
	string heard;
	if (fsq_heard->size() < 2) return heard;

	for (int i = 2; i <= fsq_heard->size(); i++)
		heard.append(fsq_heard->text(i)).append("\n");
	heard.erase(heard.length() - 1);  // remove last LF
	size_t p = heard.find(",");
	while (p != std::string::npos) {
		heard.insert(p+1," ");
		p = heard.find(",", p+2);
	}
	return heard;
}

void clear_heard_list()
{
	if (active_modem->get_mode() == MODE_FSQ) {
		fsq_heard->clear();
		fsq_heard->add("allcall");
		fsq_heard->redraw();
	} else {
		ifkp_heard->clear();
		ifkp_heard->redraw();
	}
}

int tm2int(string s)
{
	int t = (s[2]-'0')*10 + s[3] - '0';
	t += 60*((s[0] - '0')*10 + s[1] - '0');
	return t * 60;
}

void age_heard_list()
{
	string entry;
	string now = ztime(); now.erase(4);
	int tnow = tm2int(now);
	string tm;
	int aging_secs;
	switch (progdefaults.fsq_heard_aging) {
		case 1: aging_secs = 60; break;   // 1 minute
		case 2: aging_secs = 300; break;  // 5 minutes
		case 3: aging_secs = 600; break;  // 10 minutes
		case 4: aging_secs = 1200; break; // 20 minutes
		case 5: aging_secs = 1800; break; // 30 minutes
		case 6: aging_secs = 3600; break; // 60 minutes
		case 7: aging_secs = 5400; break; // 90 minutes
		case 8: aging_secs = 7200; break; // 120 minutes
		case 0:
		default: return; // no aging
	}
	if (active_modem->get_mode() == MODE_FSQ) {
		if (fsq_heard->size() < 2) return;
		for (int i = fsq_heard->size(); i > 1; i--) {
			entry = fsq_heard->text(i);
			size_t pos = entry.find(",");
			tm = entry.substr(pos+1,5);
			tm.erase(2,1);
			int tdiff = tnow - tm2int(tm);
			if (tdiff < 0) tdiff += 24*60*60;
			if (tdiff >= aging_secs)
				fsq_heard->remove(i);
		}
		fsq_heard->redraw();
	} else {
		if (ifkp_heard->size() == 0) return;
		for (int i = ifkp_heard->size(); i > 0; i--) {
			entry = ifkp_heard->text(i);
			size_t pos = entry.find(",");
			tm = entry.substr(pos+1,5);
			tm.erase(2,1);
			int tdiff = tnow - tm2int(tm);
			if (tdiff < 0) tdiff += 24*60*60;
			if (tdiff >= aging_secs)
				ifkp_heard->remove(i);
		}
		ifkp_heard->redraw();
	}
}

void add_to_heard_list(string szcall, string szdb)
{
	int found = 0;
	size_t pos_comma;
	std::string testcall;
	std::string line;
	std::string time = inpTimeOff->value();

	std::string str = szcall;
	str.append(",");
	str += time[0]; str += time[1]; str += ':'; str += time[2]; str += time[3];
	str.append(",").append(szdb);

	if (active_modem->get_mode() == MODE_FSQ) {
		if (fsq_heard->size() < 2) {
			fsq_heard->add(str.c_str());
		} else {
			for (int i = 2; i <= fsq_heard->size(); i++) {
				line = fsq_heard->text(i);
				pos_comma = line.find(",");
				if (pos_comma != std::string::npos) {
					testcall = line.substr(0, pos_comma);
					if (testcall == szcall) {
						found = i;
						break;
					}
				}
			}
			if (found)
				fsq_heard->remove(found);
			fsq_heard->insert(2, str.c_str());
		}
		fsq_heard->redraw();
	} else {
		if (ifkp_heard->size() == 0) {
			ifkp_heard->add(str.c_str());
		} else {
			for (int i = 1; i <= ifkp_heard->size(); i++) {
				line = ifkp_heard->text(i);
				pos_comma = line.find(",");
				if (pos_comma != std::string::npos) {
					testcall = line.substr(0, pos_comma);
					if (testcall == szcall) {
						found = i;
						break;
					}
				}
			}
			if (found)
				ifkp_heard->remove(found);
			ifkp_heard->insert(1, str.c_str());
		}
		ifkp_heard->redraw();
	}
	if (progStatus.spot_recv)
		spot_log(szcall.c_str(),
			inpLoc->value(),
			wf->rfcarrier(),
			active_modem->get_mode());
}

bool in_heard(string call)
{
	std::string line;
	for (int i = 1; i <= fsq_heard->size(); i++) {
		line = fsq_heard->text(i);
		if (line.find(call) == 0) return true;
	}
	return false;
}

void fsq_repeat_last_heard()
{
	fsq_tx_text->add(fsq_selected_call.c_str());
}

void cb_fsq_heard(Fl_Browser*, void*)
{
	heard_picked = fsq_heard->value();
	if (!heard_picked)
		return;
	int k = Fl::event_key();
	fsq_selected_call = fsq_heard->text(heard_picked);
	size_t p = fsq_selected_call.find(',');
	if (p != std::string::npos) fsq_selected_call.erase(p);

	switch (k) {
		case FL_Button + FL_LEFT_MOUSE:
			if (Fl::event_clicks()) {
				if (!fsq_tx_text->eot()) fsq_tx_text->add(" ");
				fsq_tx_text->add(fsq_selected_call.c_str());
			}
			break;
		case FL_Button + FL_RIGHT_MOUSE:
			if (heard_picked == 1)
				heard_popup = all_popup;
			else
				heard_popup = directed_popup;
			const Fl_Menu_Item *m = heard_popup->popup(Fl::event_x(), Fl::event_y());
			if (m && m->callback())
				m->do_callback(0);
			break;
	}
	restoreFocus();
}

void cb_ifkp_heard(Fl_Browser*, void*)
{
	heard_picked = ifkp_heard->value();
	if (!heard_picked)
		return;

	int k = Fl::event_key();
	std::string selected_call = ifkp_heard->text(heard_picked);
	size_t p = selected_call.find(',');
	if (p != std::string::npos) selected_call.erase(p);

	switch (k) {
		case FL_Button + FL_LEFT_MOUSE:
			if (Fl::event_clicks()) {
				ifkp_tx_text->add(" ");
				ifkp_tx_text->add(selected_call.c_str());
			}
			break;
		case FL_Button + FL_RIGHT_MOUSE:
			ifkp_heard->remove(heard_picked);
			break;
	}
	restoreFocus();
}

static void do_fsq_rx_text(std::string text, int style)
{
	for (size_t n = 0; n < text.length(); n++)
		rx_parser( text[n], style );
}

void display_fsq_rx_text(std::string text, int style)
{
	REQ(do_fsq_rx_text, text, style);
}

void display_fsq_mon_text(std::string text, int style)
{
	REQ(&FTextRX::addstr, fsq_monitor, text, style);
}

void cbFSQQTC(Fl_Widget *w, void *d)
{
	fsq_tx_text->add(progdefaults.fsqQTCtext.c_str());
	restoreFocus();
}

void cbFSQCQ(Fl_Widget *w, void *d)
{
	fsq_xmt("cqcqcq");
	restoreFocus();
}

void cbFSQQTH(Fl_Widget *w, void *d)
{
	fsq_tx_text->add(progdefaults.myQth.c_str());
	restoreFocus();
}

void cbMONITOR(Fl_Widget *w, void *d)
{
	Fl_Light_Button *btn = (Fl_Light_Button *)w;
	if (btn->value() == 1)
		open_fsqMonitor();
	else {
		progStatus.fsqMONITORxpos = fsqMonitor->x();
		progStatus.fsqMONITORypos = fsqMonitor->y();
		progStatus.fsqMONITORwidth = fsqMonitor->w();
		progStatus.fsqMONITORheight = fsqMonitor->h();
		fsqMonitor->hide();
	}
}

void close_fsqMonitor()
{
	if (!fsqMonitor) return;
	btn_MONITOR->value(0);
	progStatus.fsqMONITORxpos = fsqMonitor->x();
	progStatus.fsqMONITORypos = fsqMonitor->y();
	progStatus.fsqMONITORwidth = fsqMonitor->w();
	progStatus.fsqMONITORheight = fsqMonitor->h();
	fsqMonitor->hide();
}

void cbSELCAL(Fl_Widget *w, void *d)
{
	Fl_Light_Button *btn = (Fl_Light_Button *)w;
	int val = btn->value();
	if (val) {
		btn->label("ACTIVE");
	} else {
		btn->label("ASLEEP");
	}
	btn->redraw_label();
	restoreFocus();
}

void enableSELCAL()
{
	btn_SELCAL->value(1);
	cbSELCAL(btn_SELCAL, (void *)0);
}

void cbFSQCALL(Fl_Widget *w, void *d)
{
	Fl_Light_Button *btn = (Fl_Light_Button *)w;
	int mouse = Fl::event_button();
	int val = btn->value();
	if (mouse == FL_LEFT_MOUSE) {
		progdefaults.fsq_directed = val;
		progdefaults.changed = true;
		if (val == 0) {
			btn_SELCAL->value(0);
			btn_SELCAL->deactivate();
			btn->label("FSQ-OFF");
			btn->redraw_label();
		} else {
			btn_SELCAL->activate();
			btn_SELCAL->value(1);
			cbSELCAL(btn_SELCAL, 0);
			btn->label("FSQ-ON");
			btn->redraw_label();
		}
	}
	restoreFocus();
}

//======================================================================
//FeldHell resizable Rx character height
//======================================================================
extern pthread_mutex_t feld_mutex;

void FHdisp_char_height()
{
	guard_lock raster_lock(&feld_mutex);

	int rh = progdefaults.HellRcvHeight = (int)valHellRcvHeight->value();

	rh = FHdisp->change_rowheight(rh);
	if (rh != progdefaults.HellRcvHeight) {
		progdefaults.HellRcvHeight = rh;
		valHellRcvHeight->value(rh);
		valHellRcvHeight->redraw();
		fl_alert2("Selection too large for current Rx height\nIncrease Rx height");
	} else
	progdefaults.changed = true;

	trx_mode mode = active_modem->get_mode();
	if ( (mode >= MODE_HELL_FIRST) &&
		 (mode <= MODE_HELL_LAST) )
		active_modem->rx_init();

// adjust upper/lower bounds of Rx/Tx panel
	minhtext = 2 * progdefaults.HellRcvHeight + 4;//6;
	minbox->resize(
			text_panel->x(),
			text_panel->y() + minhtext,
			text_panel->w() - 100,
			text_panel->h() - 2*minhtext);
	minbox->redraw();

//	UI_select();

}

void VuMeter_cb(vumeter *vu, void *d)
{
	VuMeter->hide();
	StatusBar->show();
	progStatus.vumeter_shown = 0;
}

void StatusBar_cb(Fl_Box *bx, void *d)
{
	StatusBar->hide();
	VuMeter->show();
	progStatus.vumeter_shown = 1;
}


