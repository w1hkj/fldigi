#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#include <iostream>
#include <fstream>
#include <string>
#include <list>

#include "main.h"
#include "rtty.h"

using namespace std;

struct configuration {
	bool	rsidWideSearch;
	bool	rsid;
	bool	TransmitRSid;
	bool	slowcpu;

	bool	changed;
	double	wfRefLevel;
	double	wfAmpSpan;
	int		LowFreqCutoff;
	double	CWsweetspot;
	double	RTTYsweetspot;
	double	PSKsweetspot;
	bool	StartAtSweetSpot;
        bool    WaterfallHistoryDefault;
	bool	WaterfallQSY;
	string	WaterfallClickText;
	int	WaterfallWheelAction;
// for PSK  & PSK mail interface
	bool	PSKmailSweetSpot;
	int		SearchRange;
	int		ServerOffset;
	double	ACQsn;
// RTTY
	int			rtty_shift;
	int			rtty_baud;
	int 		rtty_bits;
	int			rtty_parity;
	int			rtty_stop;
	bool 		rtty_reverse;
	bool		rtty_msbfirst;
	bool		rtty_crcrlf;
	bool		rtty_autocrlf;
	int			rtty_autocount;
	int			rtty_afcspeed;
	bool		useFSKkeyline;		// use RTS for FSK
	bool		useFSKkeylineDTR;	// use DTR for FSK
	bool		FSKisLSB;
//	bool		RTTY_USB;
	bool		useUART;
	bool		PreferXhairScope;
	bool		PseudoFSK;
	bool		UOSrx;
	bool		UOStx;
	bool		Xagc;
// CW
	bool		useCWkeylineRTS;	// use RTS for CW
	bool		useCWkeylineDTR;	// use DTR for CW
	int			CWweight;
	int			CWspeed;
	int			defCWspeed;
	int			CWbandwidth;
	int			CWtrack;
	int			CWrange;
	int			CWlowerlimit;
	int			CWupperlimit;
	double		CWrisetime;
	double		CWdash2dot;
	bool		QSK;
	double		CWpre;
	double		CWpost;
	bool		CWid;
	int			CWIDwpm;
// FELD-HELL
	double		HELL_BW;
	bool		HellRcvWidth;
	bool		HellBlackboard;
	int			HellXmtWidth;
	bool		HellXmtIdle;
	bool		HellPulseFast;
// OLIVIA
	int			oliviatones;
	int			oliviabw;
	int			oliviasmargin;
	int			oliviasinteg;
	bool		olivia8bit;
// THOR
	double		THOR_BW;
	bool		THOR_FILTER;
	string		THORsecText;
	int			THOR_PATHS;
	bool		THOR_SOFT;
	double		ThorCWI;
// DOMINOEX
	double		DOMINOEX_BW;
	bool		DOMINOEX_FILTER;
	bool		DOMINOEX_FEC;
	int			DOMINOEX_PATHS;
	double		DomCWI;
// MT63
	bool 		mt63_8bit;
	int			mt63_interleave;
// User interface data
	uchar	red;
	uchar	green;
	uchar	blue;
	bool	MultiColorWF;
	int		wfPreFilter;
	bool	WFaveraging;
	int		latency;
	bool	UseCursorLines;
	bool	UseCursorCenterLine;
	bool	UseBWTracks;
	RGBI	cursorLineRGBI;
	RGBI	cursorCenterRGBI;
	RGBI	bwTrackRGBI;
	int		feldfontnbr;
	bool	viewXmtSignal;
	bool	sendid;
	bool	macroid;
	bool	sendtextid;
	string	strTextid;
	bool	macroCWid;
	int		videowidth;
	bool	ID_SMALL;
	bool	macrotextid;
	int		QRZ;
	string  QRZpathname;
	string	QRZusername;
	string	QRZuserpassword;
	bool	QRZchanged;
// Rig Interface data
	bool	btnusb;
	int		btnPTTis;
	bool	RTSptt;
	bool	DTRptt;
	bool	RTSplus;
	bool	DTRplus;
	int		choiceHAMLIBis;
	int		chkUSEMEMMAPis;
	int		chkUSEHAMLIBis;
	int		chkUSERIGCATis;
	int		chkUSEXMLRPCis;
	string  PTTdev;
	string	CWFSKport;
	string  HamRigDevice;
	string  HamRigName;
	int	HamRigBaudrate;
// Operator data
	string	myCall;
	string	myQth;
	string	myName;
	string	myLocator;
	string	secText;
// Sound card
	int	btnAudioIOis;
	string	OSSdevice;
	string	PAdevice;
	string	PortInDevice;
	int	PortInIndex;
	string	PortOutDevice;
	int	PortOutIndex;
	int	PortFramesPerBuffer;
	string	PulseServer;
	int		sample_rate;
	int		in_sample_rate;
	int		out_sample_rate;
	int		sample_converter;
	int		RX_corr;
	int		TX_corr;
	int		TxOffset;
// Contest stuff
	bool	UseLeadingZeros;
	int		ContestStart;
	int		ContestDigits;
// Macro timer constants and controls
	bool	useTimer;
	int		macronumber;
	int		timeout;
	bool	UseLastMacro;
	bool	DisplayMacroFilename;
	
// Mixer configuration
	string	MXdevice;
	bool	MicIn;
	bool	LineIn;
	bool	EnableMixer;
	double	PCMvolume;
	bool	MuteInput;
	
// waterfall palette
	RGBint	cfgpal[9];

// Button key color palette
	bool	useGroupColors;
	RGBint	btnGroup1;
	RGBint	btnGroup2;
	RGBint	btnGroup3;
	RGBint  btnFkeyTextColor;
	
// Rx / Tx fonts & palettes
	Fl_Font 	RxFontnbr;
	int 	RxFontsize;
	Fl_Color		RxFontcolor;
	Fl_Font 	TxFontnbr;
	int 	TxFontsize;
	Fl_Color		TxFontcolor;
	RGBint	RxColor;
	RGBint	TxColor;
	Fl_Color		XMITcolor;
	Fl_Color		CTRLcolor;
	Fl_Color		SKIPcolor;
	Fl_Color		ALTRcolor;
	
	Fl_Font		WaterfallFontnbr;
	int		WaterfallFontsize;

	string		ui_scheme;

	int rx_msgid;
	int tx_msgid;
	string arq_address;
	string arq_port;
	
// PSK viewer parameters
	bool	VIEWERmarquee;
	bool	VIEWERshowfreq;
	int		VIEWERstart;
	int		VIEWERchannels;
	double	VIEWERsquelch;
	int		VIEWERtimeout;

// XMLRPC parameters
	string xmlrpc_address;
	string xmlrpc_port;
	string xmlrpc_allow;
	string xmlrpc_deny;

public:
	void writeDefaultsXML();
	void storeDefaults();
	bool readDefaultsXML();
	void loadDefaults();
	void saveDefaults();
	int  setDefaults();
	void initOperator();
	void initInterface();
	void initMixerDevices();
	void testCommPorts();
	
	void getRigs();
	string strBaudRate();
	
	friend std::istream &operator>>(std::istream &stream, configuration &c);
	friend std::ostream &operator<<(std::ostream &ostream, configuration c);
};

extern configuration progdefaults;

extern void mixerInputs();
extern void enableMixer(bool);

enum { SAMPLE_RATE_UNSET = -1, SAMPLE_RATE_AUTO, SAMPLE_RATE_NATIVE, SAMPLE_RATE_OTHER };

#endif
