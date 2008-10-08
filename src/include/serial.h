// SerialComm.h: interface for the Cserial class.
//
//////////////////////////////////////////////////////////////////////

#ifndef SERIALCOMMH
#define SERIALCOMMH

#include <termios.h>
#include <string>

void adjust_port(std::string& port);

class Cserial  {
public:
	Cserial();
	~Cserial();

//Methods
	bool OpenPort();
	
	bool IsOpen() { return fd < 0 ? 0 : 1; };
	void ClosePort();

	void Device (std::string dev) { device = dev;};
	std::string Device() { return device;};
	
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
	std::string	device;
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
