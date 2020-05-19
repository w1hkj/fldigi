// ----------------------------------------------------------------------------
// winkeyer.cxx  --  Interface to k1el WinKeyer hardware
//
// Copyright (C) 2017
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

#include <math.h>
#include <string>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include <FL/Fl.H>

#include "debug.h"

#include "fl_digi.h"
#include "confdialog.h"

#include "winkeyer.h"
#include "status.h"
#include "serial.h"
#include "threads.h"
#include "modem.h"
#include "morse.h"

#include "icons.h"

#define LOG_WKEY  LOG_INFO

using namespace std;

//----------------------------------------------------------------------
// WinKeyer general support
//----------------------------------------------------------------------
Cserial WK_serial;

pthread_t WK_serial_thread;
pthread_mutex_t WK_mutex_serial = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t WK_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

//======================================================================
// WinKey command sequences
//======================================================================

// ADMIN MODE
static const char ADMIN				= '\x00';
//static const char CALIBRATE			= '\x00';
static const char RESET				= '\x01';
static const char HOST_OPEN			= '\x02';
static const char HOST_CLOSE		= '\x03';
static const char ECHO_TEST			= '\x04';
//static const char PADDLE_A2D		= '\x05';
//static const char SPEED_A2D			= '\x06';
//static const char GET_VALUES		= '\x07';
//static const char GET_CAL			= '\x09';
//static const char WK1_MODE			= '\x0A';
static const char WK2_MODE			= '\x0B';
//static const char DUMP_EEPROM		= '\x0C';
//static const char LOAD_EEPROM		= '\x0D';
static const char FSK_MODE			= '\x13';

//static const char *SEND_MSG_NBR		= "\x0E";

// HOST MODE
//static const char *SIDETONE			= "\x01";	// N see table page 6 Interface Manual
static const char *SET_WPM			= "\x02";	// 5 .. N .. 99 in WPM
//static const char *SET_WEIGHT		= "\x03";	// 10 .. N .. 90 %
//static const char *SET_PTT_LT		= "\x04";	// A - lead time, B - tail time
												// both 0..250 in 10 msec steps
												// "0x04<01><A0> = 10 msec lead, 1.6 sec tail
static const char *SET_SPEED_POT	= "\x05";	// A = min, B = range, C anything
//static const char *PAUSE			= "\x06";	// 1 = pause, 0 = release
static const char *GET_SPEED_POT 	= "\x07"; 	// return values as per page 7/8
//static const char *BACKSPACE		= "\x08";
//static const char *SET_PIN_CONFIG	= "\x09";	// N as per tables page 8
static const char *CLEAR_BUFFER		= "\x0A";
static const char *KEY_IMMEDIATE	= "\x0B";	// 0 = keyup, 1 = keydown
//static const char *HSCW				= "\x0C";	// N = lpm / 100
//static const char *FARNS_WPM		= "\x0D";	// 10 .. N .. 99
//static const char *SET_WK2_MODE		= "\x0E";	// N as per table page 9
static const char *LOAD_DEFAULTS	= "\x0F";
	// A = MODE REGISTER	B = SPEED IN WPM		C = SIDETONE FREQ
	// D = WEIGHT			E = LEAD-IN TIME		F = TAIL TIME
	// G = MIN_WPM			H = WPM RANGE			I = 1ST EXTENSION
	// J = KEY COMPENSATION	K = FARNSWORTH WPM		L = PADDLE SETPOINT
	// M = DIT/DAH RATIO	N = PIN_CONFIGURATION	O = DONT CARE ==> zero
//static const char *FIRST_EXT		= "\x10";		// see page 10/11
//static const char *SET_KEY_COMP		= "\x11";	// see page 11
static const char *NULL_CMD			= "\x13\x13\x13";
//static const char *PADDLE_SW_PNT	= "\x12";	// 10 .. N .. 90%
//static const char *SOFT_PADDLE		= "\x14";	// 0 - up, 1 - dit, 2 - dah, 3 - both
//static const char *GET_STATUS		= "\x15";	// request status byte, see page 12
//static const char *POINTER_CMD		= "\x16";	// see page 12
//static const char *SET_DIT_DAH		= "\x17";	// 33 .. N .. 66 N = ratio * 50 / 3

// BUFFERED COMMANDS
//static const char *PTT_ON_OFF		= "\x18";	// 1 = on 0 = off
//static const char *KEY_BUFFERED		= "\x19";	// 0 .. N .. 99 seconds
//static const char *WAIT				= "\x1A";	// 0 .. N .. 99 seconds
//static const char *MERGE			= "\x1B";	// merger CD into prosign, ie AR, SK etc
//static const char *CHANGE_BFR_SPD	= "\x1C";	// 5 .. N .. 99
//static const char *CHANGE_HSCW_SPD	= "\x1D";	// N = lpm / 100
//static const char *CANCEL_BFR_SPD	= "\x1E";
//static const char *BUFFER_NOP		= "\x1F";

//======================================================================
// loop for serial i/o thread
// runs continuously until program is closed
// only accesses the serial port if it has been successfully opened
//======================================================================

void WK_version_(int);
void WK_echo_test(int);
void WK_status_(int);
void WK_speed_(int);
void WK_echo_(int);
void WK_eeprom_(int);
bool WK_readByte(unsigned char &byte);
int WK_readString();
int WK_sendString (string &s);
void WK_clearSerialPort();
void WK_display_byte(int);

bool WK_bypass_serial_thread_loop = true;
bool WK_run_serial_thread = true;

bool WK_PTT = false;
int  WK_powerlevel = 0;

static string WK_str_out = "";
static bool WK_get_version = false;
static bool WK_test_echo = false;
static bool WK_host_is_up = false;
static bool wk2_version = false;

static bool read_EEPROM = false;
static char eeprom_image[256];
static int  eeprom_ptr = 0;

static int  wkeyer_ready = true;

static cMorse *wkmorse = 0;

static std::string hexstr(std::string &s)
{
	static std::string hex;
	static char val[3];
	hex.clear();
	for (size_t i = 0; i < s.length(); i++) {
		snprintf(val, sizeof(val), "%02x", s[i] & 0xFF);
		hex.append(" 0x").append(val);
	}
	return hex;
}

static void upcase(string &s)
{
	for (size_t n = 0; n < s.length(); n++)
		if (s[n] > ' ') s[n] = toupper(s[n]);
}

enum {NOTHING, WAIT_ECHO, WAIT_VERSION};

static void WK_send_command(string &cmd, int what = NOTHING)
{
	int cnt = 101;
	while (cnt-- && !WK_str_out.empty()) MilliSleep(1);
	if (!WK_str_out.empty())
		return;

	guard_lock wklock(&WK_mutex_serial);

	upcase(cmd);
	cnt = 101;
	WK_str_out = cmd;

	while (cnt-- && !WK_str_out.empty()) MilliSleep(1);

	switch (what) {
		case WAIT_ECHO : 
			WK_test_echo = true;
			break;
		case WAIT_VERSION :
			WK_get_version = true;
			break;
		default: ;
	}
}

static bool WK_wait_for_char = false;
static char lastchar = ' ';

int WK_send_char(int c)
{
	if (!wkmorse) wkmorse = new cMorse;

	c = toupper(c);
	if (c <= 0) {
		MilliSleep(10);
		Fl::awake();
		return 0;
	}

	struct timeval t;
	long msec_start;
	long msec_end;
	
	if (c < ' ') c = ' ';

	if (c == '0' && progStatus.WK_cut_zeronine) c = 'T';
	if (c == '9' && progStatus.WK_cut_zeronine) c = 'N';
	int n = 0;

	if (lastchar == ' ' && c == ' ') n = 7;
	else if (lastchar != ' ' && c == ' ') n = 4;
	else {
		if (c != ' ') {
			std::string code = wkmorse->tx_lookup(c);
			for (size_t i = 0; i < code.length(); i++) {
				n += 2;
				if (code[i] == '-') n += 2;
			}
			n += 2;
		}
	}
	n *= 1200 / cntCW_WPM->value();

	lastchar = c;

	if (c != ' ') {
		guard_lock wklock(&WK_buffer_mutex);
		WK_str_out += c;
		if (btn_WK_serial_echo->value())
			WK_wait_for_char = true;
	}

	gettimeofday(&t, NULL);
	msec_start = (t.tv_sec - (t.tv_sec / 10000) * 10000) * 1000;
	msec_start += t.tv_usec / 1000;
	msec_start += n;

	if (WK_wait_for_char) {
		n += 100;
		while (WK_wait_for_char) {
			MilliSleep(1);
			if (n % 50 == 0) Fl::awake();
			if (--n == 0) {
				WK_wait_for_char = false;
				LOG_ERROR("Winkeyer did not echo character!");
				return 1;
			}
			if (active_modem->get_stopflag()) {
				WK_wait_for_char = false;
				LOG_INFO("Aborted transmission");
				return 1;
			}
		}
	} else {
		while (!active_modem->get_stopflag()) {
			MilliSleep(1);
			gettimeofday(&t, NULL);
			msec_end = (t.tv_sec - (t.tv_sec / 10000) * 10000) * 1000;
			msec_end += t.tv_usec / 1000;
			if (msec_end >= msec_start) {
				break;
			}
			n--;
			if (n == 0) {
				break;
			}
			if (n % 50 == 0) Fl::awake();
		}
		REQ(WK_display_byte, c);
	}

	return 0;
}

void * WK_serial_thread_loop(void *d)
{
	SET_THREAD_ID(WKEY_TID);

unsigned char byte;
	for(;;) {
		if (!WK_run_serial_thread) break;

		MilliSleep(1);//5);

		if (WK_bypass_serial_thread_loop ||
			!WK_serial.IsOpen()) goto WK_serial_bypass_loop;

// process outgoing
		{
			guard_lock wklock(&WK_buffer_mutex);
			if (!WK_str_out.empty()) {
				WK_sendString(WK_str_out);
				WK_str_out.clear();
			}
		}

// receive WinKeyer response
		{
			guard_lock wklock(&WK_mutex_serial);
			if (WK_serial.ReadBuffer(&byte, 1) == 1) {

				if ((byte == 0xA5 || read_EEPROM))
					WK_eeprom_(byte);
				else if ((byte & 0xC0) == 0xC0)
					WK_status_(byte);
				else if ((byte & 0xC0) == 0x80)
					WK_speed_(byte);
				else if (WK_test_echo)
					WK_echo_test(byte);
				else if (WK_get_version)
					WK_version_(byte);
				else if (WK_wait_for_char)
					WK_echo_(byte);
			}
		}

WK_serial_bypass_loop: ;
	}
	return NULL;
}

void WK_display_byte(int ch)
{
	ReceiveText->add(ch, FTextBase::XMIT);
}

void WK_display_chars(std::string s)
{
	ReceiveText->add(s.c_str());
}

void WK_echo_(int byte)
{
	if (WK_wait_for_char) {
		if (byte == ' ' && lastchar == '\n')
			byte = lastchar;
	}
	REQ(WK_display_byte, byte);
	lastchar = byte;
	WK_wait_for_char = false;
}

void WK_echo_test(int byte)
{
	if (byte != 'U') return;
	LOG_WKEY("passed echo test");
	WK_test_echo = false;
}

void WK_version_(int byte)
{
	static char ver[200];
	snprintf(ver, sizeof(ver), "\nConnected to Winkeyer h/w version %d\n", 
		byte & 0xFF);
	WK_host_is_up = true;
	WK_get_version = false;
	REQ(WK_display_chars, ver);
	progStatus.WK_version = byte;
}

void WK_show_status_change(int byte)
{
	box_WK_wait->color((byte & 0x10) == 0x10 ? FL_DARK_GREEN : FL_BACKGROUND2_COLOR);
	box_WK_wait->redraw();

	box_WK_keydown->color((byte & 0x08) == 0x08 ? FL_DARK_GREEN : FL_BACKGROUND2_COLOR);
	box_WK_keydown->redraw();

	box_WK_busy->color((byte & 0x04) == 0x04 ? FL_DARK_GREEN : FL_BACKGROUND2_COLOR);
	box_WK_busy->redraw();

	box_WK_break_in->color((byte & 0x02) == 0x02 ? FL_DARK_GREEN : FL_BACKGROUND2_COLOR);
	box_WK_break_in->redraw();

	box_WK_xoff->color((byte & 0x01) == 0x01 ? FL_DARK_GREEN : FL_BACKGROUND2_COLOR);
	box_WK_xoff->redraw();
}

void WK_status_(int byte)
{
	if ((byte & 0x04)== 0x04) wkeyer_ready = false;
	else wkeyer_ready = true;
	LOG_DEBUG("Wait %c, Keydown %c, Busy %c, Breakin %c, Xoff %c", 
		byte & 0x10 ? 'T' : 'F',
		byte & 0x08 ? 'T' : 'F',
		byte & 0x04 ? 'T' : 'F',
		byte & 0x02 ? 'T' : 'F',
		byte & 0x01 ? 'T' : 'F');
	REQ(WK_show_status_change, byte);
}

void WK_show_speed_change(int wpm)
{
	if (!progStatus.WK_use_pot) {
		return;
	}
	static char szwpm[8];
	snprintf(szwpm, sizeof(szwpm), "%3d", wpm);

	txt_WK_wpm->value(szwpm);
	txt_WK_wpm->redraw();

	cntCW_WPM->value(wpm);
	cntCW_WPM->redraw();

	progdefaults.CWspeed = wpm;

	sync_cw_parameters();

	string cmd = SET_WPM;
	cmd += progdefaults.CWspeed;

LOG_WKEY("SET_WPM %.1f : %s", progdefaults.CWspeed, hexstr(cmd).c_str());

	WK_send_command(cmd);
}

void WK_speed_(int byte)
{
	int val = (byte & 0x3F) + progStatus.WK_min_wpm;
	REQ(WK_show_speed_change, val);
}

void WK_set_wpm()
{
	string cmd = SET_WPM;
	cmd += progdefaults.CWspeed;

LOG_WKEY("SET_WPM %.1f : %s", progdefaults.CWspeed, hexstr(cmd).c_str());

	WK_send_command(cmd);
}

void WK_use_pot_changed()
{
	progStatus.WK_use_pot = btn_WK_use_pot->value();
	if (progStatus.WK_use_pot) {
		string cmd = GET_SPEED_POT;

LOG_WKEY("GET_SPEED_POT : %s", hexstr(cmd).c_str());

	} else {
		string cmd = SET_WPM;
		if (cntCW_WPM->value() > 55) cntCW_WPM->value(55);
		if (cntCW_WPM->value() < 5) cntCW_WPM->value(5);
		progdefaults.CWspeed = cntCW_WPM->value();
		cmd += progdefaults.CWspeed;

LOG_WKEY("SET_WPM %.1f : %s", progdefaults.CWspeed, hexstr(cmd).c_str());

		WK_send_command(cmd);
	}
}

void WK_eeprom_(int byte)
{
	if (byte == 0xA5) {
		memset( eeprom_image, 0, 256);
		eeprom_ptr = 0;
		read_EEPROM = true;
	}
	if (eeprom_ptr < 256)
		eeprom_image[eeprom_ptr++] = byte;
	if (eeprom_ptr == 256) {
		read_EEPROM = false;
		LOG_WKEY("\n%s", str2hex(eeprom_image, 256));
		eeprom_ptr = 0;
	}
}

void load_defaults()
{
	string cmd = LOAD_DEFAULTS;

	cmd += progStatus.WK_mode_register;
	cmd += progdefaults.CWspeed;
	cmd += progStatus.WK_sidetone;
	cmd += progStatus.WK_weight;
	cmd += progStatus.WK_lead_in_time;
	cmd += progStatus.WK_tail_time;
	cmd += progStatus.WK_min_wpm;
	cmd += progStatus.WK_rng_wpm;
	cmd += progStatus.WK_first_extension;
	cmd += progStatus.WK_key_compensation;
	cmd += progStatus.WK_farnsworth_wpm;
	cmd += progStatus.WK_paddle_setpoint;
	cmd += progStatus.WK_dit_dah_ratio;
	cmd += progStatus.WK_pin_configuration;
	cmd += progStatus.WK_dont_care;

LOG_WKEY("\n\
      mode register .... %0x\n\
      CW speed ......... %.1f\n\
      side tone ........ %d\n\
      weight ........... %d\n\
      lead in time ..... %d\n\
      tail time ........ %d\n\
      min wpm .......... %d\n\
      rng wpm .......... %d\n\
      first ext ........ %d\n\
      key comp ......... %d\n\
      farnsworth wpm ... %d\n\
      paddle setpoint .. %d\n\
      dit dah ratio .... %d\n\
      pin config ....... %d\n\
      don't care ....... %d\n\
      hex string ....... %s",
	progStatus.WK_mode_register,
	progdefaults.CWspeed,
	progStatus.WK_sidetone,
	progStatus.WK_weight,
	progStatus.WK_lead_in_time,
	progStatus.WK_tail_time,
	progStatus.WK_min_wpm,
	progStatus.WK_rng_wpm,
	progStatus.WK_first_extension,
	progStatus.WK_key_compensation,
	progStatus.WK_farnsworth_wpm,
	progStatus.WK_paddle_setpoint,
	progStatus.WK_dit_dah_ratio,
	progStatus.WK_pin_configuration,
	progStatus.WK_dont_care,
	hexstr(cmd).c_str());

	WK_send_command(cmd);

	cmd = SET_SPEED_POT;
	cmd += progStatus.WK_min_wpm;
	cmd += progStatus.WK_rng_wpm;
	cmd += 0xFF;
LOG_WKEY("SET_SPEED_POT : %s", hexstr(cmd).c_str());
	WK_send_command(cmd);

	if (progStatus.WK_use_pot) {
		string cmd = GET_SPEED_POT;
LOG_WKEY("GET_SPEED_POT : %s", hexstr(cmd).c_str());
		WK_send_command(cmd);

	} else {
		string cmd = SET_WPM;
		cmd += progdefaults.CWspeed;
LOG_WKEY("SETWPM %.1f : %s", progdefaults.CWspeed, hexstr(cmd).c_str());
		WK_send_command(cmd);

	}
}

void WKCW_init()
{
	string cmd;

	if (wk2_version) {
		cmd = "  ";
		cmd[0] = ADMIN;
		cmd[1] = WK2_MODE;
LOG_WKEY("WK2_MODE %s", hexstr(cmd).c_str());
		WK_send_command(cmd);
	}

	btn_WK_use_pot->value(progStatus.WK_use_pot);

	load_defaults();

	cmd = GET_SPEED_POT;
LOG_WKEY("GET_SPEED_POT : %s", hexstr(cmd).c_str());
	WK_send_command(cmd);

	cmd = SET_WPM;
	cmd += progdefaults.CWspeed;
LOG_WKEY("SET_WPM %.1f : %s", progdefaults.CWspeed, hexstr(cmd).c_str());
	WK_send_command(cmd);

	cmd = SET_SPEED_POT;
	cmd += progStatus.WK_min_wpm;
	cmd += progStatus.WK_rng_wpm;
	cmd += 0xFF;
LOG_WKEY("SET_SPEED_POT : %s", hexstr(cmd).c_str());
	WK_send_command(cmd);

	cmd = GET_SPEED_POT;
LOG_WKEY("GET_SPEED_POT : %s", hexstr(cmd).c_str());
	WK_send_command(cmd);
}

void open_wkeyer()
{
	int cnt = 0;
	string cmd = NULL_CMD;

LOG_WKEY("NULL_CMD : %s", hexstr(cmd).c_str());

	WK_send_command(cmd);

	WK_clearSerialPort();

// This code only works for a real WinKeyer
// fails for the K3NG Arduino sketch code as written to MORTTY

	if (btn_WK_serial_echo->value() && !btnK3NG->value()) {
		cmd = "  ";
		cmd[0] = ADMIN;
		cmd[1] = ECHO_TEST;
		cmd += 'U';

LOG_WKEY("ECHO_TEST : %s", hexstr(cmd).c_str());

		WK_send_command(cmd, WAIT_ECHO);

		cnt = 5000;
		while (WK_test_echo == true && cnt) {
			MilliSleep(1);
			cnt--;
		}

		if (WK_test_echo) {
			debug::show();
			LOG_ERROR("%s", "Winkeyer not responding");
			WK_test_echo = false;
			pthread_mutex_lock(&WK_mutex_serial);
				WK_bypass_serial_thread_loop = true;
			pthread_mutex_unlock(&WK_mutex_serial);
			WK_serial.ClosePort();
			progStatus.WK_serial_port_name = "NONE";
			select_WK_CommPort->value(progStatus.WK_serial_port_name.c_str());
			return;
		}

		LOG_WKEY("Echo response in %d msec", 5000 - cnt);

	}

	cmd = "  ";
	cmd[0] = ADMIN;
	cmd[1] = HOST_OPEN;
LOG_WKEY("HOST_OPEN : %s", hexstr(cmd).c_str());

	WK_send_command(cmd, WAIT_VERSION);

	cnt = 1000;
	while (WK_get_version == true && cnt) {
		MilliSleep(1);
		cnt--;
	}


}

void close_wkeyer()
{
	string cmd = "  ";

	cmd[0] = ADMIN;
	cmd[1] = RESET;
LOG_WKEY("WKEY RESET : %s", hexstr(cmd).c_str());
	WK_send_command(cmd);

	cmd[0] = ADMIN;
	cmd[1] = HOST_CLOSE;
LOG_WKEY("HOST CLOSE : %s", hexstr(cmd).c_str());
	WK_send_command(cmd);

}

void WK_cancel_transmit()
{
	string cmd = CLEAR_BUFFER;

LOG_WKEY("CLEAR_BUFFER : %s", hexstr(cmd).c_str());

	WK_send_command(cmd);
}

void WK_tune(bool on)
{
	string cmd = KEY_IMMEDIATE;
	if (on) cmd += '\1';
	else cmd += '\0';

LOG_WKEY("KEY_IMMEDIATE : %s", hexstr(cmd).c_str());

	WK_send_command(cmd);
}

//----------------------------------------------------------------------
// WinKeyer setup support
//----------------------------------------------------------------------

void WK_change_btn_swap()
{
	progStatus.WK_mode_register &=0xF7;
	progStatus.WK_mode_register |= btn_WK_swap->value() ? 0x08 : 0x00;
	LOG_WKEY("mode reg: %2X", progStatus.WK_mode_register);
	if (progStatus.WK_online)
		load_defaults();
}

void WK_change_btn_auto_space()
{
	progStatus.WK_mode_register &=0xFD;
	progStatus.WK_mode_register |= btn_WK_auto_space->value() ? 0x02 : 0x00;
	LOG_WKEY("mode reg: %2X", progStatus.WK_mode_register);
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_btn_ct_space()
{
	progStatus.WK_mode_register &= 0xFE;
	progStatus.WK_mode_register |= btn_WK_ct_space->value();
	LOG_WKEY("mode reg: %2X", progStatus.WK_mode_register);
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_btn_paddledog()
{
	progStatus.WK_mode_register &=0x7F;
	progStatus.WK_mode_register |= btn_WK_paddledog->value() ? 0x80 : 0x00;
	LOG_WKEY("mode reg: %2X", progStatus.WK_mode_register);
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_btn_cut_zeronine()
{
	progStatus.WK_cut_zeronine = btn_WK_cut_zeronine->value();
}


void WK_change_btn_paddle_echo()
{
	progStatus.WK_mode_register &=0xBF;
	progStatus.WK_mode_register |= btn_WK_paddle_echo->value() ? 0x40 : 0x00;
	LOG_WKEY("mode reg: %2X", progStatus.WK_mode_register);
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_btn_serial_echo()
{
	progStatus.WK_mode_register &=0xFB;
	progStatus.WK_mode_register |= btn_WK_serial_echo->value() ? 0x04 : 0x00;
	LOG_WKEY("mode reg: %2X", progStatus.WK_mode_register);
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_btn_sidetone_on()
{
	progStatus.WK_sidetone = choice_WK_sidetone->index() + 1;
	progStatus.WK_sidetone |= (btn_WK_sidetone_on->value() ? 0x80 : 0x00);
	if (progStatus.WK_online)
		load_defaults();
}

void WK_change_btn_tone_on()
{
	progStatus.WK_pin_configuration = (progStatus.WK_pin_configuration & 0xFD) | (btn_WK_tone_on->value() ? 2 : 0);
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_btn_ptt_on()
{
	progStatus.WK_pin_configuration = (progStatus.WK_pin_configuration & 0xFE) | (btn_WK_ptt_on->value() ? 1 : 0);
	if (progStatus.WK_online)
		load_defaults();
}
 
void WK_change_cntr_min_wpm()
{
	progStatus.WK_min_wpm = cntr_WK_min_wpm->value();
	if (progStatus.WK_speed_wpm < progStatus.WK_min_wpm) {
		progStatus.WK_speed_wpm = progStatus.WK_min_wpm;
		cntCW_WPM->value(progStatus.WK_speed_wpm);
	}
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_cntr_rng_wpm()
{
	progStatus.WK_rng_wpm = cntr_WK_rng_wpm->value();
	if (progStatus.WK_speed_wpm > progStatus.WK_min_wpm + progStatus.WK_rng_wpm) {
		progStatus.WK_speed_wpm = progStatus.WK_min_wpm + progStatus.WK_rng_wpm;
		cntCW_WPM->value(progStatus.WK_speed_wpm);
	}
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_cntr_farnsworth()
{
	progStatus.WK_farnsworth_wpm = cntr_WK_farnsworth->value();
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_cntr_cmd_wpm()
{
	progStatus.WK_cmd_wpm = cntr_WK_cmd_wpm->value();
}


void WK_change_cntr_ratio()
{
	progStatus.WK_dit_dah_ratio = (unsigned char)(cntr_WK_ratio->value() * 50 / 3);
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_cntr_comp()
{
	progStatus.WK_key_compensation = cntr_WK_comp->value();
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_cntr_first_ext()
{
	progStatus.WK_first_extension = cntr_WK_first_ext->value();
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_cntr_sample()
{
	progStatus.WK_paddle_setpoint = cntr_WK_sample->value();
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_cntr_weight()
{
	progStatus.WK_weight = cntr_WK_weight->value();
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_cntr_leadin()
{
	progStatus.WK_lead_in_time = cntr_WK_leadin->value();
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_cntr_tail()
{
	progStatus.WK_tail_time = cntr_WK_tail->value();
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_choice_keyer_mode()
{
	int modebits = choice_WK_keyer_mode->index() << 4;
	progStatus.WK_mode_register = (progStatus.WK_mode_register & 0xCF) | modebits;
	LOG_WKEY("mode reg: %02X", progStatus.WK_mode_register);
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_choice_hang()
{
	int hangbits = choice_WK_hang->index() << 4;
	progStatus.WK_pin_configuration = (progStatus.WK_pin_configuration & 0xCF) | hangbits;
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_choice_sidetone()
{
	progStatus.WK_sidetone = choice_WK_sidetone->index() + 1;
	progStatus.WK_sidetone |= (btn_WK_sidetone_on->value() ? 0x80 : 0x00);
	if (progStatus.WK_online)
		load_defaults();
}


void WK_change_choice_output_pins()
{
	int pinbits = (choice_WK_output_pins->index() + 1) << 2;
	progStatus.WK_pin_configuration = (progStatus.WK_pin_configuration & 0xF3) | pinbits;
	if (progStatus.WK_online)
		load_defaults();
}

//----------------------------------------------------------------------
// serial support
//----------------------------------------------------------------------

extern bool test;

bool WK_start_wkey_serial()
{
	WK_bypass_serial_thread_loop = true;

	WK_serial.ClosePort();

	if (progStatus.WK_serial_port_name == "NONE") return false;

	WK_serial.Device(progStatus.WK_serial_port_name);
	WK_serial.Baud(1200);
	WK_serial.Stopbits(2);
	WK_serial.Retries(1);
	WK_serial.Timeout(1);//50);
	WK_serial.RTSptt(false);
	WK_serial.DTRptt(false);
	WK_serial.RTSCTS(false);
	WK_serial.RTS(false);
	WK_serial.DTR(true);

	if (!WK_serial.OpenPort()) {
		LOG_ERROR("Cannot access %s", progStatus.WK_serial_port_name.c_str());
		return false;
	} else  {
		LOG_WKEY("\n\
Serial port:\n\
  Port     : %s\n\
  Baud     : %d\n\
  Stopbits : %d\n\
  Timeout  : %d\n\
  DTR      : %s\n\
  RTS/CTS  : %s",
	progStatus.WK_serial_port_name.c_str(),
	WK_serial.Baud(),
	WK_serial.Stopbits(),
	WK_serial.Timeout(),
	WK_serial.DTR() ? "true" : "false",
	WK_serial.RTSCTS() ? "true" : "false");
	}

	MilliSleep(400);	// to allow WK1 to wake up

	return true;
}

#define WK_RXBUFFSIZE 512
static unsigned char WK_replybuff[WK_RXBUFFSIZE+1];
static string WK_replystr;

bool WK_readByte(unsigned char &byte)
{
	unsigned char c;
	int ret = WK_serial.ReadBuffer(&c, 1);
	byte = c;
	return ret;
}

int WK_readString()
{
	int numread = 0;
	size_t n;
	memset(WK_replybuff, 0, WK_RXBUFFSIZE + 1);
	while (numread < WK_RXBUFFSIZE) {
		if ((n = WK_serial.ReadBuffer(&WK_replybuff[numread], WK_RXBUFFSIZE - numread)) == 0) break;
		numread += n;
	}
	WK_replystr.append((const char *)WK_replybuff);
	return numread;
}

int WK_sendString (string &s)
{
	if (WK_serial.IsOpen() == false) {
		LOG_WKEY("command: %s", str2hex(s.data(), s.length()));
		return 0;
	}
	int numwrite = (int)s.length();

	WK_serial.WriteBuffer((unsigned char *)s.c_str(), numwrite);

	if (isprint(s[0]))
		LOG_WKEY("Sent %d: '%s' %s", numwrite, s.c_str(), str2hex(s.data(), s.length()));
	else
		LOG_WKEY("Sent %d: %s", numwrite, str2hex(s.data(), s.length()));
	return numwrite;
}

void WK_clearSerialPort()
{
	if (WK_serial.IsOpen() == false) return;
	WK_serial.FlushBuffer();
}

static bool WK_thread_activated = false;

void WK_exit()
{
	if (!WK_thread_activated) return;

	if (progStatus.WK_online)
		WKCW_connect(false);

	pthread_mutex_lock(&WK_mutex_serial);
		WK_run_serial_thread = false;
	pthread_mutex_unlock(&WK_mutex_serial);
	pthread_join(WK_serial_thread, NULL);

}

// =====================================================================
// Winkeyer 3.0 FSK interface support
// =====================================================================

// progStatus configuration parameters:
//
//	WKFSK_baud
//	WKFSK_stopbits
//	WKFSK_ptt
//	WKFSK_polarity
//	WKFSK_sidetone
//	WKFSK_auto_crlf
//	WKFSK_diddle
//	WKFSK_diddle_char
//	WKFSK_usos
//	WKFSK_monitor

#define wait_one_char(baud, stopbits) (int)((6 + (stopbits))*1000.0 / (baud))

void WKFSK_send_char(int ch)
{
	unsigned char c = toupper(ch);
	int n = (int)(5 * wait_one_char(45.45, 2));

	if (c == 0 || c == '\n') {
		MilliSleep(10);
		Fl::awake();
		return;
	}
	if (c == '\r') c = '}';

	{
		guard_lock wklock(&WK_buffer_mutex);
		if (c == '[' || c == ']' || c == '}' || c == '{' || c < ' ') {
			LOG_WKEY("%s", 
				(c == '[' ? "[ - ptt ON" :
				 c == ']' ? "] - ptt OFF" :
				 c == '}' ? "} - CR/LF" : 
				 c == '{' ? "{ - left brace" : "Control code"));
		} else
			LOG_WKEY("Sending %c, %x", c, c);
		if (progStatus.WKFSK_monitor)
			WK_wait_for_char = true;
		WK_str_out += c;
	}

	while (WK_wait_for_char) {
		MilliSleep(10);
		Fl::awake();
		n -= 10;
		if (n <= 0 || active_modem->get_stopflag()) {
			WK_wait_for_char = false;
			LOG_WKEY("%s",
				(n <= 0 ? "echo: NIL" : "xmt aborted") );
			return;
		}
	}
	return;
}

void WKFSK_init()
{
	std::string cmd = "    ";

LOG_WKEY("mode        %d", progStatus.WKFSK_mode);
LOG_WKEY("diddle      %d", progStatus.WKFSK_diddle);
LOG_WKEY("ptt         %d", progStatus.WKFSK_ptt);
LOG_WKEY("auto crlf   %d", progStatus.WKFSK_auto_crlf);
LOG_WKEY("monitor     %d", progStatus.WKFSK_monitor);
LOG_WKEY("polarity    %d", progStatus.WKFSK_polarity);
LOG_WKEY("baud        %d", progStatus.WKFSK_baud);
LOG_WKEY("stopbits    %d", progStatus.WKFSK_stopbits);
LOG_WKEY("sidetone    %d", progStatus.WKFSK_sidetone);
LOG_WKEY("diddle_char %d", progStatus.WKFSK_diddle_char);
LOG_WKEY("usos        %d", progStatus.WKFSK_usos);

	cmd[0] = ADMIN;
	cmd[1] = FSK_MODE;
	cmd[2] = (progStatus.WKFSK_mode ? 0x80 : 0x00);
	cmd[2] |= (progStatus.WKFSK_diddle ? 0x40 : 0x00);
	cmd[2] |= (progStatus.WKFSK_ptt ? 0x20 : 0x00);
	cmd[2] |= (progStatus.WKFSK_auto_crlf ? 0x10 : 0x00);
	cmd[2] |= (progStatus.WKFSK_monitor ? 0x08 : 0x00);
	cmd[2] |= (progStatus.WKFSK_polarity ? 0x04 : 0x00);
	cmd[2] |= progStatus.WKFSK_baud;

	cmd[3] = (progStatus.WKFSK_sidetone ? 0x10 : 0x00);
	cmd[3] |= (progStatus.WKFSK_stopbits ? 0x08 : 0x00);
	cmd[3] |= (progStatus.WKFSK_diddle_char ? 0x04 : 0x00);
	cmd[3] |= progStatus.WKFSK_usos;

	WK_send_command(cmd);

}

void WKCW_connect(bool start)
{
	LOG_WKEY("WKCW_connect(%s)", (start ? "ON" : "OFF"));

	progStatus.WKFSK_mode = false;
	btn_WKFSK_connect->value(0);
	btn_WKCW_connect->value(0);

	if (!WK_thread_activated) {
		if (pthread_create(&WK_serial_thread, NULL, WK_serial_thread_loop, NULL)) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
		WK_thread_activated = true;
	}

	if (!start) {
		close_wkeyer();
		MilliSleep(100);
		WK_bypass_serial_thread_loop = true;
		MilliSleep(50);
		WK_serial.ClosePort();
		ReceiveText->add("\nWinKeyer disconnected\n");
		progStatus.WK_online = false;
		return;
	}

// shutdown and then reconnect if currently in FSK mode
	if (progStatus.WK_online) {
		close_wkeyer();
		MilliSleep(100);
		WK_bypass_serial_thread_loop = true;
		MilliSleep(50);
		WK_serial.ClosePort();
	}

	if (WK_start_wkey_serial()) {
		WK_bypass_serial_thread_loop = false;
		open_wkeyer();
		if (!WK_serial.IsOpen()) {
			progStatus.WK_online = false;
			return;
		} else {
			progStatus.WK_online = true;
			btn_WKCW_connect->value(1);
		}
	} else
		progStatus.WK_online = false;

	WKCW_init();
}

void WKFSK_connect(bool start)
{
	LOG_WKEY("WKFSK_connect(%s)", (start ? "ON" : "OFF"));

	progStatus.WKFSK_mode = false;
	btn_WKFSK_connect->value(0);
	btn_WKCW_connect->value(0);

	if (!WK_thread_activated) {
		if (pthread_create(&WK_serial_thread, NULL, WK_serial_thread_loop, NULL)) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
		WK_thread_activated = true;
	}

	if (!start) {
		close_wkeyer();
		MilliSleep(100);
		WK_bypass_serial_thread_loop = true;
		MilliSleep(50);
		WK_serial.ClosePort();
		ReceiveText->add("\nWinKeyer disconnected\n");
		progStatus.WK_online = false;
		return;
	}

// shutdown and then reconnect if currently in CW mode
	if (progStatus.WK_online) {
		close_wkeyer();
		MilliSleep(100);
		WK_bypass_serial_thread_loop = true;
		MilliSleep(50);
		WK_serial.ClosePort();
	}

	if (WK_start_wkey_serial()) {
		WK_bypass_serial_thread_loop = false;
		open_wkeyer();
		if (!WK_serial.IsOpen()) {
			progStatus.WK_online = false;
			return;
		} else {
			progStatus.WK_online = true;
		}
	} else
		progStatus.WK_online = false;

	if (progStatus.WK_version < 31) {
		fl_alert2("Winkeyer version must be 31 or greater");

		close_wkeyer();
		MilliSleep(100);
		WK_bypass_serial_thread_loop = true;
		MilliSleep(50);
		WK_serial.ClosePort();

		ReceiveText->add("\nWinKeyer disconnected\n");

		progStatus.WK_online = false;
		progStatus.WKFSK_mode = false;
		return;
	}

	progStatus.WKFSK_mode = true;

	WKFSK_init();

	btn_WKFSK_connect->value(1);

}
