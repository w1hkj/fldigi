// SerialComm.cpp: implementation of the Cserial class.
//
//////////////////////////////////////////////////////////////////////

#include "winserial.h"

///////////////////////////////////////////////////////
// Function name	: Cserial::OpenPort
// Description	    : Opens the port specified by strPortName
// Return type		: BOOL 
// Argument         : CString strPortName
///////////////////////////////////////////////////////
BOOL Cserial::OpenPort()
{
	string COMportname = "//./";

	if (device == "/dev/ttyS0") device = "COM1";
	else if (device == "/dev/ttyS1") device = "COM2";
	else if (device == "/dev/ttyS2") device = "COM3";
	else if (device == "/dev/ttyS3") device = "COM4";
	else if (device == "/dev/ttyS4") device = "COM5";
	else if (device == "/dev/ttyS5") device = "COM6";
	else if (device == "/dev/ttyS6") device = "COM7";
	else if (device == "/dev/ttyS7") device = "COM8";
	else if (device == "/dev/ttyS8") device = "COM9";
	else if (device == "/dev/ttyS9") device = "COM10";
	else if (device == "/dev/ttyS10") device = "COM11";
	else if (device == "/dev/ttyS11") device = "COM12";
	else if (device == "/dev/ttyS12") device = "COM13";
	else if (device == "/dev/ttyS13") device = "COM14";
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
//	int retval;
//	busyflag = true;
//	retval = ReadFile(hComm, buf, nchars, &dwRead, NULL);
//	busyflag = false;
//	if (retval == 0) 
//		return 0;
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
	DWORD ReadIntervalTimeout,
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
	return SetCommunicationTimeouts (0L, 0L, 0L, 0L, 0L);
//  return SetCommunicationTimeouts ( MAXDWORD, MAXDWORD, 100L, MAXDWORD, 100L);
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
//		fl_message("GetCommState Error on %s", szPortName);
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
