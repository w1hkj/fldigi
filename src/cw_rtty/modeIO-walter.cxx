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
#include <sys/io.h>

#include "modeIO.h"
#include "configuration.h"


#define BASE 0x3f8

modeIO *KeyLine = (modeIO *)0;

void modeIO::openIO()
{
	if (fd >= 0)
		return;
	
	if ((fd = open(progdefaults.CWFSKport.c_str(), O_RDWR | O_NOCTTY | O_NDELAY, 0)) < 0)
		return;

	tcgetattr( fd, &oldtio);
	initIO();
	if (progdefaults.useFSKkeyline)
		clearRTS();
	if (progdefaults.useFSKkeylineDTR)
		clearDTR();
	if (progdefaults.useCWkeyline)
		clearDTR();
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

	int speed;
	unsigned int bauds,status,tiempo0,tiempo1,nt0,nt1,divider;

	bauds=0;
	switch (progdefaults.rtty_baud) 
	{
	case 0 :
	case 1 : 
		speed =  B50;
		bauds=50;
printf("\npase por aqui en B45.45 y en B45\n"); 		
		break;
	case 2 : 
		speed =  B50;
		bauds=50; 
		break;
	case 3 : 
		speed =  B50; 
		break;
	case 4 : speed =  B75; break;
	case 5 : speed =  B110; break;
	case 6 : speed =  B110; break;
	case 7 : speed =  B150; break;
	case 8 : speed =  B200; break;
	case 9 : speed =  B300; break;
	default: speed =  B50; break;
	}
	cfsetispeed(&temptio, speed);
	cfsetospeed(&temptio, speed);
	
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
	tcgetattr( fd, &temptio);

if(bauds==50)   // load 2534 instead of 2304 for 45.45
{
	printf("\nlos baudios son 45 o 45.45\n");
ioperm(BASE,4,1);
	status=inb(BASE+3);
	outb(status|0x80,BASE+3); // set DLB

	tiempo0=inb(BASE+1);	// read LSB of divider
	tiempo0 <<=8;
	tiempo0 &=0xff00;

	tiempo1=inb(BASE)&0x00ff; //read MSB	
	tiempo1=tiempo1+tiempo0;
	printf("\ndivisor=%d\n",tiempo1); // print divider
	outb(status,BASE+3);

status=inb(BASE+3);
outb(status|0x80,BASE+3);	// change LSB

	
	divider=2534;
	nt0=divider&0xff00;
	nt0 >>=8;
	outb(nt0,BASE+1);
	nt1=divider&0x00ff;
	outb(nt1,BASE);

outb(status,BASE+3);		// RESTORE ALL	


// check if all are ok	

ioperm(BASE,4,1);
	status=inb(BASE+3);
	outb(status|0x80,BASE+3); // set DLB

	tiempo0=inb(BASE+1);	// read LSB of divider
	tiempo0 <<=8;
	tiempo0 &=0xff00;

	tiempo1=inb(BASE)&0x00ff; //read MSB	
	tiempo1=tiempo1+tiempo0;
	printf("\ndivisor=%d\n",tiempo1); // print divider
	outb(status,BASE+3);

ioperm(BASE,4,0);
}

	
}

modeIO::modeIO()
{
	fd = -1;
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
	ioctl(fd, TIOCMGET, &status);
	status |= TIOCM_RTS;
	ioctl (fd, TIOCMSET, &status);
}

void modeIO::clearRTS()
{
	if (fd == -1) 
		return;
	ioctl(fd, TIOCMGET, &status);
	status &= ~TIOCM_RTS;
	ioctl (fd, TIOCMSET, &status);
}

void modeIO::setDTR()
{
	if (fd == -1)
		return;
	ioctl(fd, TIOCMGET, &status);
	status |= TIOCM_DTR;
	ioctl (fd, TIOCMSET, &status);
}

void modeIO::clearDTR()
{
	if (fd == -1) 
		return;
	ioctl(fd, TIOCMGET, &status);
	status &= ~TIOCM_DTR;
	ioctl (fd, TIOCMSET, &status);
}

int modeIO::writeIO(int c)
{
	static char str[2];
	memset(str, 2, 0);
	str[0] = c;
	return write(fd, str, 1);
}
