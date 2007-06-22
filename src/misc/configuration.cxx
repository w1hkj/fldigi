#include "configuration.h"
#include "Config.h"

#ifndef NOHAMLIB
	#include "hamlib.h"
	#include "rigclass.h"
#endif

#include "rigMEM.h"
#include "rigio.h"

#include "modeIO.h"

#include <iostream>
#include <fstream>

configuration progdefaults = {
	false,			// bool		changed;
	25.0,			// double 	squelch;
	-10.0,			// double	wfRefLevel;
	40.0,			// double	wfAmpSpan;
	1000,			// int		CWsweetspot;
	1000,			// int		RTTYsweetspot;
	1000,			// int		PSKsweetspot;
	true,			// bool		StartAtSweetSpot;
	true,			// bool		PSKmailSweetSpot;
// RTTY
	25.0,			// double		rtty_squelch;
	3,				// int			rtty_shift; = 170
	0,				// int			rtty_baud; = 45
	0,				// int 			rtty_bits; = 5
	PARITY_NONE,	// RTTY_PARITY	rtty_parity;
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

// FELD-HELL
	false,			// bool		FELD_IDLE;
// OLIVIA
	2,				// int		oliviatones;
	2,				// int		oliviabw;
// DOMINOEX
	2.0,			// double	DOMINOEX_BW;
//
	0, 				// int 		Font
	0, 				// int 		Fontsize
	0,				// int 		Fontcolor
	0,				// uchar 	red
	255,			// uchar 	green
	255,			// uchar 	blue
	0,				// bool 	MultiColorWF;
	1,				// int  	wfPreFilter == Blackman
	true,			// bool 	UseCursorLines;
	true,			// bool 	UseCursorCenterLine;
	true,			// bool 	UseBWTracks;
	4,				// int		feldfontnbr;
	false,			// bool		viewXmtSignal;
	false,			// bool		sendid;
	false,			// bool		macroid;
	0,				// int		QRZ;
//
	true,			// bool		btnusb;
	0, 				// int 		btnPTTis
	0, 				// int 		btnRTSDTRis
	0, 				// int 		btnPTTREVis
	false,			// bool		RTSptt;
	false,			// bool		DTRptt;
	false,			// bool		RTSplus;
	false,			// bool		DTRplus;
	0,				// int 		choiceHAMLIBis
	0,				// int 		chkUSEMEMMAPis
	0,				// int 		chkUSEHAMLIBis
	0,				// int		chkUSERIGCATis
	"",				// string	HamRigName
	"/dev/ttyS0",	// string	HamRigDevice
	1,				// int		HamRigBaudrate
	"/dev/ttyS1",	// string	CWFSKport
//
	"",				// myCall
	"",				// myName
	"",				// myQth
	"",				// myLoc
	"/dev/ttyS0",	// PTTdev
	"fldigi ",		// secondary text
// Sound card
	0,			// int		btnAudioIOis
	"/dev/dsp",		// string	SCdevice;
	"/dev/dsp",		// string	OSSdevice;
	"/dev/dsp",		// string	PAdevice;
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
	0.8,			// double	RcvMixer;
	0.6,			// double	XmtMixer;
	false,			// bool		MicIn;
	true,			// bool		LineIn;
	false,			// bool		EnableMixer;
	true,			// bool 	MuteInput;
	50.0,			// double	PCMvolume
	{{  0,  0,  0},{  0,  0,  62},{  0,  0,126}, // default palette
	 {  0,  0,214},{145,142,  96},{181,184, 48},
	 {223,226,105},{254,254,   4},{255, 58,  0} }
};

char *szBaudRates[] = {
	"", 
	"300","600","1200","2400",
	"4800","9600","19200","38400",
	"57600","115200","230400","460800"};
	
const char *szBands[] = {
	"",
	"1830", "3580", "7030", "7070", "10138",
	"14070", "18100", "21070", "21080", "24920", "28070", "28120", 0};
	
void writeXMLint(ofstream &f, const char * tag,  int val)
{
	f << "<" << tag << ">\n";
	f << "\t<INT>" << val << "</INT>\n";
	f << "</" << tag << ">\n";
}

void writeXMLdbl(ofstream &f, const char * tag, double val)
{
	f << "<" << tag << ">\n";
	f << "\t<DBL>" << val << "</DBL>\n";
	f << "</" << tag << ">\n";
}

void writeXMLstr(ofstream &f, const char * tag, string val)
{
	f << "<" << tag << ">\n";
	f << "\t<STR>" << val.c_str() << "</STR>\n";
	f << "</" << tag << ">\n";
}

void writeXMLbool(ofstream &f, const char * tag, bool val)
{
	f << "<" << tag << ">\n";
	f << "\t<BOOL>" << val << "</BOOL>\n";
	f << "</" << tag << ">\n";
}

void writeXMLtriad(ofstream &f, int r, int g, int b)
{
	f << "\t";
	f << "<R>" << r << "</R>";
	f << "<G>" << g << "</G>";
	f << "<B>" << b << "</B>";
	f << "\n";
}

void configuration::writeDefaultsXML()
{
	string deffname = HomeDir;
	deffname.append("FLDIGI_XML.DEF");
	ofstream f(deffname.c_str(), ios::out);

	f << "<FLDIGI_DEFS>\n";

	writeXMLstr(f, "MYCALL", myCall);
	writeXMLstr(f, "MYNAME", myName);
	writeXMLstr(f, "MYQTH", myQth);
	writeXMLstr(f, "MYLOC", myLocator);

	writeXMLdbl(f, "SQUELCH", squelch);
	writeXMLdbl(f, "WFREFLEVEL", wfRefLevel);
	writeXMLdbl(f, "WFAMPSPAN", wfAmpSpan);
	writeXMLint(f, "FONT", Font);
	writeXMLint(f, "FONTSIZE", FontSize);
	writeXMLint(f, "FONTCOLOR", FontColor);

	writeXMLbool(f, "STARTATSWEETSPOT", StartAtSweetSpot);
	writeXMLbool(f, "PSKMAILSWEETSPOT", PSKmailSweetSpot);
	writeXMLdbl(f, "CWSWEETSPOT", CWsweetspot);
	writeXMLdbl(f, "PSKSWEETSPOT", PSKsweetspot);
	writeXMLdbl(f, "RTTYSWEETSPOT", RTTYsweetspot);
	writeXMLdbl(f, "RTTYSQUELCH", rtty_squelch);	
	writeXMLint(f, "RTTYSHIFT", rtty_shift);
	writeXMLint(f, "RTTYBAUD", rtty_baud);
	writeXMLint(f, "RTTYSHIFT", rtty_shift);
	writeXMLint(f, "RTTYBITS", rtty_bits);
	writeXMLint(f, "RTTYPARITY", rtty_parity);
	writeXMLint(f, "RTTYSTOP", rtty_stop);
	writeXMLbool(f, "RTTYREVERSE", rtty_reverse);
	writeXMLbool(f, "RTTYMSBFIRST", rtty_msbfirst);
	writeXMLbool(f, "RTTYCRCLF", rtty_crcrlf);
	writeXMLbool(f, "RTTYAUTOCRLF", rtty_autocrlf);
	writeXMLint(f, "RTTYAUTOCOUNT", rtty_autocount);
	writeXMLint(f, "RTTYAFCSPEED", rtty_afcspeed);
	writeXMLbool(f, "PREFERXHAIRSCOPE", PreferXhairScope);
	writeXMLbool(f, "PseudoFSK", PseudoFSK);

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
	writeXMLdbl(f, "CWpre", CWpre);
	writeXMLdbl(f, "CWpost", CWpost);
	
	writeXMLint(f, "OLIVIATONES", oliviatones);
	writeXMLint(f, "OLIVIABW", oliviabw);
	writeXMLdbl(f, "DOMINOEXBW", DOMINOEX_BW);
	writeXMLint(f, "FELDFONTNBR", feldfontnbr);
	writeXMLbool(f, "FELDIDLE", FELD_IDLE);

	writeXMLint(f, "WFPREFILTER", wfPreFilter);
	writeXMLbool(f, "USECURSORLINES", UseCursorLines);
	writeXMLbool(f, "USECURSORCENTERLINE", UseCursorCenterLine);
	writeXMLbool(f, "USEBWTRACKS", UseBWTracks);	
	writeXMLbool(f, "VIEWXMTSIGNAL", viewXmtSignal);
	writeXMLbool(f, "SENDID", sendid);
	writeXMLbool(f, "MACROID", macroid);
	writeXMLint(f, "QRZ", QRZ);
	writeXMLbool(f, "BTNUSB", btnusb);
	writeXMLint(f, "BTNPTTIS", btnPTTis);
	writeXMLint(f, "BTNRTSDTRIS", btnRTSDTRis);
	writeXMLint(f, "BTNPTTREVIS", btnPTTREVis);
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
	writeXMLstr(f, "SCDEVICE", SCdevice);
	writeXMLstr(f, "OSSDEVICE", OSSdevice);
	writeXMLstr(f, "PADEVICE", PAdevice);
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
	writeXMLdbl(f, "RCVMIXER", RcvMixer);
	writeXMLdbl(f, "XMTMIXER", XmtMixer);
	writeXMLdbl(f, "PCMVOLUME", PCMvolume);
	writeXMLbool(f, "MICIN", MicIn);
	writeXMLbool(f, "LINEIN", LineIn);
	writeXMLbool(f, "ENABLEMIXER", EnableMixer);
	writeXMLbool(f, "MUTEINPUT", MuteInput);
	f << "<PALETTE>\n";
	for (int i = 0; i < 9; i++)
		writeXMLtriad(f, cfgpal[i].R, cfgpal[i].G, cfgpal[i].B);
	f << "</PALETTE>\n";

//	writeXMLbool(f, "USEFSKKEYLINE", useFSKkeyline);		
//	writeXMLbool(f, "USEFSKKEYLINEDTR", useFSKkeylineDTR);
//	writeXMLbool(f, "FSKISLSB", FSKisLSB);
//	writeXMLbool(f, "RTTYUSB", RTTY_USB);
//	writeXMLbool(f, "USEUART", useUART);	
//	writeXMLbool(f, "USECWKEYLINERTS", useCWkeylineRTS);
//	writeXMLbool(f, "USECWKEYLINEDTR", useCWkeylineDTR);
//	writeXMLstr(f, "CWFSKPORT", CWFSKport);

	f << "</FLDIGI_DEFS>\n";
	f.close();
}
	
void configuration::writeDefaults(ofstream &f)
{
	f << squelch << endl;
	f << rtty_squelch << endl;
	f << rtty_shift << endl;
	f << rtty_baud << endl;
	f << rtty_bits << endl;
	f << rtty_parity << endl;
	f << rtty_stop << endl;
	f << rtty_reverse << endl;
	f << rtty_msbfirst << endl;
	f << oliviatones << endl;
	f << oliviabw << endl;
	f << Font << endl;
	f << FontSize << endl;
	f << FontColor << endl;
	f << btnPTTis << endl;
	f << btnRTSDTRis << endl;
	f << btnPTTREVis << endl;
	f << choiceHAMLIBis << endl;
	f << chkUSEMEMMAPis << endl;
	f << chkUSEHAMLIBis << endl;
	f << HamRigBaudrate << endl;
	f << RX_corr << endl;
	f << TX_corr << endl;
	f << myCall.c_str() << endl;
	f << myName.c_str() << endl;
	f << myQth.c_str() << endl;
	f << myLocator.c_str() << endl;
	f << PTTdev.c_str() << endl;
	f << HamRigName.c_str() << endl;
	f << HamRigDevice.c_str() << endl;
	f << SCdevice.c_str() << endl;
	f << secText.c_str() << endl;
	f << red << endl;
	f << green << endl;
	f << blue << endl;
	f << wfPreFilter << endl;
	f << wfRefLevel << endl;
	f << wfAmpSpan << endl;
	f << MultiColorWF << endl;
	f << UseCursorLines << endl;
	f << UseBWTracks << endl;
	f << CWsweetspot << endl;
	f << RTTYsweetspot << endl;
	f << PSKsweetspot << endl;
	f << UseCursorCenterLine << endl;
	f << useCWkeylineRTS << endl;
	f << useFSKkeyline << endl;
	f << CWFSKport << endl;
	f << FSKisLSB << endl;
	f << feldfontnbr << endl;
	f << rtty_crcrlf << endl;
	f << rtty_autocrlf << endl;
	f << rtty_autocount << endl;
	f << FELD_IDLE << endl;
	f << QRZ << endl;
	f << RTTY_USB << endl;
	f << useUART << endl;
	f << viewXmtSignal << endl;
	f << sendid << endl;
	f << chkUSERIGCATis << endl;
	f << useCWkeylineDTR << endl;
	f << useFSKkeylineDTR << endl;
	f << StartAtSweetSpot << endl;
	f << rtty_afcspeed << endl;
	f << PreferXhairScope << endl;
	f << RTSptt << endl;
	f << DTRptt << endl;
	f << RTSplus << endl;
	f << DTRplus << endl;
	f << cfgpal[0].R << endl; f << cfgpal[0].G << endl; f << cfgpal[0].B << endl;
	f << cfgpal[1].R << endl; f << cfgpal[1].G << endl; f << cfgpal[1].B << endl;
	f << cfgpal[2].R << endl; f << cfgpal[2].G << endl; f << cfgpal[2].B << endl;
	f << cfgpal[3].R << endl; f << cfgpal[3].G << endl; f << cfgpal[3].B << endl;
	f << cfgpal[4].R << endl; f << cfgpal[4].G << endl; f << cfgpal[4].B << endl;
	f << cfgpal[5].R << endl; f << cfgpal[5].G << endl; f << cfgpal[5].B << endl;
	f << cfgpal[6].R << endl; f << cfgpal[6].G << endl; f << cfgpal[6].B << endl;
	f << cfgpal[7].R << endl; f << cfgpal[7].G << endl; f << cfgpal[7].B << endl;
	f << cfgpal[8].R << endl; f << cfgpal[8].G << endl; f << cfgpal[8].B << endl;
	f << MicIn << endl;
	f << RcvMixer << endl;
	f << XmtMixer << endl;
	f << EnableMixer << endl;
	f << PCMvolume << endl;
	f << DOMINOEX_BW << endl;
	f << LineIn << endl;
	f << CWweight << endl;
	f << CWspeed << endl;
	f << CWbandwidth << endl;
	f << CWtrack << endl;
	f << CWrange << endl;
	f << MuteInput << endl;
	f << CWlowerlimit << endl;
	f << CWupperlimit << endl;
	f << CWrisetime << endl;
	f << CWdash2dot << endl;
	f << defCWspeed << endl;
	f << QSK << endl;
	f << CWpre << endl;
	f << CWpost << endl;
	f << PseudoFSK << endl;
	f << PSKmailSweetSpot << endl;
	f << TxOffset << endl;
	f << MXdevice << endl;
	f << btnAudioIOis << endl;
	f << OSSdevice << endl;
	f << PAdevice << endl;
}

void configuration::readDefaults(ifstream &f)
{
	char buff[255];

	f >> squelch;
	f >> rtty_squelch;
	f >> rtty_shift;
	f >> rtty_baud;
	f >> rtty_bits;
	f >> rtty_parity;
	f >> rtty_stop;
	f >> rtty_reverse;
	f >> rtty_msbfirst;
	f >> oliviatones;
	f >> oliviabw;
	f >> Font;
	f >> FontSize;
	f >> FontColor;
	f >> btnPTTis;
	f >> btnRTSDTRis;
	f >> btnPTTREVis;
	f >> choiceHAMLIBis;
	f >> chkUSEMEMMAPis;
	f >> chkUSEHAMLIBis;
	f >> HamRigBaudrate;
	f >> RX_corr;
	f >> TX_corr;

	f.ignore();

	f.getline(buff,255); myCall = buff;
	f.getline(buff,255); myName = buff;;
	f.getline(buff,255); myQth = buff;
	f.getline(buff,255); myLocator = buff;
	f.getline(buff,255); PTTdev = buff;
	f.getline(buff,255); HamRigName = buff;
	f.getline(buff,255); HamRigDevice = buff;
	f.getline(buff,255); SCdevice = buff;
	f.getline(buff,255); secText = buff;
	f >> red;
	f >> green;
	f >> blue;
	f >> wfPreFilter;
	f >> wfRefLevel;
	f >> wfAmpSpan;
	f >> MultiColorWF;
	f >> UseCursorLines;
	f >> UseBWTracks;
	f >> CWsweetspot;
	f >> RTTYsweetspot;
	f >> PSKsweetspot;
	f >> UseCursorCenterLine;

	f >> useCWkeylineRTS;
	f >> useFSKkeyline;
	f.ignore();
	f.getline(buff,255); CWFSKport = buff;
	f >> FSKisLSB;
	f >> feldfontnbr;
	f >> rtty_crcrlf;
	f >> rtty_autocrlf;
	f >> rtty_autocount;
	f >> FELD_IDLE;
	f >> QRZ;
	f >> RTTY_USB;
	f >> useUART;
	f >> viewXmtSignal;
	f >> sendid;
	f >> chkUSERIGCATis;
	f >> useCWkeylineDTR;
	f >> useFSKkeylineDTR;
	f >> StartAtSweetSpot;
	f >> rtty_afcspeed;
	f >> PreferXhairScope;
	f >> RTSptt;
	f >> DTRptt;
	f >> RTSplus;
	f >> DTRplus;
	f >> cfgpal[0].R; f >> cfgpal[0].G; f >> cfgpal[0].B;
	f >> cfgpal[1].R; f >> cfgpal[1].G; f >> cfgpal[1].B;
	f >> cfgpal[2].R; f >> cfgpal[2].G; f >> cfgpal[2].B;
	f >> cfgpal[3].R; f >> cfgpal[3].G; f >> cfgpal[3].B;
	f >> cfgpal[4].R; f >> cfgpal[4].G; f >> cfgpal[4].B;
	f >> cfgpal[5].R; f >> cfgpal[5].G; f >> cfgpal[5].B;
	f >> cfgpal[6].R; f >> cfgpal[6].G; f >> cfgpal[6].B;
	f >> cfgpal[7].R; f >> cfgpal[7].G; f >> cfgpal[7].B;
	f >> cfgpal[8].R; f >> cfgpal[8].G; f >> cfgpal[8].B;
	f >> MicIn;
	f >> RcvMixer;
	f >> XmtMixer;
	f >> EnableMixer;
	f >> PCMvolume;
	f >> DOMINOEX_BW;
	f >> LineIn;
	f >> CWweight;
	f >> CWspeed;
	f >> CWbandwidth;
	f >> CWtrack;
	f >> CWrange;
	f >> MuteInput;
	f >> CWlowerlimit;
	f >> CWupperlimit;
	f >> CWrisetime;
	f >> CWdash2dot;
	f >> defCWspeed;
	f >> QSK;
	f >> CWpre;
	f >> CWpost;
	f >> PseudoFSK;
	f >> PSKmailSweetSpot;
	f >> TxOffset;
	f >> MXdevice;
	f >> btnAudioIOis;
	f >> OSSdevice;
	getline(f >> ws, PAdevice);
}

void configuration::loadDefaults() {
	Fl::lock();
	
// RTTY
	selShift->value(rtty_shift);
	selBaud->value(rtty_baud);
	selBits->value(rtty_bits);
	switch (rtty_parity) {
		case PARITY_NONE : selParity->value(0); break;
		case PARITY_EVEN : selParity->value(1); break;
		case PARITY_ODD :  selParity->value(2); break;
		case PARITY_ZERO : selParity->value(3); break;
		case PARITY_ONE :  selParity->value(4); break;
		default :          selParity->value(0); break;
	}
//	chkMsbFirst->value(rtty_msbfirst);
	selStopBits->value(rtty_stop);
	btnCRCRLF->value(rtty_crcrlf);
	btnAUTOCRLF->value(rtty_autocrlf);
	cntrAUTOCRLF->value(rtty_autocount);
	chkPseudoFSK->value(PseudoFSK);
	
	for (int i = 0; i < 3; i++)
		if (rtty_afcspeed == i)
			btnRTTYafc[i]->value(1);
		else
			btnRTTYafc[i]->value(0);
	btnPreferXhairScope->value(PreferXhairScope);
// OLIVIA
	mnuOlivia_Tones->value(oliviatones);
	mnuOlivia_Bandwidth->value(oliviabw);

	Fl::unlock();
}

void configuration::storeDefaults() {
	Fl::lock();
	
// RTTY
	rtty_shift = selShift->value();
	rtty_baud = selBaud->value();
	rtty_bits = selBits->value();
	if (rtty_bits == 0)
		rtty_parity = PARITY_NONE;
	else
		switch (selParity->value()) {
			case 0 : rtty_parity = PARITY_NONE; break;
			case 1 : rtty_parity = PARITY_EVEN; break;
			case 2 : rtty_parity = PARITY_ODD; break;
			case 3 : rtty_parity = PARITY_ZERO; break;
			case 4 : rtty_parity = PARITY_ONE; break;
			default : rtty_parity = PARITY_NONE; break;
		}
//	rtty_msbfirst = chkMsbFirst->value();
	rtty_stop = selStopBits->value();
	rtty_crcrlf = btnCRCRLF->value();
	rtty_autocrlf = btnAUTOCRLF->value();
	rtty_autocount = (int)cntrAUTOCRLF->value();
// OLIVIA
	oliviatones = mnuOlivia_Tones->value();
	oliviabw = mnuOlivia_Bandwidth->value();

	Fl::unlock();
}

void configuration::saveDefaults() {
	Fl::lock();
// strings
	myCall = inpMyCallsign->value();
	myName = inpMyName->value();
	myQth  = inpMyQth->value();
	myLocator = inpMyLocator->value();
	secText = txtSecondary->value();
	PTTdev = inpTTYdev->value();

	squelch = sldrSquelch->value();
	for (int i = 0; i < 9; i++) {
		progdefaults.cfgpal[i].R =  palette[i].R;
		progdefaults.cfgpal[i].G =  palette[i].G;
		progdefaults.cfgpal[i].B =  palette[i].B;
	}
	Fl::unlock();
	
	string deffname = HomeDir;
	deffname.append("fldigi.def");
	ofstream deffile(deffname.c_str(), ios::out);
	writeDefaults(deffile);

	deffile.close();
	
	writeDefaultsXML();
	
	changed = false;
}

int configuration::openDefaults() {
#ifndef NOHAMLIB	
	getRigs();
#endif	
	string deffname = HomeDir;
	deffname.append("fldigi.def");
	ifstream deffile(deffname.c_str(), ios::in);

	if (deffile) {
		readDefaults(deffile);
		deffile.close();
		
		Fl::lock();
			inpMyCallsign->value(myCall.c_str());
			inpMyName->value(myName.c_str());
			inpMyQth->value(myQth.c_str());
			inpMyLocator->value(myLocator.c_str());
			UseLeadingZeros = btnUseLeadingZeros->value();
			ContestStart = (int)nbrContestStart->value();
			ContestDigits = (int)nbrContestDigits->value();
			
			txtSecondary->value(secText.c_str());
			valDominoEX_BW->value(DOMINOEX_BW);
			
			for (int i = 0; i < 5; i++)
				btnPTT[i]->value(0);
			btnPTT[btnPTTis]->value(1);
#ifdef NOHAMLIB
			btnPTT[1]->hide();
#endif
			btnRTSptt->value(RTSptt);
			btnDTRptt->value(DTRptt);
			btnRTSplusV->value(RTSplus);
			btnDTRplusV->value(DTRplus);

			inpTTYdev->value(PTTdev.c_str());

			if(chkUSEMEMMAPis) {
				chkUSEMEMMAP->value(1);
#ifndef NOHAMLIB
				chkUSEHAMLIB->value(0);
#endif
				chkUSERIGCAT->value(0);
			} else if (chkUSEHAMLIBis) {
				chkUSEMEMMAP->value(0);
#ifndef NOHAMLIB
				chkUSEHAMLIB->value(1);
#endif				
				chkUSERIGCAT->value(0);
			} else if (chkUSERIGCATis) {
				chkUSEMEMMAP->value(0);
				chkUSEHAMLIB->value(0);
				chkUSERIGCAT->value(1);
			} else {
				chkUSEMEMMAP->value(0);
#ifndef NOHAMLIB
				chkUSEHAMLIB->value(0);
				chkUSERIGCAT->value(0);
#endif				
			}
#ifndef NOHAMLIB
			cboHamlibRig->value(HamRigName.c_str());
#else
			chkUSEHAMLIB->hide();
#endif
			inpRIGdev->value(HamRigDevice.c_str());
			mnuBaudRate->value(HamRigBaudrate);

			sldrSquelch->value(squelch);
			
			valCWsweetspot->value(CWsweetspot);
			valRTTYsweetspot->value(RTTYsweetspot);
			valPSKsweetspot->value(PSKsweetspot);
			btnStartAtSweetSpot->value(StartAtSweetSpot);
			btnPSKmailSweetSpot->value(PSKmailSweetSpot);
			
//			txtCWFSKport->value(CWFSKport.c_str());

//			btnUseCWkeylineRTS->value(useCWkeylineRTS);
//			btnUseCWkeylineDTR->value(useCWkeylineDTR);

//			btnUseFSKkeyline->value(useFSKkeyline);
//			btnUseFSKkeylineDTR->value(useFSKkeylineDTR);

//			btnFSKisLSB->value(FSKisLSB);

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
			
			selHellFont->value(feldfontnbr);
			btnFeldHellIdle->value(FELD_IDLE);
			
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
			if (QRZ == 0)
				btnQRZnotavailable->value(1);
			else if (QRZ == 1)
				btnQRZsocket->value(1);
			else
				btnQRZcdrom->value(1);
			
			btnRTTY_USB->value(RTTY_USB);
//			btnUSE_UART->value(useUART);
			btnViewXmtSignal->value(viewXmtSignal);
			btnsendid->value(sendid);
			
			valRcvMixer->value(RcvMixer);
			valXmtMixer->value(XmtMixer);
			valPCMvolume->value(PCMvolume);
                        btnMicIn->value(MicIn);
                        btnLineIn->value(LineIn);

                        btnAudioIO[0]->value(0);
                        btnAudioIO[0]->value(0);
                        btnAudioIO[btnAudioIOis]->value(1);

                        menuOSSDev->value(OSSdevice.c_str());
                        menuPADev->value(PAdevice.c_str());
                        if (btnAudioIOis == 1)
                            menuPADev->activate();


                        btnMixer->value(EnableMixer);
                        resetMixerControls();
                        menuMix->value(MXdevice.c_str());

			cntRxRateCorr->value(RX_corr);
			cntTxRateCorr->value(TX_corr);
			cntTxOffset->value(TxOffset);
			
		Fl::unlock();

		enableMixer(EnableMixer);
		
		ReceiveText->setFont((Fl_Font)Font);
		ReceiveText->setFontSize(FontSize);
		ReceiveText->setFontColor((Fl_Color)FontColor);
	
		TransmitText->setFont((Fl_Font)Font);
		TransmitText->setFontSize(FontSize);

		wf->setPrefilter(wfPreFilter);

//		if (useCWkeylineRTS || useCWkeylineDTR ) //||
//			useFSKkeyline || useFSKkeylineDTR ||
//			useUART )
//			if (!KeyLine) KeyLine = new modeIO();

		for (int i = 0; i < 9; i++) {
			palette[i].R = (uchar)cfgpal[i].R;
			palette[i].G = (uchar)cfgpal[i].G;
			palette[i].B = (uchar)cfgpal[i].B;
		}
		wf->setcolors();
		setColorButtons();

		return 1;
	} else {
		for (int i = 0; i < 9; i++) {
			palette[i].R = (uchar)cfgpal[i].R;
			palette[i].G = (uchar)cfgpal[i].G;
			palette[i].B = (uchar)cfgpal[i].B;
		}
		wf->setcolors();
		setColorButtons();
	}
	return 0;
}

void configuration::initOperator() {
	Fl::lock();
		myCall = inpMyCallsign->value();
		myName = inpMyName->value();
		myQth  = inpMyQth->value();
		myLocator = inpMyLocator->value();
		UseLeadingZeros = btnUseLeadingZeros->value();
		ContestStart = (int)nbrContestStart->value();
		ContestDigits = (int)nbrContestDigits->value();
	Fl::unlock();
}

void configuration::initInterface() {
	initOperator();


// close down any possible rig interface threads
#ifndef NOHAMLIB
		hamlib_close();
#endif
		rigMEM_close();
		rigCAT_close();

	Fl::lock();
		btnPTTis = (btnPTT[0]->value() ? 0 :
					btnPTT[1]->value() ? 1 :
					btnPTT[2]->value() ? 2 :
					btnPTT[3]->value() ? 3 :
					btnPTT[4]->value() ? 4 : 5);
					
		RTSptt = btnRTSptt->value();
		DTRptt = btnDTRptt->value();
		RTSplus = btnRTSplusV->value();
		DTRplus = btnDTRplusV->value();
		
		PTTdev = inpTTYdev->value();
	Fl::unlock();
		push2talk->reset(
			progdefaults.btnPTTis,
			progdefaults.btnRTSDTRis,
			progdefaults.btnPTTREVis);
	Fl::lock();	
#ifndef NOHAMLIB
		chkUSEHAMLIBis = chkUSEHAMLIB->value();
#endif		
		chkUSEMEMMAPis = chkUSEMEMMAP->value();
		chkUSERIGCATis = chkUSERIGCAT->value();

#ifndef NOHAMLIB
		HamRigName = cboHamlibRig->value();
		HamRigDevice = inpRIGdev->value();
		HamRigBaudrate = mnuBaudRate->value();
#else
		cboHamlibRig->hide();
		inpRIGdev->hide();
		mnuBaudRate->hide();
#endif		
	Fl::unlock();
		
	if (chkUSEMEMMAPis) {// start the memory mapped i/o thread
		btnPTT[2]->activate();
		rigMEM_init();
		wf->setQSY(1);
		activate_rig_menu_item(false);
	} else if (chkUSERIGCATis) { // start the rigCAT thread
		btnPTT[3]->activate();
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
#ifndef NOHAMLIB
	} else if (chkUSEHAMLIBis) { // start the hamlib thread
		btnPTT[1]->activate();
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
	wf->setRefLevel();
	wf->setAmpSpan();
		
}

string configuration::strBaudRate()
{
	return (szBaudRates[HamRigBaudrate + 1]);
}

#ifndef NOHAMLIB
void configuration::getRigs() {
list<string>::iterator pstr;
	xcvr->get_rignames();
	pstr = (xcvr->rignames).begin();
Fl::lock();
	while (pstr != (xcvr->rignames).end()) {
		cboHamlibRig->add((*pstr).c_str());
		++pstr;
	}
Fl::unlock();
}
#endif

