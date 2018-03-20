// ----------------------------------------------------------------------------
// nanoIO.cxx  --  Interface to Arduino Nano keyer
//
// Copyright (C) 2018
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
// but WITHOUT ANY WARRANTY without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------


#include <config.h>

#include "fl_digi.h"
#include "nanoIO.h"
#include "serial.h"
#include "morse.h"

#define LOG_TINYFSK  LOG_INFO

using namespace std;

Cserial nano_serial;

bool use_nanoIO = false;
bool nanoIO_isCW = false;

static cMorse *nano_morse = 0;

// use during debugging
void sent(std::string s)
{
//	std::cout << "sent:\"" << s << "\"" << std::endl;
}

void rcvd(std::string s)
{
//	std::cout << "rcvd:\"" << s << "\"" << std::endl;
}

void nano_display_io(string s, int style)
{
	if (s.empty()) return;
	REQ(&FTextBase::addstr, txt_nano_io, s, style);
	REQ(&FTextBase::addstr, txt_nano_CW_io, s, style);
//	ReceiveText->add(s.c_str());
}

int nano_serial_write(char c) {
	if (!use_nanoIO) return 1;
	unsigned char buffer[2];
	buffer[0] = c;
	buffer[1] = '\0';
	return nano_serial.WriteBuffer(buffer, 1);
}

char nano_read_byte(int msec)
{
	std::string resp;
	resp.clear();

	resp = nano_serial_read();
	int numtries = msec/100;
	while (resp.empty() && numtries) {
		MilliSleep(100);
		resp = nano_serial_read();
		numtries--;
	}
	if (resp.empty())
		return 0;
	return resp[0];
}

string nano_read_string(int msec_wait, string find)
{
	std::string resp;

	resp = nano_serial_read();

	int timer = msec_wait;
	if (timer < 100) msec_wait = timer = 100;

	if (!find.empty()) {
		while (timer && (resp.find(find) == std::string::npos)) {
			MilliSleep(100);
			Fl::awake(); Fl::flush();
			resp.append(nano_serial_read());
			timer -= 100;
		}
	} else {
		while (timer) {
			MilliSleep(100);
			Fl::awake(); Fl::flush();
			resp.append(nano_serial_read());
			timer -= 100;
		}
	}
	return resp;
}

void nano_send_char(int c)
{
	if (c == GET_TX_CHAR_NODATA && !nanoIO_isCW) {
		double baudrate = 45.45;
		if (progdefaults.nanoIO_baud == 50.0) baudrate = 50.0;
		if (progdefaults.nanoIO_baud == 75.0) baudrate = 75.0;
		if (progdefaults.nanoIO_baud == 100.0) baudrate = 100.0;
		MilliSleep(7500.0 / baudrate); // start + 5 data + 1.5 stop bits
		return;
	}
	if (c == GET_TX_CHAR_NODATA && nanoIO_isCW) {
		MilliSleep(50);
		return;
	}

	int ch = 0;
	if (nanoIO_isCW) {
		if (c == 0x0d) return;
		if (c == 0x0a || c == ' ') {
			MilliSleep(4*1200/progdefaults.CWspeed);
			ch = c;
		} else {
			if (nano_morse == 0) return;
			if (c == '^' || c == '|') {
				nano_serial_write(c);
				return;
			}
			int len = nano_morse->tx_length(c);
			if (len) {
				nano_serial_write(c);
				MilliSleep(1200 * len / progdefaults.CWspeed);
				ch = c;
			} else
				return;
		}
	} else {
		nano_serial_write(c);
		ch = nano_read_byte(100);
	}
	if (ch) {
		string szch = " ";
		szch[0] = ch;
		nano_display_io(szch, FTextBase::ALTR);
	}
}

void nano_sendString (const std::string &s)
{
	sent(s);

	for (size_t n = 0; n < s.length(); n++)
		nano_serial_write(s[n]);

	return;
}


void nano_PTT(int val)
{
	if (!use_nanoIO) return;

	nano_serial_write(val ? '[' : ']');
}

void nano_cancel_transmit()
{
	nano_serial_write('\\');
}

void nano_mark_polarity(int v)
{
	std::string resp;
	nano_serial_write('~');
	nano_serial_write(
		v == 0 ? '1' :	// MARK high
		'0');			// MARK low
	resp = nano_serial_read();
	nano_display_io(resp, FTextBase::ALTR);
}

void nano_MARK_is(int val)
{
	progdefaults.nanoIO_polarity = val;
	chk_nanoIO_polarity->value(val);
}

void nano_set_baud(int bd)
{
	std::string resp;
	nano_serial_write('~');
	nano_serial_write(
		bd == 3 ? '9' : // 100.0 baud
		bd == 2 ? '7' : // 75.0 baud
		bd == 1 ? '5' : // 50.0 baud
		'4');			// 45.45 baud
	resp = nano_serial_read();
	nano_display_io(resp, FTextBase::ALTR);
}

void nano_baud_is(int val)
{
	int index = 2;
	if (val == 45) index = 0;
	if (val == 50) index = 1;
	if (val == 100) index = 2;
	progdefaults.nanoIO_baud = index;
	sel_nanoIO_baud->index(index);
}

void nano_parse_config(std::string s)
{
	nano_display_io(s, FTextBase::ALTR);
//	ReceiveText->add(s.c_str());

	size_t p1 = 0;
	if (s.find("nanoIO") == std::string::npos) return;

	if (s.find("HIGH") != std::string::npos)  nano_MARK_is(1);
	if (s.find("LOW") != std::string::npos)   nano_MARK_is(0);
	if (s.find("45.45") != std::string::npos) nano_baud_is(45);
	if (s.find("50.0") != std::string::npos)  nano_baud_is(50);
	if (s.find("75.0") != std::string::npos)  nano_baud_is(75);
	if (s.find("100.0") != std::string::npos) nano_baud_is(100);
	if ((p1 = s.find("WPM:")) != std::string::npos) {
		p1 += 4;
		int wpm = progdefaults.CWspeed;
		if (sscanf(s.substr(p1).c_str(), "%d", &wpm)) {
			progdefaults.CWspeed = wpm;
			cntCW_WPM->value(wpm);
			cntr_nanoCW_WPM->value(wpm);
			sldrCWxmtWPM->value(wpm);
		}
	}
	if ((p1 = s.find("/", p1)) != std::string::npos) {
		p1++;
		int wpm = progdefaults.CW_keyspeed;
		if (sscanf(s.substr(p1).c_str(), "%d", &wpm)) {
			progdefaults.CW_keyspeed = wpm;
			cntr_nanoCW_paddle_WPM->value(wpm);
		}
	}
	if ((p1 = s.find("dash/dot ")) != std::string::npos) {
		p1 += 9;
		float val = progdefaults.CWdash2dot;
		if (sscanf(s.substr(p1).c_str(), "%f", &val)) {
			progdefaults.CWdash2dot = val;
			cntCWdash2dot->value(val);
			cnt_nanoCWdash2dot->value(val);
		}
	}
	return;
}

int open_port(std::string PORT)
{
	if (PORT.empty()) return false;

	nano_serial.Device(PORT);
	nano_serial.Baud(9600);
	nano_serial.Timeout(200);
	nano_serial.Retries(10);
	nano_serial.Stopbits(1);

	if (!nano_serial.OpenPort()) {
		nano_display_io("\nCould not open serial port!", FTextBase::ALTR);
//		ReceiveText->add("\nCould not open serial port!");
		LOG_ERROR("Could not open %s", progdefaults.nanoIO_serial_port_name.c_str());
		return false;
	}

	use_nanoIO = true;

	nano_display_io("\nConnected to nanoIO\n", FTextBase::RECV);
//	ReceiveText->add("\nConnected to nanoIO\n");

	return true;
}

std::string nano_serial_read()
{
	static char buffer[4096];

	memset(buffer, '\0',4096);

	int rb = nano_serial.ReadBuffer((unsigned char *)buffer, 4095);
	if (rb)
		return buffer;
	return "";
}

void nano_serial_flush()
{
	static char buffer[1025];
	rcvd("nano_serial_flush():");
	while (nano_serial.ReadBuffer((unsigned char *)buffer, 1024) ) {
		rcvd(buffer);
	}
}

void no_cmd(void *)
{
	nano_display_io("Could not read current configuration\n", FTextBase::ALTR);
}

void close_nanoIO()
{
	nano_serial.ClosePort();
	use_nanoIO = false;

	nano_display_io("\nDisconnected from nanoIO\n", FTextBase::RECV);
//	ReceiveText->add("\nDisconnected from nanoIO\n");

	if (nano_morse) {
		delete nano_morse;
		nano_morse = 0;
	}

	progStatus.nanoCW_online = false;
	progStatus.nanoFSK_online = false;

}

bool open_nano()
{
	if (use_nanoIO)
		return true;
	if (!open_port(progdefaults.nanoIO_serial_port_name))
		return false;

	nano_display_io("Initializing interface\n", FTextBase::RECV);

	return true;
}

bool open_nanoIO()
{
	std::string rsp;

	progStatus.nanoCW_online = false;
	progStatus.nanoFSK_online = false;

	if (open_nano()) {

		rsp = nano_read_string(5000, "cmd:");
		rcvd(rsp);

		set_nanoIO();

		nano_sendString("~?");
		rsp = nano_read_string(5000, "keyer");
		rcvd(rsp);


		size_t p = rsp.find("nanoIO");
		if (p != std::string::npos) rsp.erase(0,p);

		if (rsp.find("keyer") != std::string::npos)
			nano_parse_config(rsp);

		progStatus.nanoFSK_online = true;

		return true;
	}
	return false;
}

bool open_nanoCW()
{
	if (nano_morse == 0) nano_morse = new cMorse;

	progStatus.nanoCW_online = false;
	progStatus.nanoFSK_online = false;

	std::string rsp;
	if (open_nano()) {

		rsp = nano_read_string(5000, "cmd:");
		rcvd(rsp);

		set_nanoCW();

		nano_sendString("~?");
		rsp = nano_read_string(5000, "keyer");
		rcvd(rsp);

		size_t p = rsp.find("nanoIO");
		if (p != std::string::npos) rsp.erase(0,p);

		if (rsp.find("keyer") != std::string::npos)
			nano_parse_config(rsp);

		progStatus.nanoCW_online = true;

		return true;
	}
	return false;
}

void set_nanoIO()
{
	if (progStatus.nanoFSK_online) return;

	nano_sendString("~F");

	std::string rsp = nano_read_string(5000, "~F");
	rcvd(rsp);

	nanoIO_isCW = false;
}

void set_nanoCW()
{
	if (progStatus.nanoCW_online) return;

	nano_sendString("~C");

	std::string rsp = nano_read_string(5000, "~C");
	rcvd(rsp);

	if (rsp.find("~C") != std::string::npos) {
		nanoIO_isCW = true;
		set_nanoWPM(progdefaults.CWspeed);
		set_nano_dash2dot(progdefaults.CWdash2dot);
	}

}

void set_nanoWPM(int wpm)
{
	static char szwpm[10];
	if (wpm > 60 || wpm < 5) return;
	snprintf(szwpm, sizeof(szwpm), "~S%ds", wpm);
	nano_sendString(szwpm);

	std::string rsp  = nano_read_string(1000, szwpm);
	rcvd(rsp);
}

void set_nano_keyerWPM(int wpm)
{
	static char szwpm[10];
	if (wpm > 60 || wpm < 5) return;
	snprintf(szwpm, sizeof(szwpm), "~U%du", wpm);
	nano_sendString(szwpm);

	std::string rsp  = nano_read_string(1000, szwpm);
	rcvd(rsp);
}

void set_nano_dash2dot(float wt)
{
	static char szd2d[10];
	if (wt < 2.5 || wt > 3.5) return;
	snprintf(szd2d, sizeof(szd2d), "~D%3dd", (int)(wt * 100) );
	nano_sendString(szd2d);

	std::string rsp = nano_read_string(1000, szd2d);

	rcvd(rsp);
}

void nano_CW_query()
{
	nano_serial_flush();
	nano_sendString("~?");
	string resp = nano_read_string(5000, "keyer");

	rcvd(resp);
	nano_display_io(resp, FTextBase::ALTR);
}

void nano_CW_save()
{
	nano_sendString("~W");
	std::string rsp = nano_read_string(1000, "~W");
	rcvd(rsp);
}

void nanoCW_tune(int val)
{
	if (val) nano_sendString("~T");
	else     nano_sendString("]");
}

void set_nanoIO_incr()
{
	std::string s_incr = "~I";
	s_incr += progdefaults.nanoIO_CW_incr;
	nano_sendString(s_incr);
	std::string rsp = nano_read_string(1000, s_incr);
	rcvd(rsp);
}

void set_nanoIO_keyer(int indx)
{
	std::string s;
	if (indx == 0) s = "~A";
	if (indx == 1) s = "~B";
	if (indx == 2) s = "~K";
	nano_sendString(s);
	std::string rsp = nano_read_string(1000, s);
	rcvd(rsp);
}
