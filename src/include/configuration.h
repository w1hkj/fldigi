#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#include <string>

#include "main.h"
#include "rtty.h"

#if defined(__linux__)
#  define DEFAULT_PTTDEV "/dev/ttyS0"
#  define DEFAULT_CWFSKPORT "/dev/ttyS1"
#  define DEFAULT_HAMRIGDEVICE "/dev/ttyS0"
#elif defined(__CYGWIN__)
#  define DEFAULT_PTTDEV "COM1"
#  define DEFAULT_CWFSKPORT "COM2"
#  define DEFAULT_HAMRIGDEVICE "COM1"
#else // not sure
#  define DEFAULT_PTTDEV "/dev/ptt"
#  define DEFAULT_CWFSKPORT "/dev/fsk"
#  define DEFAULT_HAMRIGDEVICE "/dev/rig"
#endif


// Format: ELEM_(TYPE, VARIABLE-NAME, TAG-STRING, DEFAULT-VALUE
// Variables that are not saved to the xml file have an empty TAG-STRING
//
// No preprocessor directives or C++ comments inside this macro!

#ifdef ELEM_
#  error ELEM_ should not be defined at this point
#endif

#define CONFIG_LIST                                                                     \
        ELEM_(bool, rsidWideSearch, "RSIDWIDESEARCH", false)                            \
        ELEM_(bool, rsid, "", false)                                                    \
        ELEM_(bool, TransmitRSid, "TRANSMITRSID", false)                                \
        ELEM_(bool, slowcpu, "SLOWCPU", true)                                           \
                                                                                        \
        ELEM_(bool, changed, "", false)                                                 \
                                                                                        \
        ELEM_(double, wfRefLevel, "WFREFLEVEL", -20.0)                                  \
        ELEM_(double, wfAmpSpan, "WFAMPSPAN", 70.0)                                     \
        ELEM_(int, LowFreqCutoff, "LOWFREQCUTOFF", 300)                                 \
        ELEM_(double, CWsweetspot, "CWSWEETSPOT", 1000)                                 \
        ELEM_(double, RTTYsweetspot, "RTTYSWEETSPOT", 1000)                             \
        ELEM_(double, PSKsweetspot, "PSKSWEETSPOT", 1000)                               \
        ELEM_(bool, StartAtSweetSpot, "STARTATSWEETSPOT", false)                        \
        ELEM_(bool, WaterfallHistoryDefault, "WATERFALLHISTORYDEFAULT", false)          \
        ELEM_(bool, WaterfallQSY, "WATERFALLQSY", false)                                \
        ELEM_(std::string, WaterfallClickText, "WATERFALLCLICKTEXT", "")                \
        ELEM_(int, WaterfallWheelAction, "WATERFALLWHEELACTION", waterfall::WF_CARRIER) \
/* PSK mail interface */                                                                \
        ELEM_(bool, PSKmailSweetSpot, "PSKMAILSWEETSPOT", false)                        \
        ELEM_(int, SearchRange, "PSKSEARCHRANGE", 200)                                  \
        ELEM_(int, ServerOffset, "PSKSERVEROFFSET", 40)                                 \
        ELEM_(double, ACQsn, "ACQSN", 6.0)                                              \
/* RTTY */                                                                              \
        ELEM_(int, rtty_shift, "RTTYSHIFT", 3) /* 170 */                                \
        ELEM_(int, rtty_baud, "RTTYBAUD", 0)   /* 45 */                                 \
        ELEM_(int, rtty_bits, "RTTYBITS", 0)   /* 5 */                                  \
        ELEM_(int, rtty_parity, "RTTYPARITY", RTTY_PARITY_NONE)                         \
        ELEM_(int, rtty_stop, "RTTYSTOP", 1)                                            \
        ELEM_(bool, rtty_reverse, "RTTYREVERSE", false)                                 \
        ELEM_(bool, rtty_msbfirst, "RTTYMSBFIRST", false)                               \
        ELEM_(bool, rtty_crcrlf, "RTTYCRCLF", false)                                    \
        ELEM_(bool, rtty_autocrlf, "RTTYAUTOCRLF", true)                                \
        ELEM_(int, rtty_autocount, "RTTYAUTOCOUNT", 72)                                 \
        ELEM_(int, rtty_afcspeed, "RTTYAFCSPEED", 1)                                    \
        ELEM_(bool, useFSKkeyline, "", false)                                           \
        ELEM_(bool, useFSKkeylineDTR, "", false)                                        \
        ELEM_(bool, FSKisLSB, "", true)                                                 \
        ELEM_(bool, useUART, "", false)                                                 \
        ELEM_(bool, PreferXhairScope, "PREFERXHAIRSCOPE", false)                        \
        ELEM_(bool, PseudoFSK, "PSEUDOFSK", false)                                      \
        ELEM_(bool, UOSrx, "UOSRX", true)                                               \
        ELEM_(bool, UOStx, "UOSTX", true)                                               \
        ELEM_(bool, Xagc, "XAGC", false)                                                \
/* CW */                                                                                \
        ELEM_(bool, useCWkeylineRTS, "", false)                                         \
        ELEM_(bool, useCWkeylineDTR, "", false)                                         \
        ELEM_(int, CWweight, "CWWEIGHT", 50)                                            \
        ELEM_(int, CWspeed, "CWSPEED", 18)                                              \
        ELEM_(int, defCWspeed, "CWDEFSPEED", 24)                                        \
        ELEM_(int, CWbandwidth, "CWBANDWIDTH", 150)                                     \
        ELEM_(bool, CWtrack, "CWTRACK", true)                                           \
        ELEM_(int, CWrange, "CWRANGE", 10)                                              \
        ELEM_(int, CWlowerlimit, "CWLOWERLIMIT", 5)                                     \
        ELEM_(int, CWupperlimit, "CWUPPERLIMIT", 50)                                    \
        ELEM_(double, CWrisetime, "CWRISETIME", 4.0)                                    \
        ELEM_(double, CWdash2dot, "CWDASH2DOT", 3.0)                                    \
        ELEM_(bool, QSK, "QSK", false)                                                  \
        ELEM_(double, CWpre, "CWPRE", 4.0)                                              \
        ELEM_(double, CWpost, "CWPOST", 4.0)                                            \
        ELEM_(bool, CWid, "CWID", false)                                                \
        ELEM_(int, CWIDwpm, "IDWPM", 18)                                                \
/* FELD HELL */                                                                         \
        ELEM_(double, HELL_BW, "", 150.0)                                               \
        ELEM_(bool, HellRcvWidth, "HELLRCVWIDTH", false)                                \
        ELEM_(bool, HellBlackboard, "HELLBLACKBOARD", false)                            \
        ELEM_(int, HellXmtWidth, "HELLXMTWIDTH", 1)                                     \
        ELEM_(bool, HellXmtIdle, "HELLXMTIDLE", false)                                  \
        ELEM_(bool, HellPulseFast, "HELLPULSEFAST", false)                              \
/* OLIVIA */                                                                            \
        ELEM_(int, oliviatones, "OLIVIATONES", 2) /* 8 */                               \
        ELEM_(int, oliviabw, "OLIVIABW", 2)     /* 500 */                               \
        ELEM_(int, oliviasmargin, "OLIVIASMARGIN", 8)                                   \
        ELEM_(int, oliviasinteg, "OLIVIASINTEG", 4)                                     \
        ELEM_(bool, olivia8bit, "OLIVIA8BIT", false)                                    \
/* THOR */                                                                              \
        ELEM_(double, THOR_BW, "THORBW", 2.0)                                           \
        ELEM_(bool, THOR_FILTER, "THORFILTER", true)                                    \
        ELEM_(std::string, THORsecText, "THORSECTEXT", "")                              \
        ELEM_(int, THOR_PATHS, "THORPATHS", 5)                                          \
        ELEM_(bool, THOR_SOFT, "THORSOFT", false)                                       \
        ELEM_(double, ThorCWI, "THORCWI", 0.0)                                          \
/* DOMINOEX */                                                                          \
        ELEM_(double, DOMINOEX_BW, "DOMINOEXBW", 2.0)                                   \
        ELEM_(bool, DOMINOEX_FILTER, "DOMINOEXFILTER", true)                            \
        ELEM_(bool, DOMINOEX_FEC, "DOMINOEXFEC", false)                                 \
        ELEM_(int, DOMINOEX_PATHS, "DOMINOEXPATHS", 5)                                  \
        ELEM_(double, DomCWI, "DOMCWI", 0.0)                                            \
/* MT63 */                                                                              \
        ELEM_(bool, mt63_8bit, "MT638BIT", false)                                       \
        ELEM_(int, mt63_interleave, "MT63INTERLEAVE", 64) /* long interleave */         \
/* Waterfall & UI */                                                                    \
        ELEM_(uchar, red, "", 0)                                                        \
        ELEM_(uchar, green, "", 255)                                                    \
        ELEM_(uchar, blue, "", 255)                                                     \
        ELEM_(bool, MultiColorWF, "", false)                                            \
        ELEM_(int, wfPreFilter, "WFPREFILTER", 1) /* Blackman */                        \
        ELEM_(bool, WFaveraging, "WFAVERAGING", false)                                  \
        ELEM_(int, latency, "LATENCY", 4)                                               \
        ELEM_(bool, UseCursorLines, "USECURSORLINES", true)                             \
        ELEM_(bool, UseCursorCenterLine, "USECURSORCENTERLINE", true)                   \
        ELEM_(bool, UseBWTracks, "USEBWTRACKS", true)                                   \
        ELEM_(RGBI, cursorLineRGBI, "CLCOLORS", {255, 255, 0, 255})                     \
        ELEM_(RGBI, cursorCenterRGBI, "CCCOLORS", {255, 255, 255, 255})                 \
        ELEM_(RGBI, bwTrackRGBI, "BWTCOLORS", {255, 0, 0, 255})                         \
        ELEM_(int, feldfontnbr, "FELDFONTNBR", 4)                                       \
        ELEM_(bool, viewXmtSignal, "VIEWXMTSIGNAL", false)                              \
        ELEM_(bool, sendid, "SENDID", false)                                            \
        ELEM_(bool, macroid, "MACROID", false)                                          \
        ELEM_(bool, sendtextid, "SENDTEXTID", false)                                    \
        ELEM_(std::string, strTextid, "STRTEXTID", "CQ")                                \
        ELEM_(bool, macroCWid, "", false)                                               \
        ELEM_(int, videowidth, "VIDEOWIDTH", 1)                                         \
        ELEM_(bool, ID_SMALL, "IDSMALL", true)                                          \
        ELEM_(bool, macrotextid, "", false)                                             \
        ELEM_(bool, docked_scope, "DOCKEDSCOPE", false)                                 \
        ELEM_(bool, docked_rig_control, "DOCKEDRIGCONTROL", true)                       \
        ELEM_(int,  wfwidth, "WFWIDTH", 3000)                                           \
        ELEM_(int,  wfheight, "WFHEIGHT", 125)                                          \
        ELEM_(bool, tooltips, "TOOLTIPS", true)                                         \
/* QRZ */                                                                               \
        ELEM_(int, QRZ, "QRZTYPE", 0) /* Not available */                               \
        ELEM_(std::string, QRZpathname, "QRZPATHNAME", "")                              \
        ELEM_(std::string, QRZusername, "QRZUSER", "")                                  \
        ELEM_(std::string, QRZuserpassword, "QRZPASSWORD", "")                          \
        ELEM_(bool, QRZchanged, "", false)                                              \
/* Rig control */                                                                       \
        ELEM_(bool, btnusb, "BTNUSB", true)                                             \
        ELEM_(int, btnPTTis, "BTNPTTIS", 0)                                             \
        ELEM_(bool, RTSptt, "RTSPTT", false)                                            \
        ELEM_(bool, DTRptt, "DTRPTT", false)                                            \
        ELEM_(bool, RTSplus, "RTSPLUS", false)                                          \
        ELEM_(bool, DTRplus, "DTRPLUS", false)                                          \
        ELEM_(int, choiceHAMLIBis, "CHOICEHAMLIBIS", 0)                                 \
        ELEM_(int, chkUSEMEMMAPis, "CHKUSEMEMMAPIS", 0)                                 \
        ELEM_(int, chkUSEHAMLIBis, "CHKUSEHAMLIBIS", 0)                                 \
        ELEM_(int, chkUSERIGCATis, "CHKUSERIGCATIS", 0)                                 \
        ELEM_(int, chkUSEXMLRPCis, "CHKUSEXMLRPCIS", 0)                                 \
        ELEM_(std::string, PTTdev, "PTTDEV", DEFAULT_PTTDEV)                            \
        ELEM_(std::string, CWFSKport, "", DEFAULT_CWFSKPORT)                            \
        ELEM_(std::string, HamRigDevice, "HAMRIGDEVICE", DEFAULT_HAMRIGDEVICE)          \
        ELEM_(std::string, HamRigName, "HAMRIGNAME", "")                                \
        ELEM_(int, HamRigBaudrate, "HAMRIGBAUDRATE", 1) /* 600 baud */                  \
        ELEM_(std::string, XmlRigFilename, "XMLRIGFILENAME", "")                        \
        ELEM_(std::string, XmlRigDevice, "XMLRIGDEVICE", DEFAULT_HAMRIGDEVICE)          \
        ELEM_(int, XmlRigBaudrate, "XMLRIGBAUDRATE", 1)                                 \
/* RigCAT parameters */                                                                 \
        ELEM_(bool, RigCatRTSplus, "RIGCATRTSPLUS", 0)                                  \
        ELEM_(bool, RigCatDTRplus, "RIGCATDTRPLUS", 0)                                  \
        ELEM_(bool, RigCatRTSptt, "RIGCATRTSPTT", 0)                                    \
        ELEM_(bool, RigCatDTRptt, "RIGCATDTRPTT", 0)                                    \
        ELEM_(bool, RigCatRTSCTSflow, "RIGCATRTSCTSFLOW", 0)                            \
        ELEM_(int, RigCatRetries, "RIGCATRETRIES", 2)                                   \
        ELEM_(int, RigCatTimeout, "RIGCATTIMEOUT", 10)                                  \
        ELEM_(int, RigCatWait, "RIGCATWAIT", 50)                                        \
/* Hamlib parameters */                                                                 \
        ELEM_(bool, HamlibRTSplus, "HAMLIBRTSPLUS", 0)                                  \
        ELEM_(bool, HamlibDTRplus, "HAMLIBDTRPLUS", 0)                                  \
        ELEM_(bool, HamlibRTSCTSflow, "HAMLIBRTSCTSFLOW", 0)                            \
        ELEM_(bool, HamlibXONXOFFflow, "HAMLIBXONXOFFFLOW", 0)                          \
        ELEM_(int, HamlibRetries, "HAMLIBRETRIES", 2)                                   \
        ELEM_(int, HamlibTimeout, "HAMLIBTIMEOUT", 10)                                  \
        ELEM_(int, HamlibWait, "HAMLIBWAIT", 50)                                        \
/* Operator */                                                                          \
        ELEM_(std::string, myCall, "MYCALL", "")                                        \
        ELEM_(std::string, myQth, "MYQTH", "")                                          \
        ELEM_(std::string, myName, "MYNAME", "")                                        \
        ELEM_(std::string, myLocator, "MYLOC", "")                                      \
        ELEM_(std::string, secText, "SECONDARYTEXT", "")                                \
/* Sound card */                                                                        \
        ELEM_(int, btnAudioIOis, "AUDIOIO", SND_IDX_PORT)                               \
        ELEM_(std::string, OSSdevice, "OSSDEVICE", "")                                  \
        ELEM_(std::string, PAdevice, "PADEVICE", "")                                    \
        ELEM_(std::string, PortInDevice, "PORTINDEVICE", "")                            \
        ELEM_(int, PortInIndex, "PORTININDEX", -1)                                      \
        ELEM_(std::string, PortOutDevice, "PORTOUTDEVICE", "")                          \
        ELEM_(int, PortOutIndex, "PORTOUTINDEX", -1)                                    \
        ELEM_(int, PortFramesPerBuffer, "", 0)                                          \
        ELEM_(std::string, PulseServer, "PULSESERVER", "")                              \
        ELEM_(int, sample_rate, "SAMPLERATE", SAMPLE_RATE_UNSET)                        \
        ELEM_(int, in_sample_rate, "INSAMPLERATE", SAMPLE_RATE_UNSET)                   \
        ELEM_(int, out_sample_rate, "OUTSAMPLERATE", SAMPLE_RATE_UNSET)                 \
        ELEM_(int, sample_converter, "SAMPLECONVERTER", SRC_SINC_FASTEST)               \
        ELEM_(int, RX_corr, "RXCORR", 0)                                                \
        ELEM_(int, TX_corr, "TXCORR", 0)                                                \
        ELEM_(int, TxOffset, "TXOFFSET", 0)                                             \
/* Contest controls */                                                                  \
        ELEM_(bool, UseLeadingZeros, "USELEADINGZEROS", true)                           \
        ELEM_(int, ContestStart, "CONTESTSTART", 0)                                     \
        ELEM_(int, ContestDigits, "CONTESTDIGITS", 4)                                   \
/* Macro timer constants and controls */                                                \
        ELEM_(bool, useTimer, "USETIMER", false)                                        \
        ELEM_(int, macronumber, "MACRONUMBER", 0)                                       \
        ELEM_(int, timeout, "TIMEOUT", 0)                                               \
        ELEM_(bool, UseLastMacro, "USELASTMACRO", false)                                \
        ELEM_(bool, DisplayMacroFilename, "DISPLAYMACROFILENAME", false)                \
/* Mixer */                                                                             \
        ELEM_(std::string, MXdevice, "MXDEVICE", "")                                    \
        ELEM_(bool, MicIn, "MICIN", false)                                              \
        ELEM_(bool, LineIn, "LINEIN", true)                                             \
        ELEM_(bool, EnableMixer, "ENABLEMIXER", false)                                  \
        ELEM_(double, PCMvolume, "PCMVOLUME", 80.0)                                     \
        ELEM_(bool, MuteInput, "MUTEINPUT", true)                                       \
/* Waterfall palette */                                                                 \
        ELEM_(RGB, cfgpal0, "PALETTE0", { 0, 0, 0 })                                    \
        ELEM_(RGB, cfgpal1, "PALETTE1", { 0, 0, 62 })                                   \
        ELEM_(RGB, cfgpal2, "PALETTE2", { 0, 0, 126 })                                  \
        ELEM_(RGB, cfgpal3, "PALETTE3", { 0, 0, 214 })                                  \
        ELEM_(RGB, cfgpal4, "PALETTE4", { 145, 142, 96 })                               \
        ELEM_(RGB, cfgpal5, "PALETTE5", { 181, 184, 48 })                               \
        ELEM_(RGB, cfgpal6, "PALETTE6", { 223, 226, 105 })                              \
        ELEM_(RGB, cfgpal7, "PALETTE7", { 254, 254, 4 })                                \
        ELEM_(RGB, cfgpal8, "PALETTE8", { 255, 58, 0 })                                 \
/* Palettes for macro button groups */                                                  \
        ELEM_(bool, useGroupColors, "USEGROUPCOLORS", true)                             \
        ELEM_(RGB, btnGroup1, "FKEYGROUP1", { 80, 144, 144 })                           \
        ELEM_(RGB, btnGroup2, "FKEYGROUP2", { 144, 80, 80 })                            \
        ELEM_(RGB, btnGroup3, "FKEYGROUP3", { 80, 80, 144 })                            \
        ELEM_(RGB, btnFkeyTextColor, "FKEYTEXTCOLOR", { 255, 255, 255 })                \
/* RX / TX / Waterfall text widgets */                                                  \
        ELEM_(Fl_Font, RxFontnbr, "RXFONTNBR", FL_SCREEN)                               \
        ELEM_(int, RxFontsize, "RXFONTSIZE", 16)                                        \
        ELEM_(Fl_Color, RxFontcolor, "RXFNTCOLOR", FL_BLACK)                            \
        ELEM_(Fl_Font, TxFontnbr, "TXFONTNBR", FL_SCREEN)                               \
        ELEM_(int, TxFontsize, "TXFONTSIZE", 16)                                        \
        ELEM_(Fl_Color, TxFontcolor, "TXFNTCOLOR", FL_BLACK)                            \
        ELEM_(RGB, RxColor, "RXFONTCOLOR", { 255, 242, 190 })                           \
        ELEM_(RGB, TxColor, "TXFONTCOLOR", { 200, 235, 255 })                           \
        ELEM_(Fl_Color, XMITcolor, "XMITCOLOR", FL_RED)                                 \
        ELEM_(Fl_Color, CTRLcolor, "CTRLCOLOR", FL_DARK_GREEN)                          \
        ELEM_(Fl_Color, SKIPcolor, "SKIPCOLOR", FL_BLUE)                                \
        ELEM_(Fl_Color, ALTRcolor, "ALTRCOLOR", FL_DARK_MAGENTA)                        \
        ELEM_(Fl_Font, WaterfallFontnbr, "WATERFALLFONTNBR", FL_SCREEN)                 \
        ELEM_(int, WaterfallFontsize, "WATERFALLFONTSIZE", 12)                          \
        ELEM_(std::string, ui_scheme, "UISCHEME", "gtk+")                               \
        ELEM_(bool, wf_audioscale, "WFAUDIOSCALE", false)								\
/* Freq Display colors */                                                               \
        ELEM_(RGB, FDbackground, "FDBACKGROUND", { 0, 0, 0 })                     		\
        ELEM_(RGB, FDforeground, "FDFOREGROUND", { 0, 200, 0 })                     	\
/* PSK Viewer */                                                                        \
        ELEM_(bool, VIEWERmarquee, "VIEWERMARQUEE", true)                               \
        ELEM_(bool, VIEWERshowfreq, "VIEWERSHOWFREQ", true)                             \
        ELEM_(int, VIEWERstart, "VIEWERSTART", 500)                                     \
        ELEM_(int, VIEWERchannels, "VIEWERCHANNELS", 20)                                \
        ELEM_(double, VIEWERsquelch, "VIEWERSQUELCH", 10.0)                             \
        ELEM_(int, VIEWERtimeout, "VIEWERTIMEOUT", 15)                                  \
/* XML-RPC/ARQ servers */                                                               \
        ELEM_(std::string, xmlrpc_address, "", "127.0.0.1")                             \
        ELEM_(std::string, xmlrpc_port, "", "7362")                                     \
        ELEM_(std::string, xmlrpc_allow, "", "")                                        \
        ELEM_(std::string, xmlrpc_deny, "", "")                                         \
        ELEM_(int, rx_msgid, "", 9876)                                                  \
        ELEM_(int, tx_msgid, "", 6789)                                                  \
        ELEM_(std::string, arq_address, "", "127.0.0.1")                                \
        ELEM_(std::string, arq_port, "", "3122")

// declare the struct
#define ELEM_DECLARE_CONFIGURATION(type_, var_, tag_, ...) type_ var_;
#undef ELEM_
#define ELEM_ ELEM_DECLARE_CONFIGURATION
struct configuration
{
	CONFIG_LIST

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
	std::string strBaudRate();
	int  BaudRate(size_t);
};

extern configuration progdefaults;

extern void mixerInputs();
extern void enableMixer(bool);

enum { SAMPLE_RATE_UNSET = -1, SAMPLE_RATE_AUTO, SAMPLE_RATE_NATIVE, SAMPLE_RATE_OTHER };

#endif
