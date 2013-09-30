// ----------------------------------------------------------------------------
//
//	fl_digi.h
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
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

#ifndef FL_DIGI_H
#define FL_DIGI_H

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Pack.H>

#include "combo.h"
//#include "Fl_Tile_Check.h"
#include "Panel.h"
#include "FTextRXTX.h"
#include "raster.h"
#include "waterfall.h"
#include "digiscope.h"
#include "globals.h"
#include "mixer.h"
#include "progress.h"
#include "FreqControl.h"
#include "flinput2.h"
#include "flslider2.h"
#include "psk_browser.h"
#include "re.h"

extern fre_t seek_re;

extern Fl_Double_Window *fl_digi_main;
extern Fl_Double_Window *scopeview;
//extern Fl_Double_Window *opBrowserView;

extern Fl_Double_Window *dlgRecordLoader;

extern bool				NBEMSapps_dir;

extern FTextRX			*ReceiveText;
extern FTextTX			*TransmitText;
extern pskBrowser		*mainViewer;
extern Fl_Input2		*txtInpSeek;
extern Fl_Box			*hideViewer;
extern Fl_Group		*mvgroup;

extern Panel			*text_panel;
extern Fl_Box			*minbox;
extern int				oix;

extern Fl_Box			*Status2;
extern Fl_Box			*Status1;
extern Fl_Counter2		*cntCW_WPM;
extern Fl_Counter2		*cntTxLevel;
extern Fl_Button		*MODEstatus;
extern Fl_Slider2		*sldrSquelch;
extern Progress		*pgrsSquelch;
extern Fl_Button 		*btnMacro[];
extern Fl_Button		*btnAltMacros1;
extern Fl_Button		*btnAltMacros2;
extern Fl_Group		*macroFrame1;
extern Fl_Group		*macroFrame2;
extern Fl_Input2		*inpFreq;
extern Fl_Input2		*inpTimeOff;
extern Fl_Input2		*inpTimeOn;
extern Fl_Input2		*inpCall;
extern Fl_Input2		*inpName;
extern Fl_Input2		*inpRstIn;
extern Fl_Input2		*inpRstOut;
extern Fl_Input2		*inpQth;
extern Fl_Input2		*inpState;
extern Fl_Input2		*inpCountry;
extern Fl_Input2		*inpSerNo;
extern Fl_Input2		*outSerNo;
extern Fl_Input2		*inpXchgIn;
extern Fl_Input2		*inpVEprov;
extern Fl_Input2		*inpLoc;
extern Fl_Input2		*inpNotes;
extern Fl_Input2		*inpAZ;	// WA5ZNU
extern Fl_Button		*qsoSave;
extern Fl_Box			*txtRigName;

extern cFreqControl	*qsoFreqDisp1;
extern cFreqControl 	*qsoFreqDisp2;
extern cFreqControl	*qsoFreqDisp3;
extern Fl_Input2		*inpFreq2;

extern Fl_Input2		*inpCall1;
extern Fl_Input2		*inpCall2;
extern Fl_Input2		*inpCall3;
extern Fl_Input2		*inpCall4;
extern Fl_Input2		*inpName1;

extern Fl_Group		*QsoInfoFrame1B;
extern Fl_Group		*qsoFrameView;
extern Fl_Group		*QsoInfoFrame;
extern cFreqControl	*qsoFreqDisp;
extern Fl_ComboBox		*qso_opMODE;
extern Fl_ComboBox		*qso_opBW;
extern Fl_Button		*qso_opPICK;
extern Fl_Browser		*qso_opBrowser;

extern Fl_Value_Slider2	*mvsquelch;
extern Fl_Value_Slider2	*valRcvMixer;
extern Fl_Value_Slider2	*valXmtMixer;
extern Fl_Button			*btnAFC;
extern Fl_Button			*btnSQL;
extern Fl_Light_Button		*btnRSID;
extern Fl_Light_Button		*btnTxRSID;
extern Fl_Light_Button		*btnTune;

extern bool			bWF_only;
extern bool			withnoise;
extern int				altMacros;

extern waterfall		*wf;
extern Digiscope		*digiscope;

extern std::string		main_window_title;

extern int Qwait_time;
extern int Qidle_time;

extern void toggleRSID();

extern int rightof(Fl_Widget* w);
extern int leftof(Fl_Widget* w);
extern int above(Fl_Widget* w);
extern int below(Fl_Widget* w);

extern void set_menus();
extern void create_fl_digi_main(int argc, char** argv);
extern void update_main_title();
extern void activate_rig_menu_item(bool b);
extern void activate_test_menu_item(bool b);
extern void activate_mfsk_image_item(bool b);
extern void activate_wefax_image_item(bool b);
extern void WF_UI();

extern void set_macroLabels();
extern void UI_select();

extern void cb_mnuVisitURL(Fl_Widget*, void* arg);

extern void put_freq(double frequency);
extern void put_Bandwidth(int bandwidth);
extern void global_display_metric(double metric);
extern void put_cwRcvWPM(double wpm);

extern void set_scope_mode(Digiscope::scope_mode md);
extern void set_scope(double *data, int len, bool autoscale = true);
extern void set_phase(double phase, double quality, bool highlight);
extern void set_rtty(double, double, double);
extern void set_video(double *, int, bool = true);
extern void set_zdata(cmplx *, int);

extern void set_scope_xaxis_1(double y1);
extern void set_scope_xaxis_2(double y2);
extern void set_scope_yaxis_1(double x1);
extern void set_scope_yaxis_2(double x2);
extern void set_scope_clear_axis();

extern void set_CWwpm();
extern void put_rx_char(unsigned int data, int style = FTextBase::RECV);
extern void put_sec_char( char chr );

enum status_timeout {
	STATUS_CLEAR,
	STATUS_DIM,
	STATUS_NUM
};
extern void put_status(const char *msg, double timeout = 0.0, status_timeout action = STATUS_CLEAR);
extern void clear_StatusMessages();
extern void put_MODEstatus(const char* fmt, ...) format__(printf, 1, 2);
extern void put_MODEstatus(trx_mode mode);
extern void put_Status1(const char *msg, double timeout = 0.0, status_timeout action = STATUS_CLEAR);
extern void put_Status2(const char *msg, double timeout = 0.0, status_timeout action = STATUS_CLEAR);

extern void show_frequency(long long);
extern void show_mode(const std::string& mode);
extern void show_bw(const std::string& sWidth);
extern void show_spot(bool v);
extern void showMacroSet();
extern void setwfrange();
extern void showDTMF(const std::string s);

extern void xmtrcv_selection_color();
extern void rev_selection_color();
extern void xmtlock_selection_color();
extern void sql_selection_color();
extern void afc_selection_color();
extern void rxid_selection_color();
extern void txid_selection_color();
extern void tune_selection_color();
extern void spot_selection_color();

extern void put_WARNstatus(double);

extern void qsoSave_cb(Fl_Widget *b, void *);

extern bool que_ok;
extern void post_queue_execute(void*);
extern void queue_execute_after_rx(void*);

extern int rxtx_charset;
extern void put_rx_data(int *data, int len);

// Values returned by get_tx_char() to signal various conditions to the
// modems. These values need to be negative so they don't interfere with
// normal TX data (which is always byte-sized and positive).

#define GET_TX_CHAR_NODATA -1	// no data available
#define GET_TX_CHAR_ETX -3	// end of transmission requested

extern int get_tx_char();
extern int  get_secondary_char();
extern void put_echo_char(unsigned int data, int style = FTextBase::XMIT);
extern char *get_rxtx_data();
extern char *get_rx_data();
extern char *get_tx_data(); 

extern void resetRTTY();
extern void resetOLIVIA();
extern void resetCONTESTIA();
extern void resetTHOR();
extern void resetDOMEX();
extern void resetSoundCard();
extern void restoreFocus(Fl_Widget* w = 0);
extern void setReverse(int);
extern void clearQSO();
extern void closeRigDialog();
extern void CloseQsoView();

extern void setAfcOnOff(bool b);
extern void setSqlOnOff(bool b);
extern bool QueryAfcOnOff();
extern bool QuerySqlOnOff();

extern void init_modem(trx_mode mode, int freq = 0);
extern void init_modem_sync(trx_mode mode, int freq = 0);
extern void init_modem_squelch(trx_mode mode, int freq = 0);

extern void start_tx();
extern void abort_tx();

extern void LOGGING_colors_font();
extern void LOGBOOK_colors_font();

extern void colorize_macro(int i);
extern void colorize_macros();

extern void set_rtty_tab_widgets();
extern void set_olivia_tab_widgets();
extern void set_olivia_default_integ();
extern void set_dominoex_tab_widgets();

extern void set_contestia_tab_widgets();
extern void set_contestia_default_integ();

extern void startMacroTimer();
extern void stopMacroTimer();
extern void macro_timer(void *);
extern void macro_timed_execute(void *);
extern void startTimedExecute(std::string &);

extern void cb_ResetSerNbr();
extern void updateOutSerNo();

extern void connect_to_log_server(void *p = NULL);
extern void set_server_label(bool);
extern void activate_menu_item(const char *caption, bool val);
extern bool xml_check_dup();
extern bool xml_get_record(const char *);

extern const char* zdate(void);
extern const char* ztime(void);

extern void setTabColors();

extern void toggle_visible_modes(Fl_Widget*, void*);

extern void qsy(long long rfc, int fmid = -1);

extern void note_qrg(bool no_dup = true, const char* prefix = " ", const char* suffix = " ",
	      trx_mode mode = NUM_MODES, long long rfc = 0LL, int afreq = 0);

extern void set_olivia_bw(int bw);
extern void set_olivia_tones(int tones);

extern void set_contestia_bw(int bw);
extern void set_contestia_tones(int tones);

extern void set_rtty_shift(int shift);
extern void set_rtty_baud(float baud);
extern void set_rtty_bits(int bits);
extern void set_rtty_bw(float bw);

extern void sync_cw_parameters();

extern void open_recv_folder(const char *fname);

extern void reset_mnuPlayback();

// character set
extern void cb_charset_menu(Fl_Widget *, void *charset);
extern void populate_charset_menu(void);
extern void set_default_charset(void);

// set notch parameter on flrig
extern int  notch_frequency;
extern void notch_on(int);
extern void notch_off();

// thread terminators
extern void ADIF_RW_close(void);
extern void EQSL_close(void);

// notifier dialogs
extern Fl_Double_Window *notify_window;
extern Fl_Double_Window *dxcc_window;

extern void cb_mnuCheckUpdate(Fl_Widget *, void *);

bool ask_dir_creation( const std::string & dir );

#endif
