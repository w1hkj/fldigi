//-----------------------------------------------------------------------------
//
// Serial i/o class
//
// copyright Dave Freese 2006, w1hkj@w1hkj.com
//
//-----------------------------------------------------------------------------

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
	fd = -1;
}

Cserial::~Cserial() {
	ClosePort(); 
}

///////////////////////////////////////////////////////
// Function name	: Cserial::OpenPort
// Description	    : Opens the port specified by strPortName
// Return type		: BOOL 
// Argument         : c_string strPortName
///////////////////////////////////////////////////////
bool Cserial::OpenPort()  {
	
#ifdef __CYGWIN__
	com_to_tty(device);
#endif
	if ((fd = open( device.c_str(), O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
		return false;
// save current port settings
	tcflush (fd, TCIFLUSH);

	tcgetattr (fd, &oldtio);
	newtio = oldtio;
	
	newtio.c_cflag &= ~CSIZE;
	newtio.c_cflag |= CS8 | CLOCAL | CREAD;

    newtio.c_cflag &= ~PARENB;

	if (0)
		newtio.c_cflag &= ~CSTOPB; // 1 stop bit
	else
		newtio.c_cflag |= CSTOPB; // 2 stop bit

	if (rtscts)
		newtio.c_cflag |= CRTSCTS;
	else
		newtio.c_cflag &= ~CRTSCTS;

	newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtio.c_oflag &= ~OPOST;

    newtio.c_iflag &= ~IXON;
	
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
// Function name    : Cserial::setPTT
// Return type      : void
///////////////////////////////////////////////////////
void Cserial::SetPTT(bool b)
{
	if (fd < 0) {
		LOG_DEBUG("ptt fd < 0");
		return;
	}
	if (dtrptt || rtsptt) {
        ioctl(fd, TIOCMGET, &status);
	    LOG_INFO("h/w ptt %d, Status %X", b, status);
	    if (b == true) {                                  // ptt enabled
		    if (dtrptt && dtr)  status &= ~TIOCM_DTR;     // toggle low
		    if (dtrptt && !dtr) status |= TIOCM_DTR;      // toggle high
            if (rtscts == false) {
			    if (rtsptt && rts)  status &= ~TIOCM_RTS; // toggle low
			    if (rtsptt && !rts) status |= TIOCM_RTS;  // toggle high
		    }
	    } else {                                          // ptt disabled
		    if (dtrptt && dtr)  status |= TIOCM_DTR;      // toggle high
		    if (dtrptt && !dtr) status &= ~TIOCM_DTR;     // toggle low
		    if (rtscts == false) {
			    if (rtsptt && rts)  status |= TIOCM_RTS;  // toggle high
			    if (rtsptt && !rts) status &= ~TIOCM_RTS; // toggle low
		    }
	    }
	    LOG_INFO("Status %02X, %s", status & 0xFF, uint2bin(status, 8));
	    ioctl(fd, TIOCMSET, &status);
    }
    LOG_DEBUG("No ptt specified");
}

///////////////////////////////////////////////////////
// Function name	: Cserial::ClosePort
// Description	    : Closes the Port
// Return type		: void 
///////////////////////////////////////////////////////
void Cserial::ClosePort()
{
	if (fd < 0) return;
	LOG_INFO("Serial port closed, fd = %d", fd);
	ioctl(fd, TIOCMSET, &origstatus);
	tcsetattr (fd, TCSANOW, &oldtio);
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
// Description	    : Reads upto nchars from the selected port
// Return type		: # characters received 
// Argument         : pointer to buffer; # chars to read
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
// Description	    : Writes a string to the selected port
// Return type		: BOOL 
// Argument         : BYTE by
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
// Description	    : Opens the port specified by strPortName
// Return type		: BOOL 
// Argument         : CString strPortName
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

	if(hComm == INVALID_HANDLE_VALUE)
		return FALSE;
	ConfigurePort( baud, 8, FALSE, NOPARITY, ONESTOPBIT);
	return TRUE;
}


///////////////////////////////////////////////////////
// Function name	: Cserial::ClosePort
// Description	    : Closes the Port
// Return type		: void 
///////////////////////////////////////////////////////
void Cserial::ClosePort()
{
	if (hComm) {
		bPortReady = SetCommTimeouts (hComm, &CommTimeoutsSaved);
		CloseHandle(hComm);
	}
	return;
}


///////////////////////////////////////////////////////
// Function name	: Cserial::GetBytesRead
// Description	    : 
// Return type		: DWORD 
///////////////////////////////////////////////////////
DWORD Cserial::GetBytesRead()
{
	return nBytesRead;
}

///////////////////////////////////////////////////////
// Function name	: Cserial::GetBytesWritten
// Description	    : returns total number of bytes written to port
// Return type		: DWORD 
///////////////////////////////////////////////////////
DWORD Cserial::GetBytesWritten()
{
	return nBytesWritten;
}


///////////////////////////////////////////////////////
// Function name	: Cserial::ReadByte
// Description	    : Reads a byte from the selected port
// Return type		: BOOL 
// Argument         : BYTE& by
///////////////////////////////////////////////////////
BOOL Cserial::ReadByte(UCHAR& by)
{
static	BYTE byResByte[1024];
static	DWORD dwBytesTxD=0;

	if (!hComm) return FALSE;
	if (ReadFile (hComm, &byResByte[0], 1, &dwBytesTxD, 0))	{
		if (dwBytesTxD == 1) {
			by = (UCHAR)byResByte[0];
			return TRUE;
		}
	}
	by = 0;
	return FALSE;
}

int  Cserial::ReadData (unsigned char *buf, int nchars)
{
	DWORD dwRead = 0;
	if (!ReadFile(hComm, buf, nchars, &dwRead, NULL))
		return 0;
	return (int) dwRead;
}

int Cserial::ReadChars (unsigned char *buf, int nchars, int msec)
{
	if (msec) Sleep(msec);
	return ReadData (buf, nchars);
}
	
///////////////////////////////////////////////////////
// Function name	: Cserial::WriteByte
// Description	    : Writes a Byte to teh selected port
// Return type		: BOOL 
// Argument         : BYTE by
///////////////////////////////////////////////////////
BOOL Cserial::WriteByte(UCHAR by)
{
	if (!hComm) return FALSE;
	nBytesWritten = 0;
	if (WriteFile(hComm,&by,1,&nBytesWritten,NULL)==0)
		return FALSE;
	return TRUE;
}

///////////////////////////////////////////////////////
// Function name	: Cserial::WriteBuffer
// Description	    : Writes a string to the selected port
// Return type		: BOOL 
// Argument         : BYTE by
///////////////////////////////////////////////////////
int Cserial::WriteBuffer(unsigned char *buff, int n)
{
	if (!hComm) 
		return 0;
//	busyflag = true;
	WriteFile (hComm, buff, n, &nBytesWritten, NULL);
//	busyflag = false;
	return nBytesWritten;
}


///////////////////////////////////////////////////////
// Function name	: Cserial::SetCommunicationTimeouts
// Description	    : Sets the timeout for the selected port
// Return type		: BOOL 
// Argument         : DWORD ReadIntervalTimeout
// Argument         : DWORD ReadTotalTimeoutMultiplier
// Argument         : DWORD ReadTotalTimeoutConstant
// Argument         : DWORD WriteTotalTimeoutMultiplier
// Argument         : DWORD WriteTotalTimeoutConstant
///////////////////////////////////////////////////////
BOOL Cserial::SetCommunicationTimeouts(
	DWORD ReadIntervalTimeout, // msec
	DWORD ReadTotalTimeoutMultiplier,
	DWORD ReadTotalTimeoutConstant,
	DWORD WriteTotalTimeoutMultiplier,
	DWORD WriteTotalTimeoutConstant
)
{
	if((bPortReady = GetCommTimeouts (hComm, &CommTimeoutsSaved))==0)	{
		return FALSE;
	}

	CommTimeouts.ReadIntervalTimeout = ReadIntervalTimeout;
	CommTimeouts.ReadTotalTimeoutMultiplier = ReadTotalTimeoutMultiplier;
	CommTimeouts.ReadTotalTimeoutConstant = ReadTotalTimeoutConstant;
	CommTimeouts.WriteTotalTimeoutConstant = WriteTotalTimeoutConstant;
	CommTimeouts.WriteTotalTimeoutMultiplier = WriteTotalTimeoutMultiplier;
	
	bPortReady = SetCommTimeouts (hComm, &CommTimeouts);
	
	if(bPortReady ==0) { 
		CloseHandle(hComm);
		return FALSE;
	}
		
	return TRUE;
}

BOOL Cserial::SetCommTimeout() {
//	return SetCommunicationTimeouts (500L, 0L, 0L, 0L, 0L);
  return SetCommunicationTimeouts ( MAXDWORD, 0L, 0L, 100L, 0L);
  }

///////////////////////////////////////////////////////
// Function name	: ConfigurePort
// Description	    : Configures the Port
// Return type		: BOOL 
// Argument         : DWORD BaudRate
// Argument         : BYTE ByteSize
// Argument         : DWORD fParity
// Argument         : BYTE  Parity
// Argument         : BYTE StopBits
///////////////////////////////////////////////////////
BOOL Cserial::ConfigurePort(DWORD	BaudRate, 
								BYTE	ByteSize, 
								DWORD	dwParity, 
								BYTE	Parity, 
								BYTE	StopBits)
{
	if((bPortReady = GetCommState(hComm, &dcb))==0) {
		LOG_ERROR("GetCommState Error on %s", device.c_str());
		CloseHandle(hComm);
		return FALSE;
	}

	dcb.BaudRate		    =	BaudRate;
	dcb.ByteSize		    =	ByteSize;
	dcb.Parity		      =	Parity ;
	dcb.StopBits		    =	StopBits;
	dcb.fBinary		      =	TRUE;
	dcb.fDsrSensitivity =	FALSE;
	dcb.fParity		      =	dwParity;
	dcb.fOutX			      =	FALSE;
	dcb.fInX			      =	FALSE;
	dcb.fNull			      =	FALSE;
	dcb.fAbortOnError	  =	TRUE;
	dcb.fOutxCtsFlow	  =	FALSE;
	dcb.fOutxDsrFlow	  =	FALSE;
	
	if (dtr)
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
	else
		dcb.fDtrControl	    =	DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity =	FALSE;
	if (rts)
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
	else
		dcb.fRtsControl	    =	RTS_CONTROL_DISABLE;
	
	bPortReady = SetCommState(hComm, &dcb);
	if(bPortReady == 0) {
		CloseHandle(hComm);
		return FALSE;
	}
  return SetCommTimeout();
}

///////////////////////////////////////////////////////
// Function name    : Cserial::setPTT
// Return type      : void
///////////////////////////////////////////////////////
void Cserial::SetPTT(bool b)
{
	if(hComm == INVALID_HANDLE_VALUE)
		return;
	if ( !(dtrptt || rtsptt) )
		return;
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
