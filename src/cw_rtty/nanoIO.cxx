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

#include "strutil.h"

#define LOG_TINYFSK  LOG_INFO

using namespace std;

Cserial nano_serial;

bool use_nanoIO = false;
bool nanoIO_isCW = false;

static cMorse *nano_morse = 0;

// use during debugging
void sent(string s)
{
	if (!grp_nanoio_debug->visible()) return;

	brws_nanoio_sent->add(s.c_str());
	brws_nanoio_sent->redraw();
}

void rcvd(string s)
{
	if (!grp_nanoio_debug->visible()) return;

	if (s.empty())
		brws_nanoio_rcvd->add("...nil");
	else {
		size_t ptr = s.find("\ncmd:");
		if (ptr != std::string::npos) s.erase(ptr, 1);
		if (s[s.length() - 1] == '\n') s.erase(s.length() -1);
		ptr = s.find("\n");
		while (!s.empty()) {
			if (ptr == std::string::npos) {
				brws_nanoio_rcvd->add(string("@t").append(s).c_str());
				break;
			}
			brws_nanoio_rcvd->add(string("@t").append(s.substr(0, ptr)).c_str());
			brws_nanoio_sent->add("@t-");
			s.erase(0, ptr+1);
			ptr = s.find("\n");
		}
	}
	brws_nanoio_rcvd->redraw();
}

// subtract two timeval values and return result in seconds

double timeval_subtract (struct timeval &x, struct timeval &y)
{
	double t1, t2;
	t1 = x.tv_sec + x.tv_usec * 1e-6;
	t2 = y.tv_sec + y.tv_usec * 1e-6;
	return t2 - t1;
}

void nano_display_io(string s, int style)
{
	if (s.empty()) return;
	REQ(&FTextBase::addstr, txt_nano_io, s, style);
	REQ(&FTextBase::addstr, txt_nano_CW_io, s, style);
}

int nano_serial_write(char c)
{
	if (!use_nanoIO) {
		return 1;
	}
	string buffer = " ";
	buffer[0] = c;
	REQ(sent, buffer);
	return nano_serial.WriteBuffer((unsigned char *)(buffer.c_str()), 1);
}

char nano_read_byte(int &msec)
{
	unsigned char c = 0;
	int ret;
	int numtries = 0;
	timeval start = tmval();
	timeval end = start;
	while (timeval_subtract(start, end) < msec) {
		ret = nano_serial.ReadByte(c);
		end = tmval();
		if (ret && c > 0) break;
		if (++numtries % 50 == 0) Fl::awake();
	}
	double dmsec = timeval_subtract(start, end) * 1000;
	msec = round(dmsec);
	static char szresp[50];
	snprintf(szresp, sizeof(szresp), "'%c' [%0.2f]", c, dmsec);
std::cout << szresp << std::endl;

	REQ(rcvd, szresp);

	return c;
}

string nano_read_string(int msec_wait, string find)
{
	string resp;

	timeval start = tmval();
	int timer = msec_wait;

	if (!find.empty()) {
		while (timer > 0 && (resp.find(find) == string::npos)) {
			resp.append(nano_serial_read());
			MilliSleep(10);
			timer -= 10;
		}
	} else {
		while (timer > 0) {
			resp.append(nano_serial_read());
			MilliSleep(10);
			timer -= 10;
		}
	}
	timeval end = tmval();
	double diff = timeval_subtract(start, end);
	int msec = round(diff * 1000);

	static char szresp[1000];
	snprintf(szresp, sizeof(szresp), "[%d msec] %s", msec, resp.c_str());

	REQ(rcvd, szresp);
	return resp;
}

static bool calibrating = false;

void nano_send_cw_char(int c)
{
	if (c == GET_TX_CHAR_NODATA || c == 0x0d) {
		MilliSleep(50);
		return;
	}

	int   msec = 0;

	float tc = 1200.0 / progdefaults.CWspeed;
	float ta = 0.0;
	float tch = 3 * tc, twd = 4 * tc;

	if (progdefaults.CWusefarnsworth && (progdefaults.CWspeed > progdefaults.CWfarnsworth)) {
		ta = 60000.0 / progdefaults.CWfarnsworth - 37200 / progdefaults.CWspeed;
		tch = 3 * ta / 19;
		twd = 4 * ta / 19;
	}

	if (nano_morse == 0) {
		MilliSleep(50);
		return;
	}
	if (c == '^' || c == '|') {
		nano_serial_write(c);
		msec = 50;
		nano_read_byte(msec);
		MilliSleep(50);
		return;
	}
	int len = 4;
	if (c == 0x0a) c = ' ';
	if (c == ' ') {
		len = 4;
		msec = len * tc + 10;
		nano_serial_write(c);
		nano_read_byte(msec);
		if (!calibrating && progdefaults.CWusefarnsworth && (progdefaults.CWspeed > progdefaults.CWfarnsworth))
			MilliSleep(twd);
	} else {
		len = nano_morse->tx_length(c);
		if (len) {
		msec = len * tc + 10;
			nano_serial_write(c);
			nano_read_byte(msec);
			if (!calibrating && progdefaults.CWusefarnsworth && (progdefaults.CWspeed > progdefaults.CWfarnsworth))
				MilliSleep(tch);
		}
	}
	return;
}

void nano_send_tty_char(int c)
{
	if (c == GET_TX_CHAR_NODATA) {
		MilliSleep(50);
		return;
	}
	int msec = 165; // msec for start + 5 data + 1.5 stop bits @ 45.45
	if (progdefaults.nanoIO_baud == 50.0) msec = 150;
	if (progdefaults.nanoIO_baud == 75.0) msec = 100;
	if (progdefaults.nanoIO_baud == 100.0) msec = 75;
	nano_serial_write(c);
	msec += 20;
	c = nano_read_byte(msec);
}

void nano_send_char(int c)
{
	if (nanoIO_isCW)
		nano_send_cw_char(c);
	else
		nano_send_tty_char(c);
}

static int  setwpm = progdefaults.CWspeed;

void save_nano_state()
{
	if (!use_nanoIO) return;
	setwpm = progdefaults.CWspeed;
}

void restore_nano_state()
{
	if (!use_nanoIO) return;
	progdefaults.CWspeed = setwpm;
	sldrCWxmtWPM->value(progdefaults.CWspeed);
	cntCW_WPM->value(progdefaults.CWspeed);
	calibrating = false;
LOG_INFO("%f WPM", progdefaults.CWspeed);
}

void nanoIO_wpm_cal()
{
	save_nano_state();
	progdefaults.CWusefarnsworth = false;
	progdefaults.CWspeed = cntrWPMtest->value();
LOG_INFO("%f WPM", progdefaults.CWspeed);

	sldrCWxmtWPM->value(progdefaults.CWspeed);
	cntCW_WPM->value(progdefaults.CWspeed);

	nanoIO_isCW = true;

	set_nanoWPM(progdefaults.CWspeed);

	string s;
	for (int i = 0; i < progdefaults.CWspeed; i++) s.append("paris ");
	s.append("^r");

	TransmitText->add_text(s);

	calibrating = true;
	start_tx();

}

void nano_sendString (const string &s)
{
	REQ(sent, s);

	nano_serial.WriteBuffer((unsigned char *)(s.c_str()), s.length());

	return;
}


static timeval ptt_start;
static double  wpm_err = 0;

void dispWPMtiming(void *)
{
	corr_var_wpm->value(wpm_err + 60);
	int nucorr = progdefaults.usec_correc;
	nucorr += wpm_err * 1e6 / (cntrWPMtest->value() * 50);
	usec_correc->value(nucorr);
}


static bool nanoIO_busy = false;

void nano_PTT(int val)
{
	if (use_nanoIO) {
		nano_serial_write(val ? '[' : ']');
		int msec = 100;
		nano_read_byte(msec);
	}

	if (val) {
		ptt_start = tmval();
		nanoIO_busy = true;
	} else {
		timeval ptt_end = tmval();
		double tdiff = timeval_subtract(ptt_start, ptt_end);
//std::cout << "PTT: [ " << tdiff << " ]" << std::endl;
		if (calibrating) {
			wpm_err = tdiff - 60;
			Fl::awake(dispWPMtiming);
			restore_nano_state();
		} else {
			ptt_start.tv_sec = 0;
			ptt_start.tv_usec = 0;
			nanoIO_busy = false;
		}
	}

}

void nano_cancel_transmit()
{
	nano_serial_write('\\');
}

void nano_mark_polarity(int v)
{
	string cmd = "~";
	cmd.append(v == 0 ? "1" : "0");
	string resp;
	nano_sendString(cmd);
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
	string resp;
	string cmd = "~";
	cmd.append(bd == 3 ? "9" : bd == 2 ? "7" : bd == 1 ? "5" : "4");
	nano_sendString(cmd);
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

static int pot_min, pot_rng;
static bool nanoIO_has_pot = false;

void init_pot_min_max()
{
	nanoIO_has_pot = true;

	btn_nanoIO_pot->activate();
	nanoIO_use_pot();

	cntr_nanoIO_min_wpm->activate();
	cntr_nanoIO_rng_wpm->activate();

	cntr_nanoIO_min_wpm->value(pot_min);
	cntr_nanoIO_rng_wpm->value(pot_rng);
	cntr_nanoIO_min_wpm->redraw();
	cntr_nanoIO_rng_wpm->redraw();
}

void disable_min_max()
{
	btn_nanoIO_pot->deactivate();
	cntr_nanoIO_min_wpm->deactivate();
	cntr_nanoIO_rng_wpm->deactivate();
}

// this function must be called from within the main UI thread
// use REQ(nano_parse_config, s);

void nano_parse_config(string s)
{
	nano_display_io(s, FTextBase::ALTR);

	size_t p1 = 0;
	if (s.find("nanoIO") == string::npos) return;

	if (s.find("HIGH") != string::npos)  nano_MARK_is(1);
	if (s.find("LOW") != string::npos)   nano_MARK_is(0);
	if (s.find("45.45") != string::npos) nano_baud_is(45);
	if (s.find("50.0") != string::npos)  nano_baud_is(50);
	if (s.find("75.0") != string::npos)  nano_baud_is(75);
	if (s.find("100.0") != string::npos) nano_baud_is(100);
	if ((p1 = s.find("WPM")) != string::npos) {
		p1 += 4;
		int wpm = progdefaults.CWspeed;
		if (sscanf(s.substr(p1).c_str(), "%d", &wpm)) {
			progdefaults.CWspeed = wpm;
LOG_INFO("%f WPM", progdefaults.CWspeed);
			cntCW_WPM->value(wpm);
			cntr_nanoCW_WPM->value(wpm);
			sldrCWxmtWPM->value(wpm);
		}
	}
	if ((p1 = s.find("/", p1)) != string::npos) {
		p1++;
		int wpm = progdefaults.CW_keyspeed;
		if (sscanf(s.substr(p1).c_str(), "%d", &wpm)) {
			progdefaults.CW_keyspeed = wpm;
			cntr_nanoCW_paddle_WPM->value(wpm);
		}
	} else { // ver 1.1.x
		if ((p1 = s.find("WPM", p1 + 4)) != string::npos) {
			p1++;
			int wpm = progdefaults.CW_keyspeed;
			if (sscanf(s.substr(p1).c_str(), "%d", &wpm)) {
				progdefaults.CW_keyspeed = wpm;
				cntr_nanoCW_paddle_WPM->value(wpm);
			}
		}
	}
	if ((p1 = s.find("dash/dot ")) != string::npos) {
		p1 += 9;
		float val = progdefaults.CWdash2dot;
		if (sscanf(s.substr(p1).c_str(), "%f", &val)) {
			progdefaults.CWdash2dot = val;
			cntCWdash2dot->value(val);
			cnt_nanoCWdash2dot->value(val);
		}
	}
	if ((p1 = s.find("PTT")) != string::npos) {
		if (s.find("NO", p1 + 4) != string::npos)
			progdefaults.disable_CW_PTT = true;
		else
			progdefaults.disable_CW_PTT = false;
	} else
		progdefaults.disable_CW_PTT = false;
	nanoIO_set_cw_ptt();

	if ((p1 = s.find("Speed Pot")) != string::npos) {
		size_t p2 = s.find("ON", p1);
		int OK = 0;
		p2 = s.find("minimum", p1);
		if (p2 != string::npos)
			OK = sscanf(&s[p2 + 8], "%d", &pot_min);
		p2 = s.find("range", p1);
		if (p2 != string::npos)
			OK = sscanf(&s[p2 + 6], "%d", &pot_rng);
		if (OK)
			init_pot_min_max();
	} else
		disable_min_max();

	if ((p1 = s.find("corr usec")) != string::npos) {
		int corr;
		if (sscanf(s.substr(p1+9).c_str(), "%d", &corr)) {
			progdefaults.usec_correc = corr;
			usec_correc->value(corr);
		}
	}
	return;
}

int open_port(string PORT)
{
	if (PORT.empty()) return false;

	REQ(sent, "reset");
	nano_serial.Device(PORT);
	switch (progdefaults.nanoIO_serbaud) {
		case 0: nano_serial.Baud(1200); break;
		case 1: nano_serial.Baud(4800); break;
		case 2: nano_serial.Baud(9600); break;
		case 3: nano_serial.Baud(19200); break;
		case 4: nano_serial.Baud(38400); break;
		case 5: nano_serial.Baud(57600); break;
		case 6: nano_serial.Baud(115200); break;
		default: nano_serial.Baud(57600);
	}
	nano_serial.Timeout(10);
	nano_serial.Retries(5);
	nano_serial.Stopbits(1);

	use_nanoIO = false;

	if (!nano_serial.OpenPort()) {
		nano_display_io("\nCould not open serial port!", FTextBase::ALTR);
		LOG_ERROR("Could not open %s", progdefaults.nanoIO_serial_port_name.c_str());
		return false;
	}

	use_nanoIO = true;

	nano_read_string(1500, "cmd:");

	nano_display_io("Connected to nanoIO\n", FTextBase::RECV);

	return true;
}

string nano_serial_read()
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
	memset(buffer, 0, 1025);
	while (nano_serial.ReadBuffer((unsigned char *)buffer, 1024) ) ;
}

void no_cmd(void *)
{
	nano_display_io("Could not read current configuration\n", FTextBase::ALTR);
}

void close_nanoIO()
{
	nano_serial.ClosePort();
	use_nanoIO = false;

	nano_display_io("Disconnected from nanoIO\n", FTextBase::RECV);

	if (nano_morse) {
		delete nano_morse;
		nano_morse = 0;
	}

	progStatus.nanoCW_online = false;
	progStatus.nanoFSK_online = false;
	nanoIO_isCW = false;

	enable_rtty_quickchange();

}

bool open_nano()
{
	if (use_nanoIO)
		return true;
	if (!open_port(progdefaults.nanoIO_serial_port_name))
		return false;

	return true;
}

bool open_nanoIO()
{
	string rsp;

	progStatus.nanoCW_online = false;
	progStatus.nanoFSK_online = false;

	if (open_nano()) {

		set_nanoIO();

		nano_sendString("~?");
		rsp = nano_read_string(1000, "PTT");

		size_t p = rsp.find("~?");
		if (p == string::npos) return false;
		rsp.erase(0, p + 3);

		if (rsp.find("eyer") != string::npos)
			REQ(nano_parse_config, rsp);

		progStatus.nanoFSK_online = true;
		nanoIO_isCW = false;

		disable_rtty_quickchange();

		return true;
	}
	return false;
}

bool open_nanoCW()
{
	if (nano_morse == 0) nano_morse = new cMorse;

	progStatus.nanoCW_online = false;
	progStatus.nanoFSK_online = false;

	string rsp;
	if (open_nano()) {

		set_nanoCW();

		nano_sendString("~?"); 
		rsp = nano_read_string(1000, "PTT");

		size_t p = rsp.find("~?");
		if (p == string::npos) return false;
		rsp.erase(0, p + 3);

		if (rsp.find("eyer") != string::npos)
			REQ(nano_parse_config, rsp); 

		progStatus.nanoCW_online = true;

		return true;
	}
	return false;
}

void set_nanoIO()
{
	if (progStatus.nanoFSK_online) return;

	string cmd = "~F";
	string rsp;
	int count = 5;
	while (rsp.empty() && count--) {
		nano_sendString(cmd);
		rsp = nano_read_string(165*cmd.length(), cmd);
	}
	progStatus.nanoFSK_online = true;
	progStatus.nanoCW_online = false;
	nanoIO_isCW = false;
}

void set_nanoCW()
{
	string cmd = "~C";
	string rsp;
	int count = 5;

	while (rsp.empty() && count--) {
		nano_sendString(cmd);
		rsp = nano_read_string(165*cmd.length(), cmd);
	}

	if (rsp.find(cmd) != string::npos) {
		nanoIO_isCW = true;
		set_nanoWPM(progdefaults.CWspeed);
		set_nano_dash2dot(progdefaults.CWdash2dot);
	}

	setwpm = progdefaults.CWspeed;

	progStatus.nanoCW_online = true;
	progStatus.nanoFSK_online = false;
	nanoIO_isCW = true;
}

void set_nanoWPM(int wpm)
{
	static char szwpm[10];
	if (wpm > 60 || wpm < 5) return;
	snprintf(szwpm, sizeof(szwpm), "~S%ds", wpm);
	nano_sendString(szwpm);
	nano_read_string(165*strlen(szwpm), szwpm);
}

void nanoIO_correction()
{
	progdefaults.usec_correc = usec_correc->value();
	static char szcorr[15];
	snprintf(szcorr, sizeof(szcorr), "~E%de", progdefaults.usec_correc);
	nano_sendString(szcorr);
	nano_read_string(165*strlen(szcorr), szcorr);
}

void set_nano_keyerWPM(int wpm)
{
	static char szwpm[10];
	if (wpm > 60 || wpm < 5) return;
	snprintf(szwpm, sizeof(szwpm), "~U%du", wpm);
	nano_sendString(szwpm);
	nano_read_string(165*strlen(szwpm), szwpm);
}

void set_nano_dash2dot(float wt)
{
	static char szd2d[10];
	if (wt < 2.5 || wt > 3.5) return;
	snprintf(szd2d, sizeof(szd2d), "~D%3dd", (int)(wt * 100) );
	nano_sendString(szd2d);
	nano_read_string(165*strlen(szd2d), szd2d);
}

void nano_CW_query()
{
	nano_serial_flush();
	nano_sendString("~?");
	string resp = nano_read_string(165*3, "PTT");

	nano_display_io(resp, FTextBase::ALTR);
	REQ(nano_parse_config, resp);
}

void nano_help()
{
	nano_serial_flush();
	nano_sendString("~~");
	string resp = nano_read_string(1000, "cmds");
	nano_display_io(resp, FTextBase::ALTR);
}

void nano_CW_save()
{
	nano_sendString("~W");
	nano_read_string(165*2, "~W");
}

void nanoCW_tune(int val)
{
	if (val) {
		nano_sendString("~T");
		nano_read_string(165*2, "~T");
	} else {
		nano_sendString("]");
		nano_read_string(165, "]");
	}
}

void set_nanoIO_incr()
{
	string s_incr = "~I";
	s_incr += progdefaults.nanoIO_CW_incr;
	nano_sendString(s_incr);
	nano_read_string(165*2, s_incr);
}

void set_nanoIO_keyer(int indx)
{
	string s;
	if (indx == 0) s = "~A";
	if (indx == 1) s = "~B";
	if (indx == 2) s = "~K";
	nano_sendString(s);
	nano_read_string(165*s.length(), s);
}

void nanoIO_set_cw_ptt()
{
	string s = "~X";
	s += progdefaults.disable_CW_PTT ? '0' : '1';
	nano_sendString(s);
	nano_read_string(165*s.length(), s);
}

void nanoIO_read_pot()
{
	if (!use_nanoIO) return;
	if (!nanoIO_has_pot) return;
	if (!progdefaults.nanoIO_speed_pot) return;
	if (nanoIO_busy) return;

// reread the current pot setting
	static char szval[10];
	string rsp;
	snprintf(szval, sizeof(szval), "~P?");
	nano_sendString(szval);
	rsp = nano_read_string(165*strlen(szval), szval);
	int val = 0;
	size_t p = rsp.find("wpm");
	if (p != string::npos) {
		rsp.erase(0,p);
		if (sscanf(rsp.c_str(), "wpm %d", &val) == 1) {
			REQ(set_paddle_WPM, val);
		}
	}
}

void nanoIO_use_pot()
{
	string s = "~P";
	if (progdefaults.nanoIO_speed_pot) s += '1';
	else s += '0';
	nano_sendString(s);
	nano_read_string(165*s.length(), s);
	nanoIO_read_pot();
}

void set_paddle_WPM (int wpm)
{
	cntr_nanoCW_paddle_WPM->value(wpm);
	cntr_nanoCW_paddle_WPM->redraw();
}

void set_nanoIO_min_max()
{
	static char szval[10];
// set min value for potentiometer
	snprintf(szval, sizeof(szval), "~M%dm", (int)cntr_nanoIO_min_wpm->value());
	nano_sendString(szval);
	nano_read_string(165*strlen(szval), szval);

// set range value of potentiometer
	snprintf(szval, sizeof(szval), "~N%dn", (int)cntr_nanoIO_rng_wpm->value());
	nano_sendString(szval);
	nano_read_string(165*strlen(szval), szval);

	nanoIO_read_pot();
}

