// =====================================================================
//
// n3fjp_logger.cxx
//
// interface to multiple n3fjp tcpip logbook services
//
// Copyright (C) 2016
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
// =====================================================================

#include <iostream>
#include <cmath>
#include <cstring>
#include <vector>
#include <list>
#include <stdlib.h>

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

LOG_FILE_SOURCE(debug::LOG_N3FJP);

using namespace std;

//======================================================================
// Socket N3FJP i/o used on all platforms
//======================================================================

pthread_t n3fjp_thread;
pthread_t n3fjp_rx_socket_thread;
Socket *n3fjp_socket = 0;

pthread_mutex_t n3fjp_mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_this_mutex = PTHREAD_MUTEX_INITIALIZER;

string send_this = "";

bool n3fjp_connected = false;
bool n3fjp_enabled   = false;
bool n3fjp_exit      = false;

string n3fjp_ip_address = "";
string n3fjp_ip_port    = "";

string n3fjp_rxbuffer;

enum {UNKNOWN, N3FJP, FLDIGI};

bool n3fjp_bool_add_record = false;
int n3fjp_has_xcvr_control = UNKNOWN;

double tracked_freq = 0;
int  tracked_mode = -1;

enum {FJP_NONE, FJP_FD, FJP_CQWWRTTY, FJP_NAQP, FJP_GENERIC};
int  n3fjp_contest = FJP_NONE;

int n3fjp_wait = 0;

void adjust_freq(string s);
void n3fjp_parse_response(string s);
void n3fjp_disp_report(string s, string fm = "");
void n3fjp_send(string cmd);
void n3fjp_rcv(string &rx);
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
bool n3fjp_dupcheck();
static void n3fjp_send_data();
static void n3fjp_send_data_norig();
void get_n3fjp_frequency();
void do_n3fjp_add_record_entries();
void n3fjp_set_freq(long f);
void n3fjp_set_ptt(int on);
void n3fjp_add_record(cQsoRec &record);
void n3fjp_parse_response(string tempbuff);
void n3fjp_rcv_data();
static void connect_to_n3fjp_server();
void n3fjp_start();
void n3fjp_restart();
void n3fjp_disconnect();
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
void n3fjp_show(string s)
{
	txt_N3FJP_data->insert(s.c_str());
	txt_N3FJP_data->redraw();
}

void n3fjp_disp_report(string s, string fm)
{
	string report = fm.append(s);
	size_t p = report.find("\r\n");
	while (p != string::npos) {
		report.replace(p,2,"<crlf>\n     ");
		p = report.find("\r\n");
	}
	report.erase(report.length() - 5);

	if (!progdefaults.enable_N3FJP_log)
		return;
	txt_N3FJP_data->insert(report.c_str());
	txt_N3FJP_data->redraw();

	std::string pathname = TempDir;
	pathname.append("n3fjp_data_stream.txt");
	FILE *n3fjplog = fopen(pathname.c_str(), "a");
	fprintf(n3fjplog, "%s", report.c_str());
	fclose(n3fjplog);

}

void n3fjp_send(string cmd)
{
	try {
//LOG_INFO("%s", cmd.c_str());
		cmd.append("\r\n");
		n3fjp_socket->send(cmd);
		n3fjp_disp_report(cmd, "SEND:");
	} catch (...) {
		throw;
	}
}

void n3fjp_rcv(string &rx)
{
	try {
		n3fjp_socket->recv(rx);
		n3fjp_disp_report(rx, "RCVD:");
	} catch (...) {
		throw;
	}
}


void n3fjp_send_freq_mode()
{
	if (!active_modem) return;

	string cmd;
	char szfreq[20];
	double freq = atof(inpFreq->value()) / 1e3;

	if (active_modem->get_mode() != tracked_mode ||
		tracked_freq != freq) {
		tracked_mode = active_modem->get_mode();
		tracked_freq = freq;
		snprintf(szfreq, sizeof(szfreq), "%f", tracked_freq);
		cmd = "<CMD><SENDRIGPOLL><FREQ>";
		cmd.append(szfreq);
		cmd.append("</FREQ><MODE>");
		cmd.append( mode_info[tracked_mode].adif_name );
		cmd.append("</MODE></CMD>");
		n3fjp_send(cmd);
	}
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
		n3fjp_send(cmd);
		n3fjp_wait = 100;
//		MilliSleep(5);
	} catch (const SocketException& e) {
		LOG_ERROR("Error: %d, %s", e.error(), e.what());
	}
}

//======================================================================
//
//======================================================================
bool n3fjp_calltab = false;

void n3fjp_getfields()
{
	string cmd ="<CMD><ALLFIELDSWITHVALUES></CMD>";
	try {
		n3fjp_send(cmd);
		n3fjp_wait = 100;
	} catch (const SocketException& e) {
		LOG_ERROR("Error: %d, %s", e.error(), e.what());
		n3fjp_calltab = false;
	}
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
		n3fjp_send(cmd0);
		n3fjp_send(cmd1);

		n3fjp_send(cmd2);
		n3fjp_calltab = false;

		n3fjp_wait = 100;

	} catch (const SocketException& e) {
		LOG_ERROR("Error: %d, %s", e.error(), e.what());
		n3fjp_calltab = false;
	}
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
	string name = ParseTextField(buffer, "NAMER");
	for (size_t n = 1; n < name.length(); n++) name[n] = tolower(name[n]);
	inpName->value(name.c_str());
	inpQth->value(ParseTextField(buffer, "COUNTYR").c_str());
	inpState->value(ParseTextField(buffer, "STATE").c_str());
	inpCountry->value(ParseTextField(buffer, "COUNTRYWORKED").c_str());
	inpLoc->value(ParseTextField(buffer, "GRID").c_str());
	adjust_freq(ParseTextField(buffer, "FREQUENCY"));

	inp_CQzone->value(ParseTextField(buffer, "CQZONE").c_str());
	inp_CQstate->value(ParseTextField(buffer, "STATE").c_str());

// comments field does not contain \n delimiters
// substitute \n for each '-'
	string comments = ParseTextField(buffer, "COMMENTS");
	size_t p = comments.find(" - ");
	while (p != string::npos) {
		comments.replace(p, 3, "\n");
		p = comments.find(" - ");
	}
	inpNotes->value(comments.c_str());
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
	inpCall->value(ParseField(buffer, "CALL").c_str());
	inpCountry->value(ParseField(buffer, "COUNTRY").c_str());
//	LOG_INFO("%s", buffer.c_str());
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
		return "DIG";

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
	if (freq >= 3.5 && freq <= 7.0) return "80";
	if (freq >= 7.0 && freq <= 7.5) return "40";
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
		n3fjp_send(cmd);
		n3fjp_wait = 100;
	} catch (...) {
		throw;
	}
}

static void send_action(const string action)
{
	string cmd;
	cmd.assign("<CMD><ACTION><VALUE>");
	cmd.append(action);
	cmd.append("</VALUE></CMD>");
	try {
		n3fjp_send(cmd);
		n3fjp_wait = 100;
	} catch (...) {
		throw;
	}
}

static void send_command(const string command, string val)
{
	string cmd;
	cmd.assign("<CMD><").append(command).append(">");
	if (!val.empty())
		cmd.append("<VALUE>").append(val).append("</VALUE>");
	cmd.append("</CMD>");
	try {
		n3fjp_send(cmd);
		MilliSleep(5);
		n3fjp_wait = 100;
	} catch (...) {
		throw;
	}
}

string last_lookup = "";
bool   last_dupcheck = false;

bool n3fjp_dupcheck()
{
	guard_lock rx_lock(&n3fjp_mutex);

	string chkcall = inpCall->value();
	if (chkcall == last_lookup) return last_dupcheck;

	if (chkcall.length() < 3) return false;
	if ((chkcall.length() == 3) && isdigit(chkcall[2])) return false;

	last_lookup = chkcall;

	string cmd;
	bool isdup = false;

	cmd.assign("<CMD><DUPECHECK><CALL>");
	cmd.append(strip(inpCall->value()));
	cmd.append("</CALL><BAND>");
	cmd.append(strip(n3fjp_opband()));
	cmd.append("</BAND><MODE>");
	cmd.append(strip(n3fjp_tstmode()));
	cmd.append("</MODE></CMD>");

	try {
		n3fjp_send(cmd);

		MilliSleep(200);

		string resp;
		n3fjp_rcv(resp);

		if (resp.find("Duplicate") != string::npos)
			isdup = true;
		n3fjp_parse_response(resp);

	} catch (...) {
		throw;
	}
	last_dupcheck = isdup;

	return isdup;
}

static cQsoRec rec;

static void n3fjp_send_data()
{
	try {
		send_command("IGNORERIGPOLLS", "TRUE");
		send_control("FREQUENCY", n3fjp_freq());
		send_control("BAND", n3fjp_opband());
		send_control("MODE", n3fjp_opmode());
		send_control("CALL", strip(rec.getField(CALL)));

		if (n3fjp_contest == FJP_NONE) {
			send_control("DATE", fmt_date(rec.getField(QSO_DATE)));
			send_control("TIMEON", fmt_time(rec.getField(TIME_ON)));
			send_control("TIMEOFF", fmt_time(rec.getField(TIME_OFF)));
			send_control("RSTS", rec.getField(RST_SENT));
			send_control("RSTR", rec.getField(RST_RCVD));
			send_control("NAMER", rec.getField(NAME));
			send_control("COMMENTS", rec.getField(NOTES));
			send_control("POWER", rec.getField(TX_PWR));
			send_control("STATE", rec.getField(STATE));
			send_control("GRIDR", rec.getField(GRIDSQUARE));
			send_control("QTHGROUP", rec.getField(QTH));
		}
		if (n3fjp_contest == FJP_FD) {
			send_control("MODETST", n3fjp_tstmode());
			send_control("CLASS", strip(ucasestr(rec.getField(FDCLASS))));
			send_control("SECTION", strip(ucasestr(rec.getField(FDSECTION))));
		}
		if (n3fjp_contest == FJP_CQWWRTTY) {
			send_control("RSTS", strip(rec.getField(RST_SENT)));
			send_control("RSTR", strip(rec.getField(RST_RCVD)));
			send_control("CQZONE", strip(rec.getField(CQZ)));
			send_control("STATE", strip(rec.getField(STATE)));
		}
		if (n3fjp_contest == FJP_GENERIC) {
			send_control("RSTS", strip(rec.getField(RST_SENT)));
			send_control("RSTR", strip(rec.getField(RST_RCVD)));
			send_control("SERIALNOR", strip(rec.getField(SRX)));
			send_control("SPCNUM", strip(ucasestr(rec.getField(XCHG1))));
		}
		if (n3fjp_contest == FJP_NAQP) {
			send_control("NAMER", rec.getField(NAME));
			if (strlen(rec.getField(STATE)) > 0)
				send_control("SPCNUM", rec.getField(STATE));
			else if (strlen(rec.getField(VE_PROV)) > 0)
				send_control("SPCNUM", rec.getField(VE_PROV));
		}

		string other = "XCVR:";
		char szfreq[6];
		snprintf(szfreq, sizeof(szfreq), "%d", (int)active_modem->get_txfreq());
		other.append(ModeIsLSB(rec.getField(MODE)) ? "LSB" : "USB");
		other.append(" MODE:");
		other.append(strip(rec.getField(MODE)));
		other.append(" WF:");
		other.append(szfreq);

		send_control("OTHER8", other);

		send_command("IGNORERIGPOLLS", "FALSE");
	} catch (...) {
		throw;
	}
}

static void n3fjp_send_data_norig()
{
	try {
		string cmd = "<CMD><CHANGEBM>";
		cmd.append("<BAND>").append(n3fjp_opband()).append("</BAND>");
		cmd.append("<MODE>").append(n3fjp_opmode()).append("</MODE>");
		cmd.append("</CMD>");
		n3fjp_send(cmd);

		send_control("CALL", strip(rec.getField(CALL)));

		if (n3fjp_contest == FJP_NONE) {
			send_control("FREQUENCY", n3fjp_freq());
			send_control("MODE", n3fjp_opmode());
			send_control("DATE", fmt_date(rec.getField(QSO_DATE)));
			send_control("TIMEON", fmt_time(rec.getField(TIME_ON)));
			send_control("TIMEOFF", fmt_time(rec.getField(TIME_OFF)));
			send_control("RSTS", strip(rec.getField(RST_SENT)));
			send_control("RSTR", strip(rec.getField(RST_RCVD)));
			send_control("NAMER", strip(rec.getField(NAME)));
			send_control("COMMENTS", strip(rec.getField(NOTES)));
			send_control("POWER", strip(rec.getField(TX_PWR)));
			send_control("STATE", strip(rec.getField(STATE)));
			send_control("GRIDR", strip(rec.getField(GRIDSQUARE)));
			send_control("QTHGROUP", strip(rec.getField(QTH)));
		}
		if (n3fjp_contest == FJP_FD) {
			send_control("MODETST", n3fjp_tstmode());
			send_control("CLASS", strip(ucasestr(rec.getField(FDCLASS))));
			send_control("SECTION", strip(ucasestr(rec.getField(FDSECTION))));
		}
		if (n3fjp_contest == FJP_CQWWRTTY) {
			send_control("MODETST", n3fjp_tstmode());
			send_control("RSTS", strip(rec.getField(RST_SENT)));
			send_control("RSTR", strip(rec.getField(RST_RCVD)));
			send_control("CQZONE", strip(rec.getField(CQZ)));
			send_control("STATE", strip(rec.getField(STATE)));
		}
		if (n3fjp_contest == FJP_GENERIC) {
			send_control("RSTS", strip(rec.getField(RST_SENT)));
			send_control("RSTR", strip(rec.getField(RST_RCVD)));
			send_control("SERIALNOR", strip(rec.getField(SRX)));
			send_control("SPCNUM", strip(ucasestr(rec.getField(XCHG1))));
		}
		if (n3fjp_contest == FJP_NAQP) {
			send_control("NAMER", rec.getField(NAME));
			if (strlen(rec.getField(STATE)) > 0)
				send_control("SPCNUM", rec.getField(STATE));
			else if (strlen(rec.getField(VE_PROV)) > 0)
				send_control("SPCNUM", rec.getField(VE_PROV));
		}

		string other = "XCVR:";
		char szfreq[6];
		snprintf(szfreq, sizeof(szfreq), "%d", (int)active_modem->get_txfreq());
		other.append(ModeIsLSB(rec.getField(MODE)) ? "LSB" : "USB");
		other.append(" MODE:");
		other.append(strip(rec.getField(MODE)));
		other.append(" WF:");
		other.append(szfreq);

		send_control("OTHER8", other);

	} catch (...) {
		throw;
	}
}

void get_n3fjp_frequency()
{
	try {
		send_command("READBMF");
	} catch (...) {
		throw;
	}
}

void do_n3fjp_add_record_entries()
{
	if(!n3fjp_socket) return;
	if (!n3fjp_connected) return;

	string cmd, response, val;

	try {
		if (n3fjp_has_xcvr_control == N3FJP)
			n3fjp_send_data();
		else
			n3fjp_send_data_norig();

		send_action("ENTER");

	} catch (const SocketException& e) {
		LOG_ERROR("Error: %d, %s", e.error(), e.what());
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
	if (on)
		cmd.append("<CWCOMPORTKEYDOWN>");
	else
		cmd.append("<CWCOMPORTKEYUP>");
	cmd.append("</CMD>");

	{	guard_lock send_lock(&send_this_mutex);
		send_this = cmd;
	}
}

void n3fjp_add_record(cQsoRec &record)
{
	rec = record;
	n3fjp_bool_add_record = true;
}

void n3fjp_request_next_serial_number()
{
	send_command("<CMD><NEXTSERIALNUMBER></CMD>");
}

string n3fjp_serno = "";

void n3fjp_parse_next_serial(string buff)
{
	n3fjp_serno = ParseValueField("NEXTSERIALNUMBERRESPONSE", buff);
	clearQSO();
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
		REQ(n3fjp_request_next_serial_number);
	}
	if (tempbuff.find("<NEXTSERIALNUMBERRESPONSE>") != string::npos) {
		REQ(n3fjp_parse_next_serial, tempbuff);
	}
}

//======================================================================
//
//======================================================================
void n3fjp_rcv_data()
{
	string tempbuff = "";
	try {
		n3fjp_rcv(tempbuff);
		n3fjp_parse_response(tempbuff);
	} catch (const SocketException& e) {
		LOG_ERROR("Error %d, %s", e.error(), e.what());
	}
}

static void connect_to_n3fjp_server()
{
	try {
		n3fjp_serno.clear();

		if (!n3fjp_connected)
			n3fjp_socket->connect();

		if (!n3fjp_socket->is_connected()) return;

		std::string pathname = TempDir;
		pathname.append("n3fjp_data_stream.txt");
		FILE *n3fjplog = fopen(pathname.c_str(), "w");
		fprintf(n3fjplog, "N3FJP / fldigi tcpip log\n\n");
		fclose(n3fjplog);

		string buffer;
		string cmd = "<CMD><PROGRAM></CMD>";
		n3fjp_send(cmd);
		MilliSleep(100);

		n3fjp_rcv(buffer);
		if (buffer.empty()) return;

		n3fjp_contest = FJP_NONE;

		string info = ParseField(buffer, "PGM");
		if (info.find("Amateur Contact Log") != string::npos)
			n3fjp_contest = FJP_NONE;
		else if (info.find("CQ WW DX RTTY Contest Log") != string::npos)
			n3fjp_contest = FJP_CQWWRTTY;
		else if (info.find("Field Day Contest") != string::npos)
			n3fjp_contest = FJP_FD;
		else if (info.find("NAQP Contest Log") != string::npos)
			n3fjp_contest = FJP_NAQP;
		else {
			n3fjp_contest = FJP_GENERIC;
			REQ(n3fjp_request_next_serial_number);
		}

		info.insert(0, "Connected to ");

		string ver = ParseField(buffer, "VER");
		info.append(", Ver ").append(ver);

		n3fjp_connected = true;

		cmd = "<CMD><CALLTABENTEREVENTS><VALUE>TRUE</VALUE></CMD>";
		n3fjp_send(cmd);

		cmd = "<CMD><READOFFSETENABLED></CMD>";
		n3fjp_send(cmd);

		cmd = "<CMD><READOFFSET></CMD>";
		n3fjp_send(cmd);

		cmd = "<CMD><READMODEDEFAULTSUPPRESS></CMD>";
		n3fjp_send(cmd);

//		cmd = "<CMD><SETUPDATESTATE><VALUE>TRUE</VALUE></CMD>";
//		n3fjp_send(cmd);

		cmd = "RIGENABLED";
		send_command(cmd);

	} catch (const SocketException& e) {
		string err;
		err = e.what();
		err.append("\n");
		REQ(n3fjp_show, err);
		LOG_INFO("%s(%d)", e.what(), e.error());
	}

}

//======================================================================
//
//======================================================================

void n3fjp_start()
{
	static bool firstreport = true;

	n3fjp_ip_address =  progdefaults.N3FJP_address;
	n3fjp_ip_port = progdefaults.N3FJP_port;

	try {
		n3fjp_socket = new Socket(
				Address( n3fjp_ip_address.c_str(),
						 n3fjp_ip_port.c_str(),
						 "tcp") );
		if (!n3fjp_socket) return;
		n3fjp_socket->set_timeout(0.01);
		n3fjp_socket->set_nonblocking(true);
		LOG_INFO("Client socket %d", n3fjp_socket->fd());
		firstreport = true;
	}
	catch (const SocketException& e) {
		if (firstreport) {
			LOG_INFO("%s", e.what() );
			firstreport = false;
		}
		delete n3fjp_socket;
		n3fjp_socket = 0;
		n3fjp_connected = false;
		n3fjp_has_xcvr_control = UNKNOWN;
	}
}

void n3fjp_restart()
{
	static bool firstreport = true;

	n3fjp_ip_address =  progdefaults.N3FJP_address;
	n3fjp_ip_port = progdefaults.N3FJP_port;

	try {
		n3fjp_socket->shut_down();
		n3fjp_socket->close();
		delete n3fjp_socket;
		n3fjp_connected = false;
		n3fjp_socket = new Socket(
				Address( n3fjp_ip_address.c_str(),
						 n3fjp_ip_port.c_str(),
						 "tcp") );
		n3fjp_socket->set_timeout(0.01);
		n3fjp_socket->set_nonblocking(true);
		LOG_INFO("Client socket %d", n3fjp_socket->fd());
		firstreport = true;
	}
	catch (const SocketException& e) {
		if (firstreport) {
			LOG_INFO("%s", e.what() );
			firstreport = false;
		}
		delete n3fjp_socket;
		n3fjp_socket = 0;
		n3fjp_connected = false;
		n3fjp_has_xcvr_control = UNKNOWN;
	}
}

//======================================================================
// Disconnect from N3FJP tcpip server
//======================================================================
void n3fjp_disconnect()
{
	n3fjp_send("");
	n3fjp_socket->shut_down();
	n3fjp_socket->close();
	delete n3fjp_socket;
	n3fjp_socket = 0;
	n3fjp_connected = false;
	n3fjp_has_xcvr_control = UNKNOWN;
	LOG_INFO("Disconnected");
	n3fjp_serno.clear();
	REQ(clearQSO);
}

//======================================================================
// Thread loop
//======================================================================

void *n3fjp_loop(void *args)
{
	SET_THREAD_ID(N3FJP_TID);

	int n3fjp_looptime = 100;
	while(1) {
		if (n3fjp_exit) break;

		MilliSleep(10);
		if (n3fjp_wait) n3fjp_wait -= 10;
		if (n3fjp_looptime) n3fjp_looptime -= 10;

		if (n3fjp_wait > 0) continue;

		if (n3fjp_looptime > 0) continue;

//		if (!n3fjp_connected) n3fjp_looptime = 5000;  // test for N3FJP logger every 5 sec
//		else n3fjp_looptime = 250;  // r/w to N3FJP logger every 1/4 second
		n3fjp_looptime = 200;

		if (!n3fjp_socket || (n3fjp_socket->fd() == -1)) {
			n3fjp_start();
		}

		else if (n3fjp_socket) {
			if ((n3fjp_ip_address != progdefaults.N3FJP_address) ||
				(n3fjp_ip_port != progdefaults.N3FJP_port) ) {
				n3fjp_restart();
			}

			else if (!n3fjp_connected && progdefaults.connect_to_n3fjp ) {
				connect_to_n3fjp_server();
			} 

			else if (n3fjp_connected && !progdefaults.connect_to_n3fjp)
				n3fjp_disconnect();

			else if (n3fjp_connected) {
				try {
					if (n3fjp_has_xcvr_control == FLDIGI)
						n3fjp_send_freq_mode();
					if (!send_this.empty()) {
						guard_lock send_lock(&send_this_mutex);
						n3fjp_send(send_this);
						send_this.clear();
					} else if (n3fjp_bool_add_record)
						do_n3fjp_add_record_entries();
					else {
						guard_lock rx_lock(&n3fjp_mutex);
						n3fjp_rcv_data();
					}
				} catch (const SocketException& e) {
					LOG_ERROR("%s", e.what() );
					delete n3fjp_socket;
					n3fjp_socket = 0;
					n3fjp_connected = false;
					n3fjp_has_xcvr_control = UNKNOWN;
				}
			}
		}
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

	n3fjp_enabled = true;
}

//======================================================================
//
//======================================================================
void n3fjp_close(void)
{
	if (!n3fjp_enabled) return;

	n3fjp_exit = true;
	CANCEL_THREAD(n3fjp_thread);
	pthread_join(n3fjp_thread, NULL);
	n3fjp_enabled = false;

	LOG_INFO("%s", "N3FJP logger thread terminated. ");

	if(n3fjp_socket) {
		n3fjp_socket->shut_down();
		n3fjp_socket->close();
	}

}

