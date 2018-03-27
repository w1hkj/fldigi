// ----------------------------------------------------------------------------
// Nav.cxx  --  Interface to Arduino Nano Nav keyer
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
#include <iostream>
#include <fstream>

#include "main.h"
#include "fl_digi.h"
#include "gettext.h"
#include "rtty.h"
#include "serial.h"
#include "Nav.h"

bool use_Nav = false;
static int  tty_mode = LETTERS;

static Cserial Nav_serial;
static Cserial Nav_config;

//#define NAVDEBUG 1

static void print_string(std::string s)
{
#ifdef NAVDEBUG
	std::string fname = HomeDir;
	fname.append("nav_debug.txt");
	std::ofstream dbgfile(fname.c_str(), std::ios::app);
	dbgfile << s;
	dbgfile.close();
#else
	LOG_INFO("%s", s.c_str());
#endif
	ReceiveText->add(s.c_str());
}

static std::string Nav_config_string = "\
11101111012";
static std::string config_params = "\
||||||||||+-10  FSK stop bits    - 1:1, 2:1.5, 3: 2 bits\n\
|||||||||+---9  FSK BAUD rate    - 1: 45.45, 2: 75, 3: 100 baud\n\
||||||||+----8  FSK PTT          - 0: PTT disabled, 1: PTT enabled\n\
|||||||+-----7  FSK side tone    - 0: no tone, 1: tone\n\
||||||+------6  FSK polarity     - 0: reverse, 1: normal polarity\n\
|||||+-------5  CAT LED behavior - 0: steady, 1: on-access poll\n\
||||+--------4  LED brightness   - 0: DIM, 1: normal brightness\n\
|||+---------3  WinKey PTT       - 0: no PTT, 1: WinKey PTT\n\
||+----------2  RF attenuator    - 0: 20db atten', 1: no atten'\n\
|+-----------1  CH.2 attenuator  - 0: 15db atten', 1: no atten'\n\
+------------0  CH.1 attenuator  - 0: 15db atten', 1: no atten'\n";

void Nav_update_config_controls()
{
	switch (Nav_config_string[0]) { // channel 1 attenuator
		default:
		case '1': sel_Nav_ch1->index(1); break;
		case '0': sel_Nav_ch1->index(0); break;
	}
	switch (Nav_config_string[1]) { // channel 2 attenuator
		default:
		case '1': sel_Nav_ch2->index(1); break;
		case '0': sel_Nav_ch2->index(0); break;
	}
	switch (Nav_config_string[2]) { // rf attenuator
		default:
		case '1': sel_Nav_rf_att->index(1); break;
		case '0': sel_Nav_rf_att->index(0); break;
	}
	switch (Nav_config_string[3]) { // Winkey PTT
		default:
		case '1': sel_Nav_wk_ptt->index(0); break;
		case '0': sel_Nav_wk_ptt->index(1); break;
	}
	switch (Nav_config_string[4]) { // LED
		default:
		case '1': sel_Nav_LED->index(1); break;
		case '0': sel_Nav_LED->index(0); break;
	}
	switch (Nav_config_string[5]) { // CAT LED
		default:
		case '1': sel_Nav_CAT_LED->index(1); break;
		case '0': sel_Nav_CAT_LED->index(0); break;
	}
	switch (Nav_config_string[6]) { // polarity
		default:
		case '0': sel_Nav_FSK_polarity->index(1); break;
		case '1': sel_Nav_FSK_polarity->index(0); break;
	}
	switch (Nav_config_string[7]) { // sidetone
		default:
		case '1': sel_Nav_FSK_sidetone->index(0); break;
		case '0': sel_Nav_FSK_sidetone->index(1); break;
	}
	switch (Nav_config_string[8]) { // PTT
		default:
		case '1': sel_Nav_FSK_ptt->index(0); break;
		case '0': sel_Nav_FSK_ptt->index(1); break;
	}
	switch (Nav_config_string[9]) { // baud
		default:
		case '1' : sel_Nav_FSK_baud->index(0); break;
		case '2' : sel_Nav_FSK_baud->index(1); break;
		case '3' : sel_Nav_FSK_baud->index(2); break;
	}
	switch (Nav_config_string[10]) { // stop bits
		default:
		case '1': sel_Nav_FSK_stopbits->index(0); break;
		case '2': sel_Nav_FSK_stopbits->index(1); break;
		case '3': sel_Nav_FSK_stopbits->index(2); break;
	}

	print_string(std::string(Nav_config_string).append("\n").append(config_params));

}

void Nav_set_configuration()
{
	std::string cmd = "KL";
	cmd.append(Nav_config_string).append("\r\n");
	Nav_config.WriteBuffer((unsigned char *)cmd.c_str(), cmd.length());

	std::string resp = Nav_read_string(Nav_config, 2000, "\n");
	if (resp.find("kl") != std::string::npos) {
		print_string(_("Config change accepted\n"));
		Nav_write_eeprom();
	} else
		print_string(_("Config change NOT accepted\n"));
}

void Nav_get_configuration()
{
	std::string cmd = "KM\r\n";
	Nav_config.WriteBuffer((unsigned char *)cmd.c_str(), cmd.length());

	std::string resp = Nav_read_string(Nav_config, 2000, "\n");
	size_t p = resp.find("km");
	if (p >= 11) {
		p -= 11;
		Nav_config_string = resp.substr(0, 11);
		Nav_update_config_controls();
	} else {
		std::string err;
		err.assign(_("Nav_get_configuration: ")).append(resp).append("\n");
		print_string(err);
	}
}

void Nav_write_eeprom()
{
	std::string cmd = "KS\r\n";
	Nav_config.WriteBuffer((unsigned char *)cmd.c_str(), cmd.length());

	std::string resp = Nav_read_string(Nav_config, 2000, "\n");
	if (resp.find("ks") == std::string::npos) {
		print_string(_("\nNavigator write to EEPROM failed!\n"));
	} else
		print_string(_("\nEEPROM write reported success\n"));
}

void Nav_restore_eeprom()
{
	std::string cmd = "KR\r\n";
	Nav_config.WriteBuffer((unsigned char *)cmd.c_str(), cmd.length());

	std::string resp = Nav_read_string(Nav_config, 2000, "\n");
	if (resp.find("kr") == std::string::npos) {
		print_string(_("\nEEPROM data failed!\n"));
	} else {
		print_string(_("\nEEPROM data restored\n"));
		Nav_get_configuration();
	}
}

void Nav_set_channel_1_att(int v)
{
	Nav_config_string[0] = (v ? '1' : '0');
	Nav_set_configuration();
}

void Nav_set_channel_2_att(int v)
{
	Nav_config_string[1] = (v ? '1' : '0');
	Nav_set_configuration();
}

void Nav_set_rf_att(int v)
{
	Nav_config_string[2] = (v ? '1' : '0');
	Nav_set_configuration();
}

extern void Nav_set_wk_ptt(int v)
{
	Nav_config_string[3] = (v ? '1' : '0');
	Nav_set_configuration();
}

// "1" normal, "0" dim
void Nav_set_led(int v)
{
	Nav_config_string[4] = (v ? '1' : '0');
	Nav_set_configuration();
}

/*
void led_dim(void *)
{
	Nav_set_led(0);
}

void led_normal(void *)
{
	Nav_set_led(1);
}

void blink_led()
{
	char led = Nav_config_string[4];
	if (led == '1') {
		led_dim(NULL);
		Fl::add_timeout(1.0, led_normal);
	} else {
		led_normal(NULL);
		Fl::add_timeout(1.0, led_dim);
	}
}
*/

void Nav_set_cat_led(int v)
{
	Nav_config_string[5] = (v ? '1' : '0');
	Nav_set_configuration();
}

void Nav_set_polarity(int v)
{
	Nav_config_string[6] = (v ? '0' : '1');
	Nav_set_configuration();
}

void Nav_set_sidetone(int v)
{
	Nav_config_string[7] = (v ? '0' : '1');
	Nav_set_configuration();
}

void Nav_set_ptt(int v)
{
	Nav_config_string[8] = (v ? '0' : '1');
	Nav_set_configuration();
}

void Nav_set_baud(int v)
{
	switch (v) {
		case 0: Nav_config_string[9] = '1'; break; // 45.45 baud
		case 1: Nav_config_string[9] = '2'; break; // 75.0 baud
		case 2: Nav_config_string[9] = '3'; break; // 100.0 baud
	}
	Nav_set_configuration();
}

void Nav_set_stopbits(int v)
{
	switch (v) {
		case 0: Nav_config_string[10] = '1'; break;
		case 1: Nav_config_string[10] = '2'; break;
		case 2: Nav_config_string[10] = '3'; break;
	}
	Nav_set_configuration();
}

void Nav_PTT(int val)
{
}

char Nav_read_byte(Cserial &serial, int msec)
{
	std::string resp;
	resp.clear();

	resp = Nav_serial_read(serial);
	int numtries = msec/100;
	while (resp.empty() && numtries) {
		MilliSleep(100);
		resp = Nav_serial_read(serial);
		numtries--;
	}
	if (resp.empty())
		return 0;
	return resp[0];
}

std::string Nav_read_string(Cserial &serial, int msec_wait, std::string find)
{
	std::string resp;

	resp = Nav_serial_read(serial);

	int timer = msec_wait;
	if (timer < 100) msec_wait = timer = 100;

	if (!find.empty()) {
		while ((timer > 0) && (resp.find(find) == std::string::npos)) {
			MilliSleep(100);
			Fl::awake(); Fl::flush();
			resp.append(Nav_serial_read(serial));
			timer -= 100;
		}
	} else {
		while (timer) {
			MilliSleep(100);
			Fl::awake(); Fl::flush();
			resp.append(Nav_serial_read(serial));
			timer -= 100;
		}
	}
	if (resp.find("KR") != std::string::npos) {
		print_string(_("\nNavigator returned error code!\n"));
	}
	return resp;
}

void xmt_delay(int n)
{
	double baudrate = 45.45;
	if (progdefaults.Nav_FSK_baud == 1) baudrate = 75.0;
	if (progdefaults.Nav_FSK_baud == 2) baudrate = 100.0;
	MilliSleep(n * 7500.0 / baudrate); // start + 5 data + 1.5 stop bits
}

static void send_char(int c)
{
	if (c == LETTERS) {
		tty_mode = LETTERS;
		c = 0x1F;
	} else if (c == FIGURES) {
		tty_mode = FIGURES;
		c = 0x1B;
	} else
		c &= 0x1F;

	unsigned char cmd[2] = " ";
	cmd[0] = c;
	Nav_serial.WriteBuffer(cmd, 1);

	xmt_delay(1);
}

static std::string letters = "E\nA SIU\rDRJNFCKTZLWHYPQOBG MXV ";
static std::string figures = "3\n- \a87\r$4\',!:(5\")2#6019?& ./; ";

int baudot_enc(char data)
{
	size_t c = std::string::npos;

	data = toupper(data);

	c = letters.find(data);
	if (c != std::string::npos)
		return (c + 1) | LETTERS;

	c = figures.find(data);
	if (c != std::string::npos)
		return (c + 1) | FIGURES;

	return -1;
}

void Nav_send_char(int c)
{
	if (c == GET_TX_CHAR_NODATA) {
		send_char(LETTERS);
		return;
	}

	if (c == ' ' && tty_mode == FIGURES)
		send_char(LETTERS);

	c = baudot_enc(c & 0xFF);

	if ((tty_mode == LETTERS) && ((c & FIGURES) == FIGURES))
		send_char(FIGURES);
	if ((tty_mode == FIGURES) && ((c & LETTERS) == LETTERS))
		send_char(LETTERS);

	send_char(c);

}

std::string Nav_serial_read(Cserial &serial) {
	static char buffer[1025];

	memset(buffer, '\0',1025);

	int rb = serial.ReadBuffer((unsigned char *)buffer, 1024);
	if (rb)
		return buffer;
	return "";
}

// One time use of separate thread to read initial values

void close_NavFSK()
{
	Nav_serial.ClosePort();

	use_Nav = false;

	print_string(_("Disconnected from Navigator FSK port\n"));

	progStatus.Nav_online = false;
	enable_rtty_quickchange();
}
/* =====================================================================

Received from Clint, designer of Navigator

The B channel of the 2nd FT2232C device is configured as an 8 bit FIFO, 
only the lower 5 bits are used.

Actual FSK parameters are sent to the Navigator Configuration Port, 
consisting of output 

  Polarity (Normal or Reverse),
  Sidetone (On or Off),
  FSK PTT (must be set to ON)
  Baud Rate (45.45, 75, 100)
  Stop Bits (1, 1.5, 2)

From these settings (Baud rate, Stop bits, Data length (5) and Start bit (1) )
the character rate can be computed to determine when to send the next 
character to the FIFO (below).

Data Channel - Baudot encoded FSK 5 bit characters are sent to the FSK
FIFO device.  The FIFO is 374 characters deep.  The first two characters 
sent to the FIFO should be sent with no delay between the two characters.
Subsequent characters should be sent at a rate determined by the parameters 
in the previous paragraph. The rate should be about 0.5% faster than the 
calculated rate to ensure that the buffer is not under run.  When the 
last character is sent to the FIFO, the PTT signal should be dropped.  

The FSK Controller will maintain the PTT signal to the radio until the 
last bit of the last character has been transmitted.  Output PTT will be 
dropped at that time.  When the PTT drops to the radio, it will return 
to RX state. 

Baudot LTRS and FIGS bytes and USOS LTRS bytes are not handled by the Navigator

The Navigator does not provide any state feedback to the controlling PC, 
i.e. state of the FIFO or the PTT signal lines, timing must be approximated 
in the controlling PC.  Note: The FTDI part must provide a Busy Status to 
indicate when the FIFO is full.

In the Windows implementation, there were two configuration items that were used:

Open Port: SetCommState() function- the DCB was able to be configured as a 
45 baud, 1.5 stop bit, no parity, 5 bit bytesize device.

WriteFile() function: Windows buffer size set to 1 - this allowed the flow 
control to be done without having to time the data rate to avoid buffer under run.

======================================================================*/
bool open_NavFSK()
{
	progStatus.Nav_online = false;

	Nav_serial.Device(progdefaults.Nav_FSK_port);
	Nav_serial.Baud(1200);
	Nav_serial.Timeout(200);
	Nav_serial.Retries(10);
	Nav_serial.Stopbits(1);

	if (!Nav_serial.OpenPort()) {
		std::string err = std::string(_("Could not open ")).append(progdefaults.Nav_FSK_port).append("\n");
		print_string(err);
		return false;
	}

	use_Nav = true;

	progStatus.Nav_online = true;
	disable_rtty_quickchange();

	print_string(_("Connected to Navigator FSK port\n"));

	return true;
}

void close_NavConfig()
{
	Nav_config.ClosePort();

	print_string(_("Disconnected from Navigator config port\n"));

	progStatus.Nav_config_online = false;
}

bool open_NavConfig()
{
#ifdef NAVDEBUG
	std::string info = "Navigator debug file: ";
	info.append(zdate()).append(" ").append(ztime()).append("\n");
	print_string("========================================================\n");
	print_string(info);
#endif
	Nav_config.Device(progdefaults.Nav_config_port);
	Nav_config.Baud(1200);
	Nav_config.Timeout(200);
	Nav_config.Retries(10);
	Nav_config.Stopbits(1);

	if (!Nav_config.OpenPort()) {
		std::string err;
		err.assign(_("Could not open ")).append(progdefaults.Nav_config_port);
		print_string(err);
		return false;
	}

	std::string cmd = "KT\r\n";
	std::string resp = "";

	Nav_config.WriteBuffer((unsigned char *)(cmd.c_str()), cmd.length());
	resp = Nav_read_string(Nav_config, 2000, "\n");
	if (resp.find("kt") == std::string::npos) {
		Nav_config.WriteBuffer((unsigned char *)(cmd.c_str()), cmd.length());
		resp = Nav_read_string(Nav_config, 2000, "\n");
		if (resp.find("kt") == std::string::npos) {
			print_string(_("Navigator failed to respond to NOOP.\n"));
			close_NavConfig();
			return false;
		}
	}

	cmd = "KQ\r\n";
	Nav_config.WriteBuffer((unsigned char *)(cmd.c_str()), cmd.length());
	resp = Nav_read_string(Nav_config, 2000, "\n");

	if (resp.find("\n") == std::string::npos) {
		print_string(_("Navigator did not send Version string.\n"));
		close_NavConfig();
		return false;
	}

	print_string(std::string("Navigator ").append(resp));

	Nav_get_configuration();

//	blink_led();

	print_string(_("Connected to Navigator configuration port\n"));

	progStatus.Nav_config_online = true;

	return true;
}
