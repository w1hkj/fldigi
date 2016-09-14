// =====================================================================
//
// FD_logger.cxx
//
// interface to tcpip application fdserver.tcl
//   fdserver is a multiple client tcpip server
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

#include "logsupport.h"
#include "fd_logger.h"
#include "fd_view.h"

#include "confdialog.h"

LOG_FILE_SOURCE(debug::LOG_FD);

using namespace std;

//forward declarations of local functions
void FD_start();

//======================================================================
// Socket FD i/o used on all platforms
//======================================================================

pthread_t FD_thread;
pthread_t FD_rx_socket_thread;
pthread_mutex_t FD_mutex     = PTHREAD_MUTEX_INITIALIZER;

Socket *FD_socket = 0;

bool FD_connected = false;
bool FD_logged_on = false;
bool FD_enabled = false;
bool FD_exit = false;

string FD_ip_addr = "";
string FD_ip_port = "";

string FD_rxbuffer;

//======================================================================
// data report from fdserver.tcl
// LOGON w1hkj 40,DIG
// LOGON_OK W1HKJ 1A 5
// SCORE 35
// WORKED ... all on a single line
// {160 CW 1} {160 DIG 0} {160 PHONE 0}
// {80 CW 0} {80 DIG 0} {80 PHONE 0}
// {40 CW 1} {40 DIG 0} {40 PHONE 0}
// {20 CW 0} {20 DIG 1} {20 PHONE 0}
// {17 CW 0} {17 DIG 0} {17 PHONE 1}
// {15 CW 0} {15 DIG 0} {15 PHONE 1}
// {10 CW 0} {10 DIG 0} {10 PHONE 0}
// {2 CW 0} {2 DIG 0} {2 PHONE 0}
// {440 CW 0} {440 DIG 0} {440 PHONE 0}
//======================================================================

//======================================================================
//
//======================================================================
void post(Fl_Input2 *w, const char * s)
{
	w->value(s);
}

//======================================================================
//
//======================================================================
void view(Fl_Output *w, const char * s)
{
	w->value(s);
}

//======================================================================
//
//======================================================================
static string toparse;

void parse_logon_ok(string s)
{
	size_t p = 0;
	static string call, clss, mult, sect;
	call.clear(); clss.clear(); sect.clear();
	s.erase(0, 9);
	call = s;
	call.erase(call.find(" "));
	p = s.find(" ");
	if (p != string::npos) {
		s.erase(0, p+1);
		clss = s;
		p = clss.find(" ");
		clss.erase(p);
		p = s.find(" ");
		if (p != string::npos) {
			s.erase(0,p+1);
			mult = s;
			p = mult.find(" ");
			mult.erase(p);
			p = s.find(" ");
			if (p != string::npos) {
				s.erase(0,p+1);
				sect = s;
				p = sect.find("\r");
				if (p != string::npos) sect.erase(p);
				p = sect.find("\n");
				if (p != string::npos) sect.erase(p);
			}
		}
	}
	progdefaults.my_FD_call = call;
	REQ(&post, inp_my_FD_call, call.c_str());
	REQ(&view, view_FD_call, call.c_str());
	progdefaults.my_FD_class = clss;
	REQ(&post, inp_my_FD_class, clss.c_str());
	REQ(&view, view_FD_class, clss.c_str());
	progdefaults.my_FD_section = sect;
	REQ(&post, inp_my_FD_section, sect.c_str());
	REQ(&view, view_FD_section, sect.c_str());
	progdefaults.my_FD_mult = mult;
	REQ(&view, view_FD_mult, mult.c_str());
}

//======================================================================
//
//======================================================================
void parse_score(string s)
{
	static string sscore;
	size_t p = s.find("\r");
	if (p != string::npos) s.erase(p);
	p = s.find("\n");
	if (p != string::npos) s.erase(p);
	p = s.find(" ");
	if (p != string::npos) s.erase(0, p+1);
	sscore = s;
	REQ(&view, view_FD_score, sscore.c_str());
}

//======================================================================
//
//======================================================================
void parse_entry( string needle, Fl_Output *view1, Fl_Output *view2 )
{
	size_t p1 = toparse.find(needle);
	if (p1 == string::npos) return;
	p1 += needle.length();
	size_t p2 = toparse.find(" ", p1);
	size_t p3 = toparse.find("}", p1);
	if (p3 == string::npos) return;
	string num = "", op = "";
	num = toparse.substr(p1, p2 - p1);
	op = toparse.substr(p2+1, p3 - (p2+1));

	view1->value(num.c_str());
	view2->value(op.c_str());
}

//======================================================================
//
//======================================================================
void parse_worked()
{
// CW contacts
	parse_entry("{160 CW ", view_FD_CW[0],  view_FD_CW_OP[0]);
	parse_entry("{80 CW ",  view_FD_CW[1],  view_FD_CW_OP[1]);
	parse_entry("{40 CW ",  view_FD_CW[2],  view_FD_CW_OP[2]);
	parse_entry("{20 CW ",  view_FD_CW[3],  view_FD_CW_OP[3]);
	parse_entry("{17 CW ",  view_FD_CW[4],  view_FD_CW_OP[4]);
	parse_entry("{15 CW ",  view_FD_CW[5],  view_FD_CW_OP[5]);
	parse_entry("{12 CW ",  view_FD_CW[6],  view_FD_CW_OP[6]);
	parse_entry("{10 CW ",  view_FD_CW[7],  view_FD_CW_OP[7]);
	parse_entry("{6 CW ",   view_FD_CW[8],  view_FD_CW_OP[8]);
	parse_entry("{2 CW ",   view_FD_CW[9],  view_FD_CW_OP[9]);
	parse_entry("{220 CW ", view_FD_CW[10], view_FD_CW_OP[10]);
	parse_entry("{440 CW ", view_FD_CW[11], view_FD_CW_OP[11]);

// DIG contacts
	parse_entry("{160 DIG ", view_FD_DIG[0],  view_FD_DIG_OP[0]);
	parse_entry("{80 DIG ",  view_FD_DIG[1],  view_FD_DIG_OP[1]);
	parse_entry("{40 DIG ",  view_FD_DIG[2],  view_FD_DIG_OP[2]);
	parse_entry("{20 DIG ",  view_FD_DIG[3],  view_FD_DIG_OP[3]);
	parse_entry("{17 DIG ",  view_FD_DIG[4],  view_FD_DIG_OP[4]);
	parse_entry("{15 DIG ",  view_FD_DIG[5],  view_FD_DIG_OP[5]);
	parse_entry("{12 DIG ",  view_FD_DIG[6],  view_FD_DIG_OP[6]);
	parse_entry("{10 DIG ",  view_FD_DIG[7],  view_FD_DIG_OP[7]);
	parse_entry("{6 DIG ",   view_FD_DIG[8],  view_FD_DIG_OP[8]);
	parse_entry("{2 DIG ",   view_FD_DIG[9],  view_FD_DIG_OP[9]);
	parse_entry("{220 DIG ", view_FD_DIG[10], view_FD_DIG_OP[10]);
	parse_entry("{440 DIG ", view_FD_DIG[11], view_FD_DIG_OP[11]);

// PHONE contacts
	parse_entry("{160 PHONE ", view_FD_PHONE[0],  view_FD_PHONE_OP[0]);
	parse_entry("{80 PHONE ",  view_FD_PHONE[1],  view_FD_PHONE_OP[1]);
	parse_entry("{40 PHONE ",  view_FD_PHONE[2],  view_FD_PHONE_OP[2]);
	parse_entry("{20 PHONE ",  view_FD_PHONE[3],  view_FD_PHONE_OP[3]);
	parse_entry("{17 PHONE ",  view_FD_PHONE[4],  view_FD_PHONE_OP[4]);
	parse_entry("{15 PHONE ",  view_FD_PHONE[5],  view_FD_PHONE_OP[5]);
	parse_entry("{12 PHONE ",  view_FD_PHONE[6],  view_FD_PHONE_OP[6]);
	parse_entry("{10 PHONE ",  view_FD_PHONE[7],  view_FD_PHONE_OP[7]);
	parse_entry("{6 PHONE ",   view_FD_PHONE[8],  view_FD_PHONE_OP[8]);
	parse_entry("{2 PHONE ",   view_FD_PHONE[9],  view_FD_PHONE_OP[9]);
	parse_entry("{220 PHONE ", view_FD_PHONE[10], view_FD_PHONE_OP[10]);
	parse_entry("{440 PHONE ", view_FD_PHONE[11], view_FD_PHONE_OP[11]);

}

void clear_fd_viewer() {
	for (int i = 0; i < 12; i++) {
		view_FD_CW[i]->value("");
		view_FD_CW_OP[i]->value("");
		view_FD_DIG[i]->value("");
		view_FD_DIG_OP[i]->value("");
		view_FD_PHONE[i]->value("");
		view_FD_PHONE_OP[i]->value("");
	}
	view_FD_call->value("");
	view_FD_class->value("");
	view_FD_section->value("");
	view_FD_mult->value("");
	box_fdserver_connected->color((Fl_Color)31);
	box_fdserver_connected->redraw();
}

//======================================================================
//
//======================================================================
void parse_FD_stream(string data)
{
	size_t p = 0;
	if (data.empty()) return;

//std::cout << "RX Stream:\n" << data << std::endl;

	if (data.find("QUIT") != string::npos) {
//std::cout << "Quit\n";
		FD_disconnect();
		btn_fd_connect->value(0);
		return;
	}
	if (data.find("LOGON_DENIED") != string::npos) {
//std::cout << "Logon denied\n";
		FD_logged_on = false;
		LOG_ERROR("FD logon DENIED");
		btn_fd_connect->value(0);
		return;
	}
	if (data.find("LOGOFF_OK") != string::npos) {
//std::cout << "Log off OK\n";
		FD_logged_on = false;
		return;
	}
	if (data.find("LOGON_OK") != string::npos) {
//std::cout << "Logon OK\n";
		parse_logon_ok(data.substr(p));
		FD_logged_on = true;
		box_fdserver_connected->color((Fl_Color)2);
		box_fdserver_connected->redraw();
	}
	if ( (p = data.find("SCORE") ) != string::npos) {
//std::cout << "SCORE\n";
		parse_score(data.substr(p));
	}
	if ( (p = data.find("WORKED"))!= string::npos) {
//std::cout << "WORKED\n";
		toparse = data.substr(p);
		REQ(parse_worked);
		return;
	}
	if ( data.find("NODUP") != string::npos) {
//std::cout << "Not a duplicate\n";
		return;
	}
	else if (data.find("DUP") != string::npos) {
//std::cout << "Duplicate\n";
	}
	else if (data.find("REJECT") != string::npos) {
//std::cout << "Reject\n";
	}
	else if (data.find("ACCEPT") != string::npos) {
//std::cout << "Accept\n";
	}
}

//======================================================================
//
//======================================================================
void FD_write(string s)
{
	FD_socket->send(s.append("\n"));
}

//======================================================================
//
//======================================================================
void FD_get_record(string call)
{
	if(!FD_socket) return;
	if (!FD_connected) return;
}

//======================================================================
//
//======================================================================
void FD_add_record()
{
	if(!FD_socket) return;
	if (!FD_connected) return;
	guard_lock send_lock(&FD_mutex);
	string cmd = "ADD ";
	cmd.append(inpCall->value()).append(" ");
	cmd.append(inp_FD_section->value()).append(" ");
	cmd.append(inp_FD_class->value());
	FD_write(cmd);
}

//======================================================================
//
//======================================================================
static string fd_band;
static string fd_mode;

static string FD_opmode()
{
	if (!active_modem) {
		return "DIG";
	}
	if (active_modem->get_mode() == MODE_CW)
		return "CW";
	else if (active_modem->get_mode() < MODE_SSB)
		return "DIG";
	else if (active_modem->get_mode() == MODE_SSB)
		return "PHONE";
	return "";
}

//======================================================================
//
//======================================================================
static string FD_opband()
{
	if (!active_modem) return "40";

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

//======================================================================
//
//======================================================================
bool FD_dupcheck()
{
	if(!FD_socket) return false;
	if (!FD_connected) return false;

	string response;
	string cmd = "DUPCHECK ";
	cmd.append(inpCall->value());
	try {
		guard_lock send_lock(&FD_mutex);
		FD_write(cmd);
		MilliSleep(50);
		FD_socket->recv(response);
		if (response.empty())
			return false;
		if (response.find("NODUP") != string::npos)
			return false;
		if (response.find("DUP") != string::npos)
			return true;
		return false;
	} catch (const SocketException& e) {
		LOG_ERROR("Error %d, %s", e.error(), e.what());
		FD_exit = true;
	}
	return false;
}

//======================================================================
//
//======================================================================
void FD_logon()
{
	string ucasecall = progdefaults.fd_op_call;
	string buffer;
	string cmd = "LOGON ";

	if (ucasecall.empty()) return;
	for (size_t n = 0; n < ucasecall.length(); n++)
		ucasecall[n] = toupper(ucasecall[n]);

	fd_band = FD_opband();
	fd_mode = FD_opmode();
	if (fd_band.empty() || fd_mode.empty()) return;

	cmd.append(ucasecall).append(" ");
	cmd.append(fd_band).append(",");
	cmd.append(fd_mode);

	try {
		guard_lock send_lock(&FD_mutex);
//std::cout << "Log On: " << cmd << std::endl;
		FD_write(cmd);
		FD_socket->recv(buffer);

		parse_FD_stream(buffer);

		LOG_INFO("Logged on to fdserver");
		FD_logged_on = true;

	} catch (const SocketException& e) {
		LOG_ERROR("Error %d, %s", e.error(), e.what());
	}
}

//======================================================================
//
//======================================================================
void FD_logoff()
{
	if (!FD_socket) return;

	guard_lock send_lock(&FD_mutex);
	try {
		string buffer;
		FD_write("LOGOFF");
		MilliSleep(100);
		FD_socket->recv(buffer);
		parse_FD_stream(buffer);

		FD_disconnect();
		LOG_INFO("Logged off FD server");
	} catch (const SocketException& e) {
		LOG_ERROR("Error %d, %s", e.error(), e.what());
	}
}

//======================================================================
//
//======================================================================
void FD_band_check() {
	if (fd_band != FD_opband()) {
		FD_logoff();
		FD_start();
		FD_logon();
	}
}

//======================================================================
//
//======================================================================
void FD_mode_check() {
	if (fd_mode != FD_opmode()) {
		FD_logoff();
		FD_start();
		FD_logon();
	}
}

//======================================================================
//
//======================================================================
void FD_rcv_data()
{
	string tempbuff;
	try {
		guard_lock send_lock(&FD_mutex);
		FD_socket->recv(tempbuff);
		if (tempbuff.empty())
			return;
		parse_FD_stream(tempbuff);
	} catch (const SocketException& e) {
		LOG_ERROR("Error %d, %s", e.error(), e.what());
		FD_exit = true;
	}
}

//======================================================================
//
//======================================================================
static int fd_looptime = 1000; // in milliseconds
void FD_start()
{
	guard_lock send_lock(&FD_mutex);

	FD_ip_addr =  progdefaults.fd_tcpip_addr;
	FD_ip_port = progdefaults.fd_tcpip_port;

	try {
		FD_socket = new Socket(
				Address( FD_ip_addr.c_str(),
						 FD_ip_port.c_str(),
						 "tcp") );
		FD_socket->set_timeout(0.01);
		FD_socket->connect();
		FD_socket->set_nonblocking(true);

		LOG_INFO( "Connected to fdserver %s:%s",
			FD_ip_addr.c_str(), FD_ip_port.c_str() );

		fd_looptime = 100;
		FD_connected = true;
	}
	catch (const SocketException& e) {
//		LOG_ERROR("%s", e.what() );
		delete FD_socket;
		FD_socket = 0;
		FD_connected = false;
	}
}

//======================================================================
//
//======================================================================
void FD_restart()
{
	guard_lock send_lock(&FD_mutex);

	FD_ip_addr =  progdefaults.fd_tcpip_addr;
	FD_ip_port = progdefaults.fd_tcpip_port;

	try {
		FD_socket->shut_down();
		FD_socket->close();
		delete FD_socket;
		FD_connected = false;
		FD_socket = new Socket(
				Address( FD_ip_addr.c_str(),
						 FD_ip_port.c_str(),
						 "tcp") );
		FD_socket->set_timeout(0.01);
		FD_socket->connect();
		FD_socket->set_nonblocking(true);
		fd_looptime = 100;

		LOG_INFO( "Connected to fdserver %s:%s",
			FD_ip_addr.c_str(), FD_ip_port.c_str() );
	}
	catch (const SocketException& e) {
//		LOG_ERROR("%s", e.what() );
		delete FD_socket;
		FD_socket = 0;
		FD_connected = false;
	}
}

//======================================================================
// Disconnect from FD tcpip server
//======================================================================
void FD_disconnect()
{
	if (!FD_socket) return;

// send disconnect string
	FD_socket->shut_down();
	FD_socket->close();
	delete FD_socket;
	FD_socket = 0;
	FD_connected = false;
	FD_logged_on = false;
	REQ(clear_fd_viewer);
	LOG_INFO("Disconnected from fdserver");
}

////////////////////////////////////////////////
// NEED TO DETECT WHEN SERVER IS CLOSED OR FAILS
////////////////////////////////////////////////

//======================================================================
// Thread loop
//======================================================================
void *FD_loop(void *args)
{
	SET_THREAD_ID(FD_TID);

	while(1) {
		for (int i = 0; i < fd_looptime/50; i++) {
			MilliSleep(50);
			if (FD_exit) break;
		}

		if (!FD_connected && progdefaults.connect_to_fdserver)
			FD_start();
		if ( FD_connected &&
			 ((FD_ip_addr != progdefaults.fd_tcpip_addr) ||
			  (FD_ip_port != progdefaults.fd_tcpip_port)) )
			FD_restart();

		if (FD_exit) break;

		if (FD_connected && !progdefaults.connect_to_fdserver) {
			if (FD_logged_on) FD_logoff();
			FD_disconnect();
		} else if (	FD_connected &&
					!FD_logged_on &&
					progdefaults.connect_to_fdserver) {
			FD_logon();
		} else if (FD_connected && FD_logged_on) {
			FD_rcv_data();
			FD_mode_check();
			FD_band_check();
		}
	}

	if (FD_logged_on && FD_socket)
		FD_logoff(); // calls FD_disconnect()
	else
		FD_disconnect();

	// exit the FD thread
	SET_THREAD_CANCEL();
	return NULL;
}

//======================================================================
//
//======================================================================
void FD_init(void)
{
	FD_enabled = false;
	FD_exit = false;

	if (pthread_create(&FD_thread, NULL, FD_loop, NULL) < 0) {
		LOG_ERROR("%s", "pthread_create failed");
		return;
	}

	LOG_INFO("%s", "fdserver thread started");

	FD_enabled = true;
}

//======================================================================
//
//======================================================================
void FD_close(void)
{
	if (!FD_enabled) return;

	FD_exit = true;
	pthread_join(FD_thread, NULL);
	FD_enabled = false;

	LOG_INFO("%s", "fdserver thread terminated. ");

	if (FD_socket) {
		FD_socket->shut_down();
		FD_socket->close();
	}

}

