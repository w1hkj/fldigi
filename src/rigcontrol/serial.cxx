// ----------------------------------------------------------------------------
// serial.cxx - Serial I/O class
//
// Copyright (C) 2007-2010
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
// ----------------------------------------------------------------------------

#include <FL/fl_utf8.h>

#include <config.h>

#include <string>

#include "serial.h"
#include "debug.h"

LOG_FILE_SOURCE(debug::LOG_RIGCONTROL);


#ifndef __MINGW32__
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <memory>

Cserial::Cserial() {
	device = "/dev/ttyS0";
	baud = 1200;
	timeout = 50; //msec
	retries = 5;
	rts = dtr = false;
	rtsptt = dtrptt = false;
	rtscts = false;
	status = 0;
	stopbits = 2;
	fd = -1;
	restore_tio = true;
}

Cserial::~Cserial() {
	ClosePort();
}

///////////////////////////////////////////////////////
// Function name	: Cserial::OpenPort
// Description		: Opens the port specified by strPortName
// Return type		: BOOL
// Argument		 : c_string strPortName
///////////////////////////////////////////////////////
bool Cserial::OpenPort()  {

#ifdef __CYGWIN__
	com_to_tty(device);
#endif

	int oflags = O_RDWR | O_NOCTTY | O_NDELAY;
#	ifdef HAVE_O_CLOEXEC
		oflags = oflags | O_CLOEXEC;
#	endif

	if ((fd = fl_open( device.c_str(), oflags)) < 0)
		return false;

// the status port must be set before any other control attributes are
// changed.  Failure to do so will cause DTR/RTS h/w transients !!

	ioctl(fd, TIOCMGET, &status);
	origstatus = status;

	if (dtr)
		status |= TIOCM_DTR; 		// set the DTR bit
	else
		status &= ~TIOCM_DTR;		// clear the DTR bit

	if (rtscts == false) {			// rts OK for ptt if RTSCTS not used
		if (rts)
			status |= TIOCM_RTS;		// set the RTS bit
		else
			status &= ~TIOCM_RTS;		// clear the RTS bit
	}

	ioctl(fd, TIOCMSET, &status);

// save current port settings
	tcflush (fd, TCIFLUSH);
	tcgetattr (fd, &oldtio);
//	newtio = oldtio;

	// 8 data bits
	newtio.c_cflag &= ~CSIZE;
	newtio.c_cflag |= CS8;
	// enable receiver, set local mode
	newtio.c_cflag |= (CLOCAL | CREAD);
	// no parity
	newtio.c_cflag &= ~PARENB;

	if (stopbits == 1)
		// 1 stop bit
		newtio.c_cflag &= ~CSTOPB;
	else
		// 2 stop bit
		newtio.c_cflag |= CSTOPB;

	if (rtscts)
		// h/w handshake
		newtio.c_cflag |= CRTSCTS;
	else
		// no h/w handshake
		newtio.c_cflag &= ~CRTSCTS;

	// raw input
	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	// raw output
	newtio.c_oflag &= ~OPOST;
	// software flow control disabled
	newtio.c_iflag &= ~IXON;
	// do not translate CR to NL
	newtio.c_iflag &= ~ICRNL;

 	switch(baud) {
		case 50:
			speed = B50;
			break;
 		case 300:
			speed = B300;
			break;
  		case 1200:
			speed = B1200;
			break;
  		case 2400:
			speed = B2400;
			break;
  		case 4800:
			speed = B4800;
			break;
  		case 9600:
			speed = B9600;
			break;
  		case 19200:
			speed = B19200;
			break;
  		case 38400:
			speed = B38400;
			break;
		case 57600:
			speed = B57600;
			break;
  		case 115200:
			speed = B115200;
			break;
  		default:
  			speed = B1200;
  	}
	cfsetispeed(&newtio, speed);
	cfsetospeed(&newtio, speed);

	tcsetattr (fd, TCSANOW, &newtio);

	return true;
}

void Cserial::SetDTR(bool b)
{
	ioctl(fd, TIOCMGET, &status);
	origstatus = status;
	if (b)
		status |= TIOCM_DTR; 		// set the DTR bit
	else
		status &= ~TIOCM_DTR;		// clear the DTR bit
	ioctl(fd, TIOCMSET, &status);
}

void Cserial::SetRTS(bool b)
{
	ioctl(fd, TIOCMGET, &status);
	origstatus = status;
	if (b)
		status |= TIOCM_RTS;		// set the RTS bit
	else
		status &= ~TIOCM_RTS;		// clear the RTS bit
	ioctl(fd, TIOCMSET, &status);
}

///////////////////////////////////////////////////////
// Function name	: Cserial::setPTT
// Return type	  : void
///////////////////////////////////////////////////////
void Cserial::SetPTT(bool b)
{
	if (fd < 0) {
		LOG_DEBUG("PTT fd < 0");
		return;
	}
	if (dtrptt || rtsptt) {
		ioctl(fd, TIOCMGET, &status);
		LOG_DEBUG("H/W PTT %d, status %X", b, status);
		if (b == true) {					  // ptt enabled
			if (dtrptt && dtr)  status &= ~TIOCM_DTR;	 // toggle low
			if (dtrptt && !dtr) status |= TIOCM_DTR;	  // toggle high
			if (rtscts == false) {
				if (rtsptt && rts)  status &= ~TIOCM_RTS; // toggle low
				if (rtsptt && !rts) status |= TIOCM_RTS;  // toggle high
			}
		} else {						  // ptt disabled
			if (dtrptt && dtr)  status |= TIOCM_DTR;	  // toggle high
			if (dtrptt && !dtr) status &= ~TIOCM_DTR;	 // toggle low
			if (rtscts == false) {
				if (rtsptt && rts)  status |= TIOCM_RTS;  // toggle high
				if (rtsptt && !rts) status &= ~TIOCM_RTS; // toggle low
			}
		}
		LOG_DEBUG("Status %02X, %s", status & 0xFF, uint2bin(status, 8));
		ioctl(fd, TIOCMSET, &status);
	}
	LOG_DEBUG("No PTT specified");
}

///////////////////////////////////////////////////////
// Function name	: Cserial::ClosePort
// Description		: Closes the Port
// Return type		: void
///////////////////////////////////////////////////////
void Cserial::ClosePort()
{
	if (fd < 0) return;
	if (restore_tio) {
// Some serial drivers force RTS and DTR high immediately upon
// opening the port, so our origstatus will indicate those bits
// high (though the lines weren't actually high before we opened).
// But then when we "restore" RTS and DTR from origstatus here
// it can result in PTT activation upon program exit!  To avoid
// this possibility, we ignore the apparentl initial settings, and
// instead force RTS and DTR low before closing the port.  (Just
// omitting the ioctl(TIOCMSET) would also resolve the problem).
// Kamal Mostafa <kamal@whence.com>
		origstatus &= ~(TIOCM_RTS|TIOCM_DTR);
		ioctl(fd, TIOCMSET, &origstatus);
		tcsetattr (fd, TCSANOW, &oldtio);
	}
	close(fd);
	fd = -1;
	LOG_INFO("Serial port closed, fd = %d", fd);
	return;
}

bool  Cserial::IOselect ()
{
	fd_set rfds;
	struct timeval tv;
	int retval;

	FD_ZERO (&rfds);
	FD_SET (fd, &rfds);
	tv.tv_sec = timeout/1000;
	tv.tv_usec = (timeout % 1000) * 1000;
	retval = select (FD_SETSIZE, &rfds, (fd_set *)0, (fd_set *)0, &tv);
	if (retval <= 0) // no response from serial port or error returned
		return false;
	return true;
}

///////////////////////////////////////////////////////
// Function name	: Cserial::ReadByte
// Description		: Reads 1 char from the selected port
// Return type		: 1 - success, 0 - failure
// Argument			: reference to character
///////////////////////////////////////////////////////
bool Cserial::ReadByte(unsigned char &c)
{
	static char ch[2];
	c = ch[0] = ch[1] = 0;
	if (fd < 0) return 0;
	if (!IOselect())
		return 0;

	if (read (fd, (void *)ch, 1) < 0)
		return 0;
	c = ch[0];
	return 1;
}

///////////////////////////////////////////////////////
// Function name	: Cserial::ReadBuffer
// Description		: Reads upto nchars from the selected port
// Return type		: # characters received
// Argument		 : pointer to buffer; # chars to read
///////////////////////////////////////////////////////
int  Cserial::ReadBuffer (unsigned char *buf, int nchars)
{
	if (fd < 0) return 0;
	int retnum, nread = 0;
	while (nchars > 0) {
		if (!IOselect()) {
			return nread;
		}
		retnum = read (fd, (char *)(buf + nread), nchars);
		if (retnum < 0)
			return 0;//nread;
		if (retnum == 0)
			return nread;
		nread += retnum;
		nchars -= retnum;
	}
	return nread;
}

///////////////////////////////////////////////////////
// Function name	: Cserial::WriteBuffer
// Description		: Writes a string to the selected port
// Return type		: BOOL
// Argument		 : BYTE by
///////////////////////////////////////////////////////
int Cserial::WriteBuffer(unsigned char *buff, int n)
{
	if (fd < 0) return 0;
	int ret = write (fd, buff, n);
	return ret;
}

///////////////////////////////////////////////////////
// Function name : Cserial::FlushBuffer
// Description   : flushes the pending rx chars
// Return type   : void
///////////////////////////////////////////////////////
void Cserial::FlushBuffer()
{
	if (fd < 0)
		return;
	tcflush (fd, TCIFLUSH);
}

#else // __MINGW32__

//======================================================================
// Win32 support code
//======================================================================
#include <winbase.h>
#include "estrings.h"

#define HCOMM_DEBUG 1

///////////////////////////////////////////////////////
// Function name	: Cserial::OpenPort
// Description		: Opens the port specified by strPortName
// Return type		: BOOL
// Argument		 : CString strPortName
///////////////////////////////////////////////////////
BOOL Cserial::OpenPort()
{
	std::string COMportname = "//./";

	tty_to_com(device);
	COMportname += device;

	hComm = CreateFile(COMportname.c_str(),
			  GENERIC_READ | GENERIC_WRITE,
			  0,
			  0,
			  OPEN_EXISTING,
			  0,
			  0);

	if(hComm == INVALID_HANDLE_VALUE) {
		errno = GetLastError();
		LOG_PERROR(win_error_string(errno).c_str());
		return FALSE;
	}

	if (!ConfigurePort( baud, 8, FALSE, NOPARITY, stopbits)) {
		errno = GetLastError();
		CloseHandle(hComm);
		hComm = INVALID_HANDLE_VALUE;
		LOG_PERROR(win_error_string(errno).c_str());
		return FALSE;
	}
	LOG_INFO("COM port %s opened, handle = %p", device.c_str(), hComm);

	FlushBuffer();

	return TRUE;
}


///////////////////////////////////////////////////////
// Function name	: Cserial::ClosePort
// Description		: Closes the Port
// Return type		: void
///////////////////////////////////////////////////////

void Cserial::ClosePort()
{
	if (hComm == INVALID_HANDLE_VALUE)
		return;
if (HCOMM_DEBUG)
	LOG_INFO("Closing COM port, handle = %p", hComm);
	if (restore_tio) {
if (HCOMM_DEBUG)
	LOG_INFO("Closing COM port#1, handle = %p", hComm);
		bPortReady = SetCommTimeouts (hComm, &CommTimeoutsSaved);
}
	if (CloseHandle(hComm) == 0) {
if (HCOMM_DEBUG)
	LOG_INFO("Closing COM port#2, handle = %p", hComm);
		errno = GetLastError();
if (HCOMM_DEBUG)
	LOG_INFO("Closing COM port#3, handle = %p", hComm);
		LOG_PERROR(win_error_string(errno).c_str());
if (HCOMM_DEBUG)
	LOG_INFO("Closing COM port#4, handle = %p", hComm);
	}
if (HCOMM_DEBUG)
	LOG_INFO("Closing COM port#5, handle = %p", hComm);

	hComm = INVALID_HANDLE_VALUE;
	return;
}

int  Cserial::ReadData (unsigned char *buf, int nchars)
{
	if (hComm == INVALID_HANDLE_VALUE)
		return 0;

	DWORD dwRead = 0;
	if (ReadFile(hComm, buf, nchars, &dwRead, NULL))
		return static_cast<int>(dwRead);
	if (dwRead == 0) return 0;

	errno = GetLastError();
	LOG_PERROR(win_error_string(errno).c_str());
	return 0;
}

BOOL Cserial::ReadByte(unsigned char & by)
{
static	BYTE byResByte[2];
	if (ReadData(byResByte, 1) == 1) {
		by = byResByte[0];
		return true;
	}
	return false;
}

void Cserial::FlushBuffer()
{
	unsigned char c;
	while (ReadByte(c) == true);
}

///////////////////////////////////////////////////////
// Function name	: Cserial::WriteByte
// Description		: Writes a Byte to teh selected port
// Return type		: BOOL
// Argument		 : BYTE by
///////////////////////////////////////////////////////
BOOL Cserial::WriteByte(UCHAR by)
{
	if (hComm == INVALID_HANDLE_VALUE)
		return FALSE;

	nBytesWritten = 0;
	if (WriteFile(hComm,&by,1,&nBytesWritten,NULL)==0) {
		errno = GetLastError();
		LOG_PERROR(win_error_string(errno).c_str());
		return FALSE;
	}
	return TRUE;
}

///////////////////////////////////////////////////////
// Function name	: Cserial::WriteBuffer
// Description		: Writes a string to the selected port
// Return type		: BOOL
// Argument		 : BYTE by
///////////////////////////////////////////////////////
int Cserial::WriteBuffer(unsigned char *buff, int n)
{
	if (hComm == INVALID_HANDLE_VALUE)
		return 0;

	if (WriteFile (hComm, buff, n, &nBytesWritten, NULL) == 0) {
		errno = GetLastError();
		LOG_PERROR(win_error_string(errno).c_str());
		return 0;
	}

	return nBytesWritten;
}


///////////////////////////////////////////////////////
// Function name	: Cserial::SetCommunicationTimeouts
// Description		: Sets the timeout for the selected port
// Return type		: BOOL
// Argument		 : DWORD ReadIntervalTimeout
// Argument		 : DWORD ReadTotalTimeoutMultiplier
// Argument		 : DWORD ReadTotalTimeoutConstant
// Argument		 : DWORD WriteTotalTimeoutMultiplier
// Argument		 : DWORD WriteTotalTimeoutConstant
///////////////////////////////////////////////////////
BOOL Cserial::SetCommunicationTimeouts(
	DWORD ReadIntervalTimeout, // msec
	DWORD ReadTotalTimeoutMultiplier,
	DWORD ReadTotalTimeoutConstant,
	DWORD WriteTotalTimeoutMultiplier,
	DWORD WriteTotalTimeoutConstant
)
{
	CommTimeouts.ReadIntervalTimeout = ReadIntervalTimeout;
	CommTimeouts.ReadTotalTimeoutMultiplier = ReadTotalTimeoutMultiplier;
	CommTimeouts.ReadTotalTimeoutConstant = ReadTotalTimeoutConstant;
	CommTimeouts.WriteTotalTimeoutConstant = WriteTotalTimeoutConstant;
	CommTimeouts.WriteTotalTimeoutMultiplier = WriteTotalTimeoutMultiplier;

	LOG_DEBUG("\n\
Read Interval Timeout............... %8ld %8ld\n\
Read Total Timeout Multiplier....... %8ld %8ld\n\
Read Total Timeout Constant Timeout. %8ld %8ld\n\
Write Total Timeout Constant........ %8ld %8ld\n\
Write Total Timeout Multiplier...... %8ld %8ld",
		 CommTimeoutsSaved.ReadIntervalTimeout,
		 CommTimeouts.ReadIntervalTimeout,
		 CommTimeoutsSaved.ReadTotalTimeoutMultiplier,
		 CommTimeouts.ReadTotalTimeoutMultiplier,
		 CommTimeoutsSaved.ReadTotalTimeoutConstant,
		 CommTimeouts.ReadTotalTimeoutConstant,
		 CommTimeoutsSaved.WriteTotalTimeoutConstant,
		 CommTimeouts.WriteTotalTimeoutConstant,
		 CommTimeoutsSaved.WriteTotalTimeoutMultiplier,
		 CommTimeouts.WriteTotalTimeoutMultiplier);

	bPortReady = SetCommTimeouts (hComm, &CommTimeouts);

	if(bPortReady ==0) {
		LOG_PERROR(win_error_string(GetLastError()).c_str());
		CloseHandle(hComm);
		hComm = INVALID_HANDLE_VALUE;
		return FALSE;
	}

	return TRUE;
}

/*
 * ReadIntervalTimeout
 *
 * The maximum time allowed to elapse between the arrival of two bytes on the
 * communications line, in milliseconds. During a ReadFile operation, the time
 * period begins when the first byte is received. If the interval between the
 * arrival of any two bytes exceeds this amount, the ReadFile operation is
 * completed and any buffered data is returned. A value of zero indicates that
 * interval time-outs are not used.
 *
 * A value of MAXDWORD, combined with zero values for both the
 * ReadTotalTimeoutConstant and ReadTotalTimeoutMultiplier members, specifies
 * that the read operation is to return immediately with the bytes that have
 * already been received, even if no bytes have been received.
 *
 * ReadTotalTimeoutMultiplier
 *
 * The multiplier used to calculate the total time-out period for read
 * operations, in milliseconds. For each read operation, this value is
 * multiplied by the requested number of bytes to be read.
 *
 * ReadTotalTimeoutConstant
 *
 * A constant used to calculate the total time-out period for read operations,
 * in milliseconds. For each read operation, this value is added to the product
 * of the ReadTotalTimeoutMultiplier member and the requested number of bytes.
 *
 * A value of zero for both the ReadTotalTimeoutMultiplier and
 * ReadTotalTimeoutConstant members indicates that total time-outs are not
 * used for read operations.
 *
 * WriteTotalTimeoutMultiplier
 *
 * The multiplier used to calculate the total time-out period for write
 * operations, in milliseconds. For each write operation, this value is
 * multiplied by the number of bytes to be written.
 *
 * WriteTotalTimeoutConstant
 *
 * A constant used to calculate the total time-out period for write operations,
 * in milliseconds. For each write operation, this value is added to the product
 * of the WriteTotalTimeoutMultiplier member and the number of bytes to be
 * written.
 *
 * A value of zero for both the WriteTotalTimeoutMultiplier and
 * WriteTotalTimeoutConstant members indicates that total time-outs are not
 * used for write operations.
 *
 * Remarks
 *
 * If an application sets ReadIntervalTimeout and ReadTotalTimeoutMultiplier to
 * MAXDWORD and sets ReadTotalTimeoutConstant to a value greater than zero and
 * less than MAXDWORD, one of the following occurs when the ReadFile function
 * is called:
 *
 * If there are any bytes in the input buffer, ReadFile returns immediately
 * with the bytes in the buffer.
 *
 * If there are no bytes in the input buffer, ReadFile waits until a byte
 * arrives and then returns immediately.
 *
 * If no bytes arrive within the time specified by ReadTotalTimeoutConstant,
 * ReadFile times out.
*/
BOOL Cserial::SetCommTimeout() {
	return SetCommunicationTimeouts (
		MAXDWORD, // Read Interval Timeout
		MAXDWORD, // Read Total Timeout Multiplier
		10,       // Read Total Timeout Constant
		50,       // Write Total Timeout Constant
		5         // Write Total Timeout Multiplier
	);
  }

///////////////////////////////////////////////////////
// Function name	: ConfigurePort
// Description		: Configures the Port
// Return type		: BOOL
// Argument		 : DWORD BaudRate
// Argument		 : BYTE ByteSize
// Argument		 : DWORD fParity
// Argument		 : BYTE  Parity
// Argument		 : BYTE StopBits
///////////////////////////////////////////////////////
BOOL Cserial::ConfigurePort(DWORD	BaudRate,
				BYTE	ByteSize,
				DWORD	dwParity,
				BYTE	Parity,
				BYTE	StopBits)
{
	if((bPortReady = GetCommState(hComm, &dcb))==0) {
		errno = GetLastError();
		LOG_PERROR(win_error_string(errno).c_str());
		CloseHandle(hComm);
		hComm = INVALID_HANDLE_VALUE;
		return FALSE;
	}

	LOG_INFO("\n\
Get Comm State:\n\
DCB.DCBlength       %d\n\
DCB.Baudrate        %d\n\
DCB.ByteSize        %d\n\
DCB.Parity          %d\n\
DCB.StopBits        %d\n\
DCB.Binary          %d\n\
DCB.fDtrControl     %d\n\
DCB.fRtsControl     %d\n\
DCB.fDsrSensitivity %d\n\
DCB.fParity         %d\n\
DCB.fOutX           %d\n\
DCB.fInX            %d\n\
DCB.fNull           %d\n\
DCB.XonChar         %d\n\
DCB.XoffChar        %d\n\
DCB.fAbortOnError   %d\n\
DCB.fOutxCtsFlow    %d\n\
DCB.fOutxDsrFlow    %d\n",
	(int)dcb.DCBlength,
	(int)dcb.BaudRate,
	(int)dcb.ByteSize,
	(int)dcb.Parity,
	(int)dcb.StopBits,
	(int)dcb.fBinary,
	(int)dcb.fDtrControl,
	(int)dcb.fRtsControl,
	(int)dcb.fDsrSensitivity,
	(int)dcb.fParity,
	(int)dcb.fOutX,
	(int)dcb.fInX,
	(int)dcb.fNull,
	(int)dcb.XonChar,
	(int)dcb.XoffChar,
	(int)dcb.fAbortOnError,
	(int)dcb.fOutxCtsFlow,
	(int)dcb.fOutxDsrFlow);

	dcb.BaudRate			= BaudRate;
	dcb.ByteSize			= ByteSize;
	dcb.Parity				= Parity;
	dcb.StopBits			= (StopBits == 1) ? 0 : 2;
//	dcb.fBinary				= TRUE;
//	dcb.fDsrSensitivity		= FALSE;
//	dcb.fParity				= dwParity;
//	dcb.fOutX				= FALSE;
//	dcb.fInX				= FALSE;
//	dcb.fNull				= FALSE;
//	dcb.fAbortOnError		= TRUE;
//	dcb.fOutxCtsFlow		= FALSE;
//	dcb.fOutxDsrFlow		= FALSE;

	if (dtr)
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
	else
		dcb.fDtrControl = DTR_CONTROL_DISABLE;

	dcb.fDsrSensitivity = FALSE;

	if (rtscts)
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
	else {
		if (rts)
			dcb.fRtsControl = RTS_CONTROL_ENABLE;
		else
			dcb.fRtsControl = RTS_CONTROL_DISABLE;
	}

	LOG_INFO("\n\
Set Comm State:\n\
DCB.DCBlength       %d\n\
DCB.Baudrate        %d\n\
DCB.ByteSize        %d\n\
DCB.Parity          %d\n\
DCB.StopBits        %d\n\
DCB.Binary          %d\n\
DCB.fDtrControl     %d\n\
DCB.fRtsControl     %d\n\
DCB.fDsrSensitivity %d\n\
DCB.fParity         %d\n\
DCB.fOutX           %d\n\
DCB.fInX            %d\n\
DCB.fNull           %d\n\
DCB.XonChar         %d\n\
DCB.XoffChar        %d\n\
DCB.fAbortOnError   %d\n\
DCB.fOutxCtsFlow    %d\n\
DCB.fOutxDsrFlow    %d\n",
	(int)dcb.DCBlength,
	(int)dcb.BaudRate,
	(int)dcb.ByteSize,
	(int)dcb.Parity,
	(int)dcb.StopBits,
	(int)dcb.fBinary,
	(int)dcb.fDtrControl,
	(int)dcb.fRtsControl,
	(int)dcb.fDsrSensitivity,
	(int)dcb.fParity,
	(int)dcb.fOutX,
	(int)dcb.fInX,
	(int)dcb.fNull,
	(int)dcb.XonChar,
	(int)dcb.XoffChar,
	(int)dcb.fAbortOnError,
	(int)dcb.fOutxCtsFlow,
	(int)dcb.fOutxDsrFlow);

	bPortReady = SetCommState(hComm, &dcb);

	if(bPortReady == 0) {
		errno = GetLastError();
		LOG_PERROR(win_error_string(errno).c_str());
		CloseHandle(hComm);
		hComm = INVALID_HANDLE_VALUE;
		return FALSE;
	}

	if ( (bPortReady = GetCommTimeouts (hComm, &CommTimeoutsSaved) ) == 0)
		return FALSE;

	return SetCommTimeout();
}

///////////////////////////////////////////////////////
// Function name	: Cserial::setPTT
// Return type	  : void
///////////////////////////////////////////////////////
void Cserial::SetPTT(bool b)
{
	if ( !(dtrptt || rtsptt) )
		return;
	if(hComm == INVALID_HANDLE_VALUE) {
		LOG_PERROR("Invalid handle");
		return;
	}
	LOG_DEBUG("PTT = %d, DTRptt = %d, DTR = %d, RTSptt = %d, RTS = %d",
		  b, dtrptt, dtr, rtsptt, rts);

	if (b == true) {				// ptt enabled
		if (dtrptt && dtr)
			SetDTR(0);
		if (dtrptt && !dtr)
			SetDTR(1);
		if (rtscts == false) {
			if (rtsptt && rts)
				SetRTS(0);
			if (rtsptt && !rts)
				SetRTS(1);
		}
	} else {						// ptt disabled
		if (dtrptt && dtr)
			SetDTR(1);
		if (dtrptt && !dtr)
			SetDTR(0);
		if (rtscts == false) {
			if (rtsptt && rts)
				SetRTS(1);
			if (rtsptt && !rts)
				SetRTS(0);
		}
	}
}

void Cserial::SetDTR(bool b)
{
	if(hComm == INVALID_HANDLE_VALUE) {
		LOG_PERROR("Invalid handle");
		return;
	}
	if (b) EscapeCommFunction(hComm, SETDTR);
	else   EscapeCommFunction(hComm, CLRDTR);
}

void Cserial::SetRTS(bool b)
{
	if(hComm == INVALID_HANDLE_VALUE) {
		LOG_PERROR("Invalid handle");
		return;
	}
	if (b) EscapeCommFunction(hComm, SETRTS);
	else   EscapeCommFunction(hComm, CLRRTS);
}


//======================================================================
// end Win32 code
//======================================================================
#endif //__MINGW32__

#ifdef __WOE32__
#include <sstream>
#include "re.h"

// convert COMx to /dev/ttySy with y = x - 1
void com_to_tty(std::string& port)
{
	re_t re("com([0-9]+)", REG_EXTENDED | REG_ICASE);
	if (!(re.match(port.c_str()) && re.nsub() == 2))
		return;
	std::stringstream ss;
	int n;
	ss << re.submatch(1);
	ss >> n;
	if (--n < 0)
		n = 0;
	ss.clear(); ss.str("");
	ss << "/dev/ttyS" << n;
	ss.seekp(0);
	port = ss.str();
}

// convert  /dev/ttySx to COMy with y = x + 1
void tty_to_com(std::string& port)
{
	re_t re("/dev/tty.([0-9]+)", REG_EXTENDED | REG_ICASE);
	if (!(re.match(port.c_str()) && re.nsub() == 2))
		return;
	std::stringstream ss;
	int n;
	ss << re.submatch(1);
	ss >> n;
	ss.clear(); ss.str("");
	ss << "COM" << n + 1;
	ss.seekp(0);
	port = ss.str();
}

#endif
