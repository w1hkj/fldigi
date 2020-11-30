// =====================================================================
//
// n3fjp_logger.cxx
//
// interface to multiple n3fjp tcpip logbook services
//
// Copyright (C) 2016
//		Dave Freese, W1HKJ
//		Dave Anderson, KA3PMW
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
// =====================================================================

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <cmath>
#include <cstring>
#include <vector>
#include <list>
#include <stdlib.h>
#include <time.h>

#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>

#include "threads.h"
#include "socket.h"

#include "rigsupport.h"
#include "modem.h"
#include "trx.h"
#include "fl_digi.h"
#include "configuration.h"
#include "main.h"
#include "waterfall.h"
#include "macros.h"
#include "qrunner.h"
#include "debug.h"
#include "status.h"
#include "icons.h"
#include "logsupport.h"
#include "n3fjp_logger.h"
#include "confdialog.h"
#include "rigsupport.h"
#include "contest.h"
#include "timeops.h"

LOG_FILE_SOURCE(debug::LOG_N3FJP);

using namespace std;

static void send_log_data();

//======================================================================
// Socket N3FJP i/o used on all platforms
//======================================================================

pthread_t n3fjp_thread;
pthread_t n3fjp_rx_socket_thread;
Socket *n3fjp_socket = 0;

pthread_mutex_t n3fjp_mutex			= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_this_mutex		= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t report_mutex		= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t n3fjp_socket_mutex	= PTHREAD_MUTEX_INITIALIZER;

static string send_this = "";
static string pathname;
static stringstream result;

bool n3fjp_connected = false;
bool n3fjp_enabled   = false;
bool n3fjp_exit      = false;

string n3fjp_ip_address = "";
string n3fjp_ip_port    = "";

string n3fjp_rxbuffer;
string connected_to;

enum {UNKNOWN, N3FJP, FLDIGI};

bool n3fjp_bool_add_record = false;
int n3fjp_has_xcvr_control = UNKNOWN;

string tracked_freq = "";
int  tracked_mode = -1;

enum {
  FJP_NONE,
  FJP_ACL,			// Amateur Contact Log
  FJP_FD,			// ARRL Field Day
  FJP_WFD,			// ARRL Winter Field Day
  FJP_KD,			// ARRL Kids Day
  FJP_ARR,			// ARRL Rookie Roundup
  FJP_RTTY,			// ARRL Rtty
  FJP_ASCR,			// ARRL School Club Roundup
  FJP_JOTA,			// ARRL Jamboree On The Air
  FJP_AICW,			// ARRL International DX (CW)
  FJP_SS,			// ARRL November Sweepstakes
  FJP_CQ_WPX,		// CQ WPX
  FJP_CQWWRTTY,		// CQWW Rtty
  FJP_CQWWDX,		// CQWW DX
  FJP_IARI,			// Italian ARI International DX
  FJP_NAQP,			// North American QSO Party
  FJP_NAS,			// North American Sprint
  FJP_1010,			// Ten Ten
  FJP_AIDX,			// Africa All Mode
  FJP_VHF,			// VHF
  FJP_WAE,			// Worked All Europe
  FJP_MDQP,			// MD QSOP record format
  FJP_7QP,			// 7QP record format
  FJP_NEQP,			// New England QSO party record format
  FJP_QP1,			// QSO party record format 1 / 7QP contest
  FJP_QP2,			// QSO party record format 2
  FJP_QP3,			// QSO party record format 3
  FJP_QP4,			// QSO party record format 4
  FJP_QP5,			// QSO party record format 5
  FJP_QP6			// QSO party record format 6
};

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
//
// "7QP",      "B",   "B",  "B",    "",    "I",    "",     "",    "SSCCC"
// "New Eng",  "B",   "B",  "B",    "",    "I",    "",     "",    "CCCSS"
// "MD SQP"    "",    "B",  "B",    "",    "",     "",     "B",   ""
// "QSOP 1"    "B",   "B",  "B",    "",    "I",    "",     "",    ""
// "QSOP 2"    "B",   "B",  "B",    "",    "B",    "",     "",    ""
// "QSOP 3"    "",    "B",  "B",    "B",   "I",    "",     "",    ""
// "QSOP 4"    "",    "B",  "B",    "",    "I",    "B",    "",    ""
// "QSOP 5"    "B",   "B",  "B",    "",    "",     "",     "",    ""
// "QSOP 6"    "B",   "B",  "B",    "",    "I",    "",     "",    ""

struct N3FJP_LOGGER {
	const char *program;
	int        contest;
	bool       in_state;
} n3fjp_logger[] = {
	{"No Contest", FJP_NONE, false},
	{"Amateur Contact Log", FJP_ACL, false},
	{"Africa All-Mode International", FJP_AIDX, false},
	{"ARRL Field Day", FJP_FD, false},
	{"Winter FD", FJP_WFD, false},
	{"ARRL International DX", FJP_AICW, false},
	{"Jamboree on the Air", FJP_JOTA, false},
	{"ARRL Kids Day", FJP_KD, false},
	{"ARRL Rookie Roundup", FJP_ARR, false},
	{"ARRL RTTY Roundup", FJP_RTTY, false},
	{"School Club Roundup", FJP_ASCR, false},
	{"ARRL November Sweepstakes", FJP_SS, false},
	{"BARTG RTTY Contest", FJP_NONE, false},
	{"CQ WPX Contest Log", FJP_CQ_WPX, false},
	{"CQ WW DX Contest Log", FJP_CQWWDX, false},
	{"CQ WW DX RTTY Contest Log", FJP_CQWWRTTY, false},
	{"Italian A.R.I. International DX", FJP_IARI, false},
	{"NAQP", FJP_NAQP, false},
	{"NA Sprint", FJP_NAS, false},
	{"Ten Ten", FJP_1010, false},
	{"VHF", FJP_VHF, false},
	{"Worked All Europe", FJP_WAE, false},

	{"Alabama QSO Party Contest Log", FJP_QP1, true},
	{"ALQP Contest Log (Out of State)", FJP_QP1, false},
	{"Arkansas QSO Party Contest Log", FJP_QP1, true},
	{"ARQP Contest Log (Out of State)", FJP_QP1, false},
	{"British Columbia QSO Party Contest Log", FJP_QP1, true},
	{"BCQP Contest Log (Out of Province)", FJP_QP1, false},
	{"Florida QSO Party Contest Log", FJP_QP1, true},
	{"FLQP Contest Log (Out of State)", FJP_QP1, false},
	{"Georgia QSO Party Contest Log", FJP_QP1, true},
	{"GAQP Contest Log (Out of State)", FJP_QP1, false},
	{"Hawaii QSO Party Contest Log", FJP_QP1, true},
	{"HIQP Contest Log (Out of State)", FJP_QP1, false},
	{"Iowa QSO Party Contest Log", FJP_QP1, true},
	{"IAQP Contest Log (Out of State)", FJP_QP1, false},
	{"Idaho QSO Party Contest Log", FJP_QP1, true},
	{"IDQP Contest Log (Out of State)", FJP_QP1, false},
	{"Illinois QSO Party Contest Log", FJP_QP1, true},
	{"ILQP Contest Log (Out of State)", FJP_QP1, false},
	{"Indiana QSO Party Contest Log", FJP_QP1, true},
	{"INQP Contest Log (Out of State)", FJP_QP1, false},
	{"Kansas QSO Party Contest Log", FJP_QP1, true},
	{"KSQP Contest Log (Out of State)", FJP_QP1, false},
	{"Kentucky QSO Party Contest Log", FJP_QP1, true},
	{"KYQP Contest Log (Out of State)", FJP_QP1, false},
	{"Louisiana QSO Party Contest Log", FJP_QP1, true},
	{"LAQP Contest Log (Out of State)", FJP_QP1, false},
	{"Missouri QSO Party Contest Log", FJP_QP1, true},
	{"MOQP Contest Log (Out of State)", FJP_QP1, false},
	{"Mississippi QSO Party Contest Log", FJP_QP1, true},
	{"MSQP Contest Log (Out of State)", FJP_QP1, false},
	{"North Dakota QSO Party Contest Log", FJP_QP1, true},
	{"NDQP Contest Log (Out of State)", FJP_QP1, false},
	{"New Jersey QSO Party Contest Log", FJP_QP1, true},
	{"NJQP Contest Log (Out of State)", FJP_QP1, false},

	{"Montana QSO Party Contest Log", FJP_QP2, true},
	{"MTQP Contest Log (Out of State)", FJP_QP2, false},
	{"Nebraska QSO Party Contest Log", FJP_QP2, true},
	{"NEQP Contest Log (Out of State)", FJP_QP2, false},
	{"New York QSO Party Contest Log", FJP_QP2, true},
	{"NYQP Contest Log (Out of State)", FJP_QP2, false},
	{"Ohio QSO Party Contest Log", FJP_QP2, true},
	{"OHQP Contest Log (Out of State)", FJP_QP2, false},
	{"Oklahoma QSO Party Contest Log", FJP_QP2, true},
	{"OKQP Contest Log (Out of State)", FJP_QP2, false},
	{"Ontario QSO Party Contest Log", FJP_QP2, true},
	{"ONQP Contest Log (Out of Province)", FJP_QP2, false},
	{"South Dakota QSO Party Contest Log", FJP_QP2, true},
	{"SDQP Contest Log (Out of State)", FJP_QP2, false},
	{"Tennessee QSO Party Contest Log", FJP_QP2, true},
	{"TNQP Contest Log (Out of State)", FJP_QP2, false},
	{"Texas QSO Party Contest Log", FJP_QP2, true},
	{"TXQP Contest Log (Out of State)", FJP_QP2, false},
	{"Vermont QSO Party Contest Log", FJP_QP2, true},
	{"VTQP Contest Log (Out of State)", FJP_QP2, false},
	{"Washington Salmon Run QSO Party Contest Log", FJP_QP2, true},
	{"WAQP Contest Log (Out of State)", FJP_QP2, false},
	{"Maine QSO Party Contest Log", FJP_QP2, true},
	{"MEQP Contest Log (Out of State)", FJP_QP2, false},

	{"Arizona QSO Party Contest Log", FJP_QP3, true},
	{"AZQP Contest Log (Out of State)", FJP_QP3, false},
	{"California QSO Party Contest Log", FJP_QP3, true},
	{"CAQP Contest Log (Out of State)", FJP_QP3, false},
	{"Michigan QSO Party Contest Log", FJP_QP3, true},
	{"MIQP Contest Log (Out of State)", FJP_QP3, false},
	{"Pennsylvania QSO Party Contest Log", FJP_QP3, true},
	{"PAQP Contest Log (Out of State)", FJP_QP3, false},
	{"Virginia QSO Party Contest Log", FJP_QP3, true},
	{"VAQP Contest Log (Out of State)", FJP_QP3, false},

	{"Colorado QSO Party Contest Log", FJP_QP4, true},
	{"COQP Contest Log (Out of State)", FJP_QP4, false},
	{"Maryland QSO Party Contest Log", FJP_MDQP, true},
	{"MDQP Contest Log (Out of State)", FJP_MDQP, false},

	{"Minnesota QSO Party Contest Log", FJP_QP4, true},
	{"MNQP Contest Log (Out of State)", FJP_QP4, false},
	{"New Mexico QSO Party Contest Log", FJP_QP4, true},
	{"NMQP Contest Log (Out of State)", FJP_QP4, false},
	{"North Carolina QSO Party Contest Log", FJP_QP6, true},
	{"NCQP Contest Log (Out of State)", FJP_QP6, false},

	{"South Carolina QSO Party Contest Log", FJP_QP5, true},
	{"SCQP Contest Log (Out of State)", FJP_QP5, false},
	{"West Virginia QSO Party Contest Log", FJP_QP5, true},
	{"WVQP Contest Log (Out of State)", FJP_QP5, false},
	{"Wisconsin QSO Party Contest Log", FJP_QP6, true},
	{"WIQP Contest Log (Out of State)", FJP_QP6, false},

	{"7QP QSO Party Contest Log", FJP_7QP, true},
	{"7QP Contest Log (Out of Region)", FJP_7QP, false},

	{"New England QSO Party Contest Log", FJP_NEQP, true},
	{"NEQP Contest Log (Out of Region)", FJP_NEQP, false}
};

int  n3fjp_contest = FJP_NONE;
bool n3fjp_in_state = false;

int n3fjp_wait = 0;

void adjust_freq(string s);
void n3fjp_parse_response(string s);
void n3fjp_disp_report(string s, string fm = "", bool tofile = true);
void n3fjp_send(string cmd, bool tofile = true);
void n3fjp_rcv(string &rx, bool tofile = true);
void n3fjp_send_freq_mode();
void n3fjp_clear_record();
void n3fjp_getfields();
void n3fjp_get_record(string call);
static string ParseField(string &record, string fieldtag);
static string ParseTextField(string &record, string fieldtag);
static string ucasestr(string s);
static void n3fjp_parse_data_stream(string buffer);
static void n3fjp_parse_calltab_event(string buffer);
string fmt_date(string date);
string fmt_time(string time);
string field_rec(string fld, string val);
static string n3fjp_tstmode();
static string n3fjp_opmode();
static string n3fjp_opband();
static string n3fjp_freq();
static void send_control(const string ctl, string val);
static void send_action(const string action);
static void send_command(const string command, string val="");
static void send_data();
static void send_data_norig();
void get_n3fjp_frequency();
void do_n3fjp_add_record_entries();
void n3fjp_set_freq(long f);
void n3fjp_set_ptt(int on);
void n3fjp_add_record(cQsoRec &record);
void n3fjp_parse_response(string tempbuff);
void n3fjp_rcv_data();
static bool connect_to_n3fjp_server();
void n3fjp_start();
void n3fjp_disconnect(bool clearlog = false);
void *n3fjp_loop(void *args);
void n3fjp_init(void);
void n3fjp_close(void);

//======================================================================
//
//======================================================================
static std::string strip(std::string s)
{
	while (s.length() && (s[0] <= ' ')) s.erase(0,1);
	while (s.length() && (s[s.length()-1] <= ' ')) s.erase(s.length()-1, 1);
	return s;
}

//======================================================================
//
//======================================================================

void adjust_freq(string sfreq)
{
	long freq;
	size_t pp = sfreq.find(".");
	if (pp == string::npos) return;

	while ((sfreq.length() - pp) < 7) sfreq.append("0");
	sfreq.erase(pp,1);
	freq = atol(sfreq.c_str());

	if (freq == 0) return;

	wf->rfcarrier(freq);
	wf->movetocenter();
	show_frequency(freq);

	return;

	if (progdefaults.N3FJP_sweet_spot) {
		int afreq;
		if (active_modem->get_mode() == MODE_CW) {
			afreq = progdefaults.CWsweetspot;
		}
		else if (active_modem->get_mode() == MODE_RTTY) {
			afreq = progdefaults.RTTYsweetspot;
		}
		else if (active_modem->get_mode() < MODE_SSB)
			afreq = progdefaults.PSKsweetspot;
		else {
			wf->rfcarrier(freq);
			wf->movetocenter();
			show_frequency(freq);
			return;
		}
		freq -= (wf->USB() ? afreq : -afreq);
		wf->rfcarrier(freq);
		wf->movetocenter();
		show_frequency(freq);
		return;
	}
	wf->rfcarrier(freq);
	wf->movetocenter();
	show_frequency(freq);
}

//======================================================================
//
//======================================================================
static notify_dialog *alert_window = 0;
void set_connect_box()
{
	if (!alert_window) alert_window = new notify_dialog;
	box_n3fjp_connected->color(
		n3fjp_connected ? FL_DARK_GREEN : FL_BACKGROUND2_COLOR);
	box_n3fjp_connected->redraw();
	if (n3fjp_connected) {
		alert_window->notify(_("Connected to N3FJP logger"), 1.0);
		REQ(show_notifier, alert_window);
		REQ(update_main_title);
	}
	else {
		progdefaults.CONTESTnotes = "";
		listbox_contest->index(0);
		listbox_QP_contests->index(0);
		inp_contest_notes->value(progdefaults.CONTESTnotes.c_str());
	}

}

void n3fjp_print(string s)
{
	if (bEXITING) return;

	FILE *n3fjplog = fl_fopen(pathname.c_str(), "a");

	time_t t = time(NULL);
	struct tm stm;
	(void)localtime_r(&t, &stm);
	char sztime[12];
	memset(sztime, 0, 11);
	snprintf(sztime, sizeof(sztime), "[%02d:%02d:%02d] ", stm.tm_hour, stm.tm_min, stm.tm_sec);

	s.insert(0, sztime);

	if (n3fjplog) {
		if (s[s.length()-1]!='\n')
			fprintf(n3fjplog, "%s\n", s.c_str());
		else
			fprintf(n3fjplog, "%s", s.c_str());
		fclose(n3fjplog);
	}

	LOG_VERBOSE("%s", s.c_str());
}

void n3fjp_show(std::string s)
{
	txt_N3FJP_data->insert(s.c_str());
	txt_N3FJP_data->redraw();
}

void n3fjp_disp_report(string s, string fm, bool tofile)
{
	guard_lock report_lock(&report_mutex);

	if (s.empty()) return;

	string report = fm.append("\n").append(s);

	size_t p;
	p = report.find("\r\n");
	while (p != string::npos) {
		report.replace(p,2,"<crlf>\n");
		p = report.find("\r\n");
	}
	p = report.find("</CMD><CMD>");
	while (p != string::npos) {
		report.replace(p, 11, "</CMD>\n<CMD>");
		p = report.find("</CMD><CMD>");
	}

	if (progdefaults.enable_N3FJP_log) REQ(n3fjp_show, report);

	if (tofile) n3fjp_print(report);

}


void n3fjp_send(string cmd, bool tofile)
{
	guard_lock send_lock(&n3fjp_socket_mutex);
	if (!n3fjp_socket) {
		n3fjp_print("Socket not present");
		return;
	}
	try {
		if (cmd.empty()) return;
		n3fjp_disp_report(cmd, "SEND:", tofile);
		cmd.append("\r\n");
		n3fjp_socket->send(cmd);

	} catch (const SocketException& e) {
		result.str("");
		result << "n3fjp_send()::failed " << e.error() << " " << e.what();
		n3fjp_print(result.str());
		throw e;
	} catch (...) { throw; }
}

void n3fjp_rcv(string &rx, bool tofile)
{
	guard_lock read_lock(&n3fjp_socket_mutex);
	if (!n3fjp_socket) return;
	try {
		if (!n3fjp_socket->recv(rx))
			rx.clear();
		if (rx.empty()) return;
		n3fjp_disp_report(rx, "RCVD:", tofile);
	} catch (const SocketException& e) {
		result.str("");
		result << "n3fjp_rcv()::failed " << e.error() << " " << e.what();
		n3fjp_print(result.str());
		throw e;
	} catch (...) { throw; }
}

void n3fjp_send_freq_mode()
{
	if (!active_modem) return;

	string cmd;
	char szfreq[20];
	double freq = atof(inpFreq->value()) / 1e3;
	snprintf(szfreq, sizeof(szfreq), "%f", freq);

	if (active_modem->get_mode() != tracked_mode ||
		tracked_freq != szfreq) {
		tracked_mode = active_modem->get_mode();
		tracked_freq = szfreq;
		cmd = "<CMD><SENDRIGPOLL><FREQ>";
		cmd.append(tracked_freq);
		cmd.append("</FREQ><MODE>");
		cmd.append( mode_info[tracked_mode].adif_name );
		cmd.append("</MODE></CMD>");
		n3fjp_send(cmd, progdefaults.enable_N3FJP_log);
	}
}

//======================================================================
//
//======================================================================

static cQsoRec rec;

static void n3fjp_sendRSTS(std::string s)
{
	if (s.empty()) return;
	try {
		send_control("RSTS", s);
	} catch (...) { throw; }
}

static void n3fjp_sendRSTR(std::string s)
{
	if (s.empty()) return;
	try {
		send_control("RSTR", s);
	} catch (...) { throw; }
}

void send_call(std::string s)
{
	try {
		send_control("CALL", s.c_str());
	} catch (const SocketException& e) {
		result.str("");
		result << "send_call()::failed " << e.error() << " " << e.what();
		n3fjp_print(result.str());
		throw e;
	} catch (...) { throw; }
}

void send_freq(std::string s)
{
	try {
		send_control("FREQUENCY", s);
	} catch (const SocketException& e) {
		result.str("");
		result << "send_freq()::failed " << e.error() << " " << e.what();
		n3fjp_print(result.str());
		throw e;
	} catch (...) { throw; }
}

void send_band(std::string s)
{
	try {
		send_control("BAND", s);
	} catch (const SocketException& e) {
		result.str("");
		result << "send_band()::failed " << e.error() << " " << e.what();
		n3fjp_print(result.str());
		throw e;
	} catch (...) { throw; }
}

void send_mode(std::string s)
{
	try {
		send_control("MODE", s);
	} catch (const SocketException& e) {
		result.str("");
		result << "send_mode()::failed " << e.error() << " " << e.what();
		n3fjp_print(result.str());
		throw e;
	} catch (...) { throw; }
}

void send_state(std::string s)
{
	try {
		send_control("STATE", s);
	} catch (const SocketException& e) {
		result.str("");
		result << "send_state()::failed " << e.error() << " " << e.what();
		n3fjp_print(result.str());
		throw e;
	} catch (...) { throw; }
}

void send_county(std::string s)
{
	try {
		send_control("COUNTYR", s);
	} catch (const SocketException& e) {
		result.str("");
		result << "send_county()::failed " << e.error() << " " << e.what();
		n3fjp_print(result.str());
		throw e;
	} catch (...) { throw; }
}

void send_spcnum(std::string s)
{
	try {
		send_control("SPCNUM", s);
	} catch (const SocketException& e) {
		result.str("");
		result << "send_spcnum()::failed " << e.error() << " " << e.what();
		n3fjp_print(result.str());
		throw e;
	} catch (...) { throw; }
}

void send_name(std::string s)
{
	try {
		send_control("NAMER", s);
	} catch (const SocketException& e) {
		result.str("");
		result << "send_name()::failed " << e.error() << " " << e.what();
		n3fjp_print(result.str());
		throw e;
	} catch (...) { throw; }
}

//======================================================================
//
//======================================================================

void n3fjp_clear_record()
{
	if(!n3fjp_socket) return;
	if (!n3fjp_connected) return;

	string cmd = "<CMD><ACTION><VALUE>CLEAR</VALUE></CMD>";
	try {
		n3fjp_send(cmd, progdefaults.enable_N3FJP_log);
		n3fjp_wait = 100;
	} catch (const SocketException& e) {
		result.str("");
		result << "Error: " << e.error() << ", " << e.what();
		n3fjp_print(result.str());
	} catch (...) { throw; }
}

//======================================================================
//
//======================================================================
bool n3fjp_calltab = false;

void n3fjp_getfields()
{
	string cmd ="<CMD><ALLFIELDSWITHVALUES></CMD>";
	try {
		n3fjp_send(cmd, progdefaults.enable_N3FJP_log);
		n3fjp_wait = 100;
	} catch (const SocketException& e) {
		result.str("");
		result << "Error: " << e.error() << ", " << e.what();
		n3fjp_print(result.str());
		n3fjp_calltab = false;
	} catch (...) { throw; }
}

void n3fjp_get_record(string call)
{
	if(!n3fjp_socket) return;
	if (!n3fjp_connected) return;

	if (!n3fjp_calltab) return;

	string cmd0, cmd1, cmd2;

	cmd0.assign("<CMD><ACTION><VALUE>CLEAR</VALUE></CMD>");

	cmd1.assign("<CMD><UPDATE><CONTROL>TXTENTRYCALL</CONTROL><VALUE>");
	cmd1.append(call).append("</VALUE></CMD>");

	cmd2.assign("<CMD><ACTION><VALUE>CALLTAB</VALUE></CMD>");

	try {
		n3fjp_send(cmd0, progdefaults.enable_N3FJP_log);
		n3fjp_send(cmd1, progdefaults.enable_N3FJP_log);

		n3fjp_send(cmd2, progdefaults.enable_N3FJP_log);
		n3fjp_calltab = false;

		n3fjp_wait = 100;

	} catch (const SocketException& e) {
		result.str("");
		result << "Error: " << e.error() << ", " << e.what();
		n3fjp_print(result.str());
		n3fjp_calltab = false;
	} catch (...) { throw; }
}

//======================================================================
// parse string containing value, e.g.
// <FREQ>14.01310</FREQ>
//======================================================================
static string ParseField(string &record, string fieldtag)
{
	string fld_tag_start, fld_tag_end;
	fld_tag_start.assign("<").append(fieldtag).append(">");
	fld_tag_end.assign("</").append(fieldtag).append(">");
	size_t p1 = record.find(fld_tag_start);
	if (p1 == string::npos) return "";
	p1 += fld_tag_start.length();

	size_t p2 = record.find(fld_tag_end, p1);
	if (p2 == string::npos) return "";
	return record.substr(p1, p2 - p1);
}

//======================================================================
// parse string containing text entry values, e.g.
// <CONTROL>TXTENTRYCOUNTYR</CONTROL><VALUE>Saint Louis City</VALUE></CMD>
//======================================================================
static string ParseTextField(string &record, string fieldtag)
{
	string fld_tag_start;
	fld_tag_start.assign("<CONTROL>TXTENTRY").append(fieldtag).append("</CONTROL>");
	size_t p1 = record.find(fld_tag_start);
	if (p1 == string::npos) return "";
	size_t p2 = record.find("<VALUE>", p1);
	if (p2 == string::npos) return "";
	p2 += strlen("<VALUE>");
	size_t p3 = record.find("</VALUE>", p2);
	if (p3 == string::npos) return "";
	return record.substr(p2, p3 - p2);
}

//======================================================================
// parse value contents
// <VALUE>valuestring</VALUE>
//======================================================================
static string ParseValueField(string field, string &record)
{
	string start = "<";
	start.append(field).append("><VALUE>");
	string endvalue = "</VALUE>";
	size_t p1 = record.find(start);
	size_t p2 = record.find(endvalue, p1);
	if ((p1 == string::npos) || (p2 == string::npos) ||
		(p2 < p1) ) return "";
	p1 += start.length();
	return record.substr(p1, p2 - p1);
}

static string ucasestr(string s)
{
	for (size_t n = 0; n < s.length(); n++) s[n] = toupper(s[n]);
	return s;
}

//======================================================================
//
//======================================================================
static void n3fjp_parse_data_stream(string buffer)
{
	string field;
	field = ParseTextField(buffer, "NAMER");
	if (!field.empty() && ucasestr(field) != ucasestr(inpName->value())) {
		for (size_t n = 1; n < field.length(); n++) field[n] = tolower(field[n]);
	}

	field = ParseTextField(buffer, "COUNTYR");
	if (!field.empty() && field != inpCounty->value() &&
		 n3fjp_contest != FJP_NEQP &&
		 n3fjp_contest != FJP_7QP)

	field = ParseTextField(buffer, "STATE");
	if (!field.empty() && field != inpState->value() &&
		 n3fjp_contest != FJP_NEQP &&
		 n3fjp_contest != FJP_7QP)

	field = ParseTextField(buffer, "COUNTRYWORKED");
	if (!field.empty() && field != cboCountry->value())

	field = ParseTextField(buffer, "GRID");
	if (!field.empty() && field != inpLoc->value())

	field = ParseTextField(buffer, "FREQUENCY");
	if (!field.empty()) adjust_freq(field);

	field = ParseTextField(buffer, "CQZONE");
	if (!field.empty() && field != inp_CQzone->value())

// comments field does not contain \n delimiters
// substitute \n for each '-'
	field = ParseTextField(buffer, "COMMENTS");
	if (!field.empty()) {
		size_t p = field.find(" - ");
		while (p != string::npos) {
			field.replace(p, 3, "\n");
			p = field.find(" - ");
		}
	}
}

//======================================================================
//<CMD><CALLTABEVENT>
//  <CALL>ON6NB/P</CALL>
//  <BAND>40</BAND>
//  <MODE>SSB</MODE>
//  <MODETEST>PH</MODETEST>
//  <COUNTRY>Belgium</COUNTRY>
//</CMD>
//======================================================================
static void n3fjp_parse_calltab_event(string buffer)
{
//	inpCall->value(ParseField(buffer, "CALL").c_str());
	cboCountry->value(ParseField(buffer, "COUNTRY").c_str());
	n3fjp_getfields();
}

//======================================================================
//
//======================================================================
string fmt_date(string date)
{
	if (date.length() > 6) date.insert(6,"/");
	if (date.length() > 4) date.insert(4,"/");
	return date;
}

string fmt_time(string time)
{
	if (time.length() > 4) time.insert(4,":");
	if (time.length() > 2) time.insert(2,":");
	return time;
}

string field_rec(string fld, string val)
{
	string s;
	s.assign("<").append(fld).append(">");
	s.append(val);
	s.append("</").append(fld).append(">");
	return s;
}

static string n3fjp_tstmode()
{
	if (!active_modem)
		return "PH";

	if (active_modem->get_mode() == MODE_CW)
		return "CW";

	if (active_modem->get_mode() == MODE_SSB)
		return "PH";

	if (active_modem->get_mode() < MODE_SSB)
		return mode_info[active_modem->get_mode()].adif_name;

	return "";
}

static string n3fjp_opmode()
{
	if (!active_modem)
		return "PH";

	if (active_modem->get_mode() == MODE_CW)
		return "CW";

	if (active_modem->get_mode() == MODE_SSB)
		return "PH";

	if (active_modem->get_mode() < MODE_SSB)
		return mode_info[active_modem->get_mode()].adif_name;

	return "";
}

static string n3fjp_opband()
{
	if (!active_modem) return "";

	float freq = qsoFreqDisp->value();
	freq /= 1e6;

	if (freq >= 1.8 && freq < 3.5) return "160";
	if (freq >= 3.5 && freq < 5.3) return "80";
	if (freq >= 5.3 && freq < 5.5) return "60";
	if (freq >= 7.0 && freq < 7.5) return "40";
	if (freq >= 14.0 && freq < 18.0) return "20";
	if (freq >= 18.0 && freq < 21.0) return "17";
	if (freq >= 21.0 && freq < 24.0) return "15";
	if (freq >= 24.0 && freq < 28.0) return "12";
	if (freq >= 28.0 && freq < 50.0) return "10";
	if (freq >= 50.0 && freq < 70.0) return "6";
	if (freq >= 144.0 && freq < 222.0) return "2";
	if (freq >= 222.0 && freq < 420.0) return "222";
	if (freq >= 420.0 && freq < 444.0) return "440";
	return "";
}

static string n3fjp_freq()
{
	if (!active_modem) return "";
	float freq = qsoFreqDisp->value();
	if (progdefaults.N3FJP_modem_carrier) {
		if (ModeIsLSB(mode_info[active_modem->get_mode()].adif_name)) {
			freq -= active_modem->get_txfreq();
			if (active_modem->get_mode() == MODE_RTTY)
				freq -= progdefaults.rtty_shift / 2;
		} else {
			freq += active_modem->get_txfreq();
			if (active_modem->get_mode() == MODE_RTTY)
				freq += progdefaults.rtty_shift / 2;
		}
	}
	freq /= 1e6;
	char szfreq[20];
	snprintf(szfreq, sizeof(szfreq), "%f", freq);
	return szfreq;
}

static void send_control(const string ctl, string val)
{
	string cmd;
	cmd.assign("<CMD><UPDATE><CONTROL>TXTENTRY").append(ctl);
	cmd.append("</CONTROL><VALUE>");
	cmd.append(val);
	cmd.append("</VALUE></CMD>");
	try {
		n3fjp_send(cmd, progdefaults.enable_N3FJP_log);
		n3fjp_wait = 100;
	} catch (...) { throw; }
}

static void send_action(const string action)
{
	string cmd;
	cmd.assign("<CMD><ACTION><VALUE>");
	cmd.append(action);
	cmd.append("</VALUE></CMD>");
	try {
		n3fjp_send(cmd, progdefaults.enable_N3FJP_log);
		n3fjp_wait = 200;//100;
	} catch (...) { throw; }
}

static void send_command(const string command, string val)
{
	string cmd;
	cmd.assign("<CMD><").append(command).append(">");
	if (!val.empty())
		cmd.append("<VALUE>").append(val).append("</VALUE>");
	cmd.append("</CMD>");
	try {
		n3fjp_send(cmd, progdefaults.enable_N3FJP_log);
//		MilliSleep(5);
		n3fjp_wait = 100;
	} catch (...) { throw; }
}

static void n3fjp_send_NONE()
{
	try {
		send_control("DATE", fmt_date(rec.getField(QSO_DATE)));
		send_control("TIMEON", fmt_time(rec.getField(TIME_ON)));
		send_control("TIMEOFF", fmt_time(rec.getField(TIME_OFF)));

		send_name(strip(rec.getField(NAME)));
		send_control("COMMENTS", strip(rec.getField(NOTES)));
		send_control("POWER", strip(rec.getField(TX_PWR)));
		send_state(rec.getField(STATE));
		send_control("GRID", strip(rec.getField(GRIDSQUARE)));
		send_control("QTHGROUP", strip(rec.getField(QTH)));
		send_county(rec.getField(CNTY));
		send_control("COUNTRYWORKED", rec.getField(COUNTRY));
	} catch (...) { throw; }
}

// ARRL Field Day
static void n3fjp_send_FD()
{
	try {
		send_control("MODETST", n3fjp_tstmode());
		send_control("CLASS", strip(ucasestr(rec.getField(CLASS))));
		send_control("SECTION", strip(ucasestr(rec.getField(ARRL_SECT))));
	} catch (...) { throw; }
}

// Winter Field Day
static void n3fjp_send_WFD()
{
	try {
		send_control("MODETST", n3fjp_tstmode());
		send_control("CLASS", strip(ucasestr(rec.getField(CLASS))));
		send_control("SECTION", strip(ucasestr(rec.getField(ARRL_SECT))));
	} catch (...) { throw; }
}

// Kids Day
static void n3fjp_send_KD()
{
	try {
		send_name(rec.getField(NAME));

		send_control("AGE", rec.getField(AGE));

		std::string stprc = strip(ucasestr(rec.getField(STATE)));
		if (stprc.empty())
			stprc = strip(ucasestr(rec.getField(VE_PROV)));
		if (stprc.empty())
			stprc = strip(rec.getField(COUNTRY));
		send_spcnum(stprc);

		send_control("COMMENTS", strip(rec.getField(XCHG1)));
	} catch (...) { throw; }
}

// ARRL Rookie Roundup
static void n3fjp_send_ARR()
{
	try {
		send_name(rec.getField(NAME));
		send_control("CHECK", rec.getField(CHECK));
		if (rec.getField(XCHG1)[0])
			send_spcnum(strip(ucasestr(rec.getField(XCHG1))));
		else
			send_spcnum(strip(ucasestr(rec.getField(COUNTRY))));
	} catch (...) { throw; }
}

// ARRL RTTY
static void n3fjp_send_RTTY()
{
	try {
		if (rec.getField(SRX)[0])
			send_spcnum(strip(rec.getField(SRX)));
		else if (rec.getField(STATE)[0])
			send_spcnum(strip(ucasestr(rec.getField(STATE))));
		else if (rec.getField(VE_PROV)[0])
			send_spcnum(strip(ucasestr(rec.getField(VE_PROV))));
		send_control("COUNTRYWORKED", rec.getField(COUNTRY));
	} catch (...) { throw; }
}

// ARRL School Club Roundup
static void n3fjp_send_ASCR()
{
	try {
		send_name(strip(rec.getField(NAME)));
		send_control("CLASS", ucasestr(rec.getField(CLASS)));
		send_spcnum(ucasestr(rec.getField(XCHG1)));
	} catch (...) { throw; }
}

// ARRL Jamboree On The Air
static void n3fjp_send_JOTA()
{
	try {
		send_name(rec.getField(SCOUTR));	// received scout name
		send_control("NAMES", rec.getField(SCOUTS));	// sent scout name
		send_control("CHECK", rec.getField(TROOPR));	// received troop number
		send_control("TROOPS", rec.getField(TROOPS));	// sent troop number
		if (state_test(rec.getField(STATE)))
			send_spcnum(ucasestr(rec.getField(STATE)));
		else if (province_test(rec.getField(VE_PROV)))
			send_spcnum(ucasestr(rec.getField(VE_PROV)));
		else
			send_spcnum(rec.getField(COUNTRY));	// St / Pr / Cntry
		send_control("COMMENTS", rec.getField(NOTES));
	} catch (...) { throw; }
}

// CQ WPX
static void n3fjp_send_WPX()
{
	try {
		send_call(rec.getField(CALL));
		send_freq(n3fjp_freq());
		send_band(n3fjp_opband());
		send_mode(n3fjp_opmode());

		send_control("COUNTRYWORKED", rec.getField(COUNTRY));
		send_control("SERIALNOR", strip(rec.getField(SRX)));
	} catch (...) { throw; }
}

// Italian ARI International DX
static void n3fjp_send_IARI()
{
	try {
		if (rec.getField(SRX)[0])
			send_spcnum(rec.getField(SRX));
		else
			send_spcnum(ucasestr(rec.getField(XCHG1)));
//		send_control("COUNTRYWORKED", rec.getField(COUNTRY));
	} catch (...) { throw; }
}

// North American Sprint
static void n3fjp_send_NAS()
{
	try {
		send_name(rec.getField(NAME));
		send_control("SERIALNOR", strip(rec.getField(SRX)));
		send_spcnum(strip(ucasestr(rec.getField(XCHG1))));
	} catch (...) { throw; }
}

// CQ World Wide RTTY
static void n3fjp_send_CQWWRTTY()
{
	try {
		send_control("CQZONE", strip(rec.getField(CQZ)));
		send_state(rec.getField(STATE));
		send_control("COUNTRYWORKED", strip(rec.getField(COUNTRY)));
	} catch (...) { throw; }
}

// CQ World Wide DX
static void n3fjp_send_CQWWDX()
{
	try {
		send_control("CQZONE", strip(rec.getField(CQZ)));
		send_control("COUNTRYWORKED", strip(rec.getField(COUNTRY)));
	} catch (...) { throw; }
}

// Sweepstakes
static void n3fjp_send_SS()
{
	try {
		send_control("SERIALNOR", strip(rec.getField(SS_SERNO)));
		send_control("PRECEDENCE", strip(rec.getField(SS_PREC)));
		send_control("CHECK", strip(rec.getField(SS_CHK)));
		send_control("SECTION", strip(rec.getField(SS_SEC)));
	} catch (...) { throw; }
}

// North American QSO Party
static void n3fjp_send_NAQP()
{
	try {
		send_name(rec.getField(NAME));
		if (strlen(rec.getField(XCHG1)) > 0)
			send_spcnum(ucasestr(rec.getField(XCHG1)));
	} catch (...) { throw; }
}

// Ten Ten
static void n3fjp_send_1010()
{
	try {
		send_name(rec.getField(NAME));
		send_control("1010", rec.getField(TEN_TEN));
		if (strlen(rec.getField(XCHG1)) > 0)
			send_spcnum(ucasestr(rec.getField(XCHG1)));
	} catch (...) { throw; }
}

// Africa International DX
static void n3fjp_send_AIDX()
{
	try {
		send_control("SERIALNOR", strip(rec.getField(SRX)));
		send_control("COUNTRYWORKED", rec.getField(COUNTRY));
	} catch (...) { throw; }
}

// ARRL International DX (CW)
static void n3fjp_send_AICW()
{
	try {
		send_spcnum(rec.getField(XCHG1));
		send_control("COUNTRYWORKED", rec.getField(COUNTRY));
	} catch (...) { throw; }
}

static void n3fjp_send_GENERIC()
{
	try {
		send_control("SERIALNOR", strip(rec.getField(SRX)));
		send_spcnum(strip(ucasestr(rec.getField(XCHG1))));
	} catch (...) { throw; }
}

static void n3fjp_send_VHF()
{
	try {
		std::string grid = strip(rec.getField(GRIDSQUARE));
		if (grid.length() > 4) grid.erase(4);
		send_control("GRID", grid);
	} catch (...) { throw; }
}

static void n3fjp_send_WAE()
{
	try {
		send_control("SERIALNOR", strip(rec.getField(SRX)));
		send_control("COUNTRYWORKED", rec.getField(COUNTRY));
	} catch (...) { throw; }
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "MD SQP"    "",    "B",  "B",    "",    "",     "",     "B",   ""

static void n3fjp_send_MDQSP()
{
	try {

		if (rec.getField(XCHG1)[0])
			send_control("CATEGORY", ucasestr(strip(rec.getField(XCHG1))));
		send_county(strip(rec.getField(CNTY)));

		if (n3fjp_in_state) {
			send_county(ucasestr(strip(rec.getField(CNTY))));
			if (rec.getField(STATE)[0])
				send_spcnum(rec.getField(STATE));
			else if (rec.getField(VE_PROV)[0])
				send_spcnum(rec.getField(VE_PROV));
			else if (rec.getField(COUNTRY)[0])
				send_spcnum(rec.getField(COUNTRY));
		}
	} catch (...) { throw; }
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "New Eng",  "B",   "B",  "B",    "",    "I",    "",     "",    "CCCSS"

// MA, NH, VT, MA, CT, RI
static void n3fjp_send_NEQP()
{
	try {
		if (rec.getField(CNTY)[0])
			send_county(rec.getField(CNTY));

		if (rec.getField(STATE)[0])
			send_spcnum(rec.getField(STATE));
		else if (rec.getField(COUNTRY)[0])
			send_spcnum(rec.getField(COUNTRY));
	} catch (...) { throw; }
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "7QP",      "B",   "B",  "B",    "",    "I",    "",     "",    "SSCCC"
// AZ ID OR MT NV WA WY
static void n3fjp_send_7QP()
{
	static string st7QP = "AZ ID OR MT NV WA WY UT";
	try {
		std::string st = rec.getField(STATE);
		std::string cnty = rec.getField(CNTY);

		if (cnty.length() == 5) {
			if (st7QP.find(cnty.substr(0,2)) != std::string::npos)
				st = cnty.substr(0,2);
		}

		send_spcnum(st);
		send_county(cnty);

		if (progdefaults.SQSOinstate && !st.empty()) { // in region
			if (st7QP.find(st) != std::string::npos) {
				if (!cnty.empty()) send_county(cnty);
				send_spcnum(st);
			} else
				send_spcnum(st);
		} else {  // out of region
			if (!st.empty() && st7QP.find(st) != std::string::npos) {
				if (!cnty.empty()) send_county(cnty);
				send_spcnum(st);
			}
		}

	} catch (...) { throw; }
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "QSOP 1"    "B",   "B",  "B",    "",    "I",    "",     "",    ""
//AL, AR, FL, GA, HI, IA, ID, IL, KS, KY, LA, MO, MI, ND, NJ, BC, 7QP
static void n3fjp_send_QP1()
{
	try {
		std::string st = rec.getField(STATE);
		if (st.empty()) st = QSOparties.qso_parties[progdefaults.SQSOcontest].state;
		if (rec.getField(STATE)[0])
			send_spcnum(rec.getField(STATE));
		else if (rec.getField(VE_PROV)[0])
			send_spcnum(rec.getField(VE_PROV));
		else if (rec.getField(COUNTRY)[0])
			send_spcnum(rec.getField(COUNTRY));
		if (rec.getField(CNTY)[0]) {
			if (st == "BC") { // British Columbia
				Cstates cs;
				send_county(cs.cnty_short(st, rec.getField(CNTY)));
			} else
				send_county(rec.getField(CNTY));
		}
	} catch (...) { throw; }
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "QSOP 2"    "B",   "B",  "B",    "",    "B",    "",     "",    ""

// MT, NE, NY, OH, OK, ON, SK, TN, TX, VT, WA, ME
static void n3fjp_send_QP2()
{
	try {
		if (rec.getField(STATE)[0])
			send_spcnum(rec.getField(STATE));
		else if (rec.getField(VE_PROV)[0])
			send_spcnum(rec.getField(VE_PROV));
		else if (rec.getField(COUNTRY)[0])
			send_spcnum(rec.getField(COUNTRY));
		if (rec.getField(CNTY)[0])
			send_county(rec.getField(CNTY));
	} catch (...) { throw; }
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "QSOP 3"    "",    "B",  "B",    "B",   "I",    "",     "",    ""

// AZ, CA, MI, PA, VI
static void n3fjp_send_QP3()
{
	try {
		if (n3fjp_in_state) {  // in state log
			if (rec.getField(STATE)[0]) {
				send_spcnum(rec.getField(STATE));
//				string county = states.county(rec.getField(STATE), rec.getField(CNTY));
//				if (!county.empty())
				if (rec.getField(CNTY)[0])
					send_county(rec.getField(CNTY));
			}
			else if (rec.getField(VE_PROV)[0])
				send_spcnum(rec.getField(VE_PROV));
			else if (rec.getField(COUNTRY)[0])
				send_spcnum(rec.getField(COUNTRY));
		} else { // out of state log
			std::string county = rec.getField(CNTY);
			if (!county.empty()) 
//			{
				send_county(county);
//			} else
//				send_county(rec.getField(CNTY)); // may be ARRL section
		}
		send_control("SERIALNOR", strip(rec.getField(SRX)));
	} catch (...) { throw; }
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "QSOP 4"    "",    "B",  "B",    "",    "I",    "B",    "",    ""

// CO, MN, NM
static void n3fjp_send_QP4()
{
	try {
// RST sent/rcvd not required, but will be accepted by logger
		if (n3fjp_in_state) {
			if (rec.getField(STATE)[0])
				send_spcnum(rec.getField(STATE));
			else if (rec.getField(VE_PROV)[0])
				send_spcnum(rec.getField(VE_PROV));
			else if (rec.getField(COUNTRY)[0])
				send_spcnum(rec.getField(COUNTRY));
			if (rec.getField(NAME)[0])
				send_name(rec.getField(NAME));
			if (rec.getField(CNTY)[0])
				send_county(rec.getField(CNTY));
		} else {
//			std::string st = QSOparties.qso_parties[progdefaults.SQSOcontest].state;
//			send_county(states.county(st, rec.getField(CNTY)));
			send_county(rec.getField(CNTY));
			if (rec.getField(NAME)[0])
				send_name(rec.getField(NAME));
		}
	} catch (...) { throw; }
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "QSOP 5"    "B",   "I",  "B",    "",    "",     "",     "",    ""

// SC, WV
static void n3fjp_send_QP5()
{
	try {
		if (n3fjp_in_state) {
			if (rec.getField(STATE)[0])
				send_spcnum(rec.getField(STATE));
			else if (rec.getField(VE_PROV)[0])
				send_spcnum(rec.getField(VE_PROV));
			else if (rec.getField(COUNTRY)[0])
				send_spcnum(rec.getField(COUNTRY));
			send_county(rec.getField(CNTY));
		} else {
			send_county(rec.getField(CNTY));
		}
	} catch (...) { throw; }
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "QSOP 6"    "B",   "B",  "B",    "",    "I",    "",     "",    ""

// NC, WI
static void n3fjp_send_QP6()
{
	try {
		if (n3fjp_in_state) {
			if (rec.getField(STATE)[0])
				send_spcnum(rec.getField(STATE));
			else if (rec.getField(VE_PROV)[0])
				send_spcnum(rec.getField(VE_PROV));
			else if (rec.getField(COUNTRY)[0])
				send_spcnum(rec.getField(COUNTRY));
			if (rec.getField(CNTY)[0])
				send_county(rec.getField(CNTY));
		} else {
//			send_county(states.county(rec.getField(STATE), rec.getField(CNTY)));
			send_county(rec.getField(CNTY));
		}
	} catch (...) { throw; }
}

// check fields for duplicates

// ARRL Field Day
static void n3fjp_check_FD()
{
	try {
		send_control("MODETST", n3fjp_tstmode());
		send_control("CLASS", inpClass->value());
		send_control("SECTION", inpSection->value());
	} catch (...) { throw; }
	return;
}

// Winter Field Day
static void n3fjp_check_WFD()
{
	try {
		send_control("MODETST", n3fjp_tstmode());
		send_control("CLASS", inpClass->value());
		send_control("SECTION", inpSection->value());
	} catch (...) { throw; }
	return;
}

// Kids Day
static void n3fjp_check_KD()
{
	try {
		send_name(inpName->value());
		send_control("AGE", inp_KD_age->value());
		send_spcnum(ucasestr(inpQTH->value()));
		send_control("COMMENTS", inpXchgIn->value());
	} catch (...) { throw; }
	return;
}

// ARRL Rookie Roundup
static void n3fjp_check_ARR()
{
	try {
		send_name(inpName->value());
		send_control("CHECK", inp_ARR_check->value());
		if (inpXchgIn->value()[0])
			send_spcnum(ucasestr(inpXchgIn->value()));
		else
			send_spcnum(ucasestr(cboCountry->value()));
	} catch (...) { throw; }
	return;
}

// ARRL RTTY
static void n3fjp_check_RTTY()
{
	try {
		if (inpSerNo->value()[0])
			send_spcnum(inpSerNo->value());
		else if (inpState->value()[0])
			send_spcnum(ucasestr(inpState->value()));
		else if (inpVEprov->value()[0])
			send_spcnum(ucasestr(ucasestr(inpVEprov->value())));
		send_control("COUNTRYWORKED", cboCountry->value());
	} catch (...) { throw; }
	return;
}

// ARRL School Club Roundup
static void n3fjp_check_ASCR()
{
	try {
		send_name(inpName->value());
		send_spcnum(inpXchgIn->value());
		send_control("CLASS", ucasestr(inpClass->value()));
	} catch (...) { throw; }
	return;
}

// ARRL Jamboree On The Air
static void n3fjp_check_JOTA()
{
	try {
		send_name(inpName->value());

		send_control("CHECK", rec.getField(TROOPR));	// received troop number
		send_control("TROOPS", inp_JOTA_troop->value());	// sent troop number
		if (state_test(inpState->value()))
			send_spcnum(inpState->value());
		else if (province_test(inpVEprov->value()))
			send_spcnum(inpVEprov->value());
		else
			send_spcnum(ucasestr(cboCountry->value()));	// St / Pr / Cntry
		send_control("CHECK", inp_JOTA_troop->value());
	} catch (...) { throw; }
	return;
}

// CQ WPX
static void n3fjp_check_WPX()
{
	try {
		send_control("COUNTRYWORKED", cboCountry->value());
		send_control("SERIALNOR", inpSerNo->value());
//		send_control("CHECK", inpXchgIn->value());
	} catch (...) { throw; }
	return;
}

// Italian ARI International DX
static void n3fjp_check_IARI()
{
	try {
		if (inpSerNo->value()[0])
			send_spcnum(inpSerNo->value());
		else
			send_spcnum(inpXchgIn->value());
//		send_control("COUNTRYWORKED", cboCountry->value());
	} catch (...) { throw; }
	return;
}

// North American Sprint
static void n3fjp_check_NAS()
{
	try {
		send_name(inpName->value());
		send_control("SERIALNOR", inpSerNo->value());
		send_spcnum(inpXchgIn->value());
	} catch (...) { throw; }
	return;
}

// CQ World Wide RTTY
static void n3fjp_check_CQWWRTTY()
{
	try {
		send_control("CQZONE", inp_CQzone->value());
		send_state(inpState->value());
		send_control("COUNTRYWORKED", cboCountry->value());
	} catch (...) { throw; }
	return;
}

// CQ World Wide DX
static void n3fjp_check_CQWWDX()
{
	try {
		send_control("CQZONE", inp_CQzone->value());
		send_control("COUNTRYWORKED", cboCountry->value());
	} catch (...) { throw; }
	return;
}

// Sweepstakes
static void n3fjp_check_SS()
{
	try {
		send_control("SERIALNOR", inpSerNo->value());
		send_control("PRECEDENCE", inp_SS_Precedence->value());
		send_control("CHECK", inp_SS_Check->value());
		send_control("SECTION", inp_SS_Section->value());
	} catch (...) { throw; }
	return;
}

// North American QSO Party
static void n3fjp_check_NAQP()
{
	try {
		send_name(inpName->value());
		send_spcnum(inpXchgIn->value());
	} catch (...) { throw; }
	return;
}

// Ten Ten
static void n3fjp_check_1010()
{
	try {
		send_name(inpName->value());
		send_control("1010", inp_1010_nr->value());
		send_spcnum(inpXchgIn->value());
	} catch (...) { throw; }
	return;
}

// Africa International DX
static void n3fjp_check_AIDX()
{
	try {
		send_control("SERIALNOR", inpSerNo->value());
		send_control("COUNTRYWORKED", cboCountry->value());
	} catch (...) { throw; }
	return;
}

// ARRL International DX (CW)
static void n3fjp_check_AICW()
{
	try {
		send_spcnum(inpSPCnum->value());
		send_control("COUNTRYWORKED", cboCountry->value());
	} catch (...) { throw; }
	return;
}

static void n3fjp_check_GENERIC()
{
	try {
		send_control("SERIALNOR", inpSerNo->value());
		send_spcnum(ucasestr(inpXchgIn->value()));
	} catch (...) { throw; }
	return;
}

static void n3fjp_check_VHF()
{
	try {
		std::string grid = inpLoc->value();
		if (grid.length() > 4) grid.erase(4);
		send_control("GRID", grid);
	} catch (...) { throw; }
	return;
}

static void n3fjp_check_WAE()
{
	try {
		send_control("SERIALNOR", inpSerNo->value());
		send_control("COUNTRYWORKED", cboCountry->value());
	} catch (...) { throw; }
	return;
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "MD SQP"    "",    "B",  "B",    "",    "",     "",     "B",   ""

static void n3fjp_check_MDQSP()
{
	try {
		send_county(inpCounty->value());

		if (inpSQSO_category->value()[0])
			send_control("CATEGORY", inpSQSO_category->value());

		if (n3fjp_in_state) {
			if (inpState->value()[0])
				send_spcnum(inpState->value());
			else if (inpVEprov->value()[0])
				send_spcnum(inpVEprov->value());
			else if (cboCountry->value()[0])
				send_spcnum(cboCountry->value());
		}

	} catch (...) { throw; }
	return;
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "New Eng",  "B",   "B",  "B",    "",    "I",    "",     "",    "CCCSS"

// MA, NH, VT, MA, CT, RI
static void n3fjp_check_NEQP()
{
	static std::string stNEQP = "CT MA ME NH RI VT";
	try {
		std::string st = inpState->value();
		std::string cnty = inpCounty->value();

		if (cnty.length() == 5) {
			if (stNEQP.find(cnty.substr(cnty.length() - 3, 2)) != std::string::npos)
				st = cnty.substr(cnty.length() - 3, 2);
		}

		if (progdefaults.SQSOinstate && !st.empty()) {  // in region
			if (stNEQP.find(st) != std::string::npos) {
				if(!cnty.empty()) send_county(cnty);
				send_spcnum(st);
			} else
				send_spcnum(st);
		} else {  // out of region
			if (!st.empty() && stNEQP.find(st) != std::string::npos) {
				if (!cnty.empty()) send_county(cnty);
				send_spcnum(st);
			}
		}
	} catch (...) { throw; }

	return;
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "7QP",      "B",   "B",  "B",    "",    "I",    "",     "",    "SSCCC"
// AZ ID OR MT NV WA WY
static void n3fjp_check_7QP()
{
	try {
		static string st7QP = "AZ ID OR MT NV WA WY UT";
		std::string st = inpState->value();
		std::string cnty = inpCounty->value();

		if (cnty.length() == 5 && !inpState->value()[0]) {
			if (st7QP.find(cnty.substr(0,2)) != std::string::npos)
				st = cnty.substr(0,2);
		}

		if (progdefaults.SQSOinstate && !st.empty()) { // in region
			if (st7QP.find(st) != std::string::npos) {
				if (!cnty.empty()) send_county(cnty);
				send_spcnum(st);
			} else
				send_spcnum(st);
		} else {  // out of region
			if (!st.empty() && st7QP.find(st) != std::string::npos) {
				if (!cnty.empty()) send_county(cnty);
				send_spcnum(st);
			}
		}

	} catch (...) { throw; }

	return;
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "QSOP 1"    "B",   "B",  "B",    "",    "I",    "",     "",    ""
//AL, AR, FL, GA, HI, IA, ID, IL, IN, KS, KY, LA, MO, MI, ND, NJ, BC, 7QP
static void n3fjp_check_QP1()
{
	try {
		std::string st = inpState->value();
		std::string cntry = cboCountry->value();
		if (st.empty())
			st = QSOparties.qso_parties[progdefaults.SQSOcontest].state;

		if (inpState->value()[0])
			send_spcnum(inpState->value());
		if (inpVEprov->value()[0])
			send_spcnum(inpVEprov->value());
		if (!cntry.empty() && cntry != "USA")
			send_spcnum(cboCountry->value());

		if (inpCounty->value()[0]) {
			if (st == "BC" ) { // British Columbia
				Cstates cs;
				send_county(cs.cnty_short("BC", inpCounty->value()));
			} else
				send_county(inpCounty->value());
		}
	} catch (...) { throw; }

	return;
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "QSOP 2"    "B",   "B",  "B",    "",    "B",    "",     "",    ""

// MT, NE, NY, OH, OK, ON, SK, TN, TX, VT, WA, ME
static void n3fjp_check_QP2()
{
	try {
		if (inpState->value()[0])
			send_spcnum(inpState->value());
		else if (inpVEprov->value()[0])
			send_spcnum(inpVEprov->value());
		else if (cboCountry->value()[0])
			send_spcnum(cboCountry->value());
		if (inpCounty->value()[0])
			send_county(inpCounty->value());
	} catch (...) { throw; }

	return;
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "QSOP 3"    "",    "B",  "B",    "B",   "I",    "",     "",    ""

// AZ, CA, MI, PA, VI
static void n3fjp_check_QP3()
{
	try {
		if (n3fjp_in_state) {
			if (inpState->value()[0]) {
				send_spcnum(inpState->value());
//				string county = states.county(inpState->value(), inpCounty->value());
//				if (!county.empty())
//					send_county(county);
			}
			if (inpCounty->value()[0])
				send_county(inpCounty->value());
			if (inpVEprov->value()[0])
				send_spcnum(inpVEprov->value());
			if (cboCountry->value()[0] && (strcmp(cboCountry->value(), "USA") != 0))
				send_spcnum(cboCountry->value());
		} else {
//			std::string stsh = states.state_short(inpState->value());
//			std::string county = states.county(stsh, inpCounty->value());
//			if (!county.empty()) {
//				send_county(county);
//			} else
			if (inpCounty->value()[0])
				send_county(inpCounty->value());
		}
		send_control("SERIALNOR", inpSerNo->value());
	} catch (...) { throw; }

	return;
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "QSOP 4"    "",    "B",  "B",    "",    "I",    "B",    "",    ""

// CO, MN, NM
/*
N3FJP's Colorado QSO Party Contest Log
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYSPCNUM</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYNAMER</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYCOUNTYR</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYCALL</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>LBLDIALOGUE</CONTROL><VALUE>Ready to begin!<crlf>
*/
static void n3fjp_check_QP4()
{
	try {
		if (n3fjp_in_state) {
			std::string cntry = cboCountry->value();
			if (inpState->value()[0])
				send_spcnum(inpState->value());
			else if (inpVEprov->value()[0])
				send_spcnum(inpVEprov->value());
			else if (!cntry.empty() && cntry != "USA")
				send_county(inpCounty->value());
			if (inpCounty->value()[0])
				send_county(inpCounty->value());
			if (inpName->value()[0])
				send_name(inpName->value());
		} else {
			std::string st = QSOparties.qso_parties[progdefaults.SQSOcontest].state;
//			send_county(states.county(st, inpCounty->value()));
			send_county(inpCounty->value());
			if (inpName->value()[0])
				send_name(inpName->value());
		}
	} catch (...) { throw; }

	return;
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "QSOP 5"    "B",   "I",  "B",    "",    "",     "",     "",    ""

// SC, WV
static void n3fjp_check_QP5()
{
	try {
		if (n3fjp_in_state) {
			if (inpState->value()[0])
				send_spcnum(inpState->value());
			else if (inpVEprov->value()[0])
				send_spcnum(inpVEprov->value());
			else if (cboCountry->value()[0])
				send_spcnum(cboCountry->value());
			if (inpCounty->value()[0])
				send_county(inpCounty->value());
		}else {
			send_county(inpCounty->value());
		}
	} catch (...) { throw; }

	return;
}

// "QSO Party","rRST","rST","rCNTY","rSER","rXCHG","rNAM","rCAT","STCTY"
// "QSOP 6"    "B",   "B",  "B",    "",    "I",    "",     "",    ""

// NC, WI
static void n3fjp_check_QP6()
{
	try {
		if (n3fjp_in_state) {
			if (inpCounty->value()[0])
				send_county(inpCounty->value());
			if (inpState->value()[0])
				send_spcnum(inpState->value());
			else if (inpVEprov->value()[0])
				send_spcnum(inpVEprov->value());
			else if (cboCountry->value()[0])
				send_spcnum(cboCountry->value());
		} else {
//			send_county(states.county(inpState->value(), inpCounty->value()));
			send_county(inpCounty->value());
		}
	} catch (...) { throw; }
	return;
}

static void check_log_data()
{
	try{
	switch (n3fjp_contest) {
		case FJP_FD:
			n3fjp_check_FD();
			break;
		case FJP_WFD:
			n3fjp_check_WFD();
			break;
		case FJP_KD:
			n3fjp_check_KD();
			break;
		case FJP_ARR:
			n3fjp_check_ARR();
			break;
		case FJP_RTTY:
			n3fjp_check_RTTY();
			break;
		case FJP_ASCR:
			n3fjp_check_ASCR();
			break;
		case FJP_JOTA:
			n3fjp_check_JOTA();
			break;
		case FJP_CQ_WPX:
			n3fjp_check_WPX();
			break;
		case FJP_IARI:
			n3fjp_check_IARI();
			break;
		case FJP_NAS:
			n3fjp_check_NAS();
			break;
		case FJP_CQWWRTTY:
			n3fjp_check_CQWWRTTY();
			break;
		case FJP_CQWWDX:
			n3fjp_check_CQWWDX();
			break;
		case FJP_SS:
			n3fjp_check_SS();
			break;
		case FJP_NAQP:
			n3fjp_check_NAQP();
			break;
		case FJP_1010:
			n3fjp_check_1010();
			break;
		case FJP_AIDX:
			n3fjp_check_AIDX();
			break;
		case FJP_AICW:
			n3fjp_check_AICW();
			break;
		case FJP_VHF:
			n3fjp_check_VHF();
			break;
		case FJP_WAE:
			n3fjp_check_WAE();
			break;
		case FJP_MDQP:
			n3fjp_check_MDQSP();
			break;
		case FJP_7QP:
			n3fjp_check_7QP();
			break;
		case FJP_NEQP:
			n3fjp_check_NEQP();
			break;
		case FJP_QP1:
			n3fjp_check_QP1();
			break;
		case FJP_QP2:
			n3fjp_check_QP2();
			break;
		case FJP_QP3:
			n3fjp_check_QP3();
			break;
		case FJP_QP4:
			n3fjp_check_QP4();
			break;
		case FJP_QP5:
			n3fjp_check_QP5();
			break;
		case FJP_QP6:
			n3fjp_check_QP6();
			break;
		case FJP_ACL:
		case FJP_NONE:
		default:
			n3fjp_check_GENERIC();
	}
	} catch (...) { throw; }
}

int n3fjp_dupcheck()
{
	guard_lock rx_lock(&n3fjp_mutex);

	string chkcall = inpCall->value();

	if (chkcall.length() < 3) return false;
	if ((chkcall.length() == 3) && isdigit(chkcall[2])) return false;

	string cmd;

	try {
		send_call(chkcall);
		send_band(strip(n3fjp_opband()));
		send_mode(n3fjp_tstmode());
		n3fjp_sendRSTS(inpRstOut->value());
		n3fjp_sendRSTR(inpRstIn->value());
		check_log_data();

		send_action("CALLTAB");
	} catch (...) { throw; }
	return 0;
}

static void send_log_data()
{
	send_call(rec.getField(CALL));
	send_band(strip(n3fjp_opband()));
	send_freq(n3fjp_freq());
	send_mode(n3fjp_opmode());
	n3fjp_sendRSTS(rec.getField(RST_SENT));
	n3fjp_sendRSTR(rec.getField(RST_RCVD));

	try {
		switch (n3fjp_contest) {
			case FJP_NONE:
				n3fjp_send_NONE();
				break;
			case FJP_FD:
				n3fjp_send_FD();
				break;
			case FJP_WFD:
				n3fjp_send_WFD();
				break;
			case FJP_KD:
				n3fjp_send_KD();
				break;
			case FJP_ARR:
				n3fjp_send_ARR();
				break;
			case FJP_RTTY:
				n3fjp_send_RTTY();
				break;
			case FJP_ASCR:
				n3fjp_send_ASCR();
				break;
			case FJP_JOTA:
				n3fjp_send_JOTA();
				break;
			case FJP_CQ_WPX:
				n3fjp_send_WPX();
				break;
			case FJP_IARI:
				n3fjp_send_IARI();
				break;
			case FJP_NAS:
				n3fjp_send_NAS();
				break;
			case FJP_CQWWRTTY:
				n3fjp_send_CQWWRTTY();
				break;
			case FJP_CQWWDX:
				n3fjp_send_CQWWDX();
				break;
			case FJP_SS:
				n3fjp_send_SS();
				break;
			case FJP_NAQP:
				n3fjp_send_NAQP();
				break;
			case FJP_1010:
				n3fjp_send_1010();
				break;
			case FJP_AIDX:
				n3fjp_send_AIDX();
				break;
			case FJP_AICW:
				n3fjp_send_AICW();
				break;
			case FJP_VHF:
				n3fjp_send_VHF();
				break;
			case FJP_WAE:
				n3fjp_send_WAE();
				break;
			case FJP_MDQP:
				n3fjp_send_MDQSP();
				break;
			case FJP_7QP:
				n3fjp_send_7QP();
				break;
			case FJP_NEQP:
				n3fjp_send_NEQP();
				break;
			case FJP_QP1:
				n3fjp_send_QP1();
				break;
			case FJP_QP2:
				n3fjp_send_QP2();
				break;
			case FJP_QP3:
				n3fjp_send_QP3();
				break;
			case FJP_QP4:
				n3fjp_send_QP4();
				break;
			case FJP_QP5:
				n3fjp_send_QP5();
				break;
			case FJP_QP6:
				n3fjp_send_QP6();
				break;
			case FJP_ACL:
				n3fjp_send_NONE();
				break;
			default:
				n3fjp_send_GENERIC();
				break;
		}
		send_command("NEXTSERIALNUMBER");
	} catch (...) { throw; }
}

static void enter_log_data()
{
	try {
		send_log_data();
		send_action("ENTER");
//		if (n3fjp_contest != FJP_SS) {
//			string other = "XCVR:";
//			char szfreq[6];
//			snprintf(szfreq, sizeof(szfreq), "%d", (int)active_modem->get_txfreq());
//			other.append(ModeIsLSB(rec.getField(ADIF_MODE)) ? "LSB" : "USB");
//			other.append(" MODE:");
//			other.append(strip(rec.getField(ADIF_MODE)));
//			other.append(" WF:");
//			other.append(szfreq);
//			send_control("OTHER8", other);
//		}

	} catch (...) { throw; }
}

static void send_data()
{
	try {

		send_command("IGNORERIGPOLLS", "TRUE");

		send_call(rec.getField(CALL));
		send_freq(n3fjp_freq());
		send_band(n3fjp_opband());
		send_mode(n3fjp_opmode());

		enter_log_data();

		send_command("IGNORERIGPOLLS", "FALSE");

	} catch (...) { throw; }
}

static void send_data_norig()
{
	try {
		string cmd;

		send_call(rec.getField(CALL));
		cmd = "<CMD><CHANGEBM>";
		cmd.append("<BAND>").append(n3fjp_opband()).append("</BAND>");
		cmd.append("<MODE>").append(n3fjp_opmode()).append("</MODE>");
		cmd.append("</CMD>");
		n3fjp_send(cmd, progdefaults.enable_N3FJP_log);

		enter_log_data();

	} catch (...) { throw; }
}

void get_n3fjp_frequency()
{
	try {
		send_command("READBMF");
	} catch (...) { throw; }
}

void do_n3fjp_add_record_entries()
{
	if(!n3fjp_socket) return;
	if (!n3fjp_connected) return;

	string cmd, response, val;

	try {
		if (n3fjp_has_xcvr_control == N3FJP)
			send_data();
		else
			send_data_norig();

	} catch (const SocketException& e) {
		result.str("");
		result << "Error: " << e.error() << ", " << e.what();
		n3fjp_print(result.str());
		throw e;
	}
	n3fjp_bool_add_record = false;
}

void n3fjp_set_freq(long f)
{
	char szfreq[20];
	snprintf(szfreq, sizeof(szfreq), "%ld", f);
	string freq = szfreq;
	while (freq.length() < 7) freq.insert(0, "0");
	freq.insert(freq.length() - 6, ".");

	string cmd;

	cmd.assign("<CMD><CHANGEFREQ><VALUE>");
	cmd.append(freq);
	cmd.append("</VALUE><SUPPRESSMODEDEFAULT>TRUE</SUPPRESSMODEDEFAULT></CMD>");

	{	guard_lock send_lock(&send_this_mutex);
		send_this = cmd;
	}
}

void n3fjp_set_ptt(int on)
{
	if (n3fjp_has_xcvr_control != N3FJP) return;

	string cmd = "<CMD>";
	if (on) {
		if (progdefaults.enable_N3FJP_RIGTX)
			cmd.append("<RIGTX>");
		else
			cmd.append("<CWCOMPORTKEYDOWN>");
	} else {
		if (progdefaults.enable_N3FJP_RIGTX)
			cmd.append("<RIGRX>");
		else
			cmd.append("<CWCOMPORTKEYUP>");
	}
	cmd.append("</CMD>");

	{	guard_lock send_lock(&send_this_mutex);
		send_this = cmd;
	}
}

void n3fjp_add_record(cQsoRec &record)
{
	if (!n3fjp_connected) return;
	rec = record;
	n3fjp_bool_add_record = true;
}

std::string n3fjp_serno = "";

void n3fjp_parse_next_serial(string buff)
{
	n3fjp_serno = ParseValueField("NEXTSERIALNUMBERRESPONSE", buff);
	updateOutSerNo();
}

//======================================================================
//
//======================================================================
void n3fjp_parse_response(string tempbuff)
{
	if (tempbuff.empty()) return;
	size_t p1 = string::npos, p2 = string::npos;

	if (tempbuff.find("RIGRESPONSE") != string::npos) {
		size_t p0 = tempbuff.find("<RIG>");
		if (p0 != string::npos) {
			p0 += strlen("<RIG>");
			string rigname = tempbuff.substr(p0);
			p0 = rigname.find("</RIG>");
			if (p0 != string::npos) {
				rigname.erase(p0);
				if (rigname != "None" && rigname != "Client API") {
					n3fjp_has_xcvr_control = N3FJP;
					send_command("READBMF");
				} else
				n3fjp_has_xcvr_control = FLDIGI;
			}
		}
	}

	if (n3fjp_has_xcvr_control == N3FJP) {
		if ((p1 = tempbuff.find("<CHANGEFREQ><VALUE>")) != string::npos) {
			p1 += strlen("<CHANGEFREQ><VALUE>");
			p2 = tempbuff.find("</VALUE>", p1);
			if (p2 == string::npos) return;
			string sfreq = tempbuff.substr(p1, p2 - p1);
			REQ(adjust_freq, sfreq);
		} else if (tempbuff.find("<READBMFRESPONSE>") != string::npos) {
			string sfreq = ParseField(tempbuff, "FREQ");
			REQ(adjust_freq, sfreq);
		}
	}

	if (tempbuff.find("<CALLTABEVENT>") != string::npos) {
		n3fjp_rxbuffer = tempbuff;
		REQ(n3fjp_parse_calltab_event, tempbuff);
	}

	if (tempbuff.find("ALLFIELDSWVRESPONSE") != string::npos) {
		REQ(n3fjp_parse_data_stream, tempbuff);
	}
	if (tempbuff.find("<ENTEREVENT>") != string::npos) {
		send_command("NEXTSERIALNUMBER");
	}
	if (tempbuff.find("<NEXTSERIALNUMBERRESPONSE>") != string::npos) {
		REQ(n3fjp_parse_next_serial, tempbuff);
	}

	if (tempbuff.find("CALLTABDUPEEVENT") != string::npos &&
		tempbuff.find("Duplicate") != string::npos) {
			if (tempbuff.find("Possible") != string::npos)
				REQ(show_dup, (void*)2);
			else
				REQ(show_dup, (void*)1);
	}
}

//======================================================================
//
//======================================================================
void n3fjp_rcv_data()
{
	string tempbuff = "";
	try {
		n3fjp_rcv(tempbuff, progdefaults.enable_N3FJP_log);
		n3fjp_parse_response(tempbuff);
	} catch (const SocketException& e) {
		result.str("");
		result << "n3fjp_rcv_data()::failed " << e.error() << " " << e.what();
		n3fjp_print(result.str());
		throw e;
	} catch (...) { throw; }
}

static int logger_nbr = 0;

inline bool match(std::string s1, std::string s2)
{
	return (s1.find(s2) != std::string::npos ||
			s2.find(s1) != std::string::npos);
		}

static void select_fldigi_logging()
{
//	check for specific contest
	size_t n = 0;
	std::string logger = n3fjp_logger[logger_nbr].program;

n3fjp_print(std::string("logger: ").append(logger));

	if (logger == "Amateur Contact Log") {
		listbox_contest->index(0);
		progdefaults.logging = 0;
		UI_select();
		return;
	}

	if ((n = logger.find("Jamboree") != std::string::npos)) {
		logger.insert(0, "ARRL ");
	}
	if (logger.find("CQ WW DX RTTY") != std::string::npos)
		logger = "WW DX RTTY";

	progdefaults.CONTESTnotes = "";

	for (int n = 2; !contests[n].name.empty(); n++) {
		if (match(contests[n].name, logger)) {
			progdefaults.logging = n;
			listbox_contest->index(n);
			listbox_QP_contests->index(0);
			UI_select();
			progdefaults.CONTESTnotes = contests[progdefaults.logging].notes;
			inp_contest_notes->value(progdefaults.CONTESTnotes.c_str());
n3fjp_print(std::string("found: ").append(contests[n].name));
			return;
		}
	}
n3fjp_print(std::string("Check for SQSO: ").append(logger));

	if ((n = logger.find("QP Contest Log")) != std::string::npos) {
		logger.erase(n + 2, 12);
		progdefaults.logging = LOG_SQSO;
		listbox_contest->index(progdefaults.logging);
n3fjp_print(std::string("Out of state SQSO log: ").append(logger));
	}
	else if ((n = logger.find(" QSO Party Contest Log")) != std::string::npos) {
		logger.erase(n + 10);
		progdefaults.logging = LOG_SQSO;
		listbox_contest->index(progdefaults.logging);
n3fjp_print(std::string("In state SQSO log: ").append(logger));
	}

	for (n = 1; QSOparties.qso_parties[n].contest[0]; n++) {
		if (logger == QSOparties.qso_parties[n].contest) {
n3fjp_print(std::string("QSOparty: ").append(QSOparties.qso_parties[n].contest));
			progdefaults.SQSOcontest =  n;
			listbox_QP_contests->index(progdefaults.SQSOcontest - 1);
			inp_contest_notes->value(progdefaults.CONTESTnotes.c_str());
			progdefaults.CONTESTnotes = QSOparties.qso_parties[progdefaults.SQSOcontest].notes;
			adjust_for_contest(0);
		}
	}
	inp_contest_notes->value(progdefaults.CONTESTnotes.c_str());

	UI_select();
	clearQSO();
	return;
}

static void fldigi_no_contest()
{
	progdefaults.SQSOcontest = 0;//1;
	progdefaults.logging = 0;
	adjust_for_contest(0);
	UI_select();
	set_log_colors();
	clearQSO();
	listbox_contest->index(0);
}

static int connect_tries = 0;

static bool connect_to_n3fjp_server()
{
	try {
		n3fjp_serno.clear();

		if (!n3fjp_connected)
			n3fjp_socket->connect();

		if (!n3fjp_socket->is_connected()) {
			MilliSleep(200);
			n3fjp_socket->connect();
			if (!n3fjp_socket->is_connected()) {
				if (!connect_tries--) {
					result.str("");
					result << "Cannot connect to server: " << n3fjp_socket->fd();
					n3fjp_print(result.str());
					connect_tries = 20;
				}
				return false;
			}
			result.str("");
			result << "connected to n3fjp server: " << n3fjp_socket->fd();
			n3fjp_print(result.str());
			connect_tries = 0;
		}

		string buffer;

		string cmd = "<CMD><PROGRAM></CMD>";
		n3fjp_send(cmd, true);

		buffer.clear();
		size_t n;
		for (n = 0; n < 10; n++) {
			n3fjp_rcv(buffer, true);
			if (!buffer.empty()) break;
		}
		if (buffer.empty()) {
			n3fjp_print("N3FJP logger not responding");
			return false;
		}

		string info = ParseField(buffer, "PGM");
		connected_to = info;

		n3fjp_contest = FJP_NONE;

		n = info.find("N3FJP's ");
		if (n != std::string::npos) info.erase(n, 8);

		if (info.find("Winter") != std::string::npos)
			info = "Winter FD";

		n3fjp_print(std::string("Info: ").append(info));

		for (n = 0; n < sizeof(n3fjp_logger) / sizeof(*n3fjp_logger); n++) {
			if (info.find(n3fjp_logger[n].program) == 0) {
				n3fjp_contest = n3fjp_logger[n].contest;
				n3fjp_in_state = n3fjp_logger[n].in_state;
				logger_nbr = n;
				REQ(select_fldigi_logging);
				break;
			}
		}
		if (n == sizeof(n3fjp_logger) / sizeof(*n3fjp_logger)) {
			n3fjp_print(string(info).append(" not supported by fldigi"));
			return false;
		}
		else
			n3fjp_print(std::string("Connected to: ").append(n3fjp_logger[n].program));

		send_command("NEXTSERIALNUMBER");

		info.insert(0, "Connected to ");

		string ver = ParseField(buffer, "VER");
		info.append(", Ver ").append(ver);

		n3fjp_connected = true;
		REQ(set_connect_box);

		cmd = " <CMD><VISIBLEFIELDS></CMD>";
		n3fjp_send(cmd, true);

		buffer.clear();
		n3fjp_rcv(buffer, true);

		cmd = "<CMD><CALLTABENTEREVENTS><VALUE>TRUE</VALUE></CMD>";
		n3fjp_send(cmd, progdefaults.enable_N3FJP_log);

		cmd = "<CMD><READOFFSETENABLED></CMD>";
		n3fjp_send(cmd, progdefaults.enable_N3FJP_log);

		cmd = "<CMD><READOFFSET></CMD>";
		n3fjp_send(cmd, progdefaults.enable_N3FJP_log);

		cmd = "<CMD><READMODEDEFAULTSUPPRESS></CMD>";
		n3fjp_send(cmd, progdefaults.enable_N3FJP_log);

		cmd = "RIGENABLED";
		send_command(cmd);

	} catch (const SocketException& e) {
		result.str("");
		result << e.what() << "(" << e.error() << ")";
		n3fjp_print(result.str());
		connected_to.clear();
		LOG_ERROR("%s", result.str().c_str());
	} catch (...) {
		n3fjp_print("Caught unknown error");
		LOG_ERROR("%s", "Caught unknown error");
		connected_to.clear();
	}
	return true;
}

//======================================================================
//
//======================================================================

void n3fjp_start()
{
	n3fjp_ip_address =  progdefaults.N3FJP_address;
	n3fjp_ip_port = progdefaults.N3FJP_port;

	try {
		if (n3fjp_socket) delete n3fjp_socket;

		n3fjp_socket = new Socket(
				Address( n3fjp_ip_address.c_str(),
						 n3fjp_ip_port.c_str(),
						 "tcp") );
		if (!n3fjp_socket) return;

		n3fjp_socket->set_timeout(0.20);//0.05);
		n3fjp_socket->set_nonblocking(true);

		result.str("");
		result << "Client socket " << n3fjp_socket->fd();
		n3fjp_print(result.str());
	}
	catch (const SocketException& e) {
		result.str("");
		result << e.what() << "(" << e.error() << ")";
		n3fjp_print(result.str());
		LOG_ERROR("%s", result.str().c_str() );
		delete n3fjp_socket;
		n3fjp_socket = 0;
		n3fjp_connected = false;
		REQ(set_connect_box);
		n3fjp_has_xcvr_control = UNKNOWN;
	} catch (...) {
		n3fjp_print("Caught unknown error");
		n3fjp_print(result.str());
		LOG_ERROR("%s", result.str().c_str() );
		delete n3fjp_socket;
		n3fjp_socket = 0;
		n3fjp_connected = false;
		REQ(set_connect_box);
		n3fjp_has_xcvr_control = UNKNOWN;
	}
}

//======================================================================
// Disconnect from N3FJP tcpip server
//======================================================================
void n3fjp_disconnect(bool clearlog)
{
	if (n3fjp_socket) {
		n3fjp_send("", false);//progdefaults.enable_N3FJP_log);
		delete n3fjp_socket;
		n3fjp_socket = 0;
	}
	n3fjp_connected = false;
	n3fjp_has_xcvr_control = UNKNOWN;
	n3fjp_serno.clear();
	connected_to.clear();
	REQ(set_connect_box);
	if (clearlog) REQ(fldigi_no_contest);
	n3fjp_print("Disconnected");

}

//======================================================================
// Thread loop
//======================================================================

void *n3fjp_loop(void *args)
{
	SET_THREAD_ID(N3FJP_TID);

	int loopcount = 9;
	int n3fjp_looptime = 100; // initially 0.1 second delay to connect
	while(1) {
		if (n3fjp_exit) break;

		MilliSleep(10);
		if (n3fjp_wait) n3fjp_wait -= 10;
		if (n3fjp_looptime) n3fjp_looptime -= 10;

		if (n3fjp_wait > 0) continue;

		if (n3fjp_looptime > 0) continue;

		n3fjp_looptime = 250;  // r/w to N3FJP logger every 1/4 second

		loopcount = (loopcount + 1) % 10;
		if (progdefaults.connect_to_n3fjp) {
			if (!n3fjp_socket || (n3fjp_socket->fd() == -1))
				n3fjp_start();

			else {
				if ((n3fjp_ip_address != progdefaults.N3FJP_address) ||
					(n3fjp_ip_port != progdefaults.N3FJP_port) ) {
					n3fjp_disconnect(true);
					n3fjp_start();
				}

				if (!n3fjp_connected) {
					if (loopcount == 0)
						if (!connect_to_n3fjp_server())
							n3fjp_disconnect(false);
				} else try {  // insure connection still up (2.5 second interval)
					if (loopcount == 0) {
						guard_lock send_lock(&send_this_mutex);
						std::string buffer;
						string cmd = "<CMD><PROGRAM></CMD>";
						n3fjp_send(cmd, false);
						size_t n;
						for (n = 0; n < 10; n++) {
							n3fjp_rcv(buffer, false);
							if (!buffer.empty()) break;
						}
						if (buffer.empty()) {
							n3fjp_print(std::string("Lost server connection to ").append(connected_to));
							n3fjp_disconnect(true);
							continue;
						}
					}
					if (n3fjp_has_xcvr_control == FLDIGI)
						n3fjp_send_freq_mode();
					if (!send_this.empty()) {
						guard_lock send_lock(&send_this_mutex);
						n3fjp_send(send_this, progdefaults.enable_N3FJP_log);
						send_this.clear();
					} else if (n3fjp_bool_add_record)
						do_n3fjp_add_record_entries();
					else {
						guard_lock rx_lock(&n3fjp_mutex);
						n3fjp_rcv_data();
					}
				} catch (const SocketException& e) {
					result.str("");
					result << "Error: " << e.error() << ", " << e.what();
					n3fjp_print(result.str());
					n3fjp_disconnect(true);
				} catch (...) {
					n3fjp_print("Caught unknown error");
					n3fjp_disconnect(true);
				}
			}
		} else if (n3fjp_connected)
			n3fjp_disconnect(true);
	}
	// exit the n3fjp thread
	SET_THREAD_CANCEL();
	return NULL;
}

//======================================================================
//
//======================================================================

void n3fjp_init(void)
{
	n3fjp_enabled = false;
	n3fjp_exit = false;

	if (pthread_create(&n3fjp_thread, NULL, n3fjp_loop, NULL) < 0) {
		LOG_ERROR("pthread_create failed");
		return;
	}

	LOG_INFO("N3FJP logger thread started");

	pathname = DebugDir;
	pathname.append("n3fjp_data_stream.txt");
	rotate_log(pathname);
	FILE *n3fjplog = fl_fopen(pathname.c_str(), "w");
	fprintf(n3fjplog, "N3FJP / fldigi tcpip log\n\n");
	fclose(n3fjplog);

	n3fjp_enabled = true;
}

//======================================================================
//
//======================================================================
void n3fjp_close(void)
{
	if (!n3fjp_enabled) return;

	guard_lock close_lock(&n3fjp_socket_mutex);

	n3fjp_exit = true;
	CANCEL_THREAD(n3fjp_thread);
	pthread_join(n3fjp_thread, NULL);
	n3fjp_enabled = false;

	LOG_INFO("%s", "N3FJP logger thread terminated. ");

	if(n3fjp_socket) {
		delete n3fjp_socket;
		n3fjp_socket = 0;
	}

}

/*
FLDIGI log fields

FREQ           QSO frequency in Mhz
CALL           contacted stations CALLSIGN
MODE           QSO mode
NAME           contacted operators NAME
QSO_DATE       QSO date
QSO_DATE_OFF   QSO date OFF, according to ADIF 2.2.6
TIME_OFF       HHMM or HHMMSS in UTC
TIME_ON        HHMM or HHMMSS in UTC
QTH            contacted stations city
RST_RCVD       received signal report
RST_SENT       sent signal report
STATE          contacted stations STATE
VE_PROV        2 letter abbreviation for Canadian Province
NOTES          QSO notes

QSLRDATE       QSL received date
QSLSDATE       QSL sent date

EQSLRDATE      EQSL received date
EQSLSDATE      EQSL sent date

LOTWRDATE      EQSL received date
LOTWSDATE      EQSL sent date

GRIDSQUARE     contacted stations Maidenhead Grid Square
BAND           QSO band
CNTY           secondary political subdivision, ie: county
COUNTRY        contacted stations DXCC entity name
CQZ            contacted stations CQ Zone
DXCC           contacted stations Country Code
QSL_VIA        contacted stations QSL manager
IOTA           Islands on the air
ITUZ           ITU zone
CONT           contacted stations continent

SRX            received serial number for a contest QSO
STX            QSO transmitted serial number

XCHG1          contest exchange received
MYXCHG         contest exchange sent

CLASS        Field Day class received
ARRL_SECT      Field Day section received

TX_PWR         power transmitted by this station

OP_CALL       Callsign of person logging the QSO
STA_CALL      Callsign of transmitting station
MY_GRID       Xmt station locator
MY_CITY       Xmt station city

SS_SEC        CW sweepstakes section
SS_SERNO      CW sweepstakes serial number received
SS_PREC       CW sweepstakes precedence
SS_CHK        CW sweepstakes check

AGE           contacted operators age in years
TEN_TEN       contacted stations ten ten #
CHECK         contacted stations contest identifier

|-------------------------------------------------------------|
| N3FJP field                    | FLDIGI LOG FIELD           |
|--------------------------------|----------------------------|
| txtEntry1010                   | rec.getField(TEN_TEN)      |
| txtEntryCall                   | rec.getField(CALL)         |
| txtEntryCheck (Rookie Roundup) | rec.getField(CHECK)        |
| txtEntryCheck (1010 ??)      ) | rec.getField(CHECK)        |
| txtEntryClass                  | rec.getField(CLASS)      |
| txtEntryCountyR                | rec.getField(CNTY)         |
| txtEntryGrid                   | rec.getField(GRIDSQUARE)   |
| txtEntryNameR                  | rec.getField(NAME)         |
| txtEntryRSTR                   | rec.getField(RST_RCVD)     |
| txtEntryRSTS                   | rec.getField(RST_SENT)     |
| txtEntrySection                | rec.getField(ARRL_SECT)    |
| txtEntrySection                | rec.getField(SS_SEC)       |
| txtEntrySerialNoT              | rec.getField(STX)          |
| txtEntrySerialNoR              | rec.getField(SRX)          |
| txtEntrySpcNum                 | rec.getField(XCHG1)        |
| txtEntryState                  | rec.getField(STATE)        |
| txtEntryState                  | rec.getField(VE_PROV) ??   |
|-------------------------------------------------------------|

Use this command to query N3FJP logger to find which fields are visible
for any given program.

<CMD><VISIBLEFIELDS></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYCOUNTYR</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYSTATE</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYCOUNTRYWORKED</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYMODE</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYBAND</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYDATE</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYTIMEOFF</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYTIMEON</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYRSTS</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYRSTR</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYPOWER</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYOTHER1</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYNAMER</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYFREQUENCY</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYCOMMENTS</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>TXTENTRYCALL</CONTROL><VALUE></VALUE></CMD>
<CMD><VISIBLEFIELDSRESPONSE><CONTROL>LBLDIALOGUE</CONTROL><VALUE>Ready to begin!</VALUE></CMD>

Africa All Mode: RST / Serial Received
ARRL Field Day: Class / Section
ARRL Kids Day: Name / Age / SPCNum
ARRL Rookie Roundup: Name / Check / SPCNum
ARRL RTTY: RST / SPCNum
ARRL School Club Roundup: RST / Class / SPCNum / Name (optional)
CQ WPX: RST / Serial Received
CQ WW RTTY: RST / CQ Zone (autofilled from call) / State (or Province)
Italian ARI International DX: RST / SPCNum
Jamboree On The Air (lots of unrequired fields)
NCJ North American QSO Party: Name / SPCNum
NCJ North American Sprint: Serial Received / Name / SPCNum
State QSO Parties, varies:
	Name
	State
	County
	Serial Received
	Section
	SPCNum
Ten Ten: Name / SPCNum / 1010 number
VHF: Grid / RST
Worked All Europe: RST / Serial Received (QTCs not coded in API)
Winter Field Day: Category (use class, just like ARRL Field Day) / Section

*/
