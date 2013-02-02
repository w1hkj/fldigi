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

using namespace std;

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

	if ((fd = open( device.c_str(), oflags)) < 0)
		return false;
// save current port settings
	tcflush (fd, TCIFLUSH);

	tcgetattr (fd, &oldtio);
	newtio = oldtio;

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

	return true;
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
	LOG_DEBUG("Serial port closed, fd = %d", fd);
	if (restore_tio) {
		ioctl(fd, TIOCMSET, &origstatus);
		tcsetattr (fd, TCSANOW, &oldtio);
	}
	close(fd);
	fd = -1;
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

using namespace std;

///////////////////////////////////////////////////////
// Function name	: Cserial::OpenPort
// Description		: Opens the port specified by strPortName
// Return type		: BOOL
// Argument		 : CString strPortName
///////////////////////////////////////////////////////
BOOL Cserial::OpenPort()
{
	string COMportname = "//./";

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
		LOG_ERROR("Invalid handle");
		return FALSE;
	}

	if (!ConfigurePort( baud, 8, FALSE, NOPARITY, stopbits)) {
		CloseHandle(hComm);
		hComm = INVALID_HANDLE_VALUE;
		LOG_ERROR("Could not configure port");
		return FALSE;
	}

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
	if (hComm) {
		if (restore_tio)
			bPortReady = SetCommTimeouts (hComm, &CommTimeoutsSaved);
		CloseHandle(hComm);
		hComm = INVALID_HANDLE_VALUE;
	}
	return;
}

int  Cserial::ReadData (unsigned char *buf, int nchars)
{
	if (!hComm)
	return FALSE;

	DWORD dwRead = 0;
	ReadFile(hComm, buf, nchars, &dwRead, NULL);
	return (int) dwRead;
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
	if (!hComm)
		return FALSE;

	nBytesWritten = 0;
	if (WriteFile(hComm,&by,1,&nBytesWritten,NULL)==0)
		return FALSE;
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
	if (!hComm)
		return -1;

	WriteFile (hComm, buff, n, &nBytesWritten, NULL);
	if (!nBytesWritten) {
		LOG_DEBUG("Reopening comm port");
		ClosePort();
		OpenPort();
		WriteFile (hComm, buff, n, &nBytesWritten, NULL);
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
		LOG_ERROR("GetCommState error on %s", device.c_str());
		CloseHandle(hComm);
		hComm = INVALID_HANDLE_VALUE;
		return FALSE;
	}

	dcb.BaudRate			= BaudRate;
	dcb.ByteSize			= ByteSize;
	dcb.Parity				= Parity ;
	dcb.StopBits			= (StopBits == 1 ? ONESTOPBIT : TWOSTOPBITS);
	dcb.fBinary				= TRUE;
	dcb.fDsrSensitivity		= FALSE;
	dcb.fParity				= dwParity;
	dcb.fOutX				= FALSE;
	dcb.fInX				= FALSE;
	dcb.fNull				= FALSE;
	dcb.fAbortOnError		= TRUE;
	dcb.fOutxCtsFlow		= FALSE;
	dcb.fOutxDsrFlow		= FALSE;

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

	bPortReady = SetCommState(hComm, &dcb);
	if(bPortReady == 0) {
		CloseHandle(hComm);
		hComm = INVALID_HANDLE_VALUE;
		LOG_ERROR("Port not available");
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
		LOG_ERROR("Invalid handle");
		return;
	}
	LOG_DEBUG("PTT = %d, DTRptt = %d, DTR = %d, RTSptt = %d, RTS = %d",
		  b, dtrptt, dtr, rtsptt, rts);

	if (b == true) {				// ptt enabled
		if (dtrptt && dtr)
			dcb.fDtrControl = DTR_CONTROL_DISABLE;
		if (dtrptt && !dtr)
			dcb.fDtrControl = DTR_CONTROL_ENABLE;
		if (rtscts == false) {
			if (rtsptt && rts)
				dcb.fRtsControl = RTS_CONTROL_DISABLE;
			if (rtsptt && !rts)
				dcb.fRtsControl = RTS_CONTROL_ENABLE;
		}
	} else {						// ptt disabled
		if (dtrptt && dtr)
			dcb.fDtrControl = DTR_CONTROL_ENABLE;
		if (dtrptt && !dtr)
			dcb.fDtrControl = DTR_CONTROL_DISABLE;
		if (rtscts == false) {
			if (rtsptt && rts)
				dcb.fRtsControl = RTS_CONTROL_ENABLE;
			if (rtsptt && !rts)
				dcb.fRtsControl = RTS_CONTROL_DISABLE;
		}
	}
	SetCommState(hComm, &dcb);
}

#endif //__MINGW32__


#ifdef __WOE32__
#include <sstream>
#include "re.h"

// convert COMx to /dev/ttySy with y = x - 1
void com_to_tty(string& port)
{
	re_t re("com([0-9]+)", REG_EXTENDED | REG_ICASE);
	if (!(re.match(port.c_str()) && re.nsub() == 2))
		return;
	stringstream ss;
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
void tty_to_com(string& port)
{
	re_t re("/dev/tty.([0-9]+)", REG_EXTENDED | REG_ICASE);
	if (!(re.match(port.c_str()) && re.nsub() == 2))
		return;
	stringstream ss;
	int n;
	ss << re.submatch(1);
	ss >> n;
	ss.clear(); ss.str("");
	ss << "COM" << n + 1;
	ss.seekp(0);
	port = ss.str();
}
#endif
