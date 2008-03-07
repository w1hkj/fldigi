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
	bool	changed;
	double 	squelch;
	double	wfRefLevel;
	double	wfAmpSpan;
	int		LowFreqCutoff;
	double	CWsweetspot;
	double	RTTYsweetspot;
	double	PSKsweetspot;
	bool	StartAtSweetSpot;
// for PSK  & PSK mail interface
	bool	PSKmailSweetSpot;
	int		SearchRange;
	int		ServerOffset;
	double	ACQsn;
// RTTY
	double		rtty_squelch;
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
	bool		afconoff;
	bool		sqlonoff;
	double		sldrSquelchValue;
	bool		useFSKkeyline;		// use RTS for FSK
	bool		useFSKkeylineDTR;	// use DTR for FSK
	bool		FSKisLSB;
	bool		RTTY_USB;
	bool		useUART;
	bool		PreferXhairScope;
	bool		PseudoFSK;
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
	bool		FELD_IDLE;
// OLIVIA
	int			oliviatones;
	int			oliviabw;
	int			oliviasmargin;
	int			oliviasinteg;
	bool			olivia8bit;
// DOMINOEX
	double		DOMINOEX_BW;
// MT63
	bool 		mt63_8bit;
	int			mt63_interleave;
// User interface data
//	int		Fontnbr;
//	int		FontSize;
//	int		FontColor;
	uchar	red;
	uchar	green;
	uchar	blue;
	bool	MultiColorWF;
	int		wfPreFilter;
	bool	WFaveraging;
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
	bool	macrotextid;
	int		QRZ;
	string	QRZusername;
	string	QRZuserpassword;
// Rig Interface data
	bool	btnusb;
	int		btnPTTis;
	int		btnRTSDTRis; // obsolete
	int		btnPTTREVis; // obsolete
	bool	RTSptt;
	bool	DTRptt;
	bool	RTSplus;
	bool	DTRplus;
	int		choiceHAMLIBis;
	int		chkUSEMEMMAPis;
	int		chkUSEHAMLIBis;
	int		chkUSERIGCATis;
	string  HamRigName;
	string  HamRigDevice;
	int		HamRigBaudrate;
	string	CWFSKport;
// Operator data
	string	myCall;
	string	myQth;
	string	myName;
	string	myLocator;
	string  PTTdev;
	string	secText;
// Sound card
	int	btnAudioIOis;
	string	OSSdevice;
	string	PAdevice;
	string	PortInDevice;
	string	PortOutDevice;
	string	PulseServer;
	int		sample_rate;
	int		in_sample_rate;
	int		out_sample_rate;
	string		sample_converter;
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
	
// Mixer configuration
	string	MXdevice;
	double	RcvMixer;
	double	XmtMixer;
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
	int 	RxFontnbr;
	int 	RxFontsize;
	int		RxFontcolor;
	int 	TxFontnbr;
	int 	TxFontsize;
	int		TxFontcolor;
	RGBint	RxColor;
	RGBint	TxColor;
	
	bool alt_text_widgets;
	
	string strCommPorts;
	
	int rx_msgid;
	int tx_msgid;
	
// PSK viewer parameters
	bool	VIEWERmarquee;
	bool	VIEWERshowfreq;
	int		VIEWERstart;
	int		VIEWERchannels;
	double	VIEWERsquelch;
	int		VIEWERtimeout;

public:
	void writeDefaultsXML();
	void storeDefaults();
	bool readDefaultsXML();
	void loadDefaults();
	void saveDefaults();
	int  openDefaults();
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
