// ----------------------------------------------------------------------------
// modeIO.cxx  --  hardline control class for rtty/cw
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "modeIO.h"
#include "configuration.h"


modeIO *KeyLine = (modeIO *)0;

void modeIO::openIO()
{
	if (fd >= 0)
		return;
	
	if ((fd = open(progdefaults.CWFSKport.c_str(), O_RDWR | O_NOCTTY | O_NDELAY, 0)) < 0)
		return;

	uartBaseAddr = BASE0;		// default to "/dev/ttyS0"
	if (progdefaults.CWFSKport == "/dev/ttyS1")
		uartBaseAddr = BASE1;
	else if (progdefaults.CWFSKport == "/dev/ttyS2")
		uartBaseAddr = BASE2;
	else if (progdefaults.CWFSKport == "/dev/ttyS3")
		uartBaseAddr = BASE3;

	tcgetattr( fd, &oldtio);
	initIO();

	if (progdefaults.useCWkeylineRTS)
		clearRTS();
	if (progdefaults.useCWkeylineDTR)
		clearDTR();

	return;
}

void modeIO::closeIO()
{
	if (fd == -1) 
		return;
	tcsetattr(fd, TCSANOW, &oldtio);
	close(fd);
	fd = -1;
}

void modeIO::initIO()
{
	if (fd == -1)
		return;
	temptio = oldtio;
	temptio.c_cflag &= ~TIOCM_DTR;
	temptio.c_cflag &= ~TIOCM_RTS;

	temptio.c_cflag &= ~CSIZE;
	switch (progdefaults.rtty_bits) {
		case 0 : temptio.c_cflag |= CS5; break;
		case 1 : temptio.c_cflag |= CS7; break;
		case 2 : temptio.c_cflag |= CS8; break;
		default : temptio.c_cflag |= CS5;
	}
	switch (progdefaults.rtty_parity) {
		case PARITY_NONE : temptio.c_cflag &= ~PARENB; break;
		case PARITY_EVEN : temptio.c_cflag |= PARENB; temptio.c_cflag &= ~PARODD; break;
		case PARITY_ODD : temptio.c_cflag |= PARENB; temptio.c_cflag |= PARODD; break;
		default : temptio.c_cflag &= ~PARENB;
	}
	if (progdefaults.rtty_stop == 2)
		temptio.c_cflag |= CSTOPB;
	else
		temptio.c_cflag &= ~CSTOPB;

	tcsetattr(fd, TCSANOW, &temptio);

	int divisor;
	switch (progdefaults.rtty_baud) {
		case 0 : divisor = 2560; break; // 45 baud
		case 1 : divisor = 2535; break; // 45.45 baud
		case 2 : divisor = 2304; break; // 50 baud
		case 3 : divisor = 2057; break; // 56 baud
		case 4 : divisor = 1536; break; // 75 baud
		case 5 : divisor = 1152; break; // 100 baud
		case 6 : divisor = 1047; break; // 110 baud
		case 7 : divisor =  768; break; // 150 baud
		case 8 : divisor =  576; break; // 200 baud
		case 9 : divisor =  384; break; // 300 baud
		default: divisor = 2535; break;
	}

	ioperm (uartBaseAddr, 4, 1);
	status = inb (uartBaseAddr + 3);
	outb (status | 0x80, uartBaseAddr + 3);	// set DLB

	outb ((divisor >> 8) & 0xff, uartBaseAddr + 1);
	outb (divisor & 0xff, uartBaseAddr);

	outb (status, uartBaseAddr + 3);		// RESTORE ALL	

	ioperm (uartBaseAddr, 4, 0);

}

modeIO::modeIO()
{
	fd = -1;
	bit = 0;
	openIO();
}

modeIO::~modeIO()
{
	closeIO();
}

void modeIO::setRTS()
{
	if (fd == -1)
		return;
	int bits = TIOCM_RTS;
	ioctl (fd, TIOCMBIS, &bits);
}

void modeIO::clearRTS()
{
	if (fd == -1) 
		return;
	int bits = TIOCM_RTS;
	ioctl (fd, TIOCMBIC, &bits);
}

void modeIO::setDTR()
{
	if (fd == -1)
		return;
	int bits = TIOCM_DTR;
	ioctl (fd, TIOCMBIS, &bits);
}

void modeIO::clearDTR()
{
	if (fd == -1) 
		return;
	int bits = TIOCM_DTR;
	ioctl (fd, TIOCMBIC, &bits);
}

int modeIO::writeIO(int c)
{
	static char str[2];
	memset(str, 2, 0);
	str[0] = c;
	return write(fd, str, 1);
}
