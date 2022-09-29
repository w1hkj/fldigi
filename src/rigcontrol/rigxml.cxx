// ----------------------------------------------------------------------------
// rigxml.cxx - parse a rig control xml file
//
// Copyright (C) 2007-2009
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <fstream>
#include <string>
#include <list>

#include <FL/filename.H>

#include "gettext.h"
#include "rigio.h"
#include "rigxml.h"
#include "rigsupport.h"
#ifdef RIGCATTEST
	#include "rigCAT.h"
#else
	#include "main.h"
#endif
#include "debug.h"

#include "configuration.h"
#include "fileselect.h"
#include "confdialog.h"

#include "icons.h"
#include "fl_digi.h"

//#define DEBUGXML 1

void parseRIGDEF(size_t &);
void parseRIG(size_t &);
void parseCOMMAND(size_t &);
void parseREPLY(size_t &);
void parseMODES(size_t &);
void parseBANDWIDTHS(size_t &);
void parseBWCMD(size_t &);
void parseBWREPLY(size_t &);
void parseMODECMD(size_t &);
void parseMODEREPLY(size_t &);
void parseTITLE(size_t &);
void parseLSBMODES(size_t &);
void parseDISCARD(size_t &);
void parseDEBUG(size_t &);
void parseNOSERIAL(size_t &);
void parseASCII(size_t &);

void parseWRITE_DELAY(size_t &);
void parseINIT_DELAY(size_t &);
void parseWAIT_FOR_DEVICE(size_t &);
void parsePOST_WRITE_DELAY(size_t &);
void parseRETRIES(size_t &);
void parseTIMEOUT(size_t &);
void parseBAUDRATE(size_t &);
void parseSTOPBITS(size_t &);
void parseRTSCTS(size_t &);
void parseCMDPTT(size_t &);
void parseRTSPLUS(size_t &);
void parseDTRPLUS(size_t &);
void parseRTSPTT(size_t &);
void parseDTRPTT(size_t &);
void parseRESTORE_TIO(size_t &);
void parseECHO(size_t &);
void parseVSP(size_t &);
void parseLOGSTR(size_t &);
void parsePOLLINT(size_t &);
void parseSMETER(size_t &);
void parsePMETER(size_t &);
void parseNOTCH(size_t &);
void parsePWRLEVEL(size_t &);

void parseIOSsymbol(size_t &);
void parseIOSsize(size_t &);
void parseIOSbytes(size_t &);
void parseIOSbyte(size_t &);
void parseIOSdata(size_t &);
void parseIOSinfo(size_t &);
void parseIOSok(size_t &);
void parseIOSbad(size_t &);
void parseIOSstring(size_t &);
void parseIOSint(size_t &);
void parseIOSfill(size_t &);

void parseDTYPE(size_t &);
void parseDSIZE(size_t &);
void parseDMAX(size_t &);
void parseDMIN(size_t &);
void parseDRESOL(size_t &);
void parseDREV(size_t &);
void parseDMAKS(size_t &);
void parseDSHIFT(size_t &);

void print(size_t &);

std::list<XMLIOS>	commands;
std::list<XMLIOS>	reply;
std::list<MODE> 		lmodes;
std::list<BW> 		lbws;
std::list<BW>		lbwCMD;
std::list<BW>		lbwREPLY;
std::list<MODE>		lmodeCMD;
std::list<MODE>		lmodeREPLY;
std::list<std::string> 	LSBmodes;

XMLRIG xmlrig;

XMLIOS iosTemp;

std::string strXML;

TAGS rigdeftags[] = {
	{"<RIGDEF",		parseRIGDEF},
	{"<RIG", 		parseRIG},
	{"<COMMAND",	parseCOMMAND},
	{"<REPLY",		parseREPLY},
	{"<BANDWIDTHS", parseBANDWIDTHS},
	{"<BW-CMD",		parseBWCMD},
	{"<BW-REPLY",	parseBWREPLY},
	{"<MODES",		parseMODES},
	{"<MODE-CMD",	parseMODECMD},
	{"<MODE-REPLY", parseMODEREPLY},
	{"<TITLE",		parseTITLE},
	{"<LSBMODES",	parseLSBMODES},
	{"<PROGRAMMER", parseDISCARD},
	{"<STATUS",		parseDISCARD},
	{"<WRITE_DELAY", parseWRITE_DELAY},
	{"<INIT_DELAY", parseINIT_DELAY},
	{"<POST_WRITE_DELAY", parsePOST_WRITE_DELAY},
	{"<RETRIES", parseRETRIES},
	{"<TIMEOUT", parseTIMEOUT},
	{"<BAUDRATE", parseBAUDRATE},
	{"<RTSCTS", parseRTSCTS},
	{"<RTSPLUS", parseRTSPLUS},
	{"<DTRPLUS", parseDTRPLUS},
	{"<RTSPTT", parseRTSPTT},
	{"<DTRPTT", parseDTRPTT},
	{"<WAIT_FOR_DEVICE", parseWAIT_FOR_DEVICE},
	{"<RESTORE_TIO", parseRESTORE_TIO},
	{"<ECHO", parseECHO},
	{"<CMDPTT", parseCMDPTT},
	{"<STOPBITS", parseSTOPBITS},
	{"<VSP", parseVSP},
	{"<LOGSTR", parseLOGSTR},
	{"<POLLINT", parsePOLLINT},
	{"<SMETER", parseSMETER},
	{"<PMETER", parsePMETER},
	{"<NOTCH", parseNOTCH},
	{"<PWRLEVEL", parsePWRLEVEL},
	{"<DEBUG", parseDEBUG},
	{"<NOSERIAL", parseNOSERIAL},
	{"<ASCII", parseASCII},
	{0, 0}
};

TAGS commandtags[] = {
	{"<SIZE",	parseIOSsize},
	{"<SYMBOL",	parseIOSsymbol},
	{"<BYTES",	parseIOSbytes},
	{"<BYTE",	parseIOSbyte},
	{"<DATA",	parseIOSdata},
	{"<STRING", parseIOSstring},
	{"<INT", 	parseIOSint},
	{"<INFO",	parseIOSinfo},
	{"<OK",		parseIOSok},
	{"<BAD",	parseIOSbad},
	{0,0}
};

TAGS replytags[] = {
	{"<SIZE",	parseIOSsize},
	{"<SYMBOL",	parseIOSsymbol},
	{"<BYTES",	parseIOSbytes},
	{"<BYTE",	parseIOSbyte},
	{"<DATA",	parseIOSdata},
	{"<STRING", parseIOSstring},
	{"<INT",	parseIOSint},
	{"<FILL",	parseIOSfill},
	{0,0}
};

TAGS datatags[] = {
	{"<DTYPE",	parseDTYPE},
	{"<SIZE",	parseDSIZE},
	{"<MAX",	parseDMAX},
	{"<MIN",	parseDMIN},
	{"<RESOL",	parseDRESOL},
	{"<REV",	parseDREV},
	{"<MASK",	parseDMAKS},
	{"<SHIFT",	parseDSHIFT},
	{0,0}
}
;

//=====================================================================

void print(size_t &p0, int indent)
{
#ifdef DEBUGXML
	std::string istr(indent, '\t');
	size_t tend = strXML.find(">", p0);
	LOG_INFO("%s%s", istr.c_str(), strXML.substr(p0, tend - p0 + 1).c_str());
#endif
}

size_t tagEnd(size_t p0)
{
	size_t p1, p2, p3;
	p1 = p0;
	std::string strtag = "</";
	p2 = strXML.find(">", p0);
	p3 = strXML.find(" ", p0);
	if (p2 == std::string::npos) {
		return p2;
	}
	if (p3 < p2)
		p2 = p3;
	strtag.append(strXML.substr(p1 + 1, p2 - p1 - 1));
	strtag.append(">");
	p3 = strXML.find(strtag, p1);
	return p3;
}

size_t nextTag(size_t p0)
{
	p0 = strXML.find("<", p0+1);
	return p0;
}

std::string getElement(size_t p0)
{
	size_t p1 = strXML.find(">",p0),
		   p2 = nextTag(p1+1);
	if (p1 == std::string::npos || p2 == std::string::npos)
		return "";
	p1++; p2--;
	while (p1 < p2 && strXML[p1] == ' ') p1++; // skip leading spaces
	while (p1 < p2 && strXML[p2] == ' ') p2--; // skip trailing spaces
	return strXML.substr(p1, p2 - p1 + 1);
}

int getInt(size_t p0)
{
	std::string stemp = getElement(p0);
	if (stemp.length() == 0)
		return 0;
	return atoi(stemp.c_str());
}

float getFloat(size_t p0)
{
	std::string stemp = getElement(p0);
	if (stemp.length() == 0)
		return 0;
	return atof(stemp.c_str());
}

bool getBool( size_t p0)
{
	std::string stemp = getElement(p0);
	if (stemp.length() == 0)
		return false;
	if (strcasecmp(stemp.c_str(), "true") == 0)
		return true;
	if (stemp == "1") return true;
	return false;
}

char getByte(size_t p0)
{
	unsigned int val;
	if (sscanf( getElement(p0).c_str(), "%x", &val ) != 1)
		return 0;
	return (val & 0xFF);
}

std::string getBytes(size_t p0)
{
	unsigned int val;
	size_t space;
	std::string stemp = getElement(p0);
	std::string s;
	while ( stemp.length() ) {
		if (sscanf( stemp.c_str(), "%x", &val) != 1) {
			s = "";
			return s;
		}
		s += (char)(val & 0xFF);
		space = stemp.find(" ");
		if (space == std::string::npos) break;
		stemp.erase(0, space + 1);
	}
	return s;
}

bool isInt(size_t p0, int &i)
{
//	p0 = nextTag(p0);
	if (strXML.find("<INT", p0) != p0)
		return false;
	i = getInt(p0);
	return true;
}

bool isByte(size_t p0, char &ch)
{
//	p0 = nextTag(p0);
	if (strXML.find("<BYTE", p0) != p0)
		return false;
	ch = getByte(p0);
	return true;
}

bool isBytes( size_t p0, std::string &s )
{
//	p0 = nextTag(p0);
	if (strXML.find ("<BYTES", p0) != p0)
		return false;
	s = getBytes(p0);
	return true;
}

bool isString( size_t p0, std::string &s )
{
//	p0 = nextTag(p0);
	if (strXML.find("<STRING", p0) != p0)
		return false;
	s = getElement(p0);
	return true;
}

bool isSymbol( size_t p0, std::string &s)
{
	if (strXML.find("<SYMBOL", p0) != p0)
		return false;
	s = getElement(p0);
	return true;
}

bool tagIs(size_t &p0, std::string tag)
{
	return (strXML.find(tag,p0) == p0);
}

//---------------------------------------------------------------------
// Parse modesTO definitions
//---------------------------------------------------------------------

void parseMODEdefs(size_t &p0, std::list<MODE> &lmd)
{
	size_t pend = tagEnd(p0);
	size_t elend;
	char ch;
	int n;
	std::string stemp;
	std::string strELEMENT;
	if (pend == std::string::npos) {
		p0++;
		return;
	}
	print(p0,0);
	p0 = nextTag(p0);
	while (p0 != std::string::npos && p0 < pend && tagIs(p0, "<ELEMENT")) {
		elend = tagEnd(p0);
		p0 = nextTag(p0);
		if (isSymbol(p0, strELEMENT)) {
			p0 = tagEnd(p0);
			p0 = nextTag(p0);
			while (p0 != std::string::npos && p0 < elend) {
				print(p0,1);
				if ( isBytes(p0, stemp) ) {
					lmd.push_back(MODE(strELEMENT,stemp));
				}
				else if ( isByte(p0, ch) ) {
					stemp = ch;
					lmd.push_back(MODE(strELEMENT,stemp));
				}
				else if ( isInt(p0, n) ) {
					stemp = (char)(n & 0xFF);
					lmd.push_back(MODE(strELEMENT, stemp));
				}
				else if ( isString(p0, stemp) ) {
					lmd.push_back(MODE(strELEMENT,stemp));
				}
				p0 = tagEnd(p0);
				p0 = nextTag(p0);
			}
		}
		p0 = nextTag(p0);
	}
	p0 = pend;
}

void parseMODES(size_t &p0)
{
	parseMODEdefs(p0, lmodes);
}


void parseMODECMD(size_t &p0)
{
	parseMODEdefs(p0, lmodeCMD);
}

void parseMODEREPLY(size_t &p0)
{
	parseMODEdefs(p0, lmodeREPLY);
}

void parseLSBMODES(size_t &p0)
{
	size_t pend = tagEnd(p0);
	std::string sMode;
	print(p0,0);
	p0 = nextTag(p0);
	while (p0 < pend && isString(p0, sMode)) {
		LSBmodes.push_back(sMode);
		print (p0,1);
		p0 = tagEnd(p0);
		p0 = nextTag(p0);
	}
	p0 = pend;
}

//---------------------------------------------------------------------
// Parse Bandwidth definitions
//---------------------------------------------------------------------

void parseBWdefs(size_t &p0, std::list<BW> &lbw)
{
	size_t pend = tagEnd(p0);
	size_t elend;
	char ch;
	int n;
	std::string strELEMENT;
	std::string stemp;
	if (pend == std::string::npos) {
		LOG_ERROR("Unmatched tag %s", strXML.substr(p0, 10).c_str());
		p0++;
		return;
	}
	print(p0,0);
	size_t p1 = nextTag(p0);
	while (p1 != std::string::npos && p1 < pend && tagIs(p1, "<ELEMENT")) {
		elend = tagEnd(p1);
		if (elend == std::string::npos || elend > pend) {
			LOG_ERROR("Unmatched tag %s", "<ELEMENT");
			p0 = pend;
			return;
		}
		p1 = nextTag(p1);
		if (isSymbol(p1, strELEMENT)) {
			p1 = tagEnd(p1);
			p1 = nextTag(p1);
			while (p1 != std::string::npos && p1 < elend) {
				print(p1,1);
				if ( isBytes(p1, stemp) ) {
					lbw.push_back(BW(strELEMENT,stemp));
					p1 = tagEnd(p1);
				}
				else if ( isByte(p1, ch) ) {
					stemp = ch;
					lbw.push_back(BW(strELEMENT,stemp));
					p1 = tagEnd(p1);
				}
				else if ( isInt(p1, n) ) {
					stemp = (char)(n & 0xFF);
					lbw.push_back(BW(strELEMENT, stemp));
					p1 = tagEnd(p1);
				}
				else if ( isString(p1, stemp) ) {
					lbw.push_back(BW(strELEMENT,stemp));
					p1 = tagEnd(p1);
				} else {
					LOG_ERROR("Invalid tag: %s", strXML.substr(p1, 10).c_str());
					parseDISCARD(p1);
				}
				p1 = nextTag(p1);
			}
		}
		p1 = nextTag(p1);
	}
	p0 = pend;
}

void parseBANDWIDTHS(size_t &p0)
{
	parseBWdefs(p0, lbws);
}

void parseBWCMD(size_t &p0)
{
	parseBWdefs(p0, lbwCMD);
}

void parseBWREPLY(size_t &p0)
{
	parseBWdefs(p0, lbwREPLY);
}

//---------------------------------------------------------------------
// Parse Title definition
//---------------------------------------------------------------------

void parseTITLE(size_t &p0)
{
	size_t pend = tagEnd(p0);
	xmlrig.rigTitle = getElement(p0);
	p0 = pend;
}

//---------------------------------------------------------------------
// Parse Rig definition
//---------------------------------------------------------------------

void parseRIG(size_t &p0)
{
	size_t pend = tagEnd(p0);
	p0 = pend;
}

//---------------------------------------------------------------------
// Parse DEBUG, use during xml file creation/testing
//---------------------------------------------------------------------

void parseDEBUG(size_t &p0){
	bool val = getBool(p0);
	xmlrig.debug = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

//---------------------------------------------------------------------
// Parse NOSERIAL, use during xml file creation/testing
// suppresses serial port i/o
//---------------------------------------------------------------------

void parseNOSERIAL(size_t &p0){
	bool val = getBool(p0);
	xmlrig.noserial = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

//---------------------------------------------------------------------
// Parse ASCII, use during xml file creation/testing
// prints events as ASCII string vice HEX sequence
//---------------------------------------------------------------------

void parseASCII(size_t &p0){
	bool val = getBool(p0);
	xmlrig.ascii = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}


//---------------------------------------------------------------------
// Parse Baudrate, write_delay, post_write_delay, timeout, retries
// RTSCTS handshake
//---------------------------------------------------------------------

void parseBAUDRATE(size_t &p0)
{
	std::string sVal = getElement(p0);
	xmlrig.baud = progdefaults.nBaudRate(sVal.c_str());
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseSTOPBITS(size_t &p0){
	int val = getInt(p0);
	if (val < 0 || val > 2) val = 2;
	xmlrig.stopbits = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseWRITE_DELAY(size_t &p0){
	int val = getInt(p0);
	xmlrig.write_delay = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseINIT_DELAY(size_t &p0){
	int val = getInt(p0);
	xmlrig.init_delay = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseWAIT_FOR_DEVICE(size_t &p0){
	int val = getInt(p0);
	xmlrig.wait_for_device = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parsePOST_WRITE_DELAY(size_t &p0){
	int val = getInt(p0);
	xmlrig.post_write_delay = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseRETRIES(size_t &p0){
	int val = getInt(p0);
	xmlrig.retries = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseTIMEOUT(size_t &p0){
	int val = getInt(p0);
	xmlrig.timeout = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseRTSCTS(size_t &p0){
	bool val = getBool(p0);
	xmlrig.rtscts = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseRTSPLUS(size_t &p0)
{
	bool val = getBool(p0);
	xmlrig.rts = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseDTRPLUS(size_t &p0)
{
	bool val = getBool(p0);
	xmlrig.dtr = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseRTSPTT(size_t &p0)
{
	bool val = getBool(p0);
	xmlrig.rtsptt = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseDTRPTT(size_t &p0)
{
	bool val = getBool(p0);
	xmlrig.dtrptt = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseRESTORE_TIO(size_t &p0)
{
	bool val = getBool(p0);
	xmlrig.restore_tio = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseCMDPTT(size_t &p0) {
	bool val = getBool(p0);
	xmlrig.cmdptt = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseECHO(size_t &p0) {
	bool val = getBool(p0);
	xmlrig.echo = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseVSP(size_t &p0)
{
	bool val = getBool(p0);
	xmlrig.vsp = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseLOGSTR(size_t &p0) {
	bool val = getBool(p0);
	xmlrig.logstr = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parsePOLLINT(size_t &p0) {
	int val = getInt(p0);
	if (val < 100) val = 1000;
	if (val > 20000) val = 20000;
	xmlrig.pollinterval = val;
	size_t pend = tagEnd(p0);
	p0 = pend;
}

void parseSMETER(size_t &p0) {
	std::string strmeter = getElement(p0);
	size_t pend = tagEnd(p0);
	p0 = pend;
	xmlrig.smeter.clear();
	int val, sm;
	size_t p = strmeter.find(",");
	while ( !strmeter.empty() && (p != std::string::npos) ) {
		val = atoi(&strmeter[0]);
		sm = atoi(&strmeter[p+1]);
		xmlrig.smeter.push_back(PAIR(val,sm));
		p = strmeter.find(";");
		if (p == std::string::npos) strmeter.clear();
		else {
			strmeter.erase(0, p+1);
			p = strmeter.find(",");
		}
	}
	xmlrig.use_smeter = true;
}

void parsePMETER(size_t &p0) {
	std::string strmeter = getElement(p0);
	size_t pend = tagEnd(p0);
	p0 = pend;
	xmlrig.pmeter.clear();
	int val, sm;
	size_t p = strmeter.find(",");
	while ( !strmeter.empty() && (p != std::string::npos) ) {
		val = atoi(&strmeter[0]);
		sm = atoi(&strmeter[p+1]);
		xmlrig.pmeter.push_back(PAIR(val,sm));
		p = strmeter.find(";");
		if (p == std::string::npos) strmeter.clear();
		else {
			strmeter.erase(0, p+1);
			p = strmeter.find(",");
		}
	}
	xmlrig.use_pwrmeter = true;
}

void parseNOTCH(size_t &p0) {
	std::string strnotch = getElement(p0);
	size_t pend = tagEnd(p0);
	p0 = pend;
	xmlrig.notch.clear();
	int val, ntch;
	size_t p = strnotch.find(",");
	while ( !strnotch.empty() && (p != std::string::npos) ) {
		val = atoi(&strnotch[0]);
		ntch = atoi(&strnotch[p+1]);
		xmlrig.notch.push_back(PAIR(val,ntch));
		p = strnotch.find(";");
		if (p == std::string::npos) strnotch.clear();
		else {
			strnotch.erase(0, p+1);
			p = strnotch.find(",");
		}
	}
	xmlrig.use_notch = true;
}

void parsePWRLEVEL(size_t &p0) {
	std::string strpwrlevel = getElement(p0);
	size_t pend = tagEnd(p0);
	p0 = pend;
	xmlrig.pwrlevel.clear();
	int val, pwr;
	float min = 500, max = 0;
	size_t p = strpwrlevel.find(",");
	while ( !strpwrlevel.empty() && (p != std::string::npos) ) {
		val = atoi(&strpwrlevel[0]);
		pwr = atoi(&strpwrlevel[p+1]);
		if (pwr < min) min = pwr;
		if (pwr > max) max = pwr;
		xmlrig.pwrlevel.push_back(PAIR(val,pwr));
		p = strpwrlevel.find(";");
		if (p == std::string::npos) strpwrlevel.clear();
		else {
			strpwrlevel.erase(0, p+1);
			p = strpwrlevel.find(",");
		}
	}
	pwr_level->range(min, max);
	xmlrig.use_pwrlevel = true;
}

//---------------------------------------------------------------------
// Parse IOS (serial stream format) definitions
//---------------------------------------------------------------------

void parseIOSsize(size_t &p0)
{
	iosTemp.size = getInt(p0);
}

void parseIOSbytes(size_t &p0)
{
	if (iosTemp.data.size == 0)
		iosTemp.str1.append(getBytes(p0));
	else
		iosTemp.str2.append(getBytes(p0));
}

void parseIOSbyte(size_t &p0)
{
	if (iosTemp.data.size == 0)
		iosTemp.str1 += getByte(p0);
	else
		iosTemp.str2 += getByte(p0);
}

void parseIOSstring(size_t &p0)
{
	if (iosTemp.data.size == 0)
		iosTemp.str1 += getElement(p0);
	else
		iosTemp.str2 += getElement(p0);
}

void parseIOSint(size_t &p0)
{
	if (iosTemp.data.size == 0)
		iosTemp.str1 += (char)(getInt(p0) & 0xFF);
	else
		iosTemp.str2 += (char)(getInt(p0) & 0xFF);
}

void parseDTYPE(size_t &p1)
{
	print(p1,2);
	iosTemp.data.dtype = getElement(p1);
}

void parseDSIZE(size_t &p1)
{
	print(p1,2);
	iosTemp.data.size = getInt(p1);
}

void parseDMAX(size_t &p1)
{
	print(p1,2);
	iosTemp.data.max = getInt(p1);
}

void parseDMIN(size_t &p1)
{
	print(p1,2);
	iosTemp.data.min = getInt(p1);
}

void parseDRESOL(size_t &p1)
{
	print(p1,2);
	iosTemp.data.resolution = getFloat(p1);
}

void parseDREV(size_t &p1)
{
	print(p1,2);
	iosTemp.data.reverse = getBool(p1);
}

void parseDMAKS(size_t &p1)
{
	print(p1,2);
	iosTemp.data.andmask = getInt(p1);
}

void parseDSHIFT(size_t &p1)
{
	print(p1,2);
	iosTemp.data.shiftbits = getInt(p1);
}

void parseIOSdata(size_t &p0)
{
	size_t pend = tagEnd(p0);
	size_t p1;
	TAGS *pv;

	p1 = nextTag(p0);
	while (p1 < pend) {
		pv = datatags;
		while (pv->tag) {
			if (strXML.find(pv->tag, p1) == p1)
				break;
			pv++;
		}
		if (pv->fp) {
			print(p1, 1);
			(pv->fp)(p1);
			p1 = tagEnd(p1);
		} else {
			LOG_ERROR("Invalid tag: %s", strXML.substr(p1, 10).c_str());
			parseDISCARD(p1);
		}
		p1 = nextTag(p1);
	}
}

void parseIOSinfo(size_t &p0)
{
	std::string strR = getElement(p0);
	if (strR.empty()) return;
	iosTemp.info = strR;
}

void parseIOSok(size_t &p0)
{
	std::string strR = getElement(p0);
	if (strR.empty()) return;
	iosTemp.ok = strR;
}

void parseIOSbad(size_t &p0)
{
	std::string strR = getElement(p0);
	if (strR.empty()) return;
	iosTemp.bad = strR;
}

void parseIOSsymbol(size_t &p0)
{
	std::string strR = getElement(p0);
	if (strR.empty()) return;
	iosTemp.SYMBOL = strR;
}

void parseIOSfill(size_t &p0)
{
	if (iosTemp.data.size == 0)
		iosTemp.fill1 = getInt(p0);
	else
		iosTemp.fill2 = getInt(p0);
}

//=======================================================================

bool parseIOS(size_t &p0, TAGS *valid)
{
	size_t pend = tagEnd(p0);
	size_t p1;
	TAGS *pv;

	print(p0,0);

	iosTemp.clear();
	p1 = nextTag(p0);
	while (p1 < pend) {
		pv = valid;
		while (pv->tag) {
			if (strXML.find(pv->tag, p1) == p1)
				break;
			pv++;
		}
		if (pv->fp) {
			print(p1, 1);
			(pv->fp)(p1);
			p1 = tagEnd(p1);
		} else {
			LOG_ERROR("Invalid tag: %s", strXML.substr(p1, 10).c_str());
			parseDISCARD(p1);
		}
		p1 = nextTag(p1);
	}
	p0 = pend;
	return (!iosTemp.SYMBOL.empty());
}

void parseCOMMAND(size_t &p0)
{
	if (parseIOS(p0, commandtags))
		commands.push_back(iosTemp);
}

void parseREPLY(size_t &p0)
{
	if (parseIOS(p0, replytags))
		reply.push_back(iosTemp);
}

void parseRIGDEF(size_t &p0)
{
	print(p0,0);
	size_t p1 = tagEnd(p0);
	if (p1 != std::string::npos)
		strXML.erase(p1);
}

void parseDISCARD(size_t &p0)
{
	size_t pend = tagEnd(p0);
	if (pend == std::string::npos) p0++;
	else p0 = pend;
}

void parseXML()
{
	size_t p0 = 0;
	TAGS *pValid = rigdeftags;

	p0 = strXML.find("<");
	while (p0 != std::string::npos) {
		pValid = rigdeftags;
		while (pValid->tag) {
			if (strXML.find(pValid->tag, p0) == p0)
				break;
			pValid++;
		}
		if (pValid->tag) {
			(pValid->fp)(p0);
		 } else {
			LOG_ERROR("Invalid tag: %s", strXML.substr(p0, 10).c_str());
			parseDISCARD(p0);
		}
		p0 = nextTag(p0);
	}
}

bool remove_comments()
{
	size_t p0 = 0;
	size_t p1 = 0;

// remove comments from xml text
	while ((p0 = strXML.find("<!--")) != std::string::npos) {
		p1 = strXML.find("-->", p0);
		if (p1 == std::string::npos) {
			fl_alert2("Corrupt rig XML definition file\nMismatched comment tags!");
			return false;
		}
		strXML.erase(p0, p1 - p0 + 3);
	}
	if (strXML.find("-->") != std::string::npos) {
		fl_alert2("Corrupt rig XML definition file\nMismatched comment tags!");
		return false;
	}
	return true;
}

bool testXML()
{
	if (!remove_comments()) return false;

	return true;
}

bool readRigXML()
{
	char szLine[256];
	int lines = 0;

	commands.clear();
	reply.clear();
	lmodes.clear();
	lmodeCMD.clear();
	lmodeREPLY.clear();
	lbws.clear();
	lbwCMD.clear();
	lbwREPLY.clear();
	LSBmodes.clear();
	strXML = "";

	std::ifstream xmlfile(progdefaults.XmlRigFilename.c_str(), std::ios::in);
	if (xmlfile) {
		while (!xmlfile.eof()) {
			lines++;
			memset(szLine, 0, sizeof(szLine));
			xmlfile.getline(szLine,255);
			strXML.append(szLine);
		}
		xmlfile.close();
		if (testXML()) {
			parseXML();
			xmlrig.xmlok = true;
			return true;
		}
	}
	return false;
}

void selectRigXmlFilename()
{
	std::string deffilename;
	deffilename = progdefaults.XmlRigFilename;
	const char *p = FSEL::select(_("Open rig xml file"), _("Fldigi rig xml definition file\t*.xml"), deffilename.c_str());
	if (!p) return;
	if (!*p) return;

	progdefaults.XmlRigFilename = p;
	txtXmlRigFilename->value(fl_filename_name(p));
	loadRigXmlFile();
}

void loadRigXmlFile(void)
{
	rigCAT_close();
	readRigXML();
	rigCAT_defaults();
}
