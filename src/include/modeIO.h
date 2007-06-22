// ----------------------------------------------------------------------------
// modeIO.h  --  hardline control class for rtty/cw
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

#ifndef _MODIO_H
#define _MODIO_H

#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>

#define	BASE0	0x3f8 // ttyS0 (COM1) irq 4
#define	BASE1	0x2f8 // ttyS1 (COM2) irq 3
#define BASE2	0x3e8 // ttyS2 (COM3) irq 4
#define BASE3	0x2e8 // ttyS3 (COM4) irq 3

class modeIO {
private:
	int		fd;
	int		status;
	int		bit;
	int		setbit;
	int		uartBaseAddr;
	struct termios	oldtio;
	struct termios	temptio;
public:
	modeIO();
	~modeIO();
	void	openIO();
	void	closeIO();
	void	initIO();
	void	setRTS();
	void	clearRTS();
	void	setDTR();
	void	clearDTR();
	int		writeIO(int c);
};

extern modeIO *KeyLine;

#endif
