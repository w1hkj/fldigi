// =====================================================================
//
// dxcluster.cxx
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

#include "fl_digi.h"
#include "rigsupport.h"
#include "modem.h"
#include "trx.h"
#include "configuration.h"
#include "main.h"
#include "waterfall.h"
#include "macros.h"
#include "qrunner.h"
#include "debug.h"
#include "status.h"
#include "icons.h"
#include "threads.h"

#include "fileselect.h"

//#include "logsupport.h"
#include "dx_dialog.h"
#include "dx_cluster.h"

#include "confdialog.h"

LOG_FILE_SOURCE(debug::LOG_FD);

//#define DXC_DEBUG 1

using namespace std;

pthread_mutex_t dxcc_mutex     = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t dxc_line_mutex = PTHREAD_MUTEX_INITIALIZER;

//forward declarations of local functions
void DXcluster_start();

static char even[50];
static char odd[50];

//======================================================================
// Socket DXcluster i/o used on all platforms
//======================================================================

pthread_t DXcluster_thread;
pthread_mutex_t DXcluster_mutex     = PTHREAD_MUTEX_INITIALIZER;

Socket *DXcluster_socket = 0;

enum DXC_STATES {DISCONNECTED, CONNECTING, CONNECTED};
int  DXcluster_state = DISCONNECTED;
bool DXcluster_exit = false;
bool DXcluster_enabled = false;

#define DXCLUSTER_CONNECT_TIMEOUT 5000 // 5 second timeout
#define DXCLUSTER_SOCKET_TIMEOUT 100 // milliseconds
#define DXCLUSTER_LOOP_TIME 100 // milliseconds
int  DXcluster_connect_timeout =
	(DXCLUSTER_CONNECT_TIMEOUT) / (DXCLUSTER_SOCKET_TIMEOUT + DXCLUSTER_LOOP_TIME);


//======================================================================
// support routines all called from DXcluster thread using REQ(...)
//======================================================================

void set_btn_dxcc_connect(bool v)
{
	btn_dxcc_connect->value(v);
	btn_dxcc_connect->redraw();
}

void dxc_label()
{
	switch (DXcluster_state) {
		case CONNECTING:
			lbl_dxc_connected->color(FL_YELLOW);
			cluster_tabs->value(tabDXclusterTelNetStream);
			break;
		case CONNECTED:
			lbl_dxc_connected->color(FL_GREEN);
			cluster_tabs->value(tabDXclusterTelNetStream);
			break;
		case DISCONNECTED:
		default :
			lbl_dxc_connected->color(FL_WHITE);
	}
	lbl_dxc_connected->redraw_label();
	lbl_dxc_connected->redraw();
}

static string trim(string s)
{
	size_t p;
	while (s[s.length()-1] == ' ') s.erase(s.length()-1, 1);
	while ( (p = s.find("\x07")) != string::npos) s.erase(p, 1);
	while ((p = s.find("\r")) != string::npos) s.erase(p,1);
	while ((p = s.find("\n")) != string::npos) s.erase(p,1);
	return s;
}

static string ucasestr(string s)
{
	for (size_t n = 0; n < s.length(); n++) s[n] = toupper(s[n]);
	return s;
}

//--------------------------------------------------------------------------------
//          1         2         3         4         5         6         7
//01234567890123456789012345678901234567890123456789012345678901234567890123456789
//          1     ^   2     ^   3        ^4         5         6         ^
//DX de KB8O:      14240.0  D66D         up 10 59 Ohio                  2059Z EN81<cr><lf>
//DX de W4DL:      18082.0  V44KAI                                      2220Z<cr><lf>
//DX de K2IOF:     14240.0  D66D         up 5                           2220Z<cr><lf>
//DX de N3ZV:      10147.0  AN400G       RTTY                           2220Z FM18<BELL><BELL><cr><lf>
//DX de W4LT:      14204.0  EA3HSO                                      2218Z EL88<BELL><BELL><cr><lf>
//--------------------------------------------------------------------------------

static string tcpip_buffer;

void show_tx_stream(string buff)
{
	size_t p;
	while ((p = buff.find("\r")) != string::npos) buff.erase(p,1);
	if (buff[buff.length()-1] != '\n') buff += '\n';

	brws_tcpip_stream->insert_position(brws_tcpip_stream->buffer()->length());
	brws_tcpip_stream->addstr(buff, FTextBase::XMIT);

#ifdef DXC_DEBUG
	string pname = "TempDir";
	pname.append("dxcdebug.txt", "a");
	FILE *dxcdebug = fopen(pname.c_str(), "a");
	fprintf(dxcdebug, "[T]:%s\n", buff.c_str());
	fclose(dxcdebug);
#endif
}

void show_rx_stream(string buff)
{
	for (size_t p = 0; p < buff.length(); p++) {
		if ( (buff[p] < '\n') ||
			 (buff[p] > '\n' && buff[p] < ' ') ||
			 (buff[p] > '~') )
			buff[p] = ' ';
	}

	if (buff.empty()) buff = "\n";
	else if (buff[buff.length()-1] != '\n') buff += '\n';

	brws_tcpip_stream->insert_position(brws_tcpip_stream->buffer()->length());
	brws_tcpip_stream->addstr(buff, FTextBase::RECV);

#ifdef DXC_DEBUG
	string pname = "TempDir";
	pname.append("dxcdebug.txt", "a");
	FILE *dxcdebug = fopen(pname.c_str(), "a");
	fprintf(dxcdebug, "[R]:%s\n", buff.c_str());
	fclose(dxcdebug);
#endif
}

void show_error(string buff)
{
	if (buff.empty()) return;
	if (!brws_tcpip_stream) return;
	if (buff[buff.length()-1] != '\n') buff += '\n';
	brws_tcpip_stream->addstr(buff, FTextBase::CTRL);
	brws_tcpip_stream->redraw();

#ifdef DXC_DEBUG
	string pname = "TempDir";
	pname.append("dxcdebug.txt", "a");
	FILE *dxcdebug = fopen(pname.c_str(), "a");
	fprintf(dxcdebug, "[E]:%s\n", buff.c_str());
	fclose(dxcdebug);
#endif
}

#include <queue>
void dxc_lines()
{
	guard_lock dxcc_lock(&dxc_line_mutex);
	int n = brws_dx_cluster->size();

	if (n == 0) return;

	queue<string> lines;
	string dxc_line;

	size_t p;
	for (int i = 0; i < n; i++) {
		if (progdefaults.dxc_topline)
			dxc_line = brws_dx_cluster->text(i+1);
		else
			dxc_line = brws_dx_cluster->text(n - i);
		p = dxc_line.find(".");
		if (p != string::npos) dxc_line.erase(0,p+1);
		lines.push(dxc_line);
	}

	brws_dx_cluster->clear();

	for (int i = 0; i < n; i++) {
		if (i % 2)
			dxc_line.assign(even);
		else
		dxc_line.assign(odd);

		dxc_line.append(lines.front());
		lines.pop();

		if (progdefaults.dxc_topline)
			brws_dx_cluster->insert(1, dxc_line.c_str());
		else
			brws_dx_cluster->add(dxc_line.c_str());
	}

	if (progdefaults.dxc_topline)
		brws_dx_cluster->make_visible(1);
	else
		brws_dx_cluster->bottomline(brws_dx_cluster->size());

	brws_dx_cluster->redraw();
}

void parse_dxline(string buffer)
{
	guard_lock dxcc_lock(&dxc_line_mutex);

	snprintf(odd, sizeof(odd), "@B%u@C%u@F%d@S%d@.",
			progdefaults.DXC_odd_color,
			FL_BLACK,
			progdefaults.DXC_textfont,
			progdefaults.DXC_textsize);

	snprintf(even, sizeof(even), "@B%u@C%u@F%d@S%d@.",
		progdefaults.DXC_even_color,
		FL_BLACK,
		progdefaults.DXC_textfont,
		progdefaults.DXC_textsize);

	buffer.erase(0, strlen("DX de "));
	size_t p = buffer.find(":");
	if (p != string::npos) buffer.replace(p, 1, " ");

	string dxc_line;

	if (brws_dx_cluster->size() % 2)
		dxc_line.assign(even);
	else
		dxc_line.assign(odd);

	dxc_line.append(buffer);

	if (progdefaults.dxc_topline) {
		bool visible = brws_dx_cluster->displayed(1);
		brws_dx_cluster->insert(1, dxc_line.c_str());
		if (visible) brws_dx_cluster->make_visible(1);
	} else {
		bool visible = brws_dx_cluster->displayed(brws_dx_cluster->size());
		brws_dx_cluster->add(dxc_line.c_str());
		if (visible) brws_dx_cluster->bottomline(brws_dx_cluster->size());
	}
}

void show_help_line(string buff)
{
	brws_dxc_help->insert_position(brws_dxc_help->buffer()->length());
	brws_dxc_help->addstr(buff, FTextBase::RECV);

#ifdef DXC_DEBUG
	string pname = "TempDir";
	pname.append("dxcdebug.txt", "a");
	FILE *dxcdebug = fopen(pname.c_str(), "a");
	fprintf(dxcdebug, "[W]:%s\n", buff.c_str());
	fclose(dxcdebug);
#endif
}

enum server_type {NIL, DX_SPIDER, AR_CLUSTER, CC_CLUSTER};
static int  cluster_login = NIL;
static bool logged_in = false;

void login_to_dxspider()
{
	if (!DXcluster_socket) return;
	try {
		string login = progdefaults.dxcc_login;
		login.append("\r\n");
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		login.assign("set/page 0\r\n");
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		login.assign("set/name ").append(progdefaults.myName);
		login.append("\r\n");
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		login.assign("set/qth ").append(progdefaults.myQth);
		login.append("\r\n");
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		login.assign("set/qra ").append(progdefaults.myLocator);
		login.append("\r\n");
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		logged_in = true;
		cluster_login = NIL;
	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		REQ(show_error, e.what());
	}
}

void login_to_arcluster()
{
	string login = progdefaults.dxcc_login;
	login.append("\r\n");
	try {
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		logged_in = true;
		cluster_login = NIL;
	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		REQ(show_error, e.what());
	}
}

/*
 * telnet session with dxspots.com 7300
 *
Trying 204.221.76.52...
Connected to dxspots.com.
Escape character is '^]'.
Greetings from the AE5E CC Cluster in Thief River Falls MN USA

Running CC Cluster software version 3.101b

*************************************************************************
*                                                                       *
*     Please login with a callsign indicating your correct country      *
*                          Portable calls are ok.                       *
*                                                                       *
************************************************************************

New commands:

set/skimmer   turns on Skimmer spots.
set/noskimmer turns off Skimmer spots.

set/own      turns on Skimmer spots for own call.
set/noown    turns them off.

set/nobeacon  turns off spots for beacons.
set/beacon    turns them back on.

For information on CC Cluster software see:

http://bcdxc.org/ve7cc/ccc/CCC_Commands.htm
Please enter your call:

login: w1hkj
W1HKJ

Hello David.

CC-User is the recommended telnet acess program. It simplifies filtering,
may be used stand alone or can feed spots to your logging program.
CC_User is free at: http://ve7cc.net/default.htm#prog
CC_User group at: http://groups.yahoo.com/group/ARUser

Using telnet port 7000

Cluster: 423 nodes  459 Locals  4704 Total users   Uptime   7 days 15:32

Date        Hour   SFI   A   K Forecast                               Logger
19-Nov-2016    0    78   3   0 No Storms -> No Storms                <VE7CC>
W1HKJ de AE5E 19-Nov-2016 0154Z   CCC >
W1HKJ de AE5E 19-Nov-2016 0154Z   CCC >
bye
73 David.  W1HKJ de AE5E 19-Nov-2016 0154Z  CCC >
Connection closed by foreign host.

*/

void login_to_cccluster()
{
	if (!DXcluster_socket) return;

	try {
		string login = progdefaults.dxcc_login;
		login.append("\r\n");
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		login.assign("set/name ").append(progdefaults.myName);
		login.append("\r\n");
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		login.assign("set/qth ").append(progdefaults.myQth);
		login.append("\r\n");
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		login.assign("set/qra ").append(progdefaults.myLocator);
		login.append("\r\n");
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		login.assign("set/noskimmer");
		login.append("\r\n");
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		login.assign("set/noown");
		login.append("\r\n");
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		login.assign("set/nobeacon");
		login.append("\r\n");
		DXcluster_socket->send(login);
		REQ(show_tx_stream, login);

		logged_in = true;
		cluster_login = NIL;
	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		REQ(show_error, e.what());
	}
}

static bool help_lines = false;

void init_cluster_stream()
{
	string buffer;
	help_lines = false;
	tcpip_buffer.clear();
	brws_tcpip_stream->clear();
	brws_tcpip_stream->redraw();
}

void parse_DXcluster_stream(string input_buffer)
{
	guard_lock dxcc_lock(&dxcc_mutex);
	string buffer;
	string ucasebuffer = ucasestr(input_buffer);

	if (!logged_in) {
		if (ucasebuffer.find("AR-CLUSTER") != string::npos) { // AR cluster
			init_cluster_stream();
			cluster_login = AR_CLUSTER;
		}
		else if (ucasebuffer.find("CC CLUSTER") != string::npos) { // CC cluster
			init_cluster_stream();
			cluster_login = CC_CLUSTER;
		}
		else if (ucasebuffer.find("LOGIN:") != string::npos) { // DX Spider
			init_cluster_stream();
			cluster_login = DX_SPIDER;
		}
	}

	tcpip_buffer.append(input_buffer);

	string strm;
	string its_me = progdefaults.dxcc_login;
	its_me.append(" DE ");
	its_me = ucasestr(its_me);
	size_t p;

	while (!tcpip_buffer.empty()) {
		p = tcpip_buffer.find("\n");
		if (p != string::npos) {
			buffer = trim(tcpip_buffer.substr(0, p));
			tcpip_buffer.erase(0, p + 1);
		} else {
			buffer = trim(tcpip_buffer);
			tcpip_buffer.clear();
		}

		REQ(show_rx_stream, buffer);

		ucasebuffer = ucasestr(buffer);
		if (ucasebuffer.find("DX DE") != string::npos) {
			parse_dxline(buffer);
			continue;
		}

		if (ucasebuffer.find("HELP") != string::npos) {
			help_lines = true;
		}
		if (ucasebuffer.find(its_me) != string::npos) {
			help_lines = false;
			continue;
		}
		if (help_lines) {
			show_help_line(buffer);
			continue;
		}

		if (cluster_login == AR_CLUSTER && ucasebuffer.find("CALL:") != string::npos)
			login_to_arcluster();

		if (cluster_login == CC_CLUSTER && ucasebuffer.find("CALL:") != string::npos)
			login_to_cccluster();

		if (cluster_login == DX_SPIDER && ucasebuffer.find("LOGIN:") != string::npos)
			login_to_dxspider();
	}
}

void clear_dxcluster_viewer()
{
	guard_lock dxcc_lock(&dxcc_mutex);
	brws_dx_cluster->clear();
}

//======================================================================
// Receive tcpip data stream
//======================================================================
void DXcluster_recv_data()
{
	if (!DXcluster_socket) return;
	string tempbuff;
	try {
		guard_lock dxc_lock(&DXcluster_mutex);
		DXcluster_socket->recv(tempbuff);
		if (tempbuff.empty()) {
			if (DXcluster_state == CONNECTING) {
				DXcluster_connect_timeout--;
				if (DXcluster_connect_timeout <= 0) {
					REQ(show_error, "Connection attempt timed out");
					DXcluster_state = DISCONNECTED;
					set_btn_dxcc_connect(false);
					REQ(dxc_label);
					DXcluster_socket->shut_down();
					DXcluster_socket->close();
					delete DXcluster_socket;
					DXcluster_socket = 0;
				}
			}
			return;
		}
		if (DXcluster_state == CONNECTING) {
			DXcluster_state = CONNECTED;
			REQ(dxc_label);

			LOG_INFO( "Connected to dxserver %s:%s",
				progdefaults.dxcc_host_url.c_str(),
				progdefaults.dxcc_host_port.c_str() );
		}
		REQ(parse_DXcluster_stream, tempbuff);
	} catch (const SocketException& e) {
		LOG_ERROR("Error %d, %s", e.error(), e.what());
	}
}

//======================================================================
//
//======================================================================
const char *default_help[]={
"Help available after logging on",
"Try URL: k4zr.no-ip.org, PORT 7300",
"",
"Visit http://www.dxcluster.info/telnet/ for a listing of dx cluster servers",
NULL };

void dxc_help_query()
{
//	brws_dxc_help->clear();
	if ((DXcluster_state == DISCONNECTED) || !DXcluster_socket) {
		brws_dxc_help->clear();
		const char **help = default_help;
		while (*help) {
			brws_dxc_help->add(*help);
			help++;
		}
		return;
	}

	try {
		guard_lock dxc_lock(&DXcluster_mutex);
		string sendbuf = "help";
		if (inp_help_string->value()[0]) {
			string helptype = inp_help_string->value();
			if (ucasestr(helptype).find("HELP") != string::npos)
				sendbuf = helptype;
			else
				sendbuf.append(" ").append(helptype);
		}
		sendbuf.append("\r\n");
		DXcluster_socket->send(sendbuf.c_str());
		REQ(show_tx_stream, sendbuf);
	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		REQ(show_error, e.what());
	}

}

void dxc_help_clear()
{
	guard_lock dxcc_lock(&dxcc_mutex);
	brws_dxc_help->clear();
}

//======================================================================
//
//======================================================================
void DXcluster_submit()
{
	if (!DXcluster_socket) return;
	try {
		guard_lock dxc_lock(&DXcluster_mutex);
		string sendbuf = trim(inp_dxcluster_cmd->value());
		string test = ucasestr(sendbuf);
		if (test.find("BYE") != string::npos) {
			fl_alert2("Uncheck the \"Connect\" button to disconnect!");
			logged_in = false;
			return;
		}
		sendbuf.append("\r\n");
		DXcluster_socket->send(sendbuf.c_str());
		REQ(show_tx_stream, sendbuf);
	} catch (const SocketException& e) {
		LOG_ERROR("%s", e.what() );
		REQ(show_error, e.what());
	}
	inp_dxcluster_cmd->value("");
}

void rf_af(long &rf, long &af)
{
	mode_t md = active_modem->get_mode();

	if (md == MODE_SSB) {
		af = 0;
		return;
	}

	if (md == MODE_CW) {
		af = progdefaults.CWsweetspot;
		if (wf->USB()) rf -= af;
		else           rf += af;
	}
	else if (md == MODE_RTTY) {
		int shift;
		switch (progdefaults.rtty_shift) {
		case 0 : shift = 23; break;
		case 1 : shift = 85; break;
		case 2 : shift = 160; break;
		case 3 : shift = 170; break;
		case 4 : shift = 182; break;
		case 5 : shift = 200; break;
		case 6 : shift = 240; break;
		case 7 : shift = 350; break;
		case 8 : shift = 425; break;
		case 9 : shift = 850; break;
		default: shift = 0;
		}
		af = progdefaults.RTTYsweetspot;
		if (progdefaults.useMARKfreq) {
			if (wf->USB()) rf -= (af + shift / 2);
			else           rf += (af - shift / 2);
		}
		else {
			if (wf->USB()) rf -= af;
			else           rf += af;
		}
	} else {
		af = progdefaults.PSKsweetspot;
		if (wf->USB()) rf -= af;
		else           rf += af;
	}
	return;
}

void DXcluster_select()
{
	int sel = brws_dx_cluster->value();
	if (sel == 0) return;

//--------------------------------------------------------------------------------
//          1         2         3         4         5         6         7
//01234567890123456789012345678901234567890123456789012345678901234567890123456789
//          1^        ^         3  ^      4         5         6   ^
//KB8O       14240.0  D66D         up 10 59 Ohio                  2059Z EN81<cr><lf>
	string dxcline = brws_dx_cluster->text(sel);
	size_t p = dxcline.find("@.");
	if (p == string::npos)
		return;

// remove rendition characters
	dxcline = dxcline.substr(p + 2);

// remove reporting stations call
	p = dxcline.find(" ");
	dxcline.erase(0, p+1);

// find reported frequency
	while (dxcline[0] == ' ') dxcline.erase(0,1);
	p = dxcline.find(" ");
	string sfreq = dxcline.substr(0, p);
	dxcline.erase(0, p+1);

// find dx call
	while (dxcline[0] == ' ') dxcline.erase(0,1);
	p = dxcline.find(" ");
	string dxcall = trim(dxcline.substr(0, p));
	dxcline.erase(0, p+1);

// treat remainder as remarks
// search for a mode name in the remarks
// change to that mode if discovered
	dxcline = ucasestr(dxcline);
	for (int i = 0; i < NUM_MODES-3; i++) {
		if (dxcline.find(mode_info[i].adif_name) != string::npos) {
			if (active_modem->get_mode() != mode_info[i].mode)
				init_modem_sync(mode_info[i].mode);
			break;
		}
	}

	inpCall->value(dxcall.c_str());

	long freq = (long)(atof(sfreq.c_str()) * 1000.0 + 0.5);
// does remark section have a [nn] block?
	p = dxcline.find("[");
	if (p != string::npos) {
		dxcline.erase(0, p+1);
		p = dxcline.find("]");
		if (p == 2)
			freq += atoi(dxcline.c_str());
	}
	long af = 1500;

	rf_af(freq, af);
	qsy(freq, af);
}

//--------------------------------------------------------------------------------
//          1         2         3         4         5         6         7
//01234567890123456789012345678901234567890123456789012345678901234567890123456789
//          1     ^   2     ^   3        ^4         5         6         ^
//7080.4 CO3VR Optional Comment

void freqstrings( string &khz, string &hz )
{
	string sfreq = inpFreq->value();

	int phz = sfreq.length() - 2;
	hz = sfreq.substr(phz, 2);
	sfreq.erase(phz);

	size_t p = sfreq.find(".");
	if (p != string::npos) sfreq.erase(p,1);

	long freq = atol(sfreq.c_str());

	if (!progdefaults.dxc_hertz) {
		if (hz > "49") freq++;
	}
	char szfreq[20];
	snprintf(szfreq, sizeof(szfreq), "%ld", freq);
	khz = szfreq;
	khz.insert(khz.length() - 1, ".");

	return;
}

void send_DXcluster_spot()
{
	if (inpCall->value()[0] == 0) return;  // no call

	string hz, khz;
	freqstrings( khz, hz );

	string spot = "dx ";
	spot.append(khz)
		.append(" ")
		.append(inpCall->value())
		.append(" ");

	string comments = trim(inp_dxcluster_cmd->value());
	string currmode = mode_info[active_modem->get_mode()].adif_name;

	if (comments.find(currmode) == string::npos) {
		if (progdefaults.dxc_hertz) {
			currmode.append(" [")
					.append(hz).
					append("] ");
		}
		comments.insert(0, currmode);
	}
	spot.append(comments);

	inp_dxcluster_cmd->value(spot.c_str());
	tabDXclusterTelNetStream->show();

}

//======================================================================
//
//======================================================================

bool connect_changed;
bool connect_to_cluster;

void DXcluster_doconnect()
{
	if (connect_to_cluster) {
		REQ(clear_dxcluster_viewer);
		try {
			if (DXcluster_socket) {
				DXcluster_socket->shut_down();
				DXcluster_socket->close();
				delete DXcluster_socket;
				DXcluster_socket = 0;
				DXcluster_state = DISCONNECTED;
				REQ(dxc_label);
			}
			Address addr = Address(
				progdefaults.dxcc_host_url.c_str(),
				progdefaults.dxcc_host_port.c_str() );

			DXcluster_socket = new Socket( addr );

			DXcluster_socket->set_nonblocking(true);
			DXcluster_socket->set_timeout((double)(DXCLUSTER_SOCKET_TIMEOUT / 1000.0));

			DXcluster_socket->connect();

			DXcluster_state = CONNECTING;
			REQ(dxc_label);
			string temp = "Connecting to ";
			temp.append(progdefaults.dxcc_host_url).append(":");
			temp.append(progdefaults.dxcc_host_port);
			REQ(show_error, temp);
			DXcluster_connect_timeout =
				(DXCLUSTER_CONNECT_TIMEOUT) / (DXCLUSTER_SOCKET_TIMEOUT + DXCLUSTER_LOOP_TIME);
		} catch (const SocketException& e) {
			LOG_ERROR("%s", e.what() );
			delete DXcluster_socket;
			DXcluster_socket = 0;
			DXcluster_state = DISCONNECTED;
			progStatus.cluster_connected = false;
			REQ(show_error, e.what());
			set_btn_dxcc_connect(false);
			REQ(dxc_label);
			logged_in = false;
		}
	}
	else {
		if (!DXcluster_socket) return;

		try {
			string bye = "BYE\r\n";
			DXcluster_socket->send(bye);
			REQ(show_tx_stream, bye);
		} catch (const SocketException& e) {
			LOG_ERROR("%s", e.what() );
			REQ(show_error, e.what());
		}

		MilliSleep(50);
		logged_in = false;

		DXcluster_socket->shut_down();
		DXcluster_socket->close();
		delete DXcluster_socket;
		DXcluster_socket = 0;
		DXcluster_state = DISCONNECTED;
		lbl_dxc_connected->color(FL_WHITE);
		lbl_dxc_connected->redraw();
		progStatus.cluster_connected = false;
		set_btn_dxcc_connect(false);
		LOG_INFO("Disconnected from dxserver");
	}
	connect_changed = false;
}

void DXcluster_connect(bool val)
{
	connect_changed = true;
	connect_to_cluster = val;
}

//======================================================================
// Thread loop
//======================================================================
void *DXcluster_loop(void *args)
{
	SET_THREAD_ID(DXCC_TID);

	while(1) {
		MilliSleep(DXCLUSTER_LOOP_TIME);
		if (DXcluster_exit) break;
		if (connect_changed) DXcluster_doconnect();
		if (DXcluster_state != DISCONNECTED) DXcluster_recv_data();
	}
	// exit the DXCC thread
	SET_THREAD_CANCEL();
	return NULL;
}

//======================================================================
//
//======================================================================
void DXcluster_init(void)
{
	DXcluster_enabled = false;
	DXcluster_exit = false;

	if (pthread_create(&DXcluster_thread, NULL, DXcluster_loop, NULL) < 0) {
		LOG_ERROR("%s", "pthread_create failed");
		return;
	}

	LOG_INFO("%s", "dxserver thread started");

	DXcluster_enabled = true;

	char hdr[100];
	snprintf(hdr, sizeof(hdr), "@F%d@S%d@.Spotter    Freq     Dx Station       Notes                      UTC    LOC",
			progdefaults.DXC_textfont,
			progdefaults.DXC_textsize);
	reports_header->clear();
	reports_header->add(hdr);
	reports_header->has_scrollbar(0);

	brws_tcpip_stream->color(fl_rgb_color(
		progdefaults.DX_Color.R,
		progdefaults.DX_Color.G,
		progdefaults.DX_Color.B));
	brws_tcpip_stream->setFont(progdefaults.DXfontnbr);
	brws_tcpip_stream->setFontSize(progdefaults.DXfontsize);
	brws_tcpip_stream->setFontColor(progdefaults.DXfontcolor, FTextBase::RECV);
	brws_tcpip_stream->setFontColor(progdefaults.DXalt_color, FTextBase::XMIT);
	brws_tcpip_stream->setFontColor(
		fl_contrast(progdefaults.DXfontcolor,
			fl_rgb_color(	progdefaults.DX_Color.R,
				progdefaults.DX_Color.G,
				progdefaults.DX_Color.B) ),
		FTextBase::CTRL);

	ed_telnet_cmds->color(fl_rgb_color(
		progdefaults.DX_Color.R,
		progdefaults.DX_Color.G,
		progdefaults.DX_Color.B));
	ed_telnet_cmds->setFont(progdefaults.DXfontnbr);
	ed_telnet_cmds->setFontSize(progdefaults.DXfontsize);
	ed_telnet_cmds->setFontColor(progdefaults.DXfontcolor);

	brws_dxc_help->color(fl_rgb_color(
		progdefaults.DX_Color.R,
		progdefaults.DX_Color.G,
		progdefaults.DX_Color.B));
	brws_dxc_help->setFontColor(progdefaults.DXfontcolor, FTextBase::RECV);
	brws_dxc_help->setFont(progdefaults.DXfontnbr);
	brws_dxc_help->setFontSize(progdefaults.DXfontsize);

	brws_dxcluster_hosts->color(fl_rgb_color(
		progdefaults.DX_Color.R,
		progdefaults.DX_Color.G,
		progdefaults.DX_Color.B));
	brws_dxcluster_hosts->textcolor(progdefaults.DXfontcolor);
	brws_dxcluster_hosts->textfont(progdefaults.DXfontnbr);
	brws_dxcluster_hosts->textsize(progdefaults.DXfontsize);

	cluster_tabs->selection_color(progdefaults.TabsColor);

	if (progdefaults.dxc_auto_connect) {
		DXcluster_connect(true);
		set_btn_dxcc_connect(true);
	}

#ifdef DXC_DEBUG
	string pname = "TempDir";
	pname.append("dxcdebug.txt", "a");
	FILE *dxcdebug = fopen(pname.c_str(), "w");
	fprintf(dxcdebug, "DXC session\n\n");
	fclose(dxcdebug);
#endif
}

//======================================================================
//
//======================================================================
void DXcluster_close(void)
{
	if (!DXcluster_enabled) return;

	if ((DXcluster_state != DISCONNECTED) && DXcluster_socket) {
		DXcluster_connect(false);
		int n = 500;
		while ((DXcluster_state != DISCONNECTED) && n) {
			MilliSleep(10);
			n--;
		}
		if (n == 0) {
			LOG_ERROR("%s", _("Failed to shut down dxcluster socket"));
			fl_message2(_("Failed to shut down dxcluster socket"));
			exit(1);
			return;
		}
	}

	DXcluster_exit = true;
	pthread_join(DXcluster_thread, NULL);
	DXcluster_enabled = false;

	LOG_INFO("%s", "dxserver thread terminated. ");

	if (DXcluster_socket) {
		DXcluster_socket->shut_down();
		DXcluster_socket->close();
	}

}

void dxcluster_hosts_save()
{
	string hosts = "";
	int nlines = brws_dxcluster_hosts->size();
	if (!nlines) {
		progdefaults.dxcluster_hosts = hosts;
		return;
	}
	string hostline;
	size_t p;
	for (int n = 1; n <= nlines; n++) {
		hostline = brws_dxcluster_hosts->text(n);
		p = hostline.find("@.");
		if (p != string::npos) hostline.erase(0,p+2);
		hosts.append(hostline).append("|");
	}
	progdefaults.dxcluster_hosts = hosts;
	progdefaults.changed = true;
}

void dxcluster_hosts_load()
{
	brws_dxcluster_hosts->color(fl_rgb_color(
		progdefaults.DX_Color.R,
		progdefaults.DX_Color.G,
		progdefaults.DX_Color.B));
	brws_dxcluster_hosts->textcolor(progdefaults.DXfontcolor);
	brws_dxcluster_hosts->textfont(progdefaults.DXfontnbr);
	brws_dxcluster_hosts->textsize(progdefaults.DXfontsize);

	brws_dxcluster_hosts->clear();

	if (progdefaults.dxcluster_hosts.empty()) {
		return;
	}
	string hostline;

	string hosts = progdefaults.dxcluster_hosts;
	size_t p = hosts.find("|");
	size_t p2;
	while (p != string::npos && p != 0) {
		hostline.assign(hosts.substr(0,p+1));
		p2 = hostline.find(":|");
		if (p2 != string::npos) hostline.insert(p+1, progdefaults.myCall);
		p2 = hostline.find("|");
		if (p2 != string::npos) hostline.erase(p2, 1);
		brws_dxcluster_hosts->add(hostline.c_str());
		hosts.erase(0, p+1);
		p = hosts.find("|");
	}
	brws_dxcluster_hosts->sort(FL_SORT_ASCENDING);
	brws_dxcluster_hosts->redraw();
}

void dxcluster_hosts_select(Fl_Button*, void*)
{
	string host_line;
	int line_nbr = brws_dxcluster_hosts->value();
	if (line_nbr == 0) return;
	host_line = brws_dxcluster_hosts->text(line_nbr);
	string host_name, host_port, host_login;
	size_t p = host_line.find("@.");
	if (p != string::npos) host_line.erase(0, p + 2);
	p = host_line.find(":");
	if (p == string::npos) return;
	host_name = host_line.substr(0, p);
	host_line.erase(0, p+1);
	p = host_line.find(":");
	if (p == string::npos) return;
	host_port = host_line.substr(0, p);
	host_line.erase(0, p+1);
	host_login = host_line;

	progdefaults.dxcc_host_url = host_name;
	inp_dxcc_host_url->value(host_name.c_str());
	inp_dxcc_host_url->redraw();

	progdefaults.dxcc_host_port = host_port;
	inp_dccc_host_port->value(host_port.c_str());
	inp_dccc_host_port->redraw();

	progdefaults.dxcc_login = host_login;
	inp_dccc_login->value(host_login.c_str());
	inp_dccc_login->redraw();

}

void dxcluster_hosts_delete(Fl_Button*, void*)
{
	int line_nbr = brws_dxcluster_hosts->value();
	if (line_nbr == 0) return;
	brws_dxcluster_hosts->remove(line_nbr);
	dxcluster_hosts_save();
	brws_dxcluster_hosts->redraw();
}

void dxcluster_hosts_clear(Fl_Button*, void*)
{
	brws_dxcluster_hosts->clear();
	dxcluster_hosts_save();
	brws_dxcluster_hosts->redraw();
}

void dxcluster_hosts_add(Fl_Button*, void*)
{
	brws_dxcluster_hosts->color(fl_rgb_color(
		progdefaults.DX_Color.R,
		progdefaults.DX_Color.G,
		progdefaults.DX_Color.B));
	brws_dxcluster_hosts->textcolor(progdefaults.DXfontcolor);
	brws_dxcluster_hosts->textfont(progdefaults.DXfontnbr);
	brws_dxcluster_hosts->textsize(progdefaults.DXfontsize);

	string host_line = progdefaults.dxcc_host_url.c_str();
	host_line.append(":").append(progdefaults.dxcc_host_port.c_str());
	host_line.append(":").append(progdefaults.dxcc_login);
	if (brws_dx_cluster->size() > 0) {
		for (int i = 1; i <= brws_dxcluster_hosts->size(); i++) {
			if (host_line == brws_dxcluster_hosts->text(i))
				return;
		}
	}
	brws_dxcluster_hosts->add(host_line.c_str());
	brws_dxcluster_hosts->sort(FL_SORT_ASCENDING);
	brws_dxcluster_hosts->redraw();
	dxcluster_hosts_save();
}

void dxcluster_hosts_clear_setup(Fl_Button*, void*)
{
	ed_telnet_cmds->clear();
}

void dxcluster_hosts_load_setup(Fl_Button*, void*)
{
	const char* p = FSEL::select( _("Load dxcluster setup file"), "*.dxc", ScriptsDir.c_str());
	if (!p) return;
	if (!*p) return;
	ed_telnet_cmds->buffer()->loadfile(p);
}

void dxcluster_hosts_save_setup(Fl_Button*, void*)
{
	string defaultfilename = ScriptsDir;
	defaultfilename.append("default.dxc");
	const char* p = FSEL::saveas( _("Save dxcluster setup file"), "*.dxc",
		defaultfilename.c_str());
	if (!p) return;
	if (!*p) return;
	ed_telnet_cmds->buffer()->savefile(p);
}

void dxc_send_string(string &tosend)
{
	if (!DXcluster_socket) return;

	string line;
	size_t p;
	while (!tosend.empty()) {
		p = tosend.find("\n");
		if (p != string::npos) {
			line = tosend.substr(0,p);
			tosend.erase(0,p+1);
		} else {
			line = tosend;
			tosend.clear();
		}
		line.append("\r\n");
		try {
			DXcluster_socket->send(line);
			REQ(show_tx_stream, line);
		} catch (const SocketException& e) {
			LOG_ERROR("%s", e.what() );
			REQ(show_error, e.what());
		}
	}
}

void dxcluster_hosts_send_setup(Fl_Button*, void*)
{
	char *str = ed_telnet_cmds->buffer()->text();
	string tosend = str;
	free(str);
	dxc_send_string(tosend);
}

#include <fstream>

#include "arc-help.cxx"
void dxcluster_ar_help(Fl_Button*, void*)
{
	string fn_help = HelpDir;
	fn_help.append("arc_help.html");
	ifstream f_help(fn_help.c_str());
	if (!f_help) {
		ofstream fo_help(fn_help.c_str());
		fo_help << arc_commands;
		fo_help.close();
	} else
		f_help.close();
	cb_mnuVisitURL(0, (void*)fn_help.c_str());
}

#include "CCC_Commands.cxx"
void dxcluster_cc_help(Fl_Button*, void*)
{
	string fn_help = HelpDir;
	fn_help.append("ccc_help.html");
	ifstream f_help(fn_help.c_str());
	if (!f_help) {
		ofstream fo_help(fn_help.c_str());
		fo_help << ccc_commands;
		fo_help.close();
	} else
		f_help.close();
	cb_mnuVisitURL(0, (void*)fn_help.c_str());
}

#include "DXSpiderCommandReference.cxx"
void dxcluster_dx_help(Fl_Button*, void*)
{
	string fn_help = HelpDir;
	fn_help.append("dxc_help.html");
	ifstream f_help(fn_help.c_str());
	if (!f_help) {
		ofstream fo_help(fn_help.c_str());
		fo_help << dxspider_cmds;
		fo_help.close();
	} else
		f_help.close();
	cb_mnuVisitURL(0, (void*)fn_help.c_str());
}

#include "DXClusterServers.cxx"
void dxcluster_servers(Fl_Button*, void*)
{
	string fn_help = HelpDir;
	fn_help.append("dxc_servers.html");
	ifstream f_help(fn_help.c_str());
	if (!f_help) {
		ofstream fo_help(fn_help.c_str());
		fo_help << dxcc_servers;
		fo_help.close();
	} else
		f_help.close();
	cb_mnuVisitURL(0, (void*)fn_help.c_str());
}

void dxc_click_m1(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progdefaults.dxcm_text_1.c_str());
}

void dxc_click_m2(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progdefaults.dxcm_text_2.c_str());
}

void dxc_click_m3(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progdefaults.dxcm_text_3.c_str());
}

void dxc_click_m4(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progdefaults.dxcm_text_4.c_str());
}

void dxc_click_m5(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progdefaults.dxcm_text_5.c_str());
}

void dxc_click_m6(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progdefaults.dxcm_text_6.c_str());
}

void dxc_click_m7(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progdefaults.dxcm_text_7.c_str());
}

void dxc_click_m8(Fl_Button*, void*)
{
	inp_dxcluster_cmd->value(progdefaults.dxcm_text_8.c_str());
}
