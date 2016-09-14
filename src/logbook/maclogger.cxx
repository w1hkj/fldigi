// =====================================================================
//
// maclogger.cxx
//
// receive log data from maclogger udp broadcast message
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

#include "maclogger.h"

#include "confdialog.h"

LOG_FILE_SOURCE(debug::LOG_MACLOGGER);

using namespace std;

//======================================================================
// Socket MACLOGGER i/o used on all platforms
//======================================================================

pthread_t maclogger_thread;
pthread_t maclogger_rx_socket_thread;
pthread_mutex_t mclg_str_mutex     = PTHREAD_MUTEX_INITIALIZER;
Socket *maclogger_socket = 0;

bool maclogger_enabled = false;
bool maclogger_exit = false;

string maclogger_ip_address= "";;
string maclogger_ip_port= "";;

string mclg_str = "";
int    mclg_rxhz;
int    mclg_txhz;
string mclg_band;
string mclg_mode;
string mclg_power;
string mclg_call;
string mclg_dxccnum;
string mclg_dxccstr;
string mclg_city;
string mclg_state;
string mclg_firstname;
string mclg_lastname;
string mclg_comment;
string mclg_bearing;
string mclg_longpath;
string mclg_distance;

//======================================================================
// MacLogger UDP string parsing
//======================================================================

static string get_str(string s)
{
	size_t p = s.find(":");
	if (p == string::npos) return "";
	s.erase(0, p+1);
	p = s.find(",");
	if (p == string::npos) p = s.find("]");
	if (p == string::npos) return "";
	string s2 = s.substr(0, p);
	if (s2 == "(null)") return "";
	return s2;
}

static int get_freq(string s)
{
	string s2 = get_str(s);
	float mhz;
	sscanf(s2.c_str(), "%f", &mhz);
	int hz = mhz * 1e6;
	return hz;
}

void maclogger_set_qsy()
{
	int hz = mclg_txhz;
	if (hz <= 0) hz = mclg_rxhz;
	if (hz <= 0) return;
	wf->rfcarrier(hz);
	wf->movetocenter();
	show_frequency(hz);
}

void maclogger_set_call()
{
	inpCall->value(mclg_call.c_str());
	inpCall->do_callback();
}

void maclogger_set_name()
{
	inpName->value(mclg_firstname.c_str());
	inpName->do_callback();
}

void maclogger_set_mode()
{
//	inpMode->value(mclg_mode.c_str());
//	inpMode->do_callback();
}

void maclogger_set_qth()
{
	inpQth->value(mclg_city.c_str());
	inpQth->do_callback();
}

void maclogger_set_state()
{
	inpState->value(mclg_state.c_str());
	inpState->do_callback();
}

void maclogger_disp_report(const char * s)
{
	txt_UDP_data->insert(s);
	txt_UDP_data->redraw();
}

void show_mac_strings()
{
	SET_THREAD_ID(MACLOGGER_TID);

	if (mclg_txhz > 0) REQ(maclogger_set_qsy);
	else if (mclg_rxhz > 0) REQ(maclogger_set_qsy);
	if (!mclg_mode.empty()) REQ(maclogger_set_mode);
	if (!mclg_call.empty()) REQ(maclogger_set_call);
	if (!mclg_city.empty()) REQ(maclogger_set_qth);
	if (!mclg_state.empty()) REQ(maclogger_set_state);
	if (!mclg_firstname.empty()) REQ(maclogger_set_name);

//	if (!mclg_power.empty())
//	if (!mclg_band.empty())
//	if (!mclg_lastname.empty())
//	if (!mclg_comment.empty())
//	if (!mclg_bearing.empty())
//	if (!mclg_longpath.empty())
//	if (!mclg_distance.empty())
//	if (!mclg_dxccnum.empty())
//	if (!mclg_dxccstr.empty())

}

void parse_report(string str)
{
	size_t p;
	mclg_rxhz = 0;
	mclg_txhz = 0;
	mclg_band.clear();
	mclg_mode.clear();
	mclg_power.clear();
	mclg_call.clear();
	mclg_dxccnum.clear();
	mclg_dxccstr.clear();
	mclg_city.clear();
	mclg_state.clear();
	mclg_firstname.clear();
	mclg_lastname.clear();
	mclg_comment.clear();
	mclg_bearing.clear();
	mclg_longpath.clear();
	mclg_distance.clear();

	if ((p = str.find("RxMHz:")) != string::npos)
		mclg_rxhz = get_freq(str.substr(p));
	if ((p = str.find("TxMHz:")) != string::npos)
		mclg_txhz = get_freq(str.substr(p));
	if ((p = str.find("Mode:"))  != string::npos)
		mclg_mode = get_str(str.substr(p));
	if ((p = str.find("Call:"))  != string::npos)
		mclg_call = get_str(str.substr(p));
	if ((p = str.find("city:")) != string::npos)
		mclg_city = get_str(str.substr(p));
	if ((p = str.find("state:")) != string::npos)
		mclg_state = get_str(str.substr(p));
	if ((p = str.find("first_name:")) != string::npos)
		mclg_firstname = get_str(str.substr(p));

//	if ((p = mclg_str.find("dxcc_num:")) != string::npos)
//		mclg_dxccnum = get_str(mclg_str.substr(p));
//	if ((p = mclg_str.find("dxcc_string:")) != string::npos)
//		mclg_dxccstr = get_str(mclg_str.substr(p));
//	if ((p = mclg_str.find("Power:")) != string::npos)
//		mclg_power = get_str(mclg_str.substr(p));
//	if ((p = mclg_str.find("Band:"))  != string::npos)
//		mclg_band = get_str(mclg_str.substr(p));
//	if ((p = mclg_str.find("last_name:")) != string::npos)
//		mclg_lastname = get_str(mclg_str.substr(p));
//	if ((p = mclg_str.find("Comment:")) != string::npos)
//		mclg_comment = get_str(mclg_str.substr(p));
//	if ((p = mclg_str.find("Bearing:")) != string::npos)
//		mclg_bearing = get_str(mclg_str.substr(p));
//	if ((p = mclg_str.find("LongPath:")) != string::npos)
//		mclg_longpath = get_str(mclg_str.substr(p));
//	if ((p = mclg_str.find("Distance:")) != string::npos)
//		mclg_distance = get_str(mclg_str.substr(p));

	show_mac_strings();
}

void parse_maclog()
{
	size_t p1, p2;
	string str, srep;
	static string sreport[20];
	int repnbr = 0;
	while (!mclg_str.empty()) {
		p1 = mclg_str.find("[");
		if (p1 == string::npos) return;
		if (p1 != 0) mclg_str.erase(0, p1);
		p2 = mclg_str.find("]");
		if (p2 == string::npos) return;

		str = mclg_str.substr(0, p2 + 1);
		srep = str;
		srep.append("\n");
		if (repnbr < 20) {
			sreport[repnbr] = srep;
			REQ(maclogger_disp_report, sreport[repnbr].c_str());
			repnbr++;
		}

		if (progdefaults.enable_maclogger_log) {
			std::string pathname = TempDir;
			pathname.append("maclogger_udp_strings.txt");
			FILE *maclog = fl_fopen(pathname.c_str(), "a");
			fprintf(maclog, "%s", srep.c_str());
			fclose(maclog);
		}

		if (progdefaults.capture_maclogger_radio &&
			(mclg_str.find("[Radio Report:") == 0)) parse_report(str);

		else if (progdefaults.capture_maclogger_spot_tune &&
			(mclg_str.find("[SpotTune:") == 0)) parse_report(str);

		else if (progdefaults.capture_maclogger_spot_report &&
			(mclg_str.find("[Spot Report:") == 0)) parse_report(str);

		else if (progdefaults.capture_maclogger_log &&
			(mclg_str.find("[Log Report:") == 0)) parse_report(str);

		else if (progdefaults.capture_maclogger_lookup &&
			(mclg_str.find("[Lookup Report") == 0)) parse_report(str);

		mclg_str.erase(0, p2 + 1);

	}
}

//======================================================================
// uncomment to use UDP test strings
//
// #define TESTSTRINGS 1
//
//======================================================================

#ifdef TESTSTRINGS
string tstring[6] = {
	"[Radio Report:RxMHz:24.96400, TxMHz:24.96400, Band:12M, Mode:USB, Power:5]",
	"[SpotTune:RxMHz:28.49500, TxMHz:28.49600, Band:10M, Mode:USB]",
	"[Log Report: Call:N2BJ, RxMHz:21.08580, TxMHz:21.08580, Band:15M, Mode:FSK, Power:5, dxcc_num:291, dxcc_string:United States, city:NEW LENOX, state:IL, first_name:Barry, last_name:COHEN]",
	"[Spot Report: RxMHz:3.50300, TxMHz:3.50300, Band:80M, Mode:CW, Call:EP6T, dxcc_string:Iran, Comment:UP , TNX CARLO , GL]",
	"[Rotor Report: Bearing:304.7, LongPath:0, Distance:0.0]",
	"[Lookup Report:Call:YC8RBI, RxMHz:21.32500, Band:15M, Mode:USB, dxcc_num:327, dxcc_string:Indonesia, Bearing:328.1, city:SANGIHE ISLAND NORTH SULAWESI, state:(null), first_name:RICHARD, last_name:BYL ( ICHA )]"
};
int tnbr = 0;
#endif

void get_maclogger_udp()
{
	if(!maclogger_socket) return;
	if (!progdefaults.connect_to_maclogger) return;

#ifdef TESTSTRINGS
	if (tnbr == 0) {
		mclg_str = "bogus start chars dkoe13.chfff ";
		for (int n = 0; n < 6; n++) mclg_str.append(tstring[n]);
		mclg_str.append(" and garbage at the end");
		parse_maclog();
		tnbr = 1;
	}
#else
	char buffer[MACLOGGER_BUFFER_SIZE];
	size_t count = 0;

	memset(buffer, 0, sizeof(buffer));

	try {
		count = maclogger_socket->recvFrom(
			(void *) buffer,
			sizeof(buffer) - 1);
	} catch (...) {
		LOG_WARN("MAC_logger socket error");
		count = 0;
	}

	if (count) {
		mclg_str.append(buffer, count);
		parse_maclog();
	}
#endif
}

//======================================================================
//
//======================================================================
void *maclogger_loop(void *args)
{
	SET_THREAD_ID(MACLOGGER_TID);

	LOG_INFO("%s", "MAC_logger loop started. ");

	while(1) {
		for (int i = 0; i < 100; i++) {
			MilliSleep(10);
			if (maclogger_exit) break;
		}
		if (maclogger_exit) break;
		get_maclogger_udp();
	}
	// exit the maclogger thread
	SET_THREAD_CANCEL();
	return NULL;
}

//======================================================================
//
//======================================================================
bool maclogger_start(void)
{
	maclogger_ip_address = "255.255.255.255";
	maclogger_ip_port = "9932";

	try {
		maclogger_socket = new Socket(
				Address( maclogger_ip_address.c_str(),
						 maclogger_ip_port.c_str(),
						 "udp") );
		maclogger_socket->set_autoclose(true);
		maclogger_socket->set_nonblocking(false);
		maclogger_socket->bindUDP();
	}
	catch (const SocketException& e) {
		LOG_ERROR(
			"Could not resolve %s: %s",
			maclogger_ip_address.c_str(),
			e.what() );
		return false;
	}

	return true;
}

//======================================================================
//
//======================================================================
void maclogger_init(void)
{
	maclogger_enabled = false;
	maclogger_exit = false;

	if(!maclogger_start()) return;

	LOG_INFO("%s", "UDP Init - OK");

	if (pthread_create(&maclogger_thread, NULL, maclogger_loop, NULL) < 0) {
		LOG_ERROR("MACLOGGER maclogger_thread: pthread_create failed");
		return;
	}

	LOG_INFO("MACLOGGER thread started");

	maclogger_enabled = true;
}

//======================================================================
//
//======================================================================
void maclogger_close(void)
{
	if (!maclogger_enabled) return;

	if(maclogger_socket) {
		maclogger_socket->shut_down();
		maclogger_socket->close();
	}

	maclogger_exit = true;
	pthread_join(maclogger_thread, NULL);

	LOG_INFO("%s", "MAC_logger loop terminated. ");

	maclogger_enabled = false;
#ifdef TESTSTRINGS
	tnbr = 0;
#endif
}

