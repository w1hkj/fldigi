// ----------------------------------------------------------------------------
// Copyright (C) 2021
//              David Freese, W1HKJ
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// This interface is based on the CM108 HID support code found in hamlib
//
// CM108 Audio chips found on many USB audio interfaces have controllable
// General Purpose Input/Output pins.
// ----------------------------------------------------------------------------

#include <iostream>
#include <fstream>

#include <string>
#include <map>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#ifndef __WIN32__
#include <termios.h>
#include <glob.h>
#  ifndef __APPLE__
#    include <error.h>
#  endif
#endif

#include "config.h"
#include "threads.h"
#include "debug.h"
#include "util.h"
#include "confdialog.h"

#include "hidapi.h"
#include "cmedia.h"

static int ptt_state_cmedia = 0;

static map<std::string, std::string> paths;
static hid_device *ptt_dev = (hid_device *)0;

void test_hid_ptt()
{
	hid_device *test_dev = (hid_device *)0;

	if (ptt_dev) test_dev = ptt_dev;
	else if (progdefaults.cmedia_device == "NONE")
		return;
	else
		test_dev = hid_open_path(paths[progdefaults.cmedia_device]);

	if (test_dev == (hid_device *)0) {
		LOG_ERROR("Could not open %s", progdefaults.cmedia_device.c_str());
		return;
	}

	if (test_dev == ptt_dev)
		LOG_INFO("Testing using ptt device: %s", progdefaults.cmedia_device.c_str());
	else
		LOG_INFO("Testing using test_dev: %s", progdefaults.cmedia_device.c_str());

	int bitnbr = 2;  // GPIO-3
	unsigned char out_rep[5];
	for (int i = 0; i < 5; out_rep[i++] = 0x00);

	out_rep[3] = 0x01 << bitnbr;

	for (int j = 0; j < 20; j++) {
		out_rep[2] = 0x01 << bitnbr;
		test_dev->hid_write(out_rep, 5);
		MilliSleep(50);
		out_rep[2] = 0x00;
		test_dev->hid_write(out_rep, 5);
		MilliSleep(50);
	}
	if (test_dev != ptt_dev) delete test_dev;
	return;
}

int open_cmedia(std::string str_device)
{
	if (str_device == "NONE")
		return -1;

	std::string dev_path = paths[str_device];

	if (ptt_dev) close_cmedia();

	LOG_DEBUG("Device path: %s", dev_path.c_str());

	ptt_dev = hid_open_path(dev_path);

	if (!ptt_dev) {
		LOG_ERROR( "unable to open device");
		return -1;
	}
	LOG_INFO("C-Media device %s opened for GPIO i/o", str_device.c_str());
	return 0;
}

void close_cmedia()
{
	delete ptt_dev;
	ptt_dev = (hid_device *)0;
}


//
// Set or unset the Push To Talk bit on a CM108 GPIO.
//
// param ptt 1 - Set PTT, 0 - unset PTT.
//
// return true on success, false otherwise

// For a CM108 USB audio device PTT is wired up to one of the GPIO
// pins.  Usually this is GPIO3 (bit 2 of the GPIO register) because it
// is on the corner of the chip package (pin 13) so it's easily accessible.
// Some CM108 chips are epoxy-blobbed onto the PCB, so no GPIO
// pins are accessible.  The SSS1623 chips have a different pinout, so
// we make the GPIO bit number configurable.

bool set_cmedia(int bitnbr, int ptt)
{
	if (!ptt_dev) return false;

	// Build a packet for CM108 HID to turn GPIO bit on or off.
	// Packet is 4 bytes, preceded by a 'report number' byte
	// 0x00 report number
	// Write data packet (from CM108 documentation)
	// byte 0: 00xx xxxx     Write GPIO
	// byte 1: xxxx dcba     GPIO3-0 output values (1=high)
	// byte 2: xxxx dcba     GPIO3-0 data-direction register (1=output)
	// byte 3: xxxx xxxx     SPDIF

	unsigned char out_rep[5];
	out_rep[0] = 0x00;
	out_rep[1] = 0x00;
	out_rep[2] = ptt ? (0x01 << bitnbr) : 0x00;
	out_rep[3] = 0x01 << bitnbr;
	out_rep[4] = 0x00;

	LOG_DEBUG("bit %d : %d; %s", bitnbr, ptt ? 1 : 0, str2hex(out_rep, 5));

	// Send the HID packet
	int nw = ptt_dev->hid_write(out_rep, 5);

	if (nw < 0) return false;

	ptt_state_cmedia = ptt;

	return true;
}

// Get the state of Push To Talk from a CM108 GPIO.
// return 1 if ON, 0 if OFF

int get_cmedia()
{
	return ptt_state_cmedia;
}

void init_hids()
{
	std::string hidstr = "NONE";
	hid_device_info *devs = 0;

	inp_cmedia_dev->clear();
	paths.clear();

	if (hid_init()) {
	inp_cmedia_dev->add(hidstr.c_str());
		return;
	}

	devs = hid_enumerate(0x0d8c, 0x0);  // find all C-Media devices

	std::string dev_name = "C-Media-A";
	while (devs) {
		LOG_INFO("\n\
HID           : %s\n\
vendor id     : %04hx\n\
product id    : %04hx\n\
Manufacturer  : %s\n\
Product       : %s\n\
Release       : %hx",
			dev_name.c_str(),
			devs->vendor_id,
			devs->product_id,
			devs->str_manufacturer_string.c_str(),
			devs->str_product_string.c_str(),
			devs->release_number);

		hidstr.append("|").append(dev_name);
		paths[dev_name] = devs->path;
		++dev_name[8]; // increment A->B->C...
		devs = devs->next;
	}
	inp_cmedia_dev->add(hidstr.c_str());
}
