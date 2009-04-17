// SerialComm.h: interface for the Cserial class.
//
//////////////////////////////////////////////////////////////////////

#ifndef SERIAL_H
#define SERIAL_H

#include <string>
void com_to_tty(std::string& port);
void tty_to_com(std::string& port);

#ifndef __MINGW32__

#include <termios.h>

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

#else //__MINGW32__

#include <windows.h>

class Cserial  {
public:
	Cserial() {
		rts = dtr = false;
		rtsptt = dtrptt = false;
		rtscts = false;
		baud = CBR_9600;
	};
	Cserial( std::string portname) {
		device = portname;
		Cserial();
//		OpenPort();
	};
	virtual	~Cserial() {};

//Methods
	BOOL OpenPort();
	void ClosePort();
	BOOL ConfigurePort(DWORD BaudRate,BYTE ByteSize,DWORD fParity,BYTE  Parity,BYTE StopBits);

	bool IsBusy() { return busyflag; };
	void IsBusy(bool val) { busyflag = val; };

	BOOL ReadByte(unsigned char &resp);
	int  ReadData (unsigned char *b, int nbr);
	int  ReadBuffer (unsigned char *b, int nbr) {
	  return ReadData (b,nbr);
	}
	int  ReadChars (unsigned char *b, int nbr, int msec);
	DWORD GetBytesRead();

	BOOL WriteByte(unsigned char bybyte);
	DWORD GetBytesWritten();
	int WriteBuffer(unsigned char *str, int nbr);

	BOOL SetCommunicationTimeouts(DWORD ReadIntervalTimeout,DWORD ReadTotalTimeoutMultiplier,DWORD ReadTotalTimeoutConstant,DWORD WriteTotalTimeoutMultiplier,DWORD WriteTotalTimeoutConstant);
	BOOL SetCommTimeout();

	void Timeout(int tm) { timeout = tm; return; };
	int  Timeout() { return timeout; };
	void FlushBuffer() {};

	void Device (std::string dev) { device = dev;};
	std::string Device() { return device;};
	
	void Baud(int b) { baud = b;};
	int  Baud() { return baud;};
	
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

//Members
private:
	std::string device;
	//For use by CreateFile
	HANDLE			hComm;

	//DCB Defined in WinBase.h
	DCB				dcb;
	COMMTIMEOUTS	CommTimeoutsSaved;
	COMMTIMEOUTS	CommTimeouts;

	//Is the Port Ready?
	BOOL			bPortReady;
	
	//Number of Bytes Written to port
	DWORD			nBytesWritten;

	//Number of Bytes Read from port
	DWORD			nBytesRead;

	//Number of bytes Transmitted in the cur session
	DWORD			nBytesTxD;

	int  timeout;
	bool busyflag;

	int		baud;
	int		retries;
	
	bool	dtr;
	bool	dtrptt;
	bool	rts;
	bool	rtsptt;
	bool	rtscts;
	
};

#endif // __MINGW32__

#endif // SERIAL_H
