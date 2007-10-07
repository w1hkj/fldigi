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
// for PSK mail interface
	bool	PSKmailSweetSpot;
	int		SearchRange;
	int		ServerOffset;
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

// FELD-HELL
	bool		FELD_IDLE;
// OLIVIA
	int			oliviatones;
	int			oliviabw;
// DOMINOEX
	double		DOMINOEX_BW;
// User interface data
	int		Fontnbr;
	int		FontSize;
	int		FontColor;
	uchar	red;
	uchar	green;
	uchar	blue;
	bool	MultiColorWF;
	int		wfPreFilter;
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
	string	SCdevice;
	string	OSSdevice;
	string	PAdevice;
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
	
	bool alt_text_widgets;
	
	string strCommPorts;
	
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

#endif
