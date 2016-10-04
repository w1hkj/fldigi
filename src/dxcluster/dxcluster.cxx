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

//#include "logsupport.h"
#include "dx_dialog.h"
#include "dx_cluster.h"

#include "confdialog.h"

LOG_FILE_SOURCE(debug::LOG_FD);

//#define DXC_DEBUG 1

using namespace std;

pthread_mutex_t dxcc_mutex     = PTHREAD_MUTEX_INITIALIZER;

//forward declarations of local functions
void DXcluster_start();

//======================================================================
// Socket DXcluster i/o used on all platforms
//======================================================================

pthread_t DXcluster_thread;
pthread_mutex_t DXcluster_mutex     = PTHREAD_MUTEX_INITIALIZER;

Socket *DXcluster_socket = 0;

bool DXcluster_connected = false;
bool DXcluster_exit = false;
bool DXcluster_enabled = false;

//======================================================================
// support routines all called from DXcluster thread using REQ(...)
//======================================================================

static string trim(string s)
{
	while (s[0] == ' ') s.erase(0,1);
	while (s[s.length()-1] == ' ' || s[s.length()-1] == 0x07) s.erase(s.length()-1, 1);
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
	char clr[50];
	string strm;
	snprintf(clr, sizeof(clr), "@B%d@C%d@F%d@S%d@.", 
		progdefaults.DXC_color,
		progdefaults.DXC_textcolor,
		progdefaults.DXC_textfont,
		progdefaults.DXC_textsize);
	strm.assign(clr).append(buff);
	brws_tcpip_stream->add(strm.c_str());
	brws_tcpip_stream->bottomline(brws_tcpip_stream->size());

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
	char clr[50];
	snprintf(clr, sizeof(clr), "@B%d@C%d@F%d@S%d@.", 
		progdefaults.DXC_color,
		progdefaults.DXC_textcolor,
		progdefaults.DXC_textfont,
		progdefaults.DXC_textsize);
	string strm = clr;
	strm.append(buff);
	brws_tcpip_stream->add(strm.c_str());
	brws_tcpip_stream->bottomline(brws_tcpip_stream->size());

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
	char clr[50];
	snprintf(clr, sizeof(clr), "@B%d@C%d@F%d@S%d@.", 
		progdefaults.DXC_color,
		72,
		progdefaults.DXC_textfont,
		progdefaults.DXC_textsize);
	string strm = clr;
	strm.append(buff);
	brws_tcpip_stream->add(strm.c_str());
	brws_tcpip_stream->bottomline(brws_tcpip_stream->size());

#ifdef DXC_DEBUG
	string pname = "TempDir";
	pname.append("dxcdebug.txt", "a");
	FILE *dxcdebug = fopen(pname.c_str(), "a");
	fprintf(dxcdebug, "[E]:%s\n", buff.c_str());
	fclose(dxcdebug);
#endif
}

int dxcc_line_nbr = 0;

void parse_dxline(string buffer)
{
	buffer.erase(0, strlen("DX de "));
	size_t p = buffer.find(":");
	if (p != string::npos) buffer.replace(p, 1, " ");

	string dxc_line;

	char bkgnd[50];

	if (dxcc_line_nbr % 2)
		snprintf(bkgnd, sizeof(bkgnd), "@B%d@C%d@F%d@S%d@.", 
			progdefaults.DXC_odd_color,
			FL_BLACK,
			progdefaults.DXC_textfont,
			progdefaults.DXC_textsize);
	else
		snprintf(bkgnd, sizeof(bkgnd), "@B%d@C%d@F%d@S%d@.", 
			progdefaults.DXC_even_color,
			FL_BLACK,
			progdefaults.DXC_textfont,
			progdefaults.DXC_textsize);

	dxc_line.assign(bkgnd).append(buffer);

	brws_dx_cluster->add(dxc_line.c_str());
	brws_dx_cluster->bottomline(brws_dx_cluster->size());

	dxcc_line_nbr++;
}

void show_wwv_line(string buff)
{
	char clr[50];
	snprintf(clr, sizeof(clr), "@B%d@C%d@F%d@S%d@.", 
		progdefaults.DXC_color,
		progdefaults.DXC_textcolor,
		progdefaults.DXC_textfont,
		progdefaults.DXC_textsize);
	string strm = clr;
	strm.append(buff);
	brws_dxc_wwv->add(strm.c_str());

#ifdef DXC_DEBUG
	string pname = "TempDir";
	pname.append("dxcdebug.txt", "a");
	FILE *dxcdebug = fopen(pname.c_str(), "a");
	fprintf(dxcdebug, "[W]:%s\n", buff.c_str());
	fclose(dxcdebug);
#endif
}

void show_help_line(string buff)
{
	char clr[50];
	snprintf(clr, sizeof(clr), "@B%d@C%d@F%d@S%d@.", 
		progdefaults.DXC_color,
		progdefaults.DXC_textcolor,
		progdefaults.DXC_textfont,
		progdefaults.DXC_textsize);
	string strm = clr;
	strm.append(buff);
	brws_dxc_help->add(strm.c_str());

#ifdef DXC_DEBUG
	string pname = "TempDir";
	pname.append("dxcdebug.txt", "a");
	FILE *dxcdebug = fopen(pname.c_str(), "a");
	fprintf(dxcdebug, "[W]:%s\n", buff.c_str());
	fclose(dxcdebug);
#endif
}

static bool wwv_lines = false;
static bool help_lines = false;

void parse_DXcluster_stream(string input_buffer)
{
	guard_lock dxcc_lock(&dxcc_mutex);
	size_t p;
	string buffer;
	string ucasebuffer = ucasestr(input_buffer);

	if (ucasebuffer.find("LOGIN: ") != string::npos ||  // DXspider
		ucasebuffer.find("CALL:") != string::npos ) {   // ARcluster

		string login = progdefaults.dxcc_login;
		show_tx_stream(login);
		login.append("\r\n");
		DXcluster_socket->send(login);

		login.assign("set/page 0");
		show_tx_stream(login);
		login.append("\r\n");
		DXcluster_socket->send(login);

		login.assign("set/name ").append(progdefaults.myName);
		show_tx_stream(login);
		login.append("\r\n");
		DXcluster_socket->send(login);

		login.assign("set/qth ").append(progdefaults.myQth);
		show_tx_stream(login);
		login.append("\r\n");
		DXcluster_socket->send(login);

		login.assign("set/qra ").append(progdefaults.myLocator);
		show_tx_stream(login);
		login.append("\r\n");
		DXcluster_socket->send(login);

		input_buffer.clear();
		wwv_lines = false;
		help_lines = false;

		brws_dxc_wwv->clear();

		return;
	}

	tcpip_buffer.append(input_buffer);

	string strm;
	string its_me = progdefaults.dxcc_login;
	its_me.append(" DE ");
	its_me = ucasestr(its_me);

	while ( (p = tcpip_buffer.find("\r\n")) != string::npos) {
		buffer = trim(tcpip_buffer.substr(0, p));
		tcpip_buffer.erase(0, p + 2);

		show_rx_stream(buffer);

		ucasebuffer = ucasestr(buffer);
		if (ucasebuffer.find("DX DE") != string::npos) {
			parse_dxline(buffer);
			continue;
		}

		if (ucasebuffer.find("SH/WWV") != string::npos) {
			wwv_lines = true;
			continue;
		}
		if (ucasebuffer.find("HELP") != string::npos) {
			help_lines = true;
		}
		if (ucasebuffer.find(its_me) != string::npos) {
			wwv_lines = false;
			help_lines = false;
			continue;
		}
		if (wwv_lines) {
			show_wwv_line(buffer);
			continue;
		}
		if (help_lines) {
			show_help_line(buffer);
			continue;
		}

	}
}

void clear_dxcluster_viewer()
{
	guard_lock dxcc_lock(&dxcc_mutex);
	brws_dx_cluster->clear();
	dxcc_line_nbr = 0;
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
		if (tempbuff.empty())
			return;
		REQ(parse_DXcluster_stream, tempbuff);
	} catch (const SocketException& e) {
		LOG_ERROR("Error %d, %s", e.error(), e.what());
	}
}

//======================================================================
//
//======================================================================
void dxc_wwv_query()
{
	brws_dxc_wwv->clear();
	try {
		guard_lock dxc_lock(&DXcluster_mutex);
		string sendbuf = "SH/WWV";
		show_tx_stream(sendbuf);
		sendbuf.append("\r\n");
		DXcluster_socket->send(sendbuf.c_str());
	} catch (const SocketException& e) {
		LOG_ERROR("Error %d, %s", e.error(), e.what());
	}
}

void dxc_wwv_clear()
{
	guard_lock dxcc_lock(&dxcc_mutex);
	brws_dxc_wwv->clear();
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
	if (!DXcluster_connected) {
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
		show_tx_stream(sendbuf);
		sendbuf.append("\r\n");
		DXcluster_socket->send(sendbuf.c_str());
	} catch (const SocketException& e) {
		LOG_ERROR("Error %d, %s", e.error(), e.what());
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
	try {
		guard_lock dxc_lock(&DXcluster_mutex);
		string sendbuf = trim(inp_dxcluster_cmd->value());
		string test = ucasestr(sendbuf);
		if (test.find("BYE") != string::npos) {
			fl_alert2("Uncheck the \"Connect\" button to disconnect!");
			return;
		}
		REQ(show_tx_stream, sendbuf);
		sendbuf.append("\r\n");
		DXcluster_socket->send(sendbuf.c_str());
	} catch (const SocketException& e) {
		LOG_ERROR("Error %d, %s", e.error(), e.what());
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

	dxcline = dxcline.substr(p+2);

	string sfreq = dxcline.substr(10, 10);
	string scall = trim(dxcline.substr(20, 10));
	string srem  = ucasestr(dxcline.substr(33, 30));

	for (int i = 0; i < NUM_MODES-3; i++) {
		if (srem.find(mode_info[i].adif_name) != string::npos) {
			if (active_modem->get_mode() != mode_info[i].mode)
				init_modem_sync(mode_info[i].mode);
			break;
		}
	}

	inpCall->value(scall.c_str());

	long freq = (long)(atof(sfreq.c_str()) * 1000.0 + 0.5);
	long af = 1500;

	rf_af(freq, af);
	qsy(freq, af);
}

//--------------------------------------------------------------------------------
//          1         2         3         4         5         6         7
//01234567890123456789012345678901234567890123456789012345678901234567890123456789
//          1     ^   2     ^   3        ^4         5         6         ^
//7080.4 CO3VR Optional Comment

void send_DXcluster_spot()
{
	if (inpCall->value()[0] == 0) return;  // no call

	string spot = "dx ";
	spot.append(inpFreq->value());
	string hz = spot.substr(spot.length() - 2, 2);
	spot.erase(spot.length()-2); // truncate to nearest 100 Hz

	spot.append(" ").append(inpCall->value()).append(" ");

	string comments = trim(inp_dxcluster_cmd->value());
	string currmode = mode_info[active_modem->get_mode()].adif_name;
	if (comments.find(currmode) == string::npos) {
		currmode.append(" [");
		currmode.append(hz).append("] ");
		comments.insert(0, currmode);
	}
	spot.append(comments);

// send spot report to cluster
	try {
		guard_lock dxc_lock(&DXcluster_mutex);
		REQ(show_tx_stream, spot);
		spot.append("\r\n");
		DXcluster_socket->send(spot.c_str());
	} catch (const SocketException& e) {
		LOG_ERROR("Error %d, %s", e.error(), e.what());
		show_error(e.what());
	}

	inp_dxcluster_cmd->value("");
}

//======================================================================
//
//======================================================================

static int dxc_looptime = 5000; // in milliseconds

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
				DXcluster_connected = false;
			}
			Address addr = Address(	progdefaults.dxcc_host_url.c_str(),
									progdefaults.dxcc_host_port.c_str() );
			DXcluster_socket = new Socket( addr );

			DXcluster_socket->set_timeout(0.01);

			DXcluster_socket->connect();

			DXcluster_socket->set_nonblocking(true);

			dxc_looptime = 1000; // rx data once per second

			DXcluster_connected = true;

			LOG_INFO( "Connected to dxserver %s:%s",
					progdefaults.dxcc_host_url.c_str(),
					 progdefaults.dxcc_host_port.c_str() );
		}
		catch (const SocketException& e) {
			LOG_ERROR("%s", e.what() );
			delete DXcluster_socket;
			DXcluster_socket = 0;
			DXcluster_connected = false;
			progStatus.cluster_connected = false;
			show_error(e.what());
			btn_dxcc_connect->value(0);
		}
	}
	else {
		if (!DXcluster_socket) return;

		show_tx_stream("BYE");
		DXcluster_socket->send("BYE\r\n");
		MilliSleep(50);
		DXcluster_socket->shut_down();
		DXcluster_socket->close();
		delete DXcluster_socket;
		DXcluster_socket = 0;
		DXcluster_connected = false;
		clear_dxcluster_viewer();
		progStatus.cluster_connected = false;
		btn_dxcc_connect->value(0);
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
		MilliSleep(100);
		if (DXcluster_exit) break;
		if (DXcluster_connected) DXcluster_recv_data();
		if (connect_changed) DXcluster_doconnect();
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

	DXcluster_exit = true;
	pthread_join(DXcluster_thread, NULL);
	DXcluster_enabled = false;

	if (DXcluster_connected && DXcluster_socket) {
		DXcluster_connect(false);
	}

	LOG_INFO("%s", "dxserver thread terminated. ");

	if (DXcluster_socket) {
		DXcluster_socket->shut_down();
		DXcluster_socket->close();
	}

}

