// SerialComm.h: interface for the Cserial class.
//
//////////////////////////////////////////////////////////////////////

#ifndef SERIALCOMMH
#define SERIALCOMMH

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdio.h>
#include <sys/time.h>

#include <memory>
#include <string>

using namespace std;

class Cserial  {
public:
	Cserial();
	~Cserial();

//Methods
	bool OpenPort();
	
	bool IsOpen() { return fd < 0 ? 0 : 1; };
	void ClosePort();

	void Device (string dev) { device = dev;};
	string Device() { return device;};
	
	void Baud(int b) { baud = b;};
	int  Baud() { return baud;};
	
	void Timeout(int tm) { timeout = tm;}
	int  Timeout() { return timeout; }
	
	void Retries(int r) { retries = r;}
	int  Retries() { return retries;}
	
	void RTS(bool r){rts = r;}
	bool RTS(){return rts;}
	
	void RTSptt(bool b){rtsptt = b;}
	bool RTSptt(){return rtsptt;}
	
	void DTR(bool d){dtr = d;}
	bool DTR(){return dtr;}
	
	void DTRptt(bool b){dtrptt = b;}
	bool DTRptt(){return dtrptt;}
	
	void RTSCTS(bool b){rtscts = b;}
	bool RTSCTS(){return rtscts;}
	void SetPTT(bool b);
	
	int  ReadBuffer (unsigned char *b, int nbr);
	int  WriteBuffer(unsigned char *str, int nbr);
	void FlushBuffer();
	
	
private:
//Members
	string	device;
	int		fd;
	int		baud;
	int		speed;
	struct	termios oldtio, newtio;
	int		timeout;
	int		retries;
	int		status, origstatus;
	bool	dtr;
	bool	dtrptt;
	bool	rts;
	bool	rtsptt;
	bool	rtscts;
	char	bfr[2048];
//Methods
	bool	IOselect();
};

#endif
