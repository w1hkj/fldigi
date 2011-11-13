// ----------------------------------------------------------------------------
// lookupcall.cxx  -- a part of fldigi
//
// Copyright (C) 2006-2009
//		Dave Freese, W1HKJ
// Copyright (C) 2006-2007
//		Leigh Klotz, WA5ZNU
// Copyright (C) 2008-2009
//              Stelios Bounanos, M0GLD
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

#ifdef __MINGW32__
#  include "compat.h"
#endif

#include <sys/time.h>
#include "signal.h"
#include <string>
#include <iostream>
#include <cstring>
#include <cmath>
#include <cctype>

#include "threads.h"

#include "misc.h"
#include "configuration.h"

#include "lookupcall.h"
#include "main.h"
#include "confdialog.h"
#include "fl_digi.h"
#include "qrzlib.h"

#include "xmlreader.h"

#include "qrunner.h"
#include "debug.h"
#include "network.h"
#include "locator.h"

using namespace std;


string qrzhost = "xml.qrz.com"; //"online.qrz.com";
string qrzSessionKey;
string qrzalert;
string qrzerror;

string callsign;

string lookup_name;
string lookup_addr1;
string lookup_addr2;
string lookup_state;
string lookup_province;
string lookup_zip;
string lookup_country;
string lookup_born;
string lookup_fname;
string lookup_qth;
string lookup_grid;
string lookup_latd;
string lookup_lond;
string lookup_notes;

qrz_query_t DB_query = QRZNONE;

enum TAG {
	QRZ_IGNORE,	QRZ_KEY,	QRZ_ALERT,	QRZ_ERROR,	QRZ_CALL,
	QRZ_FNAME,	QRZ_NAME,	QRZ_ADDR1,	QRZ_ADDR2,	QRZ_STATE,
	QRZ_ZIP,	QRZ_COUNTRY,	QRZ_LATD,	QRZ_LOND,	QRZ_GRID,
	QRZ_DOB
};

pthread_t* QRZ_thread = 0;
pthread_mutex_t qrz_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t qrz_cond = PTHREAD_COND_INITIALIZER;

static void *LOOKUP_loop(void *args);

bool parseSessionKey();
bool parse_xml();

bool getSessionKey(string& sessionpage);
bool QRZGetXML(string& xmlpage);
int  bearing(const char *, const char *);
void qra(const char *, double &, double &);
void QRZ_disp_result();
void QRZ_CD_query();
void Lookup_init(void);
void QRZclose(void);
void qthappend(string &qth, string &datum);
void QRZAlert();
bool QRZLogin(string& sessionpage);
void QRZquery();
void parse_html(const string& htmlpage);
bool HAMCALLget(string& htmlpage);
void HAMCALLquery();

void QRZ_DETAILS_query();

QRZ *qCall = 0;

void clear_Lookup()
{
	lookup_name.clear();
	lookup_addr1.clear();
	lookup_addr2.clear();
	lookup_state.clear();
	lookup_province.clear();
	lookup_zip.clear();
	lookup_born.clear();
	lookup_fname.clear();
	lookup_qth.clear();
	lookup_grid.clear();
	lookup_latd.clear();
	lookup_lond.clear();
	lookup_notes.clear();
	lookup_country.clear();
}

// ----------------------------------------------------------------------------
// QRZ subscription query
// ----------------------------------------------------------------------------

bool parseSessionKey(const string& sessionpage)
{
	IrrXMLReader* xml = createIrrXMLReader(new IIrrXMLStringReader(sessionpage));
	TAG tag=QRZ_IGNORE;
	while(xml && xml->read()) {
		switch(xml->getNodeType())
		{
		case EXN_TEXT:
		case EXN_CDATA:
			switch (tag)
			{
			default:
				break;
			case QRZ_KEY:
				qrzSessionKey = xml->getNodeData();
				break;
			case QRZ_ALERT:
				qrzalert = xml->getNodeData();
				break;
			case QRZ_ERROR:
				qrzerror = xml->getNodeData();
				break;
			}
			break;

		case EXN_ELEMENT_END:
			tag=QRZ_IGNORE;
			break;

		case EXN_ELEMENT:
		{
			const char *nodeName = xml->getNodeName();
			if (!strcmp("Key", nodeName)) tag=QRZ_KEY;
			else if (!strcmp("Alert", nodeName)) tag=QRZ_ALERT;
			else if (!strcmp("Error", nodeName)) tag=QRZ_ERROR;
			else tag=QRZ_IGNORE;
			break;
		}

		case EXN_NONE:
		case EXN_COMMENT:
		case EXN_UNKNOWN:
			break;
		}
	}
	delete xml;
	return true;
}


bool parse_xml(const string& xmlpage)
{
//printf("%s\n", xmlpage.c_str());
	IrrXMLReader* xml = createIrrXMLReader(new IIrrXMLStringReader(xmlpage));

// If we got any result back, clear the session key so that it will be
// refreshed by this response, or if not present, will be removed and we'll
// know to log in next time.
	if (xml) {
		qrzSessionKey.clear();
		qrzalert.clear();
		qrzerror.clear();
		clear_Lookup();
	}

	TAG tag = QRZ_IGNORE;

// parse the file until end reached
	while(xml && xml->read()) {
		switch(xml->getNodeType()) {
			case EXN_TEXT:
			case EXN_CDATA:
				switch (tag) {
					default:
					case QRZ_IGNORE:
						break;
					case QRZ_CALL:
						break;
					case QRZ_FNAME:
						lookup_fname =  xml->getNodeData();
						break;
					case QRZ_NAME:
						lookup_name =  xml->getNodeData();
						break;
					case QRZ_ADDR1:
						{
						lookup_addr1 =  xml->getNodeData();
						size_t apt = lookup_addr1.find("#");
						if (apt != string::npos)
							lookup_addr1.erase(apt, lookup_addr1.length() - apt);
						break;
						}
					case QRZ_ADDR2:
						lookup_addr2 =  xml->getNodeData();
						break;
					case QRZ_STATE:
						lookup_state =  xml->getNodeData();
						break;
					case QRZ_ZIP:
						lookup_zip =  xml->getNodeData();
						break;
					case QRZ_COUNTRY:
						lookup_country =  xml->getNodeData();
						break;
					case QRZ_LATD:
						lookup_latd =  xml->getNodeData();
						break;
					case QRZ_LOND:
						lookup_lond =  xml->getNodeData();
						break;
					case QRZ_GRID:
						lookup_grid =  xml->getNodeData();
						break;
					case QRZ_DOB:
						lookup_notes = "DOB: ";
						lookup_notes += xml->getNodeData();
						break;
					case QRZ_ALERT:
						qrzalert = xml->getNodeData();
						break;
					case QRZ_ERROR:
						qrzerror = xml->getNodeData();
						break;
					case QRZ_KEY:
						qrzSessionKey = xml->getNodeData();
						break;
				}
				break;

			case EXN_ELEMENT_END:
				tag=QRZ_IGNORE;
				break;

			case EXN_ELEMENT:
				{
				const char *nodeName = xml->getNodeName();
				if (!strcmp("call", nodeName)) 			tag = QRZ_CALL;
				else if (!strcmp("fname", nodeName)) 	tag = QRZ_FNAME;
				else if (!strcmp("name", nodeName)) 	tag = QRZ_NAME;
				else if (!strcmp("addr1", nodeName)) 	tag = QRZ_ADDR1;
				else if (!strcmp("addr2", nodeName)) 	tag = QRZ_ADDR2;
				else if (!strcmp("state", nodeName)) 	tag = QRZ_STATE;
				else if (!strcmp("zip", nodeName)) 		tag = QRZ_ZIP;
				else if (!strcmp("country", nodeName))	tag = QRZ_COUNTRY;
				else if (!strcmp("lat", nodeName)) 		tag = QRZ_LATD;
				else if (!strcmp("lon", nodeName)) 		tag = QRZ_LOND;
				else if (!strcmp("grid", nodeName)) 	tag = QRZ_GRID;
				else if (!strcmp("dob", nodeName)) 		tag = QRZ_DOB;
				else if (!strcmp("Alert", nodeName)) 	tag = QRZ_ALERT;
				else if (!strcmp("Error", nodeName)) 	tag = QRZ_ERROR;
				else if (!strcmp("Key", nodeName)) 		tag = QRZ_KEY;
				else tag = QRZ_IGNORE;
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

bool getSessionKey(string& sessionpage)
{

	string detail;
	detail =  "GET /bin/xml?username=";
	detail += progdefaults.QRZusername;
	detail += ";password=";
	detail += progdefaults.QRZuserpassword;
	detail += ";version=";
	detail += PACKAGE_NAME;
	detail += "/";
	detail += PACKAGE_VERSION;
	detail += " HTTP/1.0\n";
	detail += "Host: ";
	detail += qrzhost;
	detail += "\n";
	detail += "Connection: close\n";
	detail += "\n";

	return request_reply(qrzhost, "http", detail, sessionpage, 5.0);
}

bool QRZGetXML(string& xmlpage)
{
	string detail;
	detail = "GET /bin/xml?s=";
	detail += qrzSessionKey;
	detail += ";callsign=";
	detail += callsign;
	detail += " HTTP/1.0\n";
	detail += "Host: ";
	detail += qrzhost;
	detail += "\n";
	detail += "Connection: close\n";
	detail += "\n";

//	return request_reply(qrzhost, "http", detail, xmlpage, 5.0);
	bool res = request_reply(qrzhost, "http", detail, xmlpage, 5.0);
	LOG_DEBUG("result = %d", res);
	return res;
}

void camel_case(string &s)
{
	bool first_letter = true;
	for (size_t n = 0; n < s.length(); n++) {
		if (s[n] == ' ') first_letter = true;
		else if (first_letter) {
			s[n] = toupper(s[n]);
			first_letter = false;
		} else s[n] = tolower(s[n]);
	}
}

void QRZ_disp_result()
{
	ENSURE_THREAD(FLMAIN_TID);

	if (lookup_fname.length() > 0) {
		camel_case(lookup_fname);
		string::size_type spacePos = lookup_fname.find(" ");
		//    if fname is "ABC" then display "ABC"
		// or if fname is "A BCD" then display "A BCD"
		if (spacePos == string::npos || (spacePos == 1)) {
			inpName->value(lookup_fname.c_str());
		}
		// if fname is "ABC Y" then display "ABC"
		else if (spacePos > 2) {
			string fname;
			fname.assign(lookup_fname, 0, spacePos);
			inpName->value(fname.c_str());
		}
		// fname must be "ABC DEF" so display "ABC DEF"
		else {
			inpName->value(lookup_fname.c_str());
		}
	} else if (lookup_name.length() > 0) {
		// only name is set; don't know first/last, so just show all
		inpName->value(lookup_name.c_str());
	}

	inpQth->value(lookup_qth.c_str());

	inpState->value(lookup_state.c_str());

	inpVEprov->value(lookup_province.c_str());

	inpLoc->value(lookup_grid.c_str());

	if (!lookup_country.empty())
		inpCountry->value(lookup_country.c_str());

	if (!progdefaults.myLocator.empty() && !lookup_grid.empty()) {
		char buf[10];
		buf[0] = '\0';
		double distance, azimuth, lon[2], lat[2];
		if (locator2longlat(&lon[0], &lat[0], progdefaults.myLocator.c_str()) == RIG_OK &&
		    locator2longlat(&lon[1], &lat[1], lookup_grid.c_str()) == RIG_OK &&
		    qrb(lon[0], lat[0], lon[1], lat[1], &distance, &azimuth) == RIG_OK)
			snprintf(buf, sizeof(buf), "%03.0f", round(azimuth));
		inpAZ->value(buf);
	}
	inpNotes->value(lookup_notes.c_str());
}

void QRZ_CD_query()
{
	ENSURE_THREAD(QRZ_TID);

	char srch[20];
	size_t snip;

	memset( srch, 0, sizeof(srch) );
	strncpy( srch, callsign.c_str(), 6 );
	for (size_t i = 0; i < strlen(srch); i ++ )
		srch[i] = toupper(srch[i]);

	if( qCall->FindRecord( srch ) == 1) {
		lookup_fname = qCall->GetFname();
		camel_case(lookup_fname);
		snip = lookup_fname.find(' ');
		if (snip != string::npos)
			lookup_fname.erase(snip, lookup_fname.length() - snip);
		lookup_qth = qCall->GetCity();
		lookup_state = qCall->GetState();
		lookup_grid.clear();
		lookup_notes.clear();
	} else {
		lookup_fname.clear();
		lookup_qth.clear();
		lookup_grid.clear();
		lookup_born.clear();
		lookup_notes = "Not found in CD database";
	}
	REQ(QRZ_disp_result);
}

void Lookup_init(void)
{
	ENSURE_THREAD(FLMAIN_TID);

	if (QRZ_thread)
		return;
	QRZ_thread = new pthread_t;
	if (pthread_create(QRZ_thread, NULL, LOOKUP_loop, NULL) != 0) {
		LOG_PERROR("pthread_create");
		return;
	}
	MilliSleep(10);
}

void QRZclose(void)
{
	ENSURE_THREAD(FLMAIN_TID);

	if (!QRZ_thread)
		return;

	CANCEL_THREAD(*QRZ_thread);

	DB_query = QRZ_EXIT;
	pthread_mutex_lock(&qrz_mutex);
	pthread_cond_signal(&qrz_cond);
	pthread_mutex_unlock(&qrz_mutex);

	pthread_join(*QRZ_thread, NULL);
	delete QRZ_thread;
	QRZ_thread = 0;
}

void qthappend(string &qth, string &datum) {
	if (datum.empty()) return;
	if (!qth.empty()) qth += ", ";
	qth += datum;
}

void QRZAlert()
{
	ENSURE_THREAD(FLMAIN_TID);

	string qrznote;
	if (!qrzalert.empty()) {
		qrznote.append("QRZ alert notice:\n");
		qrznote.append(qrzalert);
		qrznote.append("\n");
		qrzalert.clear();
	}
	if (!qrzerror.empty()) {
		qrznote.append("QRZ error notice:\n");
		qrznote.append(qrzalert);
		qrzerror.clear();
	}
	inpNotes->value(qrznote.c_str());
}

bool QRZLogin(string& sessionpage)
{
	bool ok = true;
	if (qrzSessionKey.empty()) {
		ok = getSessionKey(sessionpage);
		if (ok) ok = parseSessionKey(sessionpage);
	}
	if (!ok) {
		LOG_DEBUG("failed");
		REQ(QRZAlert);
	}

	return ok;
}

void QRZquery()
{
	ENSURE_THREAD(QRZ_TID);

	bool ok = true;

	string qrzpage;

	if (qrzSessionKey.empty())
		ok = QRZLogin(qrzpage);
	if (ok)
		ok = QRZGetXML(qrzpage);
	if (!ok) { // change to negative for MS not getting on first try
		if (qrzSessionKey.empty())
			ok = QRZLogin(qrzpage);
		if (ok)
			ok = QRZGetXML(qrzpage);
	}
	if (ok) {
		parse_xml(qrzpage);
		if (!qrzalert.empty() || !qrzerror.empty())
			REQ(QRZAlert);
		else {
			lookup_qth = lookup_addr2;
			string isCAN = "vV";
			if (isCAN.find(callsign[0]) != string::npos) { // Can callsign
				size_t pos = lookup_qth.find(',');
				if (pos != string::npos) {
					lookup_province = lookup_qth.substr(pos);
					lookup_qth = lookup_qth.substr(0, pos);
					pos = lookup_province.find_first_not_of(", ");
					if (pos != string::npos)
						lookup_province = lookup_province.substr(pos);
					pos = lookup_province.find(' ');
					if (pos != string::npos)
						lookup_province = lookup_province.substr(0,pos);
				}
			}
			REQ(QRZ_disp_result);
		}
	}
	else {
		qrzerror = qrzpage;
		REQ(QRZAlert);
	}
}

// ---------------------------------------------------------------------
// HTTP:://callook.info queries
// ---------------------------------------------------------------------

string node_data(const string &xmlpage, const string nodename)
{
	size_t pos1, pos2;
	pos1 = xmlpage.find(string("<").append(nodename).append(">"));
	if (pos1 == string::npos) return "";
	pos2 = xmlpage.find(string("</").append(nodename).append(">"));
	if (pos2 == string::npos) return "";
	pos1 += (string("<").append(nodename).append(">")).length();
	return xmlpage.substr(pos1, pos2 - pos1);
}

void parse_callook(string& xmlpage)
{
	string nodestr;
	nodestr = node_data(xmlpage, "current");
	if (nodestr.empty()) {
		lookup_notes = "no data from callook.info";
		return;
	}
	xmlpage = xmlpage.substr(xmlpage.find("</trustee>"));
	lookup_fname = node_data(xmlpage, "name");
	camel_case(lookup_fname);
	nodestr = node_data(xmlpage, "address");
	if (!nodestr.empty()) {
		lookup_addr1 = node_data(nodestr, "line1");
		lookup_addr2 = node_data(nodestr, "line2");
	}
	nodestr = node_data(xmlpage, "location");
	if (!nodestr.empty()) {
		lookup_lond = node_data(nodestr, "longitude");
		lookup_latd = node_data(nodestr, "latitude");
		lookup_grid = node_data(nodestr, "gridsquare");
	}
	size_t p;
	p = lookup_addr2.find(",");
	if (p != string::npos) {
		lookup_qth = lookup_addr2.substr(0, p);
		lookup_addr2.erase(0, p+2);
		p = lookup_addr2.find(" ");
		if (p != string::npos)
			lookup_state = lookup_addr2.substr(0, p);
	}
}

bool CALLOOKGetXML(string& xmlpage)
{
	string url = string("http://callook.info/").append(callsign).append("/xml");
	bool res = fetch_http(url, xmlpage, 5.0);
	LOG_DEBUG("result = %d", res);
	return res;
}

void CALLOOKquery()
{
	ENSURE_THREAD(QRZ_TID);

	bool ok = true;

	string CALLOOKpage;

	clear_Lookup();
	ok = CALLOOKGetXML(CALLOOKpage);
	if (!ok) // change to negative for MS not getting on first try
		ok = CALLOOKGetXML(CALLOOKpage);
	if (ok)
		parse_callook(CALLOOKpage);
	REQ(QRZ_disp_result);
}

// ---------------------------------------------------------------------
// Hamcall specific functions
// ---------------------------------------------------------------------

#define HAMCALL_CALL 181
#define HAMCALL_FIRST 184
#define HAMCALL_CITY 191
#define HAMCALL_STATE 192
#define HAMCALL_GRID 202
#define HAMCALL_DOB 194

void parse_html(const string& htmlpage)
{
	size_t p;

	clear_Lookup();

	if ((p = htmlpage.find(HAMCALL_FIRST)) != string::npos) {
		p++;
		while ((uchar)htmlpage[p] < 128 && p < htmlpage.length() )
			lookup_fname += htmlpage[p++];
			camel_case(lookup_fname);
	}
	if ((p = htmlpage.find(HAMCALL_CITY)) != string::npos) {
		p++;
		while ((uchar)htmlpage[p] < 128 && p < htmlpage.length())
			lookup_qth += htmlpage[p++];
	}
	if ((p = htmlpage.find(HAMCALL_STATE)) != string::npos) {
		p++;
		while ((uchar)htmlpage[p] < 128 && p < htmlpage.length())
			lookup_state += htmlpage[p++];
	}
	if ((p = htmlpage.find(HAMCALL_GRID)) != string::npos) {
		p++;
		while ((uchar)htmlpage[p] < 128 && p < htmlpage.length())
			lookup_grid += htmlpage[p++];
	}
	if ((p = htmlpage.find(HAMCALL_DOB)) != string::npos) {
		p++;
		lookup_notes = "DOB: ";
		while ((uchar)htmlpage[p] < 128 && p < htmlpage.length())
			lookup_notes += htmlpage[p++];
	}
}

bool HAMCALLget(string& htmlpage)
{
	string url_detail;
	url_detail =  "GET /call?username=";
	url_detail += progdefaults.QRZusername;
	url_detail += "&password=";
	url_detail += progdefaults.QRZuserpassword;
	url_detail += "&rawlookup=1&callsign=";
	url_detail += callsign;
	url_detail += "&program=fldigi-";
	url_detail += VERSION;
	url_detail += "\r\n";

	return request_reply("www.hamcall.net", "http", url_detail, htmlpage, 5.0);
}

void HAMCALLquery()
{
	ENSURE_THREAD(QRZ_TID);

	string htmlpage;

	if (HAMCALLget(htmlpage))
		parse_html(htmlpage);
	else
		lookup_notes = htmlpage;
	REQ(QRZ_disp_result);
}

// ---------------------------------------------------------------------
// Hamcall specific functions
// ---------------------------------------------------------------------

static string HAMQTH_session_id = "";
static string HAMQTH_reply = "";

#define HAMQTH_DEBUG 1
#undef HAMQTH_DEBUG

bool HAMQTH_get_session_id()
{
	string url = "";
	string retstr = "";
	size_t p1 = string::npos;
	size_t p2 = string::npos;

	url.append("http://www.hamqth.com/xml.php?u=").append(progdefaults.QRZusername);
	url.append("&p=").append(progdefaults.QRZuserpassword);

	HAMQTH_session_id.clear();
	if (!fetch_http(url, retstr, 5.0)) {
		return false;
	}
	if ((p1 = retstr.find("<error>")) != string::npos) {
		p2 = retstr.find("</error>");
		lookup_notes = retstr.substr(p1 + 7, p2 - p1 - 7);
		return false;
	}
	if ((p1 = retstr.find("<session_id>")) == string::npos) {
		lookup_notes = "HamQTH not available";
		return false;
	}
	p2 = retstr.find("</session_id>");
	HAMQTH_session_id = retstr.substr(p1 + 12, p2 - p1 - 12);
//#ifdef HAMQTH_DEBUG
//	printf("session id = %s\n", HAMQTH_session_id.c_str());
//#endif
	return true;
}

void parse_HAMQTH_html(const string& htmlpage)
{
	size_t p = string::npos;
	size_t p1 = string::npos;

	clear_Lookup();

	lookup_fname.clear();
	lookup_qth.clear();
	lookup_state.clear();
	lookup_grid.clear();
	lookup_notes.clear();
	lookup_country.clear();

	if ((p = htmlpage.find("<error>")) != string::npos) {
		p += 7;
		p1 = htmlpage.find("</error>");
		if (p1 != string::npos) 
			lookup_notes.append(htmlpage.substr(p, p1 - p));
		return;
	}
	if ((p = htmlpage.find("<nick>")) != string::npos) {
		p += 6;
		p1 = htmlpage.find("</nick>", p);
		if (p1 != string::npos) {
			lookup_fname = htmlpage.substr(p, p1 - p);
			camel_case(lookup_fname);
		}
	}
	if ((p = htmlpage.find("<qth>")) != string::npos) {
		p += 5;
		p1 = htmlpage.find("</qth>", p);
		if (p1 != string::npos)
			lookup_qth = htmlpage.substr(p, p1 - p);
	}
	if ((p = htmlpage.find("<country>")) != string::npos) {
		p += 9;
		p1 = htmlpage.find("</country>", p);
		if (p1 != string::npos)
			lookup_country = htmlpage.substr(p, p1 - p);
	}
	if ((p = htmlpage.find("<us_state>")) != string::npos) {
		p += 10;
		p1 = htmlpage.find("</us_state>");
		if (p1 != string::npos)
			lookup_state = htmlpage.substr(p, p1 - p);
	}
	if ((p = htmlpage.find("<grid>")) != string::npos) {
		p += 6;
		p1 = htmlpage.find("</grid>");
		if (p1 != string::npos)
			lookup_grid = htmlpage.substr(p, p1 - p);
	}
	if ((p = htmlpage.find("<qsl_via>")) != string::npos) {
		p += 9;
		p1 = htmlpage.find("</qsl_via>");
		if (p1 != string::npos)
			lookup_notes.append("QSL via: ").append(htmlpage.substr(p, p1 - p)).append("\n");
	}
	if ((p = htmlpage.find("<adr_name>")) != string::npos) {
		p += 10;
		p1 = htmlpage.find("</adr_name>");
		if (p1 != string::npos)
			lookup_notes.append(htmlpage.substr(p, p1 - p)).append("\n");
	}
	if ((p = htmlpage.find("<adr_street1>")) != string::npos) {
		p += 13;
		p1 = htmlpage.find("</adr_street1>");
		if (p1 != string::npos)
			lookup_notes.append(htmlpage.substr(p, p1 - p)).append("\n");
	}
	if ((p = htmlpage.find("<adr_city>")) != string::npos) {
		p += 10;
		p1 = htmlpage.find("</adr_city>");
		if (p1 != string::npos)
			lookup_notes.append(htmlpage.substr(p, p1 - p)).append(", ").append(lookup_state);
	}
	if ((p = htmlpage.find("<adr_zip>")) != string::npos) {
		p += 9;
		p1 = htmlpage.find("</adr_zip>");
		if (p1 != string::npos)
			lookup_notes.append("  ").append(htmlpage.substr(p, p1 - p));
	}
	if ((p = htmlpage.find("<adr_country>")) != string::npos) {
		p += 13;
		p1 = htmlpage.find("</adr_country>");
		if (p1 != string::npos)
			lookup_notes.append("  ").append(htmlpage.substr(p, p1 - p));
	}
}

bool HAMQTHget(string& htmlpage)
{
	string url = "";
	bool ret;
	if (HAMQTH_session_id.empty()) {
		if (!HAMQTH_get_session_id()) return false;
	}
	url.append("http://www.hamqth.com/xml.php?id=").append(HAMQTH_session_id);
	url.append("&callsign=").append(callsign);
	url.append("&prg=fldigi-").append(VERSION);

	ret = fetch_http(url, htmlpage, 5.0);
	if (htmlpage.find("<error>") != string::npos) {
		htmlpage.clear();
		if (!HAMQTH_get_session_id()) {
			lookup_notes = "Get session id failed!\n";
			return false;
		}
		ret = fetch_http(url, htmlpage, 5.0);
	}
#ifdef HAMQTH_DEBUG
	FILE *fetchit = fopen("fetchit.txt", "a");
	fprintf(fetchit, "%s\n", htmlpage.c_str());
	fclose(fetchit);
#endif
	return ret;
}

void HAMQTHquery()
{
	ENSURE_THREAD(QRZ_TID);

	string htmlpage;

	if (!HAMQTHget(htmlpage)) return;

	parse_HAMQTH_html(htmlpage);
	REQ(QRZ_disp_result);

}

// ----------------------------------------------------------------------------

void QRZ_DETAILS_query()
{
	string qrzurl = "http://www.qrz.com/db/";
	qrzurl.append(callsign);

	cb_mnuVisitURL(0, (void*)qrzurl.c_str());
}

void HAMCALL_DETAILS_query()
{
	string hamcallurl = "http://www.hamcall.net/call?callsign=";
	hamcallurl.append(callsign);

	cb_mnuVisitURL(0, (void*)hamcallurl.c_str());
}

// ----------------------------------------------------------------------------

static void *LOOKUP_loop(void *args)
{
	SET_THREAD_ID(QRZ_TID);

	SET_THREAD_CANCEL();

	for (;;) {
		TEST_THREAD_CANCEL();
		pthread_mutex_lock(&qrz_mutex);
		pthread_cond_wait(&qrz_cond, &qrz_mutex);
		pthread_mutex_unlock(&qrz_mutex);

		switch (DB_query) {
		case QRZCD :
			QRZ_CD_query();
			break;
		case QRZNET :
			QRZquery();
			break;
		case HAMCALLNET :
			HAMCALLquery();
			break;
		case QRZHTML :
			QRZ_DETAILS_query();
			break;
		case HAMCALLHTML :
			HAMCALL_DETAILS_query();
			break;
		case CALLOOK:
			CALLOOKquery();
			break;
		case HAMQTH:
			HAMQTHquery();
			break;
		case QRZ_EXIT:
			return NULL;
		default:
			LOG_ERROR("Bad query type %d", DB_query);
			break;
		}
	}

	return NULL;
}

void CALLSIGNquery()
{
	ENSURE_THREAD(FLMAIN_TID);

	if (!QRZ_thread)
		Lookup_init();

	// Filter callsign for nonsense characters (remove all but [A-Za-z0-9/])
	callsign.clear();
	for (const char* p = inpCall->value(); *p; p++)
		if (isalnum(*p) || *p == '/')
			callsign += *p;
	if (callsign.empty())
		return;
	if (callsign != inpCall->value())
		inpCall->value(callsign.c_str());

	switch (DB_query = static_cast<qrz_query_t>(progdefaults.QRZ)) {
	case QRZNET:
		inpNotes->value("Request sent to\nqrz.com...");
		break;
	case QRZHTML: case HAMCALLHTML:
		break;
	case HAMCALLNET:
		inpNotes->value("Request sent to\nwww.hamcall.net...");
		break;
	case QRZCD:
		if (!qCall)
			qCall = new QRZ( "callbkc" );
		if (progdefaults.QRZchanged) {
			qCall->NewDBpath("callbkc");
			progdefaults.QRZchanged = false;
		}
		if (!qCall->getQRZvalid()) {
			inpNotes->value("QRZ DB error");
			DB_query = QRZNONE;
			return;
		}
		break;
	case CALLOOK:
		inpNotes->value("Request sent to\nhttp://callook.info...");
		break;
	case HAMQTH:
		inpNotes->value("Request sent to \nhttp://hamqth.com...");
		break;
	default:
		LOG_ERROR("Bad query type %d", DB_query);
		return;
	}

	pthread_mutex_lock(&qrz_mutex);
	pthread_cond_signal(&qrz_cond);
	pthread_mutex_unlock(&qrz_mutex);
}

//======================================================================
// thread to support sending log entry to eQSL
//======================================================================

pthread_t* EQSLthread = 0;
pthread_mutex_t EQSLmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t EQSLcond = PTHREAD_COND_INITIALIZER;

static void *EQSL_loop(void *args);
static void EQSL_init(void);

void EQSL_close(void);
void EQSL_send();

static std::string EQSL_url = "";
static std::string EQSL_xmlpage = "";

static bool EQSLEXIT = false;

static void *EQSL_loop(void *args)
{
	SET_THREAD_ID(EQSL_TID);

	SET_THREAD_CANCEL();

	for (;;) {
		TEST_THREAD_CANCEL();
		pthread_mutex_lock(&EQSLmutex);
		pthread_cond_wait(&EQSLcond, &EQSLmutex);
		pthread_mutex_unlock(&EQSLmutex);

		if (EQSLEXIT)
			return NULL;

		size_t p;
		if (fetch_http(EQSL_url, EQSL_xmlpage, 5.0) == -1)
			LOG_ERROR("%s", "eQSL not available");

		else if ((p = EQSL_xmlpage.find("Error:")) != std::string::npos) {
			size_t p2 = EQSL_xmlpage.find('\n', p);
			LOG_ERROR("%s", EQSL_xmlpage.substr(p, p2 - p - 1).c_str());
		} else
			LOG_INFO("eQSL logged %s", EQSL_url.c_str()); 

	}
	return NULL;
}

void EQSL_close(void)
{
	ENSURE_THREAD(FLMAIN_TID);

	if (!EQSLthread)
		return;

	CANCEL_THREAD(*EQSLthread);

	pthread_mutex_lock(&qrz_mutex);
	EQSLEXIT = true;
	pthread_cond_signal(&qrz_cond);
	pthread_mutex_unlock(&qrz_mutex);

	pthread_join(*QRZ_thread, NULL);
	delete QRZ_thread;
	QRZ_thread = 0;
}

static void EQSL_init(void)
{
	ENSURE_THREAD(FLMAIN_TID);

	if (EQSLthread)
		return;
	EQSLthread = new pthread_t;
	EQSLEXIT = false;
	if (pthread_create(EQSLthread, NULL, EQSL_loop, NULL) != 0) {
		LOG_PERROR("pthread_create");
		return;
	}
	MilliSleep(10);
}

void sendEQSL(const char *url)
{
	ENSURE_THREAD(FLMAIN_TID);

	if (!EQSLthread)
		EQSL_init();

	pthread_mutex_lock(&EQSLmutex);
	EQSL_url = url;
	pthread_cond_signal(&EQSLcond);
	pthread_mutex_unlock(&EQSLmutex);
}
