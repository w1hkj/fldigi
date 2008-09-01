//-----------------------------------------------------------------------------
//
// Serial i/o class
//
// copyright Dave Freese 2006, w1hkj@w1hkj.com
//
//-----------------------------------------------------------------------------

#include <config.h>
#include <string>
#include <sstream>

#include "serial.h"
#include "re.h"
#include "debug.h"

#ifdef __CYGWIN__
// convert COMx to /dev/ttySy with y = x - 1
static void adjust_port(string& port)
{
	re_t re("com([0-9]+)", REG_EXTENDED | REG_ICASE);
	const char* s;
	if (!(re.match(port.c_str()) && (s = re.submatch(1))))
		return;
	stringstream ss;
	int n;
	ss << s;
	ss >> n;
	if (--n < 0)
		n = 0;
	ss.clear(); ss.str("");
	ss << "/dev/ttyS" << n;
	ss.seekp(0);
	port = ss.str();
}
#endif


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
	adjust_port(device);
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
	if ( !(dtrptt || rtsptt) ) {
		LOG_DEBUG("No h/w PTT");
		return;
	}

	ioctl(fd, TIOCMGET, &status);
	LOG_DEBUG("Status %X", status);
	if (b == true) {				// ptt enabled
		if (dtrptt && dtr)
			status &= ~TIOCM_DTR;	// toggle low
		if (dtrptt && !dtr)
			status |= TIOCM_DTR;	// toggle high
		if (rtscts == false) {
			if (rtsptt && rts)
				status &= ~TIOCM_RTS;	// toggle low
			if (rtsptt && !rts)
				status |= TIOCM_RTS;	// toggle high
		}
	} else {						// ptt disabled
		if (dtrptt && dtr)
			status |= TIOCM_DTR;	// toggle high
		if (dtrptt && !dtr)
			status &= ~TIOCM_DTR;	// toggle low
		if (rtscts == false) {
			if (rtsptt && rts)
				status |= TIOCM_RTS;	// toggle high
			if (rtsptt && !rts)
				status &= ~TIOCM_RTS;	// toggle low
		}
	}
	LOG_DEBUG("Status %X", status);
	ioctl(fd, TIOCMSET, &status);
}

///////////////////////////////////////////////////////
// Function name	: Cserial::ClosePort
// Description	    : Closes the Port
// Return type		: void 
///////////////////////////////////////////////////////
void Cserial::ClosePort()
{
	if (fd < 0) return;
	LOG_DEBUG("Serial port closed");
	ioctl(fd, TIOCMSET, &origstatus);
	tcsetattr (fd, TCSANOW, &oldtio);
	close(fd);
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
	if (fd < 0) 
		return 0;
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

