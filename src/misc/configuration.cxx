#include <config.h>

#include "configuration.h"
#include "confdialog.h"
#include "xmlreader.h"
#include "soundconf.h"
#include "waterfall.h"

#if USE_HAMLIB
	#include "hamlib.h"
	#include "rigclass.h"
#endif

#include "rigMEM.h"
//#include "rigio.h"
#include "debug.h"

#include <iostream>
#include <fstream>

#include <map>

#ifdef __linux__
#  include <dirent.h>
#  include <limits.h>
#  include <errno.h>
#endif
#ifdef __APPLE__
#  include <glob.h>
#endif
#ifndef __CYGWIN__
#  include <sys/stat.h>
#endif

configuration progdefaults = {
	false,			// bool		rsid;
	false,			// bool		rsidWideSearch;
	false,			// bool		TransmitRSid;
	true,			// bool		slowcpu;

	false,			// bool		changed;
	-20.0,			// double	wfRefLevel;
	70.0,			// double	wfAmpSpan;
	300,			// int		LowFreqCutoff;
	1000,			// int		CWsweetspot;
	1000,			// int		RTTYsweetspot;
	1000,			// int		PSKsweetspot;
	false,			// bool		StartAtSweetSpot;
	false,			// bool		WaterfallHistoryDefault;
	false,			// bool		WaterfallQSY;
	"",			// string	WaterfallClickText;
	waterfall::WF_CARRIER,	// int		WaterfallWheelAction;

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
//	true,			// bool		RTTY_USB;
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
	false,			// bool		QSKv;
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
	"",             // string   QRZpathname;
	"",				// string	QRZusername;
	"",				// string	QRZuserpassword;
	false,			// bool     QRZchanged;
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
	0,				// int		chkUSEXMLRPCis
#if defined(__linux__)
	"/dev/ttyS0",		// string	PTTdev
	"/dev/ttyS1",		// string	CWFSKport
	"/dev/ttyS0",		// string	HamRigDevice
#elif defined(__CYGWIN__)
	"COM1",			// string	PTTdev
	"COM2",			// string	CWFSKport
	"COM1",			// string	HamRigDevice
#else // not sure
	"/dev/ptt",		// string	PTTdev
	"/dev/fsk",		// string	CWFSKport
	"/dev/rig",		// string	HamRigDevice
#endif
	"",			// string	HamRigName
	1,			// int		HamRigBaudrate
//
	"",				// myCall
	"",				// myName
	"",				// myQth
	"",				// myLoc
#if defined(__linux__)

#elif defined(__CYGWIN__)

#else

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
	0,				// bool		UseLastMacro;
	0,				// bool		DisplayMacroFilename;
	
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
	{  80, 144, 144}, // RGB btnGroup1;
	{ 144,  80,  80}, // RGB btnGroup2;
	{  80,  80, 144}, // RGB btnGroup3;
	{ 255, 255, 255}, // RGB btnFkeyTextColor;

// Rx / Tx / Waterfall Text Widgets

	FL_SCREEN,		// Fl_Font 		RxFontnbr
	16,				// int 		RxFontsize
	FL_BLACK,				// Fl_Color		RxFontcolor
	FL_SCREEN,		// Fl_Font 		TxFontnbr
	16,				// int 		TxFontsize
	FL_BLACK,				// Fl_Color		TxFontcolor
	{ 255, 242, 190}, // RGB RxColor;
	{ 200, 235, 255}, // RGB TxColor;
	
	FL_RED,			// Fl_Color		XMITcolor;
	FL_DARK_GREEN,	// Fl_Color		CTRLcolor;
	FL_BLUE,		// Fl_Color		SKIPcolor;
	FL_DARK_MAGENTA,// Fl_Color		ALTRcolor;
	
	FL_SCREEN,		// Fl_Font		WaterfallFontnbr
	12,				// int		WaterfallFontsize

	"gtk+",				// string	ui_scheme

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
	"127.0.0.1",		// string xmlrpc_address
	"7362",			// string xmlrpc_port
	"",			// string xmlrpc_allow
	"",			// string xmlrpc_deny

	false			// bool docked_scope
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


#define TAG_LIST                                                                          \
        NOP_(IGNORE),                                                                     \
                                                                                          \
        STR_(MYCALL, myCall),  STR_(MYNAME, myName),                                      \
        STR_(MYQTH, myQth),  STR_(MYLOC, myLocator),                                      \
                                                                                          \
        DBL_(WFREFLEVEL, wfRefLevel),  DBL_(WFAMPSPAN, wfAmpSpan),                        \
        INT_(LOWFREQCUTOFF, LowFreqCutoff),                                               \
                                                                                          \
        BOOL_(WATERFALLHISTORYDEFAULT, WaterfallHistoryDefault),                          \
        BOOL_(WATERFALLQSY, WaterfallQSY),  STR_(WATERFALLCLICKTEXT, WaterfallClickText), \
        INT_(WATERFALLWHEELACTION, WaterfallWheelAction),                                 \
                                                                                          \
        BOOL_(STARTATSWEETSPOT, StartAtSweetSpot),                                        \
        BOOL_(PSKMAILSWEETSPOT, PSKmailSweetSpot),  INT_(PSKSEARCHRANGE, SearchRange),    \
        INT_(PSKSERVEROFFSET, ServerOffset),  DBL_(CWSWEETSPOT, CWsweetspot),             \
        DBL_(PSKSWEETSPOT, PSKsweetspot), DBL_(ACQSN, ACQsn),                             \
        DBL_(RTTYSWEETSPOT, RTTYsweetspot),                                               \
                                                                                          \
        INT_(RTTYSHIFT, rtty_shift),  INT_(RTTYBAUD, rtty_baud),                          \
        INT_(RTTYBITS, rtty_bits),  INT_(RTTYPARITY, rtty_parity),                        \
        INT_(RTTYSTOP, rtty_stop),  BOOL_(RTTYREVERSE, rtty_reverse),                     \
        BOOL_(RTTYMSBFIRST, rtty_msbfirst),  BOOL_(RTTYCRCLF, rtty_crcrlf),               \
        BOOL_(RTTYAUTOCRLF, rtty_autocrlf),  INT_(RTTYAUTOCOUNT, rtty_autocount),         \
        INT_(RTTYAFCSPEED, rtty_afcspeed),  NOP_(RTTYUSB),                                \
                                                                                          \
        BOOL_(PREFERXHAIRSCOPE, PreferXhairScope),                                        \
                                                                                          \
        BOOL_(PSEUDOFSK, PseudoFSK),                                                      \
                                                                                          \
        BOOL_(UOSRX, UOSrx),  BOOL_(UOSTX, UOStx),                                        \
                                                                                          \
        BOOL_(XAGC, Xagc),                                                                \
                                                                                          \
        INT_(CWWEIGHT, CWweight),  INT_(CWSPEED, CWspeed),  INT_(CWDEFSPEED, defCWspeed), \
        INT_(CWBANDWIDTH, CWbandwidth),  INT_(CWRANGE, CWrange),                          \
        INT_(CWLOWERLIMIT, CWlowerlimit),  INT_(CWUPPERLIMIT, CWupperlimit),              \
        BOOL_(CWTRACK, CWtrack),  DBL_(CWRISETIME, CWrisetime),                           \
        DBL_(CWDASH2DOT, CWdash2dot),  BOOL_(QSK, QSKv),  DBL_(CWPRE, CWpre),             \
        DBL_(CWPOST, CWpost),  BOOL_(CWID, CWid),  INT_(IDWPM, CWIDwpm),                  \
                                                                                          \
        INT_(OLIVIATONES, oliviatones),  INT_(OLIVIABW, oliviabw),                        \
        INT_(OLIVIASMARGIN, oliviasmargin),  INT_(OLIVIASINTEG, oliviasinteg),            \
        BOOL_(OLIVIA8BIT, olivia8bit),                                                    \
                                                                                          \
        DBL_(THORBW, THOR_BW),  BOOL_(THORFILTER, THOR_FILTER),                           \
        STR_(THORSECTEXT, THORsecText),  INT_(THORPATHS, THOR_PATHS),                     \
        BOOL_(THORSOFT, THOR_SOFT),  DBL_(THORCWI, ThorCWI),                              \
                                                                                          \
        DBL_(DOMINOEXBW, DOMINOEX_BW),  BOOL_(DOMINOEXFILTER, DOMINOEX_FILTER),           \
        BOOL_(DOMINOEXFEC, DOMINOEX_FEC),  INT_(DOMINOEXPATHS, DOMINOEX_PATHS),           \
        DBL_(DOMCWI, DomCWI),                                                             \
                                                                                          \
        INT_(FELDFONTNBR, feldfontnbr),  BOOL_(HELLRCVWIDTH, HellRcvWidth),               \
        INT_(HELLXMTWIDTH, HellXmtWidth),                                                 \
        BOOL_(HELLBLACKBOARD, HellBlackboard),  BOOL_(HELLPULSEFAST, HellPulseFast),      \
        BOOL_(HELLXMTIDLE, HellXmtIdle),                                                  \
                                                                                          \
        INT_(WFPREFILTER, wfPreFilter),  INT_(LATENCY, latency),                          \
        BOOL_(USECURSORLINES, UseCursorLines),                                            \
        BOOL_(USECURSORCENTERLINE, UseCursorCenterLine),                                  \
        BOOL_(USEBWTRACKS, UseBWTracks),  RGB_(CLCOLORS, cursorLineRGBI),                 \
        RGB_(CCCOLORS, cursorCenterRGBI),  RGB_(BWTCOLORS, bwTrackRGBI),                  \
                                                                                          \
        BOOL_(VIEWXMTSIGNAL, viewXmtSignal),  BOOL_(SENDID, sendid),                      \
        BOOL_(MACROID, macroid),  BOOL_(SENDTEXTID, sendtextid),                          \
        STR_(STRTEXTID, strTextid),  INT_(VIDEOWIDTH, videowidth),                        \
        BOOL_(IDSMALL, ID_SMALL),                                                         \
                                                                                          \
        INT_(QRZTYPE, QRZ),  STR_(QRZPATHNAME, QRZpathname),                              \
        STR_(QRZUSER, QRZusername),  STR_(QRZPASSWORD, QRZuserpassword),                  \
                                                                                          \
        BOOL_(BTNUSB, btnusb),  INT_(BTNPTTIS, btnPTTis),  BOOL_(RTSPTT, RTSptt),         \
        BOOL_(DTRPTT, DTRptt),  BOOL_(RTSPLUS, RTSplus),  BOOL_(DTRPLUS, DTRplus),        \
                                                                                          \
        INT_(CHOICEHAMLIBIS, choiceHAMLIBis),  INT_(CHKUSEMEMMAPIS, chkUSEMEMMAPis),      \
        INT_(CHKUSEHAMLIBIS, chkUSEHAMLIBis),  INT_(CHKUSERIGCATIS, chkUSERIGCATis),      \
        INT_(CHKUSEXMLRPCIS, chkUSEXMLRPCis),                                             \
                                                                                          \
        STR_(HAMRIGNAME, HamRigName),  STR_(HAMRIGDEVICE, HamRigDevice),                  \
        INT_(HAMRIGBAUDRATE, HamRigBaudrate),  STR_(PTTDEV, PTTdev),                      \
                                                                                          \
        STR_(SECONDARYTEXT, secText),                                                     \
                                                                                          \
        INT_(AUDIOIO, btnAudioIOis),  STR_(OSSDEVICE, OSSdevice),                         \
        STR_(PADEVICE, PAdevice),  STR_(PORTINDEVICE, PortInDevice),                      \
        INT_(PORTININDEX, PortInIndex),  STR_(PORTOUTDEVICE, PortOutDevice),              \
        INT_(PORTOUTINDEX, PortOutIndex),  STR_(PULSESERVER, PulseServer),                \
                                                                                          \
        INT_(SAMPLERATE, sample_rate),  INT_(INSAMPLERATE, in_sample_rate),               \
        INT_(OUTSAMPLERATE, out_sample_rate),  INT_(SAMPLECONVERTER, sample_converter),   \
        INT_(RXCORR, RX_corr),  INT_(TXCORR, TX_corr),  INT_(TXOFFSET, TxOffset),         \
                                                                                          \
        BOOL_(USELEADINGZEROS, UseLeadingZeros),  INT_(CONTESTSTART, ContestStart),       \
        INT_(CONTESTDIGITS, ContestDigits),  BOOL_(USETIMER, useTimer),                   \
        INT_(MACRONUMBER, macronumber),  INT_(TIMEOUT, timeout),                          \
                                                                                          \
        BOOL_(USELASTMACRO, UseLastMacro),                                                \
        BOOL_(DISPLAYMACROFILENAME, DisplayMacroFilename),                                \
                                                                                          \
        STR_(MXDEVICE, MXdevice),  DBL_(PCMVOLUME, PCMvolume),  BOOL_(MICIN, MicIn),      \
        BOOL_(LINEIN, LineIn),  BOOL_(ENABLEMIXER, EnableMixer),                          \
        BOOL_(MUTEINPUT, MuteInput),                                                      \
                                                                                          \
        RGB_(PALETTE0, cfgpal[0]), RGB_(PALETTE1, cfgpal[1]), RGB_(PALETTE2, cfgpal[2]),  \
        RGB_(PALETTE3, cfgpal[3]), RGB_(PALETTE4, cfgpal[4]), RGB_(PALETTE5, cfgpal[5]),  \
        RGB_(PALETTE6, cfgpal[6]), RGB_(PALETTE7, cfgpal[7]), RGB_(PALETTE8, cfgpal[8]),  \
                                                                                          \
        BOOL_(VIEWERMARQUEE, VIEWERmarquee),  BOOL_(VIEWERSHOWFREQ, VIEWERshowfreq),      \
        INT_(VIEWERSTART, VIEWERstart),  INT_(VIEWERCHANNELS, VIEWERchannels),            \
        DBL_(VIEWERSQUELCH, VIEWERsquelch),  INT_(VIEWERTIMEOUT, VIEWERtimeout),          \
                                                                                          \
        BOOL_(WFAVERAGING, WFaveraging),                                                  \
                                                                                          \
        BOOL_(USEGROUPCOLORS, useGroupColors),  RGB_(FKEYGROUP1, btnGroup1),              \
        RGB_(FKEYGROUP2, btnGroup2),  RGB_(FKEYGROUP3, btnGroup3),                        \
        RGB_(FKEYTEXTCOLOR, btnFkeyTextColor),                                            \
                                                                                          \
        INT_(RXFONTNBR, RxFontnbr),  INT_(RXFONTSIZE, RxFontsize),                        \
        INT_(RXFNTCOLOR, RxFontcolor),  INT_(TXFONTNBR, TxFontnbr),                       \
        INT_(TXFONTSIZE, TxFontsize),  INT_(TXFNTCOLOR, TxFontcolor),                     \
                                                                                          \
        INT_(XMITCOLOR, XMITcolor),  INT_(CTRLCOLOR, CTRLcolor),                          \
        INT_(SKIPCOLOR, SKIPcolor),  INT_(ALTRCOLOR, ALTRcolor),                          \
        RGB_(RXFONTCOLOR, RxColor),  RGB_(TXFONTCOLOR, TxColor),                          \
                                                                                          \
        INT_(WATERFALLFONTNBR, WaterfallFontnbr),                                         \
        INT_(WATERFALLFONTSIZE, WaterfallFontsize),                                       \
                                                                                          \
        STR_(UISCHEME, ui_scheme),                                                        \
                                                                                          \
        BOOL_(RSIDWIDESEARCH, rsidWideSearch),  BOOL_(TRANSMITRSID, TransmitRSid),        \
                                                                                          \
        BOOL_(SLOWCPU, slowcpu),                                                          \
                                                                                          \
        BOOL_(MT638BIT, mt63_8bit),  INT_(MT63INTERLEAVE, mt63_interleave),               \
                                                                                          \
        BOOL_(DOCKEDSCOPE, docked_scope)


void read_xml_int(IrrXMLReader* xml, void* var)
{
	*((int*)var) = atoi(xml->getNodeData());
}
void read_xml_bool(IrrXMLReader* xml, void* var)
{
	*((bool*)var) = atoi(xml->getNodeData());
}
void read_xml_dbl(IrrXMLReader* xml, void* var)
{
	*((double*)var) = atof(xml->getNodeData());
}
void read_xml_str(IrrXMLReader* xml, void* var)
{
	*((string*)var) = xml->getNodeData();
}
void read_xml_rgb(IrrXMLReader* xml, void* var)
{
	RGBI* rgb = (RGBI*)var;
	sscanf(xml->getNodeData(), "%hhu %hhu %hhu", &rgb->R, &rgb->G, &rgb->B);
}

void write_xml_int(ofstream& out, const char* tag, void* var)
{
	out << '<' << tag << '>' << (*(int*)var) << "</" << tag << ">\n";
}
void write_xml_bool(ofstream& out, const char* tag, void* var)
{
	out << '<' << tag << '>' << (*(bool*)var) << "</" << tag << ">\n";
}
void write_xml_dbl(ofstream& out, const char* tag, void* var)
{
	out << '<' << tag << '>' << (*(double*)var) << "</" << tag << ">\n";
}
void write_xml_str(ofstream& out, const char* tag, void* var)
{
	string& s = *((string*)var);
	string::size_type i = s.find('&');

	while (i != string::npos) {
		s.replace(i, 1, "&amp;");
		i = s.find('&', i + 1);
	}
	while ((i = s.find('<')) != string::npos)
		s.replace(i, 1, "&lt;");
	while ((i = s.find('>')) != string::npos)
		s.replace(i, 1, "&gt;");
	while ((i = s.find('"')) != string::npos)
		s.replace(i, 1, "&quot;");
	while ((i = s.find('\'')) != string::npos)
		s.replace(i, 1, "&apos;");

	out << '<' << tag << '>' << s << "</" << tag << ">\n";
}
void write_xml_rgb(ofstream& out, const char* tag, void* var)
{
	RGBI* rgb = (RGBI*)var;
	out << '<' << tag << '>' << (int)rgb->R << ' ' << (int)rgb->G
	    << ' ' << (int)rgb->B << "</" << tag << ">\n";
}

#undef INT_
#undef BOOL_
#undef DBL_
#undef STR_
#undef RGB_
#undef NOP_
#define INT_(elem_, var_)  elem_
#define BOOL_(elem_, var_) elem_
#define DBL_(elem_, var_)  elem_
#define STR_(elem_, var_)  elem_
#define RGB_(elem_, var_)  elem_
#define NOP_(elem_)        elem_
enum TAG {
	TAG_LIST
};

struct tag_elem_t {
	const char* tag;
	void* ptr;
	void (*rfunc)(IrrXMLReader*, void*);
	void (*wfunc)(ofstream&, const char*, void*);
};
#undef INT_
#undef BOOL_
#undef DBL_
#undef STR_
#undef RGB_
#undef NOP_
#define INT_(elem_, var_)  { #elem_, &progdefaults.var_, &read_xml_int, &write_xml_int }
#define BOOL_(elem_, var_) { #elem_, &progdefaults.var_, &read_xml_bool, &write_xml_bool }
#define DBL_(elem_, var_)  { #elem_, &progdefaults.var_, &read_xml_dbl, &write_xml_dbl }
#define STR_(elem_, var_)  { #elem_, &progdefaults.var_, &read_xml_str, &write_xml_str }
#define RGB_(elem_, var_)  { #elem_, &progdefaults.var_, &read_xml_rgb, &write_xml_rgb }
#define NOP_(elem_)        { #elem_, NULL, NULL, NULL }
struct tag_elem_t tag_list[] = {
	TAG_LIST
};


void configuration::writeDefaultsXML()
{
	string deffname(HomeDir);
	deffname.append("fldigi_def.xml");

	string deffname_backup(deffname);
	deffname_backup.append("-old");
	rename(deffname.c_str(), deffname_backup.c_str());

	ofstream f(deffname.c_str());
	if (!f) {
		LOG_ERROR("Could not write %s", deffname.c_str());
		return;
	}

	f << "<FLDIGI_DEFS>\n";
	struct tag_elem_t* e;
	for (size_t i = 0; i < sizeof(tag_list)/sizeof(*tag_list); i++)
		if (likely((e = &tag_list[i])->ptr))
			(*e->wfunc)(f, e->tag, e->ptr);
	f << "</FLDIGI_DEFS>\n";
	f.close();
}

bool configuration::readDefaultsXML()
{
	string deffname = HomeDir;
	deffname.append("fldigi_def.xml");
	ifstream f(deffname.c_str());
	if (!f)
		return false;

	string xmlbuf;
	f.seekg(0, ios::end);
	xmlbuf.reserve(f.tellg());
	f.seekg(0, ios::beg);

	char line[BUFSIZ];
	while (f.getline(line, sizeof(line)))
		xmlbuf.append(line).append("\n");
	f.close();

	IrrXMLReader* xml = createIrrXMLReader(new IIrrXMLStringReader(xmlbuf));
	if (!xml)
		return false;

	map<string, enum TAG> tag_map;
	for (size_t i = 0; i < sizeof(tag_list)/sizeof(*tag_list); i++)
		tag_map[tag_list[i].tag] = (enum TAG)i;
	map<string, enum TAG>::const_iterator itag;

	TAG tag = IGNORE;

	// parse the file until end reached
	while(xml->read()) {
		switch(xml->getNodeType()) {
		case EXN_TEXT:
		case EXN_CDATA:
			if (likely(tag != IGNORE))
			    (*tag_list[tag].rfunc)(xml, tag_list[tag].ptr);
			break;
		case EXN_ELEMENT_END:
			tag = IGNORE;
			break;
		case EXN_ELEMENT:
			if ((itag = tag_map.find(xml->getNodeName())) != tag_map.end())
				tag = itag->second;
			else
				tag = IGNORE;
			break;
		case EXN_NONE: case EXN_COMMENT: case EXN_UNKNOWN:
			break;
		}
	}

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
		cfgpal[i].R =  palette[i].R;
		cfgpal[i].G =  palette[i].G;
		cfgpal[i].B =  palette[i].B;
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
		chkUSEMEMMAP->value(1); 
		chkUSEHAMLIB->value(0); chkUSERIGCAT->value(0); chkUSEXMLRPC->value(0);
		cboHamlibRig->deactivate();
		inpRIGdev->deactivate();
		mnuBaudRate->deactivate();
		btnPTT[1]->deactivate(); btnPTT[2]->activate(); btnPTT[3]->deactivate();
	} else if (chkUSEHAMLIBis) {
		chkUSEHAMLIB->value(1);
		chkUSEMEMMAP->value(0); chkUSERIGCAT->value(0);  chkUSEXMLRPC->value(0);
		cboHamlibRig->activate();
		inpRIGdev->activate();
		mnuBaudRate->activate();
		btnPTT[1]->activate(); btnPTT[2]->deactivate(); btnPTT[3]->deactivate();
	} else if (chkUSERIGCATis) {
		chkUSERIGCAT->value(1);
		chkUSEMEMMAP->value(0); chkUSEHAMLIB->value(0); chkUSEXMLRPC->value(0);
		cboHamlibRig->deactivate();
		inpRIGdev->deactivate();
		mnuBaudRate->deactivate();
		btnPTT[1]->deactivate(); btnPTT[2]->deactivate(); btnPTT[3]->activate();
	} else if (chkUSEXMLRPCis) {
		chkUSEXMLRPC->value(1);
		chkUSEMEMMAP->value(0); chkUSEHAMLIB->value(0); chkUSERIGCAT->value(0);
		cboHamlibRig->deactivate();
		inpRIGdev->deactivate();
		mnuBaudRate->deactivate();
		btnPTT[1]->deactivate(); btnPTT[2]->deactivate(); btnPTT[3]->deactivate();
	} else {
		chkUSEMEMMAP->value(0); chkUSEHAMLIB->value(0); 
		chkUSERIGCAT->value(0);	chkUSEHAMLIB->value(0); chkUSEXMLRPC->value(0);
		btnPTT[1]->deactivate(); btnPTT[2]->deactivate(); btnPTT[3]->deactivate();
	}

	inpRIGdev->value(HamRigDevice.c_str());
	mnuBaudRate->value(HamRigBaudrate);

	valCWsweetspot->value(CWsweetspot);
	valRTTYsweetspot->value(RTTYsweetspot);
	valPSKsweetspot->value(PSKsweetspot);
	btnWaterfallHistoryDefault->value(WaterfallHistoryDefault);
	btnWaterfallQSY->value(WaterfallQSY);
	inpWaterfallClickText->input_type(FL_MULTILINE_INPUT);
	inpWaterfallClickText->value(WaterfallClickText.c_str());

	mnuWaterfallWheelAction->add(waterfall::wf_wheel_action);
	mnuWaterfallWheelAction->value(WaterfallWheelAction);

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
	btnQSK->value(QSKv);
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
	txtQRZpathname->value(QRZpathname.c_str());
			
//	btnRTTY_USB->value(RTTY_USB);
	btnsendid->value(sendid);
	btnsendvideotext->value(sendtextid);
	chkID_SMALL->value(ID_SMALL);
				
	FL_UNLOCK();

	wf->setPrefilter(wfPreFilter);
	valLatency->value(latency);
	btnWFaveraging->value(WFaveraging);
	
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
	} else if (chkUSEXMLRPCis) {
		cboBand->hide();
		btnSideband->hide();
		wf->USB(true);
		wf->setXMLRPC(1);
		activate_rig_menu_item(false);
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
#ifndef PATH_MAX
#  define PATH_MAX 1024
#endif
#ifndef __CYGWIN__
	struct stat st;
#endif
#ifndef __APPLE__
	char ttyname[PATH_MAX + 1];
#endif

#ifdef __linux__
	bool ret = false;
	DIR* sys = NULL;
	char cwd[PATH_MAX] = { '.', '\0' };
	if (getcwd(cwd, sizeof(cwd)) == NULL || chdir("/sys/class/tty") == -1 ||
	    (sys = opendir(".")) == NULL)
		goto out;

	ssize_t len;
	struct dirent* dp;
	while ((dp = readdir(sys))) {
#  ifdef _DIRENT_HAVE_D_TYPE
		if (dp->d_type != DT_LNK)
			continue;
#  endif
		if ((len = readlink(dp->d_name, ttyname, sizeof(ttyname)-1)) == -1)
			continue;
		ttyname[len] = '\0';
		if (!strstr(ttyname, "/devices/virtual/")) {
			snprintf(ttyname, sizeof(ttyname), "/dev/%s", dp->d_name);
			if (stat(ttyname, &st) == -1 || !S_ISCHR(st.st_mode))
				continue;
			LOG_INFO("Found serial port %s", ttyname);
			inpTTYdev->add(ttyname);
#if USE_HAMLIB
			inpRIGdev->add(ttyname);
#endif
		}
	}
	ret = true;

out:
	if (sys)
		closedir(sys);
	chdir(cwd);
	if (ret) // do we need to fall back to the probe code below?
		return;
#endif // __linux__


	const char* tty_fmt[] = {
#if defined(__linux__)
		"/dev/ttyS%u",
		"/dev/ttyUSB%u",
		"/dev/usb/ttyUSB%u"
#elif defined(__FreeBSD__)
		"/dev/ttyd%u"
#elif defined(__CYGWIN__)
		"/dev/ttyS%u"
#elif defined(__APPLE__)
		"/dev/cu.*",
		"/dev/tty.*"
#endif
	};

#if defined(__CYGWIN__)
	int fd;
#  define TTY_MAX 255
#elif defined(__APPLE__)
	glob_t gbuf;
#else
#  define TTY_MAX 8
#endif

	for (size_t i = 0; i < sizeof(tty_fmt)/sizeof(*tty_fmt); i++) {
#ifndef __APPLE__
		for (unsigned j = 0; j < TTY_MAX; j++) {
			snprintf(ttyname, sizeof(ttyname), tty_fmt[i], j);
#  ifndef __CYGWIN__
			if ( !(stat(ttyname, &st) == 0 && S_ISCHR(st.st_mode)) )
				continue;
#  else // __CYGWIN__
			if ((fd = open(ttyname, O_RDWR | O_NOCTTY | O_NDELAY)) == -1)
				continue;
			snprintf(ttyname, sizeof(ttyname), "COM%u", j+1);
			close(fd);
#  endif // __CYGWIN__

			LOG_INFO("Found serial port %s", ttyname);
			inpTTYdev->add(ttyname);
#  if USE_HAMLIB
			inpRIGdev->add(ttyname);
#  endif
		}
#else // __APPLE__
		glob(tty_fmt[i], 0, NULL, &gbuf);
		for (size_t j = 0; j < gbuf.gl_pathc; j++) {
			if ( !(stat(gbuf.gl_pathv[j], &st) == 0 && S_ISCHR(st.st_mode)) ||
			     strstr(gbuf.gl_pathv[j], "modem") )
				continue;
			LOG_INFO("Found serial port %s", gbuf.gl_pathv[j]);
			inpTTYdev->add(gbuf.gl_pathv[j]);
#  if USE_HAMLIB
			inpRIGdev->add(gbuf.gl_pathv[j]);
#  endif

		}
		globfree(&gbuf);
#endif // __APPLE__
	}
}
