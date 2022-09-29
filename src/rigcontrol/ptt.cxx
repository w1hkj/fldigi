// ----------------------------------------------------------------------------
//
//    ptt.cxx --  PTT control
//
// Copyright (C) 2006-2009
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2009
//		Stelios Bounanos, M0GLD
// Copyright (C) 2009
//		Diane Bruce, VA3DB
//
// Added gpio for PTT (Lior KK6BWA)
//
// This file is part of fldigi.  Adapted from code contained in gmfsk source code
// distribution.
//  gmfsk Copyright (C) 2001, 2002, 2003
//  Tomi Manninen (oh2bns@sral.fi)
//  Copyright (C) 2004
//  Lawrence Glaister (ve7it@shaw.ca)
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
// ----------------------------------------------------------------------------

#include <config.h>

#include <iostream>

#include <unistd.h>
#include <sys/types.h>
#if HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#if HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
#endif
#if HAVE_TERMIOS_H
#  include <termios.h>
#endif
#include <errno.h>
#include <cstring>
#include <stdint.h>

#include "trx.h"
#include "ptt.h"
#include "configuration.h"
#include "rigio.h"
#if USE_HAMLIB
	#include "hamlib.h"
#endif
#include "serial.h"
#include "re.h"
#include "debug.h"

#include "fl_digi.h"
#include "confdialog.h"

#include "n3fjp_logger.h"

#include "cmedia.h"

LOG_FILE_SOURCE(debug::LOG_RIGCONTROL);

extern Cserial CW_KEYLINE_serial;

int cmedia_fd = -1;

PTT::PTT(ptt_t dev) : pttdev(PTT_INVALID), oldtio(0)
{
	reset(dev);
}

PTT::~PTT()
{
	close_all();
}

void PTT::reset(ptt_t dev)
{
	close_all();

	switch (pttdev = dev) {
#if HAVE_UHROUTER
        case PTT_UHROUTER:
		if (progdefaults.PTTdev.find(UHROUTER_FIFO_PREFIX) == 0) {
			pttdev = PTT_UHROUTER;
			open_uhrouter();
			break;
		}
		else {
		        pttdev = PTT_NONE;
		        break;
		}
#endif
#if HAVE_PARPORT
        case PTT_PARPORT:
		open_parport();
		if (pttfd < 0)
		        pttdev = PTT_NONE;
		break;
#endif
	case PTT_TTY:
		open_tty();
		break;
	case PTT_CMEDIA:
		cmedia_fd = open_cmedia(progdefaults.cmedia_device);
		break;
	default:
		break; // nothing to open
	}
	open_gpio();
	set(false);
}

void PTT::set(bool ptt)
{
	std::string ptt_temp =
		pttdev == PTT_NONE ? "NONE" :
		pttdev == PTT_HAMLIB ? "HAMLIB" :
		pttdev == PTT_RIGCAT ? "RIGCAT" :
		pttdev == PTT_TTY ? "TTY" :
		pttdev == PTT_GPIO ? "GPIO" :
		pttdev == PTT_CMEDIA ? "CMEDIA" :
		pttdev == PTT_PARPORT ? "PARPORT" :
		pttdev == PTT_UHROUTER ? "UHROUTER" : "UNKNOWN";
	LOG_INFO("PTT via %s : %s", ptt_temp.c_str(), ptt ? "ON" : "OFF");

	// Let  WinKeyer handle PTT when it is the CW modem, so we don't lose QSK.
	if (active_modem == cw_modem && progStatus.WK_online) {
		return;
	}

// add milliseconds - no audio to clear virtual audio card used by Flex systems
	if (!ptt && progdefaults.PTT_off_delay)
		MilliSleep(progdefaults.PTT_off_delay);

	if (active_modem == cw_modem && (CW_KEYLINE_isopen || progdefaults.CW_KEYLINE_on_cat_port)) {
		guard_lock lk(&cwio_ptt_mutex);
	}

	switch (pttdev) {
	case PTT_NONE:
		noCAT_setPTT(ptt);
		break;
#if USE_HAMLIB
	case PTT_HAMLIB:
		hamlib_set_ptt(ptt);
		break;
#endif
	case PTT_RIGCAT:
		rigCAT_set_ptt(ptt);
		break;
	case PTT_TTY:
		set_tty(ptt);
		break;
	case PTT_GPIO:
		set_gpio(ptt);
		break;
#if HAVE_PARPORT
	case PTT_PARPORT:
		set_parport(ptt);
		break;
#else
	btnUsePPortPTT->hide();
#endif
#if HAVE_UHROUTER
	case PTT_UHROUTER:
		set_uhrouter(ptt);
		break;
#endif
	case PTT_CMEDIA:
		if (cmedia_fd != -1) {
			int bitnbr = 2;
			if (progdefaults.cmedia_gpio_line == "GPIO-1") bitnbr = 0;
			else if (progdefaults.cmedia_gpio_line == "GPIO-2") bitnbr = 1;
			else if (progdefaults.cmedia_gpio_line == "GPIO-3") bitnbr = 2;
			else if (progdefaults.cmedia_gpio_line == "GPIO-4") bitnbr = 3;
			set_cmedia(bitnbr, ptt);
		}
		break;
	default:
		if (n3fjp_connected)
			n3fjp_set_ptt(ptt);
	}

	nano_PTT(ptt);

	if (ptt && progdefaults.PTT_on_delay)
		MilliSleep(progdefaults.PTT_on_delay);

	if (ptt) start_tx_timer();
	else     stop_tx_timer();
}

void PTT::close_all(void)
{
	set(false);

	switch (pttdev) {
	case PTT_TTY:
		close_tty();
		break;
#if HAVE_PARPORT
	case PTT_PARPORT:
		close_parport();
		break;
#endif
#if HAVE_UHROUTER
	case PTT_UHROUTER:
		close_uhrouter();
		break;
#endif
	default:
		break;
	}
	close_gpio();
	pttfd = -1;
}

//-------------------- gpio port PTT --------------------//
#ifndef __MINGW32__
static void gpioEXEC(std::string execstr)
{
	int pfd[2];
	if (pipe(pfd) == -1) {
		LOG_PERROR("pipe");
		return;
	}
	pid_t pid;
	switch (pid = fork()) {
		case -1:
			LOG_PERROR("fork");
			return;
		case 0: // child
			close(pfd[0]);
			if (dup2(pfd[1], STDOUT_FILENO) != STDOUT_FILENO) {
				LOG_PERROR("dup2");
				exit(EXIT_FAILURE);
			}
			close(pfd[1]);
			execl("/bin/sh", "sh", "-c", execstr.c_str(), (char *)NULL);
			perror("execl");
			exit(EXIT_FAILURE);
	}

	// parent
	close(pfd[1]);

}
#else // !__MINGW32__

static void gpioEXEC(std::string execstr)
{
	char* cmd = strdup(execstr.c_str());

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));
	if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		LOG_ERROR("CreateProcess failed with error code %ld", GetLastError());
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	free(cmd);

}
#endif // !__MINGW32__


static const char *gpio_name[] = {
		"17", "18", "27", "22", "23",
		"24", "25", "4",  "5",  "6",
		"13", "19", "26", "12", "16",
		"20", "21"};

void export_gpio(int bcm)
{
	if (bcm < 0 || bcm > 16) return;
	std::string exec_str = "gpio export ";
	exec_str.append(gpio_name[bcm]).append(" out");
	gpioEXEC(exec_str);
	LOG_INFO("%s", exec_str.c_str());
}

void unexport_gpio(int bcm)
{
	if (bcm < 0 || bcm > 16) return;
	std::string exec_str = "gpio unexport ";
	exec_str.append(gpio_name[bcm]);
	gpioEXEC(exec_str);
	LOG_INFO("%s", exec_str.c_str());
}

void PTT::open_gpio(void)
{
	bool enabled = false;
	for (int i = 0; i < 17; i++) {
		enabled = (progdefaults.enable_gpio >> i) & 0x01;
		if (enabled) export_gpio(i);
	}
}

void PTT::close_gpio(void)
{
	bool enabled = false;
	for (int i = 0; i < 17; i++) {
		enabled = (progdefaults.enable_gpio >> i) & 0x01;
		if (enabled) unexport_gpio(i);
	}
}

void PTT::set_gpio(bool ptt)
{
#define VALUE_MAX 30
	static const char s_values_str[] = "01";

	std::string portname = "/sys/class/gpio/gpio";
	std::string ctrlport;
	bool enabled = false;
	int val = 0;
	int fd;

	for (int i = 0; i < 17; i++) {
		enabled = (progdefaults.enable_gpio >> i) & 0x01;

		if (enabled) {
			val = (progdefaults.gpio_on >> i) & 0x01;
			ctrlport = portname;
			ctrlport.append(gpio_name[i]);
			ctrlport.append("/value");
			fd = fl_open(ctrlport.c_str(), O_WRONLY);

			bool ok = false;
			if (fd == -1) {
				LOG_ERROR("Failed to open gpio (%s) for writing!", ctrlport.c_str());
			} else {
				if (progdefaults.gpio_pulse_width == 0) {
					if (ptt) { if (val == 1) val = 1; else val = 0;}
					if (!ptt)  { if (val == 1) val = 0; else val = 1;}
					if (write(fd, &s_values_str[val], 1) == 1)
						ok = true;
				} else {
					if (write(fd, &s_values_str[val], 1) == 1) {
						MilliSleep(progdefaults.gpio_pulse_width);
						if (write(fd, &s_values_str[val == 0 ? 1 : 0], 1) == 1)
							ok = true;
					}
				}
				if (ok)
					LOG_INFO("Set GPIO ptt on %s %s%s",
						ctrlport.c_str(),
						(progdefaults.gpio_pulse_width > 0) ?
							"pulsed " : "",
						(val == 1 ? "HIGH" : "LOW")
					);
				else
					LOG_ERROR("Failed to write value!");

				close(fd);
			}
		}
	}
}

//-------------------- serial port PTT --------------------//

void PTT::open_tty(void)
{
	serPort.Baud(progdefaults.BaudRate(progdefaults.XmlRigBaudrate));
	serPort.Device(progdefaults.PTTdev);
	serPort.RTS(progdefaults.RTSplus);
	serPort.DTR(progdefaults.DTRplus);
	serPort.RTSptt(progdefaults.RTSptt);
	serPort.DTRptt(progdefaults.DTRptt);
	if (progdefaults.SCU_17) serPort.Stopbits(0);//1);
	else serPort.Stopbits(2);
	if (serPort.OpenPort() == false) {
		LOG_ERROR("Cannot open serial port %s", rigio.Device().c_str());
		pttfd = -1;
		return;
	}
	LOG_INFO("Serial port %s open", progdefaults.PTTdev.c_str());
}

void PTT::close_tty(void)
{
	serPort.ClosePort();
	pttfd = -1;
	LOG_DEBUG("Serial port %s closed", progdefaults.PTTdev.c_str());
}

void PTT::set_tty(bool ptt)
{
	serPort.SetPTT(ptt);
}

#if HAVE_PARPORT
//-------------------- parallel port PTT --------------------//

#if HAVE_LINUX_PPDEV_H
#  include <linux/ppdev.h>
#  include <linux/parport.h>
#elif HAVE_DEV_PPBUS_PPI_H
#  include <dev/ppbus/ppi.h>
#  include <dev/ppbus/ppbconf.h>
#endif

void PTT::open_parport(void)
{
    if (progdefaults.PTTdev.find("tty") != std::string::npos) return;

	int oflags = O_RDWR | O_NDELAY;
#	ifdef HAVE_O_CLOEXEC
		oflags = oflags | O_CLOEXEC;
#	endif

	if ((pttfd = fl_open(progdefaults.PTTdev.c_str(),  oflags)) == -1) {
		LOG_ERROR("Could not open %s: %s", progdefaults.PTTdev.c_str(), strerror(errno));
		return;
	}

	bool isparport = false;

	struct stat st;
	int status;

#if HAVE_LINUX_PPDEV_H     // Linux (ppdev)
	isparport = (fstat(pttfd, &st) == 0 && S_ISCHR(st.st_mode) &&
		     ioctl(pttfd, PPGETMODE, &status) != -1);
#elif HAVE_DEV_PPBUS_PPI_H // FreeBSD (ppbus/ppi) */
	isparport = (fstat(pttfd, &st) == 0 && S_ISCHR(st.st_mode) &&
		     ioctl(pttfd, PPISSTATUS, &status) != -1);
#else                      // Fallback (nothing)
	isparport = false;
#endif

	if (!isparport) {
		LOG_VERBOSE("%s: not a supported parallel port device", progdefaults.PTTdev.c_str());
		close_parport();
		pttfd = -1;
	}
}

void PTT::close_parport(void)
{
	close(pttfd);
}

void PTT::set_parport(bool ptt)
{
#ifdef HAVE_LINUX_PPDEV_H
	struct ppdev_frob_struct frob;

	frob.mask = PARPORT_CONTROL_INIT;
	frob.val = !ptt;
	ioctl(pttfd, PPFCONTROL, &frob);
#elif HAVE_DEV_PPBUS_PPI_H
	u_int8_t val;

	ioctl(pttfd, PPIGCTRL, &val);
	if (ptt)
		val |= nINIT;
	else
		val &= ~nINIT;
	ioctl(pttfd, PPISCTRL, &val);
#endif
}
#endif // HAVE_PARPORT


#if HAVE_UHROUTER
//-------------------- uhRouter PTT --------------------//

// See interface documentation at:
// http://homepage.mac.com/chen/w7ay/Router/Contents/routerInterface.html

#define	FUNCTIONMASK     0x1f

#define ROUTERFUNCTION   0x80
#define OPENMICROKEYER   (ROUTERFUNCTION + 0x01) // get a port to the microKEYER router
#define OPENCWKEYER      (ROUTERFUNCTION + 0x02) // get a port to the CW KEYER router
#define OPENDIGIKEYER    (ROUTERFUNCTION + 0x03) // get a port to the DIGI KEYER router
#define QUITIFNOKEYER    (ROUTERFUNCTION + 0x1f) // quit if there are no keyers
#define QUITIFNOTINUSE   (ROUTERFUNCTION + 0x1e) // quit if not connected
#define QUITALWAYS       (ROUTERFUNCTION + 0x1d) // quit
#define CLOSEKEYER       (ROUTERFUNCTION + FUNCTIONMASK)

#define KEYERFUNCTION    0x40
#define OPENPTT          (KEYERFUNCTION + 0x04)  // get a port to the PTT flag bit

#ifndef PATH_MAX
#  define PATH_MAX 1024
#endif

static ssize_t tm_read(int fd, void* buf, size_t len, const struct timeval* to)
{
	fd_set s;
	FD_ZERO(&s);
	FD_SET(fd, &s);

	struct timeval t;
	memcpy(&t, to, sizeof(t));

	ssize_t n;
	if ((n = select(fd + 1, &s, 0, 0, &t)) != 1)
		return n;

	return read(fd, buf, len);
}

static ssize_t tm_write(int fd, const void* buf, size_t len, const struct timeval* to)
{
	fd_set s;
	FD_ZERO(&s);
	FD_SET(fd, &s);

	struct timeval t;
	memcpy(&t, to, sizeof(t));

	ssize_t n;
	if ((n = select(fd + 1, 0, &s, 0, &t)) != 1)
		return n;

	return write(fd, buf, len);
}

static bool open_fifos(const char* base, int fd[2])
{
	struct stat st;
	std::string fifo = base;
	size_t len = fifo.length();

	fifo += "Read";
	if (stat(fifo.c_str(), &st) == -1 || !S_ISFIFO(st.st_mode)) {
		LOG_ERROR("%s is not a fifo", fifo.c_str());
		return false;
	}

	int oflags = O_RDONLY | O_NONBLOCK;
#	ifdef HAVE_O_CLOEXEC
		oflags = oflags | O_CLOEXEC;
#	endif

	if ((fd[0] = fl_open(fifo.c_str(), oflags)) == -1) {
		LOG_ERROR("Could not open %s: %s", fifo.c_str(), strerror(errno));
		return false;
	}

	fifo.erase(len);
	fifo += "Write";
	if (stat(fifo.c_str(), &st) == -1 || !S_ISFIFO(st.st_mode)) {
		LOG_ERROR("%s is not a fifo", fifo.c_str());
		return false;
	}
	oflags = O_WRONLY | O_NONBLOCK;

#	ifdef HAVE_O_CLOEXEC
		oflags = oflags | O_CLOEXEC;
#	endif

	if ((fd[1] = fl_open(fifo.c_str(), oflags)) == -1) {
		LOG_ERROR("Could not open %s: %s", fifo.c_str(), strerror(errno));
		return false;
	}

	return true;
}

static bool get_fifos(const int fd[2], const unsigned char* msg, size_t msglen, char* base, size_t baselen)
{
	struct timeval to = { 2, 0 };
	if (tm_write(fd[1], msg, msglen, &to) < (ssize_t)msglen) {
		LOG_PERROR("Could not write request");
		return false;
	}
	ssize_t r;
	if ((r = tm_read(fd[0], base, baselen-1, &to)) <= 0) {
		LOG_PERROR("Could not read FIFO name");
		return false;
	}
	base[r] = '\0';
	return true;
}

#ifdef __APPLE__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#endif

static bool start_uhrouter(void)
{
	bool found = false;
#ifdef __APPLE__
	FILE *fd = (FILE *)0;
	std::string appPath;
	std::string buffer;

	appPath.assign("/Applications/µH Router.app/Contents/MacOS/µH Router");

	fd = fl_fopen(appPath.c_str(), "r");
	if(fd) {
		found = true;
		fclose(fd);
	}

	if(found) {
		buffer.clear();
		buffer.assign("\"");
		buffer.append(appPath);
		buffer.append("\" & ");
		system(buffer.c_str());
	} else {
		LOG_ERROR("File: /Applications/\265H Router.app Not Found!");
	}

#endif // __APPLE__

	return found;
}

void PTT::open_uhrouter(void)
{
	struct {
		unsigned char keyer;
		const char* name;
		const char* abbrev;
	} keyers[] = {
		{ OPENMICROKEYER, "microKeyer", "MK" },
		{ OPENCWKEYER,    "CWKeyer",    "CK" },
		{ OPENDIGIKEYER,  "DigiKeyer",  "DK" }
	};
	size_t start = 0, end = sizeof(keyers)/sizeof(*keyers);

	// If the device string is something like /tmp/microHamRouter/microKeyer,
	// or /tmp/microHamRouter/MK, try that keyer only.
	re_t keyer_re("^" UHROUTER_FIFO_PREFIX "/(.+)$", REG_EXTENDED);
	if (keyer_re.match(progdefaults.PTTdev.c_str()) && keyer_re.nsub() == 2) {
		const char* keyer = keyer_re.submatch(1).c_str();
		// do we recognise this keyer name?
		for (size_t i = 0; i < sizeof(keyers)/sizeof(*keyers); i++) {
			if (!strcasecmp(keyers[i].name, keyer) || !strcasecmp(keyers[i].abbrev, keyer)) {
				start = i;
				end = start + 1;
				break;
			}
		}
	}
	LOG_VERBOSE("Will try %s", (start == end ? keyers[start].name : "all keyers"));

	int uhrfd[2];
	uhrfd[0] = uhrfd[1] = uhkfd[0] = uhkfd[1] = uhfd[0] = uhfd[1] = -1;

	if (!open_fifos(UHROUTER_FIFO_PREFIX, uhrfd)) {
		// if we just started uhrouter we will retry open_fifos a few times
		unsigned retries = start_uhrouter() ? 30 : 0;
		while (retries-- && !open_fifos(UHROUTER_FIFO_PREFIX, uhrfd))
			MilliSleep(100);
		if (uhrfd[0] == -1 || uhrfd[1] == -1) {
			LOG_ERROR("Could not open router");
			return;
		}
	}
	char fifo_name[PATH_MAX];
	size_t len = PATH_MAX - 8;
	memset(fifo_name, 0, sizeof(fifo_name));
	for (size_t i = start; i < end; i++) {
		// open keyer
		if (!get_fifos(uhrfd, &keyers[i].keyer, 1, fifo_name, len) || *fifo_name == '\0') {
			LOG_VERBOSE("Keyer \"%s\" not found", keyers[i].name);
			continue;
		}

		// open ptt port
		if (!open_fifos(fifo_name, uhkfd)) {
			LOG_ERROR("Could not open keyer %s", keyers[i].name);
			continue;
		}
		LOG_VERBOSE("Opened keyer %s", keyers[i].name);

		unsigned char port = OPENPTT;
		if (!get_fifos(uhkfd, &port, 1, fifo_name, len)) {
			LOG_ERROR("Could not get PTT port");
			continue;
		}
		if (!open_fifos(fifo_name, uhfd)) {
			LOG_ERROR("Could not open PTT port %s", fifo_name);
			continue;
		}

		LOG_VERBOSE("Successfully opened PTT port of keyer %s", keyers[i].name);
		break;
	}

	// close router FIFOs
	close(uhrfd[0]);
	close(uhrfd[1]);
}

void PTT::close_uhrouter(void)
{
	close(uhfd[0]);
	close(uhfd[1]);

	unsigned char c = QUITIFNOTINUSE;
	write(uhkfd[1], &c, 1);
	close(uhkfd[0]);
	close(uhkfd[1]);
}

void PTT::set_uhrouter(bool ptt)
{
	if (uhfd[0] == -1 || uhfd[1] == -1)
		return;

	unsigned char buf[_POSIX_PIPE_BUF];
	// empty the fifo
	while (read(uhfd[0], buf, sizeof(buf)) > 0);

	// send command
	*buf = '0' + ptt;
	LOG_VERBOSE("Sending PTT=%uc", *buf);
	struct timeval t = { 2, 0 };
	if (tm_write(uhfd[1], buf, 1, &t) != 1) {
		LOG_ERROR("Could not set PTT: %s", strerror(errno));
		return;
	}

	// wait for status
	ssize_t n = tm_read(uhfd[0], buf, sizeof(buf), &t);
	switch (n) {
	case -1:
		LOG_PERROR("tm_read");
		break;
	case 0:
		LOG_ERROR("No reply to PTT command within %jd seconds", (intmax_t)t.tv_sec);
		break;
	default:
		LOG_VERBOSE("Received \"%s\"", str2hex(buf, n));
		// last received char should be '1'(?)
		break;
	}
}

#endif // HAVE_UHROUTER
