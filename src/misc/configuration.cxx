#include <config.h>

#include "configuration.h"
#include "confdialog.h"
#include "xmlreader.h"
#include "soundconf.h"

#if USE_HAMLIB
	#include "hamlib.h"
	#include "rigclass.h"
#endif

#include "rigMEM.h"
//#include "rigio.h"
#include "debug.h"

#include <iostream>
#include <fstream>

configuration progdefaults = {
	false,			// bool		rsid;
	false,			// bool		rsidWideSearch;
	false,			// bool		TransmitRSid;
	true,			// bool		slowcpu;
	false,			// bool		experimental;
	
	false,			// bool		changed;
	-20.0,			// double	wfRefLevel;
	70.0,			// double	wfAmpSpan;
	300,			// int		LowFreqCutoff;
	1000,			// int		CWsweetspot;
	1000,			// int		RTTYsweetspot;
	1000,			// int		PSKsweetspot;
	false,			// bool		StartAtSweetSpot;
//  for PSK mail interface	
	false,			// bool		PSKmailSweetSpot;
	200,			// int		SearchRange;
	40,				// int		ServerOffset;
	6.0,			// double	ACQsn;
// RTTY
	3,				// int			rtty_shift; = 170
	0,				// int			rtty_baud; = 45
	0,				// int 			rtty_bits; = 5
	RTTY_PARITY_NONE,	// RTTY_PARITY	rtty_parity;
	1,				// int			rtty_stop;
	false,			// bool 		rtty_reverse;
	false,			// bool		rtty_msbfirst;
	false,  		// bool		rtty_crcrlf;
	true,			// bool		rtty_autocrlf;
	72,				// int		rtty_autocount;
	1,				// int		rtty_afcspeed;
	false,			// bool		useFSKkeyline;
	false,			// bool		useFSKkeylineDTR;	
	true,			// bool		FSKisLSB;
	false,			// bool		RTTY_USB;
	false,			// bool		useUART;
	false,			// bool		PreferXhairScope;
	false,			// bool		PseudoFSK;
	true,			// bool		UOSrx; // unshift on space - receive
	true,			// bool		UOStx; // unshift on space - transmit
	false,			// bool		Xagc; // use agc for X-scope
// CW
	false,			// bool		useCWkeylineRTS;
	false,			// bool		useCWkeylineDTR;
	50,				// int		CWweight;
	18,				// int		CWspeed;
	24,				// int		defCWspeed;
	150,			// int		CWbandwidth;
	true,			// int		CWtrack;
	10,				// int		CWrange;
	5,				// int		CWlowerlimit;
	50,				// int		CWupperlimit;
	4.0,			// double	CWrisetime;
	3.0,			// double	CWdash2dot;
	false,			// bool		QSK;
	4.0,			// double	CWpre;
	4.0,			// double	CWpost;
	false,			// bool		CWid;
	18,				// int		CWIDwpm;

// FELD-HELL
	150.0,			// double	HELL_BW;
	false,			// bool		HellRcvWidth;
	false,			// bool		HellBlackboard;
	1,				// bool		HellXmtWidth;
	true,			// bool		HellXmtIdle;
	false,			// bool		HellPulseFast;
// OLIVIA
	2,				// int		oliviatones;
	2,				// int		oliviabw;
	8,				// int		oliviasmargin
	4,				// int		oliviasinteg
	false,			// bool		olivia8bit
// THOR
	2.0,			// double	THOR_BW;
	true,			// bool		THOR_FILTER;
	"",				// string	THORsecText;
	5,				// int		THOR_PATHS;
	false,			// bool		THOR_SOFT;
	0.0,			// double	ThorCWI;
// DOMINOEX
	2.0,			// double	DOMINOEX_BW;
	true,			// bool		DOMINOEX_FILTER
	false,			// bool		DOMINOEX_FEC
	5,				// int		DOMINOEX_PATHS
	0.0,			// double	DomCWI;
// MT63
	false,			// bool 	mt63_8bit;
	32,				// int		mt63_interleave;
//
	0,				// uchar 	red
	255,			// uchar 	green
	255,			// uchar 	blue
	0,				// bool 	MultiColorWF;
	1,				// int  	wfPreFilter == Blackman
	false,			// bool		WFaveraging
	4,				// int		latency;
	true,			// bool 	UseCursorLines;
	true,			// bool 	UseCursorCenterLine;
	true,			// bool 	UseBWTracks;
	{255,255,0,255},		// RGBI	cursorLineRGBI;
	{255, 255, 255, 255},	// RGBI	cursorCenterRGBI;
	{255,0,0,255},			// RGBI	bwTrackRGBI;
	4,				// int		feldfontnbr;
	false,			// bool		viewXmtSignal;
	false,			// bool		sendid;
	false,			// bool		macroid;
	false,			// bool		sendtextid;
	"CQ",			// string	strTextid;
	false,			// bool		macroCWid;
	1,				// int		videowidth;
	true,			// bool		ID_SMALL;
	false,			// bool		macrotextid;
	0,				// int		QRZ;
	"",				// string	QRZusername;
	"",				// string	QRZuserpassword;
//
	true,			// bool		btnusb;
	0, 				// int 		btnPTTis
	false,			// bool		RTSptt;
	false,			// bool		DTRptt;
	false,			// bool		RTSplus;
	false,			// bool		DTRplus;
	0,				// int 		choiceHAMLIBis
	0,				// int 		chkUSEMEMMAPis
	0,				// int 		chkUSEHAMLIBis
	0,				// int		chkUSERIGCATis
	"",				// string	HamRigName
#ifdef __CYGWIN__
	"COM1",			// string	HamRigDevice
	1,				// int		HamRigBaudrate
	"COM2",			// string	CWFSKport
#else
	"/dev/ttyS0",	// string	HamRigDevice
	1,				// int		HamRigBaudrate
	"/dev/ttyS1",	// string	CWFSKport
#endif
//
	"",				// myCall
	"",				// myName
	"",				// myQth
	"",				// myLoc
#ifdef __CYGWIN__
	"COM1",			// PTTdev
#else
	"/dev/ttyS0",	// PTTdev
#endif
	"",				// secondary text
// Sound card
	SND_IDX_PORT,		// int		btnAudioIOis
	"",		// string	OSSdevice;
	"",		// string	PAdevice;
	"",		// string	PortIndevice;
	-1,		// int		PortInIndex;
	"",		// string	PortOutDevice;
	-1,		// int		PortOutIndex;
	0,		// int		PortFramesPerBuffer
	"",		// string	PulseServer
	SAMPLE_RATE_UNSET,		// int		sample_rate;
	SAMPLE_RATE_UNSET,		// int		in_sample_rate;
	SAMPLE_RATE_UNSET,		// int		out_sample_rate;
	SRC_SINC_FASTEST,		// int		sample_converter;
	0,				// int		RX_corr;
	0,				// int		TX_corr;
	0,				// int		TxOffset;
// Contest controls
	true,			// bool	UseLeadingZeros;
	0,				// int		ContestStart;
	4,				// int		ContestDigits;
// Macro timer constants and controls
	false,			// bool	useTimer;
	0,				// int		macronumber;
	0,				// int		timeout;
	
	"",			// string	MXdevice
	false,			// bool		MicIn;
	true,			// bool		LineIn;
	false,			// bool		EnableMixer;
	true,			// bool 	MuteInput;
	80.0,			// double	PCMvolume
	{{  0,  0,  0},{  0,  0,  62},{  0,  0,126}, // default palette
	 {  0,  0,214},{145,142,  96},{181,184, 48},
	 {223,226,105},{254,254,   4},{255, 58,  0} },
	 	
// Button key color palette
	true,			  // bool useGroupColors;
	{  80, 144, 144}, // RGBint btnGroup1;
	{ 144,  80,  80}, // RGBint btnGroup2;
	{  80,  80, 144}, // RGBint btnGroup3;
	{ 255, 255, 255}, // RGBint btnFkeyTextColor;

// Rx / Tx / Waterfall Text Widgets

	FL_SCREEN,		// int 		RxFontnbr
	16,				// int 		RxFontsize
	0,				// int		RxFontcolor
	FL_SCREEN,		// int 		TxFontnbr
	16,				// int 		TxFontsize
	0,				// int		TxFontcolor
	{ 255, 242, 190}, // RGBint RxColor;
	{ 200, 235, 255}, // RGBint TxColor;
	FL_SCREEN,		// int		WaterfallFontnbr
	12,				// int		WaterfallFontsize

	"",				// string	strCommPorts
        9876,		// int		rx_msgid
        6789,		// int		tx_msgid
	"127.0.0.1",	// string	arq_address
	"3122",		// string	arq_port
// PSK viewer parameters
	true,			// bool	VIEWERmarquee
	true,			// bool	VIEWERshowfreq
	500,			// int		VIEWERstart
	20,				// int		VIEWERchannels
	10.0,			// double	VIEWERsquelch
	15,				// int  VIEWERtimeout
	false,			// bool xmlrpc_server
	"127.0.0.1",		// string xmlrpc_address
	"7362",			// string xmlrpc_port
};

const char *szBaudRates[] = {
	"", 
	"300","600","1200","2400",
	"4800","9600","19200","38400",
	"57600","115200","230400","460800"};
	
const char *szBands[] = {
	"",
	"1830", "3580", "7030", "7070", "10138",
	"14070", "18100", "21070", "21080", "24920", "28070", "28120", 0};


// XML config file support
enum TAG { \
	IGNORE,
	MYCALL, MYNAME, MYQTH, MYLOC, 
	SQUELCH, WFREFLEVEL, WFAMPSPAN, LOWFREQCUTOFF, 
	STARTATSWEETSPOT, PSKMAILSWEETSPOT, 
	PSKSEARCHRANGE, PSKSERVEROFFSET,
	ACQSN,
	CWSWEETSPOT, PSKSWEETSPOT, RTTYSWEETSPOT,
	RTTYSHIFT, RTTYBAUD,
	RTTYBITS, RTTYPARITY, RTTYSTOP, RTTYREVERSE,
	RTTYMSBFIRST, RTTYCRCLF, RTTYAUTOCRLF,
	RTTYAUTOCOUNT, RTTYAFCSPEED,
	RTTYUSB,
	PREFERXHAIRSCOPE, 
	PSEUDOFSK,
	UOSRX, UOSTX,
	XAGC,
	CWWEIGHT, CWSPEED, CWDEFSPEED,
	CWBANDWIDTH, CWRANGE, CWLOWERLIMIT, CWUPPERLIMIT,
	CWTRACK, CWRISETIME, CWDASH2DOT,
	XQSK, CWPRE, CWPOST, CWID, CWIDWPM,
	OLIVIATONES, OLIVIABW, OLIVIASMARGIN, OLIVIASINTEG, OLIVIA8BIT,
	THORBW, THORFILTER, THORSECTEXT, THORPATHS, THORSOFT, THORCWI,
	DOMINOEXBW, DOMINOEXFILTER, DOMINOEXFEC, DOMINOEXPATHS, DOMCWI,
	FELDFONTNBR,
	HELLRCVWIDTH, HELLXMTWIDTH, HELLBLACKBOARD, HELLPULSEFAST, HELLXMTIDLE,
	WFPREFILTER, LATENCY,
	USECURSORLINES, USECURSORCENTERLINE, USEBWTRACKS,
	CLCOLORS,
	CCCOLORS,
	BWTCOLORS,
	VIEWXMTSIGNAL, SENDID, MACROID, SENDTEXTID, STRTEXTID, VIDEOWIDTH, IDSMALL,
	QRZTYPE, QRZUSER, QRZPASSWORD,
	BTNUSB, BTNPTTIS,
	RTSPTT, DTRPTT, RTSPLUS, DTRPLUS,
	CHOICEHAMLIBIS, CHKUSEMEMMAPIS,
	CHKUSEHAMLIBIS, CHKUSERIGCATIS,
	HAMRIGNAME, HAMRIGDEVICE, HAMRIGBAUDRATE,
	PTTDEV,
	SECONDARYTEXT, 
	AUDIOIO, OSSDEVICE, PADEVICE, PORTINDEVICE, PORTININDEX, PORTOUTDEVICE, PORTOUTINDEX, PULSESERVER,
	SAMPLERATE, INSAMPLERATE, OUTSAMPLERATE, SAMPLECONVERTER, RXCORR, TXCORR, TXOFFSET,
	USELEADINGZEROS, CONTESTSTART, CONTESTDIGITS,
	USETIMER, MACRONUMBER, TIMEOUT,
	MXDEVICE, 
	PCMVOLUME,
	MICIN, LINEIN, ENABLEMIXER, MUTEINPUT,
	PALETTE0, PALETTE1, PALETTE2, PALETTE3, PALETTE4, 
	PALETTE5, PALETTE6, PALETTE7, PALETTE8,
	VIEWERMARQUEE, VIEWERSHOWFREQ, VIEWERSTART, 
	VIEWERCHANNELS, VIEWERSQUELCH, VIEWERTIMEOUT, WFAVERAGING,
	USEGROUPCOLORS, FKEYGROUP1, FKEYGROUP2, FKEYGROUP3,
	FKEYTEXTCOLOR,
	RXFONTNBR, RXFONTSIZE, TXFONTNBR, TXFONTSIZE,
	RXFONTCOLOR, TXFONTCOLOR,
	WATERFALLFONTNBR, WATERFALLFONTSIZE,
	RSIDWIDESEARCH, TRANSMITRSID, SLOWCPU
};
	
void writeXMLint(ofstream &f, const char * tag,  int val)
{
	f << "<" << tag << ">" << val << "</" << tag << ">\n";
}

void writeXMLdbl(ofstream &f, const char * tag, double val)
{
	f << "<" << tag << ">" << val << "</" << tag << ">\n";
}

void writeXMLstr(ofstream &f, const char * tag, string val)
{
	f << "<" << tag << ">" << val.c_str() << "</" << tag << ">\n";
}

void writeXMLbool(ofstream &f, const char * tag, bool val)
{
	f << "<" << tag << ">" << val << "</" << tag << ">\n";
}

void writeXMLPalette(ofstream &f, int n, int r, int g, int b)
{
	f << "<PALETTE" << n << ">";
	f << r << " " << g << " " << b;
	f << "</PALETTE" << n << ">\n";
}

void writeXMLrgb(ofstream &f, const char *tag, int r, int g, int b)
{
	f << "<" << tag << ">" << r << " " << g << " " << b ;
	f << "</" << tag << ">\n";
}

void configuration::writeDefaultsXML()
{
	string deffname(HomeDir);
	deffname.append("fldigi_def.xml");

	string deffname_backup(deffname);
	deffname_backup.append("-old");
	rename(deffname.c_str(), deffname_backup.c_str());

	ofstream f(deffname.c_str(), ios::out);
	if (!f) {
		LOG_ERROR("Could not write %s", deffname.c_str());
		return;
	}

	f << "<FLDIGI_DEFS>\n";

	writeXMLstr(f, "MYCALL", myCall);
	writeXMLstr(f, "MYNAME", myName);
	writeXMLstr(f, "MYQTH", myQth);
	writeXMLstr(f, "MYLOC", myLocator);

	writeXMLdbl(f, "WFREFLEVEL", wfRefLevel);
	writeXMLdbl(f, "WFAMPSPAN", wfAmpSpan);
	writeXMLint(f, "LOWFREQCUTOFF", LowFreqCutoff);

	writeXMLbool(f, "STARTATSWEETSPOT", StartAtSweetSpot);
	writeXMLbool(f, "PSKMAILSWEETSPOT", PSKmailSweetSpot);
	writeXMLint(f, "PSKSEARCHRANGE", SearchRange);
	writeXMLint(f, "PSKSERVEROFFSET", ServerOffset);
	writeXMLdbl(f, "CWSWEETSPOT", CWsweetspot);
	writeXMLdbl(f, "PSKSWEETSPOT", PSKsweetspot);
	writeXMLdbl(f, "ACQSN", ACQsn);
	writeXMLdbl(f, "RTTYSWEETSPOT", RTTYsweetspot);
	writeXMLint(f, "RTTYSHIFT", rtty_shift);
	writeXMLint(f, "RTTYBAUD", rtty_baud);
	writeXMLint(f, "RTTYBITS", rtty_bits);
	writeXMLint(f, "RTTYPARITY", rtty_parity);
	writeXMLint(f, "RTTYSTOP", rtty_stop);
	writeXMLbool(f, "RTTYREVERSE", rtty_reverse);
	writeXMLbool(f, "RTTYMSBFIRST", rtty_msbfirst);
	writeXMLbool(f, "RTTYCRCLF", rtty_crcrlf);
	writeXMLbool(f, "RTTYAUTOCRLF", rtty_autocrlf);
	writeXMLint(f, "RTTYAUTOCOUNT", rtty_autocount);
	writeXMLint(f, "RTTYAFCSPEED", rtty_afcspeed);
	writeXMLbool(f, "RTTYUSB", RTTY_USB);
	writeXMLbool(f, "PREFERXHAIRSCOPE", PreferXhairScope);
	writeXMLbool(f, "PSEUDOFSK", PseudoFSK);
	writeXMLbool(f, "UOSRX", UOSrx);
	writeXMLbool(f, "UOSTX", UOStx);
	writeXMLbool(f, "XAGC", Xagc);

	writeXMLint(f, "CWWEIGHT", CWweight);	
	writeXMLint(f, "CWSPEED", CWspeed);
	writeXMLint(f, "CWDEFSPEED", defCWspeed);
	writeXMLint(f, "CWBANDWIDTH", CWbandwidth);
	writeXMLint(f, "CWRANGE", CWrange);
	writeXMLint(f, "CWLOWERLIMIT", CWlowerlimit);
	writeXMLint(f, "CWUPPERLIMIT", CWupperlimit);
	writeXMLbool(f, "CWTRACK", CWtrack);
	writeXMLdbl(f, "CWRISETIME", CWrisetime);
	writeXMLdbl(f, "CWDASH2DOT", CWdash2dot);
	writeXMLbool(f, "QSK", QSK);
	writeXMLdbl(f, "CWPRE", CWpre);
	writeXMLdbl(f, "CWPOST", CWpost);
	writeXMLbool(f, "CWID", CWid);
	writeXMLint(f, "IDWPM", CWIDwpm);
	
	writeXMLint(f, "OLIVIATONES", oliviatones);
	writeXMLint(f, "OLIVIABW", oliviabw);
	writeXMLint(f, "OLIVIASMARGIN", oliviasmargin);
	writeXMLint(f, "OLIVIASINTEG", oliviasinteg);
	writeXMLbool(f, "OLIVIA8BIT", olivia8bit);
	
	writeXMLdbl(f,  "THORBW", THOR_BW);
	writeXMLbool(f, "THORFILTER", THOR_FILTER);
	writeXMLstr(f,  "THORSECTEXT", THORsecText);		
	writeXMLint(f, "THORPATHS", THOR_PATHS);
	writeXMLbool(f, "THORSOFT", THOR_SOFT);
	writeXMLdbl(f, "THORCWI", ThorCWI);
	
	writeXMLdbl(f, "DOMINOEXBW", DOMINOEX_BW);
	writeXMLbool(f, "DOMINOEXFILTER", DOMINOEX_FILTER);
	writeXMLbool(f, "DOMINOEXFEC", DOMINOEX_FEC);
	writeXMLint(f, "DOMINOEXPATHS", DOMINOEX_PATHS);
	writeXMLdbl(f, "DOMCWI", DomCWI);
	
	writeXMLint(f, "FELDFONTNBR", feldfontnbr);
	writeXMLbool(f, "HELLRCVWIDTH", HellRcvWidth);
	writeXMLint(f, "HELLXMTWIDTH", HellXmtWidth);
	writeXMLbool(f, "HELLBLACKBOARD", HellBlackboard);
	writeXMLbool(f, "HELLPULSEFAST", HellPulseFast);
	writeXMLbool(f, "HELLXMTIDLE", HellXmtIdle);

	writeXMLint(f, "WFPREFILTER", wfPreFilter);
	writeXMLint(f, "LATENCY", latency);
	writeXMLbool(f, "USECURSORLINES", UseCursorLines);
	writeXMLbool(f, "USECURSORCENTERLINE", UseCursorCenterLine);
	writeXMLbool(f, "USEBWTRACKS", UseBWTracks);
	writeXMLrgb(f, "CLCOLORS", 
		cursorLineRGBI.R,
		cursorLineRGBI.G,
		cursorLineRGBI.B);	
	writeXMLrgb(f, "CCCOLORS", 
		cursorCenterRGBI.R,
		cursorCenterRGBI.G,
		cursorCenterRGBI.B);
	writeXMLrgb(f, "BWTCOLORS",
		bwTrackRGBI.R,
		bwTrackRGBI.G,
		bwTrackRGBI.B);	
	writeXMLbool(f, "VIEWXMTSIGNAL", viewXmtSignal);
	writeXMLbool(f, "SENDID", sendid);
	writeXMLbool(f, "MACROID", macroid);
	writeXMLbool(f, "SENDTEXTID", sendtextid);
	writeXMLstr(f, "STRTEXTID", strTextid);
	writeXMLint(f, "VIDEOWIDTH", videowidth);
	writeXMLbool(f, "IDSMALL", ID_SMALL);
	writeXMLint(f, "QRZTYPE", QRZ);
	writeXMLstr(f, "QRZUSER", QRZusername);
	writeXMLstr(f, "QRZPASSWORD", QRZuserpassword);
	writeXMLbool(f, "BTNUSB", btnusb);
	writeXMLint(f, "BTNPTTIS", btnPTTis);
	writeXMLbool(f, "RTSPTT", RTSptt);
	writeXMLbool(f, "DTRPTT", DTRptt);
	writeXMLbool(f, "RTSPLUS", RTSplus);
	writeXMLbool(f, "DTRPLUS", DTRplus);
	writeXMLint(f, "CHOICEHAMLIBIS", choiceHAMLIBis);	
	writeXMLint(f, "CHKUSEMEMMAPIS", chkUSEMEMMAPis);
	writeXMLint(f, "CHKUSEHAMLIBIS", chkUSEHAMLIBis);
	writeXMLint(f, "CHKUSERIGCATIS", chkUSERIGCATis);
	writeXMLstr(f, "HAMRIGNAME", HamRigName);
	writeXMLstr(f, "HAMRIGDEVICE", HamRigDevice);
	writeXMLint(f, "HAMRIGBAUDRATE", HamRigBaudrate);

	writeXMLstr(f, "PTTDEV", PTTdev);
	writeXMLstr(f, "SECONDARYTEXT", secText);		
	writeXMLint(f, "AUDIOIO", btnAudioIOis);
	writeXMLstr(f, "OSSDEVICE", OSSdevice);
	writeXMLstr(f, "PADEVICE", PAdevice);
	writeXMLstr(f, "PORTINDEVICE", PortInDevice);
	writeXMLint(f, "PORTININDEX", PortInIndex);
	writeXMLstr(f, "PORTOUTDEVICE", PortOutDevice);
	writeXMLint(f, "PORTOUTINDEX", PortOutIndex);
	writeXMLstr(f, "PULSESERVER", PulseServer);
	writeXMLint(f, "SAMPLERATE", sample_rate);
	writeXMLint(f, "INSAMPLERATE", in_sample_rate);
	writeXMLint(f, "OUTSAMPLERATE", out_sample_rate);
	writeXMLint(f, "SAMPLECONVERTER", sample_converter);
	writeXMLint(f, "RXCORR", RX_corr);		
	writeXMLint(f, "TXCORR", TX_corr);
	writeXMLint(f, "TXOFFSET", TxOffset);
	writeXMLbool(f, "USELEADINGZEROS", UseLeadingZeros);
	writeXMLint(f, "CONTESTSTART", ContestStart);
	writeXMLint(f, "CONTESTDIGITS", ContestDigits);
	writeXMLbool(f, "USETIMER", useTimer);
	writeXMLint(f, "MACRONUMBER", macronumber);
	writeXMLint(f, "TIMEOUT", timeout);	
	writeXMLstr(f, "MXDEVICE", MXdevice);
	writeXMLdbl(f, "PCMVOLUME", PCMvolume);
	writeXMLbool(f, "MICIN", MicIn);
	writeXMLbool(f, "LINEIN", LineIn);
	writeXMLbool(f, "ENABLEMIXER", EnableMixer);
	writeXMLbool(f, "MUTEINPUT", MuteInput);
	for (int i = 0; i < 9; i++)
		writeXMLPalette(f, i, cfgpal[i].R, cfgpal[i].G, cfgpal[i].B);

	writeXMLbool(f, "VIEWERMARQUEE", VIEWERmarquee);
	writeXMLbool(f, "VIEWERSHOWFREQ", VIEWERshowfreq);
	writeXMLint(f, "VIEWERSTART", VIEWERstart);
	writeXMLint(f, "VIEWERCHANNELS", VIEWERchannels);
	writeXMLdbl(f, "VIEWERSQUELCH", VIEWERsquelch);
	writeXMLint(f, "VIEWERTIMEOUT", VIEWERtimeout);
	writeXMLbool(f,"WFAVERAGEING", WFaveraging);

	writeXMLbool(f,"USEGROUPCOLORS", useGroupColors);
	writeXMLrgb(f, "FKEYGROUP1", btnGroup1.R, btnGroup1.G, btnGroup1.B);
	writeXMLrgb(f, "FKEYGROUP2", btnGroup2.R, btnGroup2.G, btnGroup2.B);
	writeXMLrgb(f, "FKEYGROUP3", btnGroup3.R, btnGroup3.G, btnGroup3.B);
	writeXMLrgb(f, "FKEYTEXTCOLOR", 
		btnFkeyTextColor.R, btnFkeyTextColor.G, btnFkeyTextColor.B);
	
	writeXMLint(f, "RXFONTNBR", RxFontnbr);
	writeXMLint(f, "RXFONTSIZE", RxFontsize);
	writeXMLint(f, "TXFONTNBR", TxFontnbr);
	writeXMLint(f, "TXFONTSIZE", TxFontsize);
	writeXMLrgb(f, "RXFONTCOLOR", RxColor.R, RxColor.G, RxColor.B);
	writeXMLrgb(f, "TXFONTCOLOR", TxColor.R, TxColor.G, TxColor.B);
	writeXMLint(f, "WATERFALLFONTNBR", WaterfallFontnbr);
	writeXMLint(f, "WATERFALLFONTSIZE", WaterfallFontsize);
	
	writeXMLbool(f, "RSIDWIDESEARCH", rsidWideSearch);
	writeXMLbool(f, "TRANSMITRSID", TransmitRSid);
	writeXMLbool(f, "SLOWCPU", slowcpu);
	
	f << "</FLDIGI_DEFS>\n";
	f.close();
}
	
bool configuration::readDefaultsXML()
{
	string deffname = HomeDir;
	deffname.append("fldigi_def.xml");
	ifstream f_in(deffname.c_str(), ios::in);

	if (!f_in) return false;
	string xmlpage;
	char str[255];
	while (f_in) {
		f_in.getline(str, 255);
		xmlpage += str;
		xmlpage += '\n';
	}
	f_in.close();
		
	IrrXMLReader* xml = createIrrXMLReader(new IIrrXMLStringReader(xmlpage));

// strings for storing the data we want to get out of the file
	TAG tag = IGNORE;
	
// parse the file until end reached
	while(xml && xml->read()) {
		switch(xml->getNodeType()) {
			case EXN_TEXT:
			case EXN_CDATA:
				switch (tag) {
					default:
					case IGNORE:
						break;
					case MYCALL :
						myCall = xml->getNodeData();
						break;
					case MYNAME:
						myName = xml->getNodeData();
						break;
					case MYQTH:
						myQth = xml->getNodeData();
						break;
					case MYLOC:
						myLocator = xml->getNodeData();
						break;
					case WFREFLEVEL:
						wfRefLevel = atof(xml->getNodeData());
						break;
					case WFAMPSPAN :
						wfAmpSpan = atof(xml->getNodeData());
						break;
					case LOWFREQCUTOFF :
						LowFreqCutoff = atoi(xml->getNodeData());
						break;
					case STARTATSWEETSPOT :
						StartAtSweetSpot = atoi(xml->getNodeData());
						break;
					case PSKMAILSWEETSPOT :
						PSKmailSweetSpot = atoi(xml->getNodeData());
						break;
					case PSKSEARCHRANGE :
						SearchRange = atoi(xml->getNodeData());
						break;
					case PSKSERVEROFFSET :
						ServerOffset = atoi(xml->getNodeData());
						break;
					case ACQSN :
						ACQsn = atof(xml->getNodeData());
						break;
					case CWSWEETSPOT :
						CWsweetspot = atof(xml->getNodeData());
						break;
					case PSKSWEETSPOT :
						PSKsweetspot = atof(xml->getNodeData());
						break;
					case RTTYSWEETSPOT :
						RTTYsweetspot = atof(xml->getNodeData());
						break;
					case RTTYSHIFT :
						rtty_shift = atoi(xml->getNodeData());
						break;
					case RTTYBAUD :
						rtty_baud = atoi(xml->getNodeData());
						break;
					case RTTYBITS :
						rtty_bits = atoi(xml->getNodeData());
						break;
					case RTTYPARITY :
						rtty_parity = atoi(xml->getNodeData());
						break;
					case RTTYSTOP :
						rtty_stop = atoi(xml->getNodeData());
						break;
					case RTTYREVERSE :
						rtty_reverse = atoi(xml->getNodeData());
						break;
					case RTTYMSBFIRST :
						rtty_msbfirst = atoi(xml->getNodeData());
						break;
					case RTTYCRCLF :
						rtty_crcrlf = atoi(xml->getNodeData());
						break;
					case RTTYAUTOCRLF :
						rtty_autocrlf = atoi(xml->getNodeData());
						break;
					case RTTYAUTOCOUNT :
						rtty_autocount = atoi(xml->getNodeData());
						break;
					case RTTYAFCSPEED :
						rtty_afcspeed = atoi(xml->getNodeData());
						break;
					case RTTYUSB :
						RTTY_USB = atoi(xml->getNodeData());
						break;
					case PREFERXHAIRSCOPE :
						PreferXhairScope = atoi(xml->getNodeData());
						break;
					case PSEUDOFSK :
						PseudoFSK = atoi(xml->getNodeData());
						break;
					case UOSRX :
						UOSrx = atoi(xml->getNodeData());
						break;
					case UOSTX :
						UOStx = atoi(xml->getNodeData());
						break;
					case XAGC :
						Xagc = atoi(xml->getNodeData());
						break;
					case CWWEIGHT :
						CWweight = atoi(xml->getNodeData());
						break;
					case CWSPEED :
						CWspeed = atoi(xml->getNodeData());
						break;
					case CWDEFSPEED :
						defCWspeed = atoi(xml->getNodeData());
						break;
					case CWBANDWIDTH :
						CWbandwidth = atoi(xml->getNodeData());
						break;
					case CWRANGE :
						CWrange = atoi(xml->getNodeData());
						break;
					case CWLOWERLIMIT :
						CWlowerlimit = atoi(xml->getNodeData());
						break;
					case CWUPPERLIMIT :
						CWupperlimit = atoi(xml->getNodeData());
						break;
					case CWTRACK :
						CWtrack = atoi(xml->getNodeData());
						break;
					case CWRISETIME :
						CWrisetime = atof(xml->getNodeData());
						break;
					case CWDASH2DOT :
						CWdash2dot = atof(xml->getNodeData());
						break;
					case XQSK :
						QSK = atoi(xml->getNodeData());
						break;
					case CWPRE :
						CWpre = atof(xml->getNodeData());
						break;
					case CWPOST :
						CWpost = atof(xml->getNodeData());
						break;
					case CWID :
						CWid = atoi(xml->getNodeData());
						break;
					case CWIDWPM :
						CWIDwpm = atoi(xml->getNodeData());
						break;
					case OLIVIATONES :
						oliviatones = atoi(xml->getNodeData());
						break;
					case OLIVIABW :
						oliviabw = atoi(xml->getNodeData());
						break;
					case OLIVIASMARGIN :
						oliviasmargin = atoi(xml->getNodeData());
						break;
					case OLIVIASINTEG :
						oliviasinteg = atoi(xml->getNodeData());
						break;
					case OLIVIA8BIT :
						olivia8bit = atoi(xml->getNodeData());
						break;
					case THORBW :
						THOR_BW = atof(xml->getNodeData());
						break;
					case THORFILTER :
						THOR_FILTER = atoi(xml->getNodeData());
						break;
					case THORSECTEXT :
						THORsecText = xml->getNodeData();
						break;
					case THORPATHS :
						THOR_PATHS = atoi(xml->getNodeData());
						break;
					case THORSOFT :
						THOR_SOFT = atoi(xml->getNodeData());
						break;
					case THORCWI :
						ThorCWI = atof(xml->getNodeData());
						break;
					case DOMINOEXBW :
						DOMINOEX_BW = atof(xml->getNodeData());
						break;
					case DOMINOEXFILTER :
						DOMINOEX_FILTER = atoi(xml->getNodeData());
						break;
					case DOMINOEXFEC :
						DOMINOEX_FEC = atoi(xml->getNodeData());
						break;
					case DOMINOEXPATHS :
						DOMINOEX_PATHS = atoi(xml->getNodeData());
						break;
					case DOMCWI :
						DomCWI = atof(xml->getNodeData());
						break;
					case FELDFONTNBR :
						feldfontnbr = atoi(xml->getNodeData());
						break;
					case HELLRCVWIDTH :
						HellRcvWidth = atoi(xml->getNodeData());
						break;
					case HELLXMTWIDTH :
						HellXmtWidth = atoi(xml->getNodeData());
						if (HellXmtWidth == 0) HellXmtWidth = 1;
						break;
					case HELLBLACKBOARD :
						HellBlackboard = atoi(xml->getNodeData());
						break;
					case HELLPULSEFAST :
						HellPulseFast = atoi(xml->getNodeData());
						break;
					case HELLXMTIDLE :
						HellXmtIdle = atoi(xml->getNodeData());
						break;
					case WFPREFILTER :
						wfPreFilter = atoi(xml->getNodeData());
						break;
					case LATENCY :
						latency = atoi(xml->getNodeData());
						break;
					case USECURSORLINES :
						UseCursorLines = atoi(xml->getNodeData());
						break;
					case USECURSORCENTERLINE :
						UseCursorCenterLine = atoi(xml->getNodeData());
						break;
					case USEBWTRACKS :
						UseBWTracks = atoi(xml->getNodeData());
						break;
					case CLCOLORS :
						sscanf( xml->getNodeData(), "%hhu %hhu %hhu",
							&cursorLineRGBI.R,
							&cursorLineRGBI.G,
							&cursorLineRGBI.B );	
						break;
					case CCCOLORS :
						sscanf( xml->getNodeData(), "%hhu %hhu %hhu",
							&cursorCenterRGBI.R,
							&cursorCenterRGBI.G,
							&cursorCenterRGBI.B );	
						break;
					case BWTCOLORS :
						sscanf( xml->getNodeData(), "%hhu %hhu %hhu",
							&bwTrackRGBI.R,
							&bwTrackRGBI.G,
							&bwTrackRGBI.B );	
						break;
					case VIEWXMTSIGNAL :
						viewXmtSignal = atoi(xml->getNodeData());
						break;
					case SENDID :
						sendid = atoi(xml->getNodeData());
						break;
					case MACROID :
						macroid = atoi(xml->getNodeData());
						break;
					case SENDTEXTID :
						sendtextid = atoi(xml->getNodeData());
						break;
					case STRTEXTID :
						strTextid = xml->getNodeData();
					case VIDEOWIDTH :
						videowidth = atoi(xml->getNodeData());
					case IDSMALL :
						ID_SMALL = atoi(xml->getNodeData());
					case QRZTYPE :
						QRZ = atoi(xml->getNodeData());
						break;
					case QRZUSER :
						QRZusername = xml->getNodeData();
						break;
					case QRZPASSWORD :
						QRZuserpassword = xml->getNodeData();
						break;
					case BTNUSB :
						btnusb = atoi(xml->getNodeData());
						break;
					case BTNPTTIS :
						btnPTTis = atoi(xml->getNodeData());
						break;
					case RTSPTT :
						RTSptt = atoi(xml->getNodeData());
						break;
					case DTRPTT :
						DTRptt = atoi(xml->getNodeData());
						break;
					case RTSPLUS :
						RTSplus = atoi(xml->getNodeData());
						break;
					case DTRPLUS :
						DTRplus = atoi(xml->getNodeData());
						break;
					case CHOICEHAMLIBIS :
						choiceHAMLIBis = atoi(xml->getNodeData());
						break;
					case CHKUSEMEMMAPIS :
						chkUSEMEMMAPis = atoi(xml->getNodeData());
						break;
					case CHKUSEHAMLIBIS :
						chkUSEHAMLIBis = atoi(xml->getNodeData());
						break;
					case CHKUSERIGCATIS :
						chkUSERIGCATis = atoi(xml->getNodeData());
						break;
					case HAMRIGNAME :
						HamRigName = xml->getNodeData();
						break;
					case HAMRIGDEVICE :
						HamRigDevice = xml->getNodeData();
						break;
					case HAMRIGBAUDRATE :
						HamRigBaudrate = atoi(xml->getNodeData());
						break;
					case PTTDEV :
						PTTdev = xml->getNodeData();
						break;
					case SECONDARYTEXT :
						secText = xml->getNodeData();
						break;
					case AUDIOIO :
						btnAudioIOis = atoi(xml->getNodeData());
						break;
					case OSSDEVICE :
						OSSdevice = xml->getNodeData();
						break;
					case PADEVICE :
						PAdevice = xml->getNodeData();
						break;
					case PORTINDEVICE :
						PortInDevice = xml->getNodeData();
						break;
					case PORTININDEX :
						PortInIndex = atoi(xml->getNodeData());
						break;
					case PORTOUTDEVICE :
						PortOutDevice = xml->getNodeData();
						break;
					case PORTOUTINDEX :
						PortOutIndex = atoi(xml->getNodeData());
						break;
					case PULSESERVER :
						PulseServer = xml->getNodeData();
						break;
					case SAMPLERATE :
						sample_rate = atoi(xml->getNodeData());
						break;
					case INSAMPLERATE :
						in_sample_rate = atoi(xml->getNodeData());
						break;
					case OUTSAMPLERATE :
						out_sample_rate = atoi(xml->getNodeData());
						break;
					case SAMPLECONVERTER :
						sample_converter = atoi(xml->getNodeData());
						break;
					case RXCORR :
						RX_corr = atoi(xml->getNodeData());
						break;
					case TXCORR :
						TX_corr = atoi(xml->getNodeData());
						break;
					case TXOFFSET :
						TxOffset = atoi(xml->getNodeData());
						break;
					case USELEADINGZEROS :
						UseLeadingZeros = atoi(xml->getNodeData());
						break;
					case CONTESTSTART :
						ContestStart = atoi(xml->getNodeData());
						break;
					case CONTESTDIGITS :
						ContestDigits = atoi(xml->getNodeData());
						break;
					case USETIMER :
						useTimer = atoi(xml->getNodeData());
						break;
					case MACRONUMBER :
						macronumber = atoi(xml->getNodeData());
						break;
					case TIMEOUT :
						timeout = atoi(xml->getNodeData());
						break;
					case MXDEVICE :
						MXdevice = xml->getNodeData();
						break;
					case PCMVOLUME :
						PCMvolume = atof(xml->getNodeData());
						break;
					case MICIN :
						MicIn = atoi(xml->getNodeData());
						break;
					case LINEIN :
						LineIn = atoi(xml->getNodeData());
						break;
					case ENABLEMIXER :
						EnableMixer = atoi(xml->getNodeData());
						break;
					case MUTEINPUT :
						MuteInput = atoi(xml->getNodeData());
						break;
					case PALETTE0 :
						sscanf( xml->getNodeData(), "%d %d %d",
								&cfgpal[0].R, &cfgpal[0].G, &cfgpal[0].B );
						break;
					case PALETTE1 :
						sscanf( xml->getNodeData(), "%d %d %d",
								&cfgpal[1].R, &cfgpal[1].G, &cfgpal[1].B );
						break;
					case PALETTE2 :
						sscanf( xml->getNodeData(), "%d %d %d",
								&cfgpal[2].R, &cfgpal[2].G, &cfgpal[2].B );
						break;
					case PALETTE3 :
						sscanf( xml->getNodeData(), "%d %d %d",
								&cfgpal[3].R, &cfgpal[3].G, &cfgpal[3].B );
						break;
					case PALETTE4 :
						sscanf( xml->getNodeData(), "%d %d %d",
								&cfgpal[4].R, &cfgpal[4].G, &cfgpal[4].B );
						break;
					case PALETTE5 :
						sscanf( xml->getNodeData(), "%d %d %d",
								&cfgpal[5].R, &cfgpal[5].G, &cfgpal[5].B );
						break;
					case PALETTE6 :
						sscanf( xml->getNodeData(), "%d %d %d",
								&cfgpal[6].R, &cfgpal[6].G, &cfgpal[6].B );
						break;
					case PALETTE7 :
						sscanf( xml->getNodeData(), "%d %d %d",
								&cfgpal[7].R, &cfgpal[7].G, &cfgpal[7].B );
						break;
					case PALETTE8 :
						sscanf( xml->getNodeData(), "%d %d %d",
								&cfgpal[8].R, &cfgpal[8].G, &cfgpal[8].B );
						break;
					case VIEWERMARQUEE :
						VIEWERmarquee = atoi(xml->getNodeData());
						break;
					case VIEWERSHOWFREQ :
						VIEWERshowfreq = atoi(xml->getNodeData());
						break;
					case VIEWERSTART :
						VIEWERstart = atoi(xml->getNodeData());
						break;
					case VIEWERCHANNELS :
						VIEWERchannels = atoi(xml->getNodeData());
						break;
					case VIEWERSQUELCH :
						VIEWERsquelch = atof(xml->getNodeData());
						break;
					case VIEWERTIMEOUT :
						VIEWERtimeout = atoi(xml->getNodeData());
						break;
					case WFAVERAGING :
						WFaveraging = atoi(xml->getNodeData());
						break;
					case USEGROUPCOLORS :
						useGroupColors = atoi(xml->getNodeData());
					case FKEYGROUP1 :
						sscanf( xml->getNodeData(), "%d %d %d",
							&btnGroup1.R, &btnGroup1.G, &btnGroup1.B);
						break;
					case FKEYGROUP2 :
						sscanf( xml->getNodeData(), "%d %d %d",
							&btnGroup2.R, &btnGroup2.G, &btnGroup2.B);
						break;
					case FKEYGROUP3 :
						sscanf( xml->getNodeData(), "%d %d %d",
							&btnGroup3.R, &btnGroup3.G, &btnGroup3.B);
						break;
					case FKEYTEXTCOLOR : 
						sscanf( xml->getNodeData(), "%d %d %d",
							&btnFkeyTextColor.R, 
							&btnFkeyTextColor.G, 
							&btnFkeyTextColor.B);
						break;
					case RXFONTNBR :
						RxFontnbr = atoi(xml->getNodeData());
						break;
					case RXFONTSIZE :
						RxFontsize = atoi(xml->getNodeData());
						break;
					case TXFONTNBR :
						TxFontnbr = atoi(xml->getNodeData());
						break;
					case TXFONTSIZE :
						TxFontsize = atoi(xml->getNodeData());
						break;
					case RXFONTCOLOR :
						sscanf( xml->getNodeData(), "%d %d %d",
							&RxColor.R, &RxColor.G, &RxColor.B);
						break;
					case WATERFALLFONTNBR :
						WaterfallFontnbr = atoi(xml->getNodeData());
						break;
					case WATERFALLFONTSIZE :
						WaterfallFontsize = atoi(xml->getNodeData());
						break;
					case TXFONTCOLOR :
						sscanf( xml->getNodeData(), "%d %d %d",
							&TxColor.R, &TxColor.G, &TxColor.B);
						break;
					case RSIDWIDESEARCH :
						rsidWideSearch = atoi(xml->getNodeData());
						break;
					case TRANSMITRSID :
						TransmitRSid = atoi(xml->getNodeData());
						break;
					case SLOWCPU :
						slowcpu = atoi(xml->getNodeData());
						break;
				}
				break;
				
			case EXN_ELEMENT_END:
				tag=IGNORE;
				break;

			case EXN_ELEMENT: 
				{
				const char *nodeName = xml->getNodeName();
				if (!strcmp("MYCALL", nodeName)) 		tag = MYCALL;
				else if (!strcmp("MYNAME", nodeName)) 	tag = MYNAME;
				else if (!strcmp("MYQTH", nodeName)) 	tag = MYQTH;
				else if (!strcmp("MYLOC", nodeName)) 	tag = MYLOC;
				else if (!strcmp("SQUELCH", nodeName)) 	tag = SQUELCH;
				else if (!strcmp("WFREFLEVEL", nodeName)) 	tag = WFREFLEVEL;
				else if (!strcmp("WFAMPSPAN", nodeName)) 	tag = WFAMPSPAN;
				else if (!strcmp("LOWFREQCUTOFF", nodeName)) 	tag = LOWFREQCUTOFF;
				else if (!strcmp("STARTATSWEETSPOT", nodeName)) 	tag = STARTATSWEETSPOT;
				else if (!strcmp("PSKMAILSWEETSPOT", nodeName)) 	tag = PSKMAILSWEETSPOT;
				else if (!strcmp("PSKSEARCHRANGE", nodeName)) 	tag = PSKSEARCHRANGE;
				else if (!strcmp("PSKSERVEROFFSET", nodeName)) 	tag = PSKSERVEROFFSET;
				else if (!strcmp("ACQSN", nodeName)) tag = ACQSN;
				else if (!strcmp("CWSWEETSPOT", nodeName)) 	tag = CWSWEETSPOT;
				else if (!strcmp("PSKSWEETSPOT", nodeName)) 	tag = PSKSWEETSPOT;
				else if (!strcmp("RTTYSWEETSPOT", nodeName)) 	tag = RTTYSWEETSPOT;
				else if (!strcmp("RTTYSHIFT", nodeName)) 	tag = RTTYSHIFT;
				else if (!strcmp("RTTYBAUD", nodeName)) 	tag = RTTYBAUD;
				else if (!strcmp("RTTYBITS", nodeName)) 	tag = RTTYBITS;
				else if (!strcmp("RTTYPARITY", nodeName)) 	tag = RTTYPARITY;
				else if (!strcmp("RTTYSTOP", nodeName)) 	tag = RTTYSTOP;
				else if (!strcmp("RTTYREVERSE", nodeName)) 	tag = RTTYREVERSE;
				else if (!strcmp("RTTYMSBFIRST", nodeName)) 	tag = RTTYMSBFIRST;
				else if (!strcmp("RTTYCRCLF", nodeName)) 	tag = RTTYCRCLF;
				else if (!strcmp("RTTYAUTOCRLF", nodeName)) 	tag = RTTYAUTOCRLF;
				else if (!strcmp("RTTYAUTOCOUNT", nodeName)) 	tag = RTTYAUTOCOUNT;
				else if (!strcmp("RTTYAFCSPEED", nodeName)) 	tag = RTTYAFCSPEED;
				else if (!strcmp("RTTYUSB", nodeName))		tag = RTTYUSB;
				else if (!strcmp("PREFERXHAIRSCOPE", nodeName)) 	tag = PREFERXHAIRSCOPE;
				else if (!strcmp("PSEUDOFSK", nodeName)) 	tag = PSEUDOFSK;
				else if (!strcmp("UOSRX", nodeName)) 	tag = UOSRX;
				else if (!strcmp("UOSTX", nodeName)) 	tag = UOSTX;
				else if (!strcmp("XAGC", nodeName)) 	tag = XAGC;
				else if (!strcmp("CWWEIGHT", nodeName)) 	tag = CWWEIGHT;
				else if (!strcmp("CWSPEED", nodeName)) 	tag = CWSPEED;
				else if (!strcmp("CWDEFSPEED", nodeName)) 	tag = CWDEFSPEED;
				else if (!strcmp("CWBANDWIDTH", nodeName)) 	tag = CWBANDWIDTH;
				else if (!strcmp("CWRANGE", nodeName)) 	tag = CWRANGE;
				else if (!strcmp("CWLOWERLIMIT", nodeName)) 	tag = CWLOWERLIMIT;
				else if (!strcmp("CWUPPERLIMIT", nodeName)) 	tag = CWUPPERLIMIT;
				else if (!strcmp("CWTRACK", nodeName)) 	tag = CWTRACK;
				else if (!strcmp("CWRISETIME", nodeName)) 	tag = CWRISETIME;
				else if (!strcmp("CWDASH2DOT", nodeName)) 	tag = CWDASH2DOT;
				else if (!strcmp("QSK", nodeName)) 	tag = XQSK;
				else if (!strcmp("CWPRE", nodeName)) 	tag = CWPRE;
				else if (!strcmp("CWPOST", nodeName)) 	tag = CWPOST;
				else if (!strcmp("CWID", nodeName))	tag = CWID;
				else if (!strcmp("IDWPM", nodeName)) tag = CWIDWPM;
				else if (!strcmp("OLIVIATONES", nodeName)) 	tag = OLIVIATONES;
				else if (!strcmp("OLIVIABW", nodeName)) 	tag = OLIVIABW;
				else if (!strcmp("OLIVIASMARGIN", nodeName)) 	tag = OLIVIASMARGIN;
				else if (!strcmp("OLIVIASINTEG", nodeName)) 	tag = OLIVIASINTEG;
				else if (!strcmp("OLIVIA8BIT", nodeName)) 	tag = OLIVIA8BIT;
				else if (!strcmp("THORBW", nodeName)) 	tag = THORBW;
				else if (!strcmp("THORFILTER", nodeName))	tag = THORFILTER;
				else if (!strcmp("THORSECTEXT", nodeName))	tag = THORSECTEXT;
				else if (!strcmp("THORPATHS", nodeName)) tag = THORPATHS;
				else if (!strcmp("THORSOFT", nodeName)) tag = THORSOFT;
				else if (!strcmp("THORCWI", nodeName)) tag = THORCWI;
				else if (!strcmp("DOMINOEXBW", nodeName)) 	tag = DOMINOEXBW;
				else if (!strcmp("DOMINOEXFILTER", nodeName))	tag = DOMINOEXFILTER;
				else if (!strcmp("DOMINOEXFEC", nodeName))	tag = DOMINOEXFEC;
				else if (!strcmp("DOMINOEXPATHS", nodeName)) tag = DOMINOEXPATHS;
				else if (!strcmp("DOMCWI", nodeName)) tag = DOMCWI;
				else if (!strcmp("FELDFONTNBR", nodeName)) 	tag = FELDFONTNBR;
				else if (!strcmp("HELLRCVWIDTH", nodeName)) 	tag = HELLRCVWIDTH;
				else if (!strcmp("HELLXMTWIDTH", nodeName)) 	tag = HELLXMTWIDTH;
				else if (!strcmp("HELLBLACKBOARD", nodeName)) 	tag = HELLBLACKBOARD;
				else if (!strcmp("HELLPULSEFAST", nodeName)) 	tag = HELLPULSEFAST;
				else if (!strcmp("HELLXMTIDLE", nodeName)) 	tag = HELLXMTIDLE;
				else if (!strcmp("WFPREFILTER", nodeName)) 	tag = WFPREFILTER;
				else if (!strcmp("LATENCY", nodeName)) 	tag = LATENCY;
				else if (!strcmp("USECURSORLINES", nodeName)) 	tag = USECURSORLINES;
				else if (!strcmp("USECURSORCENTERLINE", nodeName)) 	tag = USECURSORCENTERLINE;
				else if (!strcmp("USEBWTRACKS", nodeName)) 	tag = USEBWTRACKS;
				else if (!strcmp("CLCOLORS", nodeName)) 	tag = CLCOLORS;
				else if (!strcmp("CCCOLORS", nodeName)) 	tag = CCCOLORS;
				else if (!strcmp("BWTCOLORS", nodeName)) 	tag = BWTCOLORS;
				else if (!strcmp("VIEWXMTSIGNAL", nodeName)) 	tag = VIEWXMTSIGNAL;
				else if (!strcmp("SENDID", nodeName)) 	tag = SENDID;
				else if (!strcmp("MACROID", nodeName)) 	tag = MACROID;
				else if (!strcmp("SENDTEXTID", nodeName))	tag = SENDTEXTID;
				else if (!strcmp("STRTEXTID", nodeName))	tag = STRTEXTID;
				else if (!strcmp("VIDEOWIDTH", nodeName))	tag = VIDEOWIDTH;
				else if (!strcmp("IDSMALL", nodeName))	tag = IDSMALL;
				else if (!strcmp("QRZUSER", nodeName)) 	tag = QRZUSER;
				else if (!strcmp("QRZPASSWORD", nodeName)) 	tag = QRZPASSWORD;
				else if (!strcmp("QRZTYPE", nodeName)) 	tag = QRZTYPE;
				else if (!strcmp("BTNUSB", nodeName)) 	tag = BTNUSB;
				else if (!strcmp("BTNPTTIS", nodeName)) 	tag = BTNPTTIS;
				else if (!strcmp("RTSPTT", nodeName)) 	tag = RTSPTT;
				else if (!strcmp("DTRPTT", nodeName)) 	tag = DTRPTT;
				else if (!strcmp("RTSPLUS", nodeName)) 	tag = RTSPLUS;
				else if (!strcmp("DTRPLUS", nodeName)) 	tag = DTRPLUS;
				else if (!strcmp("CHOICEHAMLIBIS", nodeName)) 	tag = CHOICEHAMLIBIS;
				else if (!strcmp("CHKUSEMEMMAPIS", nodeName)) 	tag = CHKUSEMEMMAPIS;
				else if (!strcmp("CHKUSEHAMLIBIS", nodeName)) 	tag = CHKUSEHAMLIBIS;
				else if (!strcmp("CHKUSERIGCATIS", nodeName)) 	tag = CHKUSERIGCATIS;
				else if (!strcmp("HAMRIGNAME", nodeName)) 	tag = HAMRIGNAME;
				else if (!strcmp("HAMRIGDEVICE", nodeName)) 	tag = HAMRIGDEVICE;
				else if (!strcmp("HAMRIGBAUDRATE", nodeName)) 	tag = HAMRIGBAUDRATE;
				else if (!strcmp("PTTDEV", nodeName)) 	tag = PTTDEV;
				else if (!strcmp("SECONDARYTEXT", nodeName)) 	tag = SECONDARYTEXT;
				else if (!strcmp("AUDIOIO", nodeName)) 	tag = AUDIOIO;
				else if (!strcmp("OSSDEVICE", nodeName)) 	tag = OSSDEVICE;
				else if (!strcmp("PADEVICE", nodeName)) 	tag = PADEVICE;
				else if (!strcmp("PORTINDEVICE", nodeName)) 	tag = PORTINDEVICE;
				else if (!strcmp("PORTININDEX", nodeName)) 	tag = PORTININDEX;
				else if (!strcmp("PORTOUTDEVICE", nodeName)) 	tag = PORTOUTDEVICE;
				else if (!strcmp("PORTOUTINDEX", nodeName)) 	tag = PORTOUTINDEX;
				else if (!strcmp("SAMPLERATE", nodeName)) 	tag = SAMPLERATE;
				else if (!strcmp("INSAMPLERATE", nodeName)) 	tag = INSAMPLERATE;
				else if (!strcmp("OUTSAMPLERATE", nodeName)) 	tag = OUTSAMPLERATE;
				else if (!strcmp("SAMPLECONVERTER", nodeName)) 	tag = SAMPLECONVERTER;
				else if (!strcmp("RXCORR", nodeName)) 	tag = RXCORR;
				else if (!strcmp("TXCORR", nodeName)) 	tag = TXCORR;
				else if (!strcmp("TXOFFSET", nodeName)) 	tag = TXOFFSET;
				else if (!strcmp("USELEADINGZEROS", nodeName)) 	tag = USELEADINGZEROS;
				else if (!strcmp("CONTESTSTART", nodeName)) 	tag = CONTESTSTART;
				else if (!strcmp("CONTESTDIGITS", nodeName)) 	tag = CONTESTDIGITS;
				else if (!strcmp("USETIMER", nodeName)) 	tag = USETIMER;
				else if (!strcmp("MACRONUMBER", nodeName)) 	tag = MACRONUMBER;
				else if (!strcmp("TIMEOUT", nodeName)) 	tag = TIMEOUT;
				else if (!strcmp("MXDEVICE", nodeName)) 	tag = MXDEVICE;
				else if (!strcmp("PCMVOLUME", nodeName)) 	tag = PCMVOLUME;
				else if (!strcmp("MICIN", nodeName)) 	tag = MICIN;
				else if (!strcmp("LINEIN", nodeName)) 	tag = LINEIN;
				else if (!strcmp("ENABLEMIXER", nodeName)) 	tag = ENABLEMIXER;
				else if (!strcmp("MUTEINPUT", nodeName)) 	tag = MUTEINPUT;
				else if (!strcmp("PALETTE0", nodeName)) 	tag = PALETTE0;
				else if (!strcmp("PALETTE1", nodeName)) 	tag = PALETTE1;
				else if (!strcmp("PALETTE2", nodeName)) 	tag = PALETTE2;
				else if (!strcmp("PALETTE3", nodeName)) 	tag = PALETTE3;
				else if (!strcmp("PALETTE4", nodeName)) 	tag = PALETTE4;
				else if (!strcmp("PALETTE5", nodeName)) 	tag = PALETTE5;
				else if (!strcmp("PALETTE6", nodeName)) 	tag = PALETTE6;
				else if (!strcmp("PALETTE7", nodeName)) 	tag = PALETTE7;
				else if (!strcmp("PALETTE8", nodeName)) 	tag = PALETTE8;
				else if (!strcmp("VIEWERMARQUEE", nodeName))	tag = VIEWERMARQUEE;
				else if (!strcmp("VIEWERSHOWFREQ", nodeName))	tag = VIEWERSHOWFREQ;
				else if (!strcmp("VIEWERSTART", nodeName))		tag = VIEWERSTART;
				else if (!strcmp("VIEWERCHANNELS", nodeName))	tag = VIEWERCHANNELS;
				else if (!strcmp("VIEWERSQUELCH", nodeName))	tag = VIEWERSQUELCH;
				else if (!strcmp("VIEWERTIMEOUT", nodeName))	tag = VIEWERTIMEOUT;
				else if (!strcmp("WFAVERAGING", nodeName))	tag = WFAVERAGING;
				else if (!strcmp("USEGROUPCOLORS", nodeName)) tag = USEGROUPCOLORS;
				else if (!strcmp("FKEYGROUP1", nodeName)) tag = FKEYGROUP1;
				else if (!strcmp("FKEYGROUP2", nodeName)) tag = FKEYGROUP2;
				else if (!strcmp("FKEYGROUP3", nodeName)) tag = FKEYGROUP3;
				else if (!strcmp("FKEYTEXTCOLOR", nodeName)) tag = FKEYTEXTCOLOR;
				else if (!strcmp("RXFONTNBR", nodeName)) tag = RXFONTNBR;
				else if (!strcmp("RXFONTSIZE", nodeName)) tag = RXFONTSIZE;
				else if (!strcmp("TXFONTNBR", nodeName)) tag = TXFONTNBR;
				else if (!strcmp("TXFONTSIZE", nodeName)) tag = TXFONTSIZE;
				else if (!strcmp("RXFONTCOLOR", nodeName)) tag = RXFONTCOLOR;
				else if (!strcmp("TXFONTCOLOR", nodeName)) tag = TXFONTCOLOR;
				else if (!strcmp("WATERFALLFONTNBR", nodeName)) tag = WATERFALLFONTNBR;
				else if (!strcmp("WATERFALLFONTSIZE", nodeName)) tag = WATERFALLFONTSIZE;
				else if (!strcmp("RSIDWIDESEARCH", nodeName)) tag = RSIDWIDESEARCH;
				else if (!strcmp("TRANSMITRSID", nodeName)) tag = TRANSMITRSID;
				else if (!strcmp("SLOWCPU", nodeName)) tag = SLOWCPU;
				else tag = IGNORE;
				}
				break;

			case EXN_NONE:
			case EXN_COMMENT:
			case EXN_UNKNOWN:
				break;
		}
	}
// delete the xml parser after usage
	delete xml;
	return true;
}

void configuration::loadDefaults() {
	FL_LOCK_D();
	
// RTTY
	selShift->value(rtty_shift);
	selBaud->value(rtty_baud);
	selBits->value(rtty_bits);
	selParity->value(rtty_parity);
//	chkMsbFirst->value(rtty_msbfirst);
	selStopBits->value(rtty_stop);
	btnCRCRLF->value(rtty_crcrlf);
	btnAUTOCRLF->value(rtty_autocrlf);
	cntrAUTOCRLF->value(rtty_autocount);
	chkPseudoFSK->value(PseudoFSK);
	chkUOSrx->value(UOSrx);
	chkUOStx->value(UOStx);
	chkXagc->value(Xagc);
	
	for (int i = 0; i < 3; i++)
		if (rtty_afcspeed == i)
			btnRTTYafc[i]->value(1);
		else
			btnRTTYafc[i]->value(0);
	btnPreferXhairScope->value(PreferXhairScope);
// OLIVIA
	mnuOlivia_Tones->value(oliviatones);
	mnuOlivia_Bandwidth->value(oliviabw);
	cntOlivia_smargin->value(oliviasmargin);
	cntOlivia_sinteg->value(oliviasinteg);
	btnOlivia_8bit->value(olivia8bit);

	chkDominoEX_FEC->value(DOMINOEX_FEC);

	btnmt63_interleave->value(mt63_interleave == 64);

	FL_UNLOCK_D();
}

void configuration::storeDefaults() { }

void configuration::saveDefaults() {
	FL_LOCK();
// strings
	myCall = inpMyCallsign->value();
	myName = inpMyName->value();
	myQth  = inpMyQth->value();
	myLocator = inpMyLocator->value();
	secText = txtSecondary->value();
	THORsecText = txtTHORSecondary->value();
	PTTdev = inpTTYdev->value();

	for (int i = 0; i < 9; i++) {
		progdefaults.cfgpal[i].R =  palette[i].R;
		progdefaults.cfgpal[i].G =  palette[i].G;
		progdefaults.cfgpal[i].B =  palette[i].B;
	}
	FL_UNLOCK();
	
	writeDefaultsXML();
	changed = false;
}

int configuration::setDefaults() {
#if USE_HAMLIB	
	getRigs();
#endif	
	FL_LOCK();
	inpMyCallsign->value(myCall.c_str());
	inpMyName->value(myName.c_str());
	inpMyQth->value(myQth.c_str());
	inpMyLocator->value(myLocator.c_str());
	UseLeadingZeros = btnUseLeadingZeros->value();
	ContestStart = (int)nbrContestStart->value();
	ContestDigits = (int)nbrContestDigits->value();
		
	txtSecondary->value(secText.c_str());

	txtTHORSecondary->value(THORsecText.c_str());
	valTHOR_BW->value(THOR_BW);
	valTHOR_FILTER->value(THOR_FILTER);
	valTHOR_PATHS->value(THOR_PATHS);
	valTHOR_SOFT->value(THOR_SOFT);
	valThorCWI->value(ThorCWI);
		
	valDominoEX_BW->value(DOMINOEX_BW);
	valDominoEX_FILTER->value(DOMINOEX_FILTER);
	chkDominoEX_FEC->value(DOMINOEX_FEC);
	valDominoEX_PATHS->value(DOMINOEX_PATHS);
	valDomCWI->value(DomCWI);
				
	for (int i = 0; i < 5; i++) {
		btnPTT[i]->value(0);
		btnPTT[i]->activate();
	}
	btnPTT[btnPTTis]->value(1);
#if !USE_HAMLIB
	btnPTT[1]->deactivate();
	chkUSEHAMLIB->deactivate();
    inpRIGdev->hide();
    mnuBaudRate->hide();
    cboHamlibRig->hide();
#else
    btnPTT[1]->activate();
	chkUSEHAMLIB->activate();
	inpRIGdev->show();
	mnuBaudRate->show();
    cboHamlibRig->show();
	cboHamlibRig->value(HamRigName.c_str());
#endif
	btnRTSptt->value(RTSptt);
	btnDTRptt->value(DTRptt);
	btnRTSplusV->value(RTSplus);
	btnDTRplusV->value(DTRplus);

	inpTTYdev->value(PTTdev.c_str());

	if(chkUSEMEMMAPis) {
		chkUSEMEMMAP->value(1); chkUSEHAMLIB->value(0); chkUSERIGCAT->value(0);
		cboHamlibRig->deactivate();
		inpRIGdev->deactivate();
		mnuBaudRate->deactivate();
		btnPTT[1]->deactivate(); btnPTT[2]->activate(); btnPTT[3]->deactivate();
	} else if (chkUSEHAMLIBis) {
		chkUSEMEMMAP->value(0); chkUSERIGCAT->value(0); chkUSEHAMLIB->value(1);
		cboHamlibRig->activate();
		inpRIGdev->activate();
		mnuBaudRate->activate();
		btnPTT[1]->activate(); btnPTT[2]->deactivate(); btnPTT[3]->deactivate();
	} else if (chkUSERIGCATis) {
		chkUSEMEMMAP->value(0); chkUSEHAMLIB->value(0); chkUSERIGCAT->value(1);
		cboHamlibRig->deactivate();
		inpRIGdev->deactivate();
		mnuBaudRate->deactivate();
		btnPTT[1]->deactivate(); btnPTT[2]->deactivate(); btnPTT[3]->activate();
	} else {
		chkUSEMEMMAP->value(0); chkUSEHAMLIB->value(0); chkUSERIGCAT->value(0);
		btnPTT[1]->deactivate(); btnPTT[2]->deactivate(); btnPTT[3]->deactivate();
	}

	inpRIGdev->value(HamRigDevice.c_str());
	mnuBaudRate->value(HamRigBaudrate);

	valCWsweetspot->value(CWsweetspot);
	valRTTYsweetspot->value(RTTYsweetspot);
	valPSKsweetspot->value(PSKsweetspot);
	btnStartAtSweetSpot->value(StartAtSweetSpot);
	btnPSKmailSweetSpot->value(PSKmailSweetSpot);
	cntSearchRange->value(SearchRange);
	cntServerOffset->value(ServerOffset);
	cntACQsn->value(ACQsn);
			
	btnCursorBWcolor->color(
		fl_rgb_color(cursorLineRGBI.R, cursorLineRGBI.G, cursorLineRGBI.B) );
	btnCursorCenterLineColor->color(
		fl_rgb_color(cursorCenterRGBI.R, cursorCenterRGBI.G, cursorCenterRGBI.B) );
	btnBwTracksColor->color(
		fl_rgb_color(bwTrackRGBI.R, bwTrackRGBI.G, bwTrackRGBI.B) );
				
	cntCWweight->value(CWweight);
	sldrCWxmtWPM->value(CWspeed);
	cntCWdefWPM->value(defCWspeed);
	sldrCWbandwidth->value(CWbandwidth);
	btnCWrcvTrack->value(CWtrack);
	cntCWrange->value(CWrange);
	cntCWlowerlimit->value(CWlowerlimit);
	cntCWupperlimit->value(CWupperlimit);
	cntCWlowerlimit->maximum(CWupperlimit - 20);
	cntCWupperlimit->minimum(CWlowerlimit + 20);
	cntCWrisetime->value(CWrisetime);
	cntCWdash2dot->value(CWdash2dot);
	sldrCWxmtWPM->minimum(CWlowerlimit);
	sldrCWxmtWPM->maximum(CWupperlimit);
	btnQSK->value(QSK);
	cntPreTiming->maximum((int)(2400/CWspeed)/2.0); 
	cntPreTiming->value(CWpre);
	cntPostTiming->maximum((int)(2400/CWspeed)/2.0);
	cntPostTiming->value(CWpost);
	btnCWID->value(CWid);
			
	selHellFont->value(feldfontnbr);
	btnFeldHellIdle->value(HellXmtIdle);
			
	chkTransmitRSid->value(TransmitRSid);
	chkRSidWideSearch->value(rsidWideSearch);
	chkSlowCpu->value(slowcpu);
	
	string bandsfname = HomeDir;
	bandsfname.append("frequencies.def");
	ifstream bandsfile(bandsfname.c_str(), ios::in);
	if (bandsfile) {
		string sBand;
		cboBand->add(" ");
		while (!bandsfile.eof()) {
			sBand = "";
			bandsfile >> sBand; bandsfile.ignore();
			if (sBand.length() > 0)
				cboBand->add(sBand.c_str());
		}
		bandsfile.close();
	} else {
		int i = 0;
		while (szBands[i]) {
			cboBand->add((char *)szBands[i]);
			i++;
		}
	}
	btnQRZnotavailable->value(0);
	btnQRZsocket->value(0);
	btnQRZcdrom->value(0);
	btnHAMCALLsocket->value(0);
	if (QRZ == 0)
		btnQRZnotavailable->value(1);
	else if (QRZ == 1)
		btnQRZsocket->value(1);
	else if (QRZ == 2)
		btnQRZcdrom->value(1);
	else if (QRZ == 3)
		btnHAMCALLsocket->value(1);
			
	btnRTTY_USB->value(RTTY_USB);
	btnsendid->value(sendid);
	btnsendvideotext->value(sendtextid);
	chkID_SMALL->value(ID_SMALL);
				
	FL_UNLOCK();

	ReceiveText->setFont((Fl_Font)RxFontnbr);
	ReceiveText->setFontSize(RxFontsize);
	
	TransmitText->setFont((Fl_Font)TxFontnbr);
	TransmitText->setFontSize(TxFontsize);

	wf->setPrefilter(wfPreFilter);
	valLatency->value(latency);

	for (int i = 0; i < 9; i++) {
		palette[i].R = (uchar)cfgpal[i].R;
		palette[i].G = (uchar)cfgpal[i].G;
		palette[i].B = (uchar)cfgpal[i].B;
	}
	wf->setcolors();
	setColorButtons();

	return 1;
}

void configuration::initOperator() {
	FL_LOCK();
		myCall = inpMyCallsign->value();
		myName = inpMyName->value();
		myQth  = inpMyQth->value();
		myLocator = inpMyLocator->value();
		UseLeadingZeros = btnUseLeadingZeros->value();
		ContestStart = (int)nbrContestStart->value();
		ContestDigits = (int)nbrContestDigits->value();
	FL_UNLOCK();
}

#include "rigio.h"

void configuration::initInterface() {
	initOperator();


// close down any possible rig interface threads
#if USE_HAMLIB
		hamlib_close();
		MilliSleep(100);
#endif
		rigMEM_close();
		MilliSleep(100);
		rigCAT_close();
		MilliSleep(100);

	FL_LOCK();
		btnPTTis = (btnPTT[0]->value() ? 0 :
					btnPTT[1]->value() ? 1 :
					btnPTT[2]->value() ? 2 :
					btnPTT[3]->value() ? 3 :
					btnPTT[4]->value() ? 4 : 0); // default is None
					
		RTSptt = btnRTSptt->value();
		DTRptt = btnDTRptt->value();
		RTSplus = btnRTSplusV->value();
		DTRplus = btnDTRplusV->value();
		
		PTTdev = inpTTYdev->value();

#if USE_HAMLIB
		chkUSEHAMLIBis = chkUSEHAMLIB->value();
#endif		
		chkUSEMEMMAPis = chkUSEMEMMAP->value();
		chkUSERIGCATis = chkUSERIGCAT->value();

#if USE_HAMLIB
		HamRigName = cboHamlibRig->value();
		HamRigDevice = inpRIGdev->value();
		HamRigBaudrate = mnuBaudRate->value();
#else
		cboHamlibRig->hide();
		inpRIGdev->hide();
		mnuBaudRate->hide();
#endif		
	FL_UNLOCK();
		
	if (chkUSEMEMMAPis) {// start the memory mapped i/o thread
		btnPTT[2]->activate();
		rigMEM_init();
		wf->setQSY(1);
		activate_rig_menu_item(false);
	} else if (chkUSERIGCATis) { // start the rigCAT thread
		if (rigCAT_init() == false) {
			wf->USB(true);
			cboBand->show();
			btnSideband->show();
			wf->rfcarrier(atoi(cboBand->value())*1000L);
			wf->setQSY(0);
			activate_rig_menu_item(false);
		} else {
			cboBand->hide();
			btnSideband->hide();
			wf->setQSY(1);
			activate_rig_menu_item(true);
		}
#if USE_HAMLIB
	} else if (chkUSEHAMLIBis) { // start the hamlib thread
		if (hamlib_init(btnPTTis == 1 ? true : false) == false) {
			wf->USB(true);
			cboBand->show();
			btnSideband->show();
			wf->rfcarrier(atoi(cboBand->value())*1000L);
			wf->setQSY(0);
			activate_rig_menu_item(false);
		} else {
			cboBand->hide();
			btnSideband->hide();
			wf->setQSY(1);
			activate_rig_menu_item(true);
		}
#endif		
	} else {
		wf->USB(true);
		cboBand->show();
		btnSideband->show();
		wf->rfcarrier(atoi(cboBand->value())*1000L);
		wf->setQSY(0);
		activate_rig_menu_item(false);
	}
	
	push2talk->reset(btnPTTis);
	wf->setRefLevel();
	wf->setAmpSpan();
	cntLowFreqCutoff->value(LowFreqCutoff);
		
}

string configuration::strBaudRate()
{
	return (szBaudRates[HamRigBaudrate + 1]);
}

#if USE_HAMLIB
void configuration::getRigs() {
list<string>::iterator pstr;
	xcvr->get_rignames();
	pstr = (xcvr->rignames).begin();
FL_LOCK();
	while (pstr != (xcvr->rignames).end()) {
		cboHamlibRig->add((*pstr).c_str());
		++pstr;
	}
FL_UNLOCK();
}
#endif

void configuration::testCommPorts()
{
	int fd;

	strCommPorts = "Ports:";
	char COM[7] = "COMxxx";
	char sztty[20] = "/dev/usb/ttyUSBxxx";
#ifdef __CYGWIN__
	for (int i = 0; i < 255; i++) {
#else
	for (int i = 0; i < 8; i++) {
#endif
		snprintf(sztty, sizeof(sztty), "/dev/ttyS%-d", i);
		snprintf(COM, sizeof(COM), "COM%-d", i+1);
		if ((fd = open( sztty, O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
			continue;
		strCommPorts += '\n';
#ifdef __CYGWIN__
		strCommPorts.append(COM);
#else
		strCommPorts.append(sztty);
#endif
		close(fd);
    }
	for (int i = 0; i < 8; i++) {
		snprintf(sztty, sizeof(sztty), "/dev/ttyUSB%-d", i);
		if ((fd = open( sztty, O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
			continue;
		strCommPorts += '\n';
		strCommPorts.append(sztty);
		close(fd);
    }
	for (int i = 0; i < 8; i++) {
		snprintf(sztty, sizeof(sztty), "/dev/usb/ttyUSB%-d", i);
		if ((fd = open( sztty, O_RDWR | O_NOCTTY | O_NDELAY)) < 0)
			continue;
		strCommPorts += '\n';
		strCommPorts.append(sztty);
		close(fd);
    }
}

