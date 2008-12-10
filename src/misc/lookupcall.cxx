// ----------------------------------------------------------------------------
// lookupcall.cxx  -- a part of fldigi
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
//		Leigh Klotz, WA5ZNU
// Copyright (C) 2008
//              Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#include <config.h>

#include <sys/time.h>
#include "signal.h"
#include <string>
#include <iostream>
#include <cstring>
#include <cmath>
#include <cctype>

#include <FL/fl_ask.H>

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

using namespace std;

string qrzhost = "online.qrz.com";
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
	IGNORE,	KEY,	ALERT,	ERROR,	CALL,
	FNAME,	NAME,	ADDR1,	ADDR2,	STATE,
	ZIP,	COUNTRY,LATD,	LOND,	GRID,
	DOB
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

int bearing(const char *myqra, const char *dxqra) {
	double	lat1, lat1r, lon1;
	double	lat2, lat2r, lon2;
	double	dlong, arg1, arg2a, arg2b, arg2, bearingr, bearing;
	double	k=180.0/M_PI;

	qra(dxqra, lat2, lon2);
	qra(myqra, lat1, lon1);

	lat1r=lat1/k;
	lat2r=lat2/k;

	dlong = lon2/k - lon1/k;

	arg1 = sin(dlong) * cos(lat2r);
	arg2a = cos(lat1r) * sin(lat2r);
	arg2b = sin(lat1r) * cos(lat2r) * cos(dlong);
	arg2 =  arg2a -  arg2b;
	bearingr = atan2(arg1, arg2);

	bearing = floor(0.5+fmod(360.0 + (bearingr * k), 360.0));
	return (int)bearing;
}

void qra(const char *szqra, double &lat, double &lon) {
	int c1 = toupper(szqra[0])-'A';
	int c2 = toupper(szqra[1])-'A';
	int c3 = szqra[2]-'0';
	int c4 = szqra[3]-'0';
	int c5, c6;
	if (strlen(szqra) > 4) {
    	c5 = toupper(szqra[4])-'A';
    	c6 = toupper(szqra[5])-'A';
		lat = (((c2 * 10.0) + c4 + ((c6 + 0.5)/24.0)) - 90.0);
		lon = (((c1 * 20.0) + (c3 * 2.0) + ((c5 + 0.5) / 12.0)) - 180.0);
	} else {
		lat = (((c2 * 10.0) + c4 ) - 90.0);
		lon = (((c1 * 20.0) + (c3 * 2.0)) - 180.0);
	}
}

void clear_Lookup()
{
	lookup_name.clear();
	lookup_addr1.clear();
	lookup_addr2.clear();
	lookup_state.clear();
	lookup_province.clear();
	lookup_zip.clear();
	lookup_country.clear();
	lookup_born.clear();
	lookup_fname.clear();
	lookup_qth.clear();
	lookup_grid.clear();
	lookup_latd.clear();
	lookup_lond.clear();
	lookup_notes.clear();
}

// ----------------------------------------------------------------------------
// QRZ subscription query
// ----------------------------------------------------------------------------

bool parseSessionKey(const string& sessionpage)
{
	IrrXMLReader* xml = createIrrXMLReader(new IIrrXMLStringReader(sessionpage));
	TAG tag=IGNORE;
	while(xml && xml->read()) {
		switch(xml->getNodeType())
		{
		case EXN_TEXT:
		case EXN_CDATA:
			switch (tag) 
			{
			default:
				break;
			case KEY:
				qrzSessionKey = xml->getNodeData();
				break;
			case ALERT:
				qrzalert = xml->getNodeData();
				break;
			case ERROR:
				qrzerror = xml->getNodeData();
				break;
			}
			break;

		case EXN_ELEMENT_END:
			tag=IGNORE;
			break;

		case EXN_ELEMENT:
		{
			const char *nodeName = xml->getNodeName();
			if (!strcmp("Key", nodeName)) tag=KEY;
			else if (!strcmp("Alert", nodeName)) tag=ALERT;
			else if (!strcmp("Error", nodeName)) tag=ERROR;
			else tag=IGNORE;
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
// strings for storing the data we want to get out of the file
	string	call, 
			fname, 
			name, 
			addr1, 
			addr2, 
			state, 
			zip, 
			country, 
			latd, 
			lond, 
			grid, 
			dob;
			
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
					case CALL:
						call = xml->getNodeData();
						break;
					case FNAME:
						lookup_fname =  xml->getNodeData();
						break;
					case NAME:
						lookup_name =  xml->getNodeData();
						break;
					case ADDR1:
						lookup_addr1 =  xml->getNodeData();
						break;
					case ADDR2:
						lookup_addr2 =  xml->getNodeData();
						break;
					case STATE:
						lookup_state =  xml->getNodeData();
						break;
					case ZIP:
						lookup_zip =  xml->getNodeData();
						break;
					case COUNTRY:
						lookup_country =  xml->getNodeData();
						break;
					case LATD:
						lookup_latd =  xml->getNodeData();
						break;
					case LOND:
						lookup_lond =  xml->getNodeData();
						break;
					case GRID:
						lookup_grid =  xml->getNodeData();
						break;
					case DOB:
						lookup_notes = "DOB: ";
						lookup_notes += xml->getNodeData();
						break;
					case ALERT:
						qrzalert = xml->getNodeData();
						break;
					case ERROR:
						qrzerror = xml->getNodeData();
						break;
					case KEY:
						qrzSessionKey = xml->getNodeData();
						break;
				}
				break;
				
			case EXN_ELEMENT_END:
				tag=IGNORE;
				break;

			case EXN_ELEMENT: 
				{
				const char *nodeName = xml->getNodeName();
				if (!strcmp("call", nodeName)) 			tag = CALL;
				else if (!strcmp("fname", nodeName)) 	tag = FNAME;
				else if (!strcmp("name", nodeName)) 	tag = NAME;
				else if (!strcmp("addr1", nodeName)) 	tag = ADDR1;
				else if (!strcmp("addr2", nodeName)) 	tag = ADDR2;
				else if (!strcmp("state", nodeName)) 	tag = STATE;
				else if (!strcmp("zip", nodeName)) 		tag = ZIP;
				else if (!strcmp("country", nodeName))	tag = COUNTRY;
				else if (!strcmp("latd", nodeName)) 	tag = LATD;
				else if (!strcmp("lond", nodeName)) 	tag = LOND;
				else if (!strcmp("grid", nodeName)) 	tag = GRID;
				else if (!strcmp("dob", nodeName)) 		tag = DOB;
				else if (!strcmp("Alert", nodeName)) 	tag = ALERT;
				else if (!strcmp("Error", nodeName)) 	tag = ERROR;
				else if (!strcmp("Key", nodeName)) 		tag = KEY;
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

	return request_reply(qrzhost, "http", detail, xmlpage, 5.0);
}

void QRZ_disp_result()
{
	ENSURE_THREAD(FLMAIN_TID);

	if (lookup_fname.length() > 0) {
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
	if (!progdefaults.myLocator.empty()) {
		char buf[10];
		buf[0] = '\0';
		if (!lookup_grid.empty()) {
			int b = bearing( progdefaults.myLocator.c_str(), lookup_grid.c_str() );
			if (b<0) b+=360;
			if (b>=360) b-=360;
			snprintf(buf, sizeof(buf), "%03d", b);
		}
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
}

void QRZclose(void)
{
	ENSURE_THREAD(FLMAIN_TID);

	if (!QRZ_thread)
		return;

	pthread_kill(*QRZ_thread, SIGUSR2);
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

	// test alert first as QRZ.com requires it be shown
	if (!qrzalert.empty()) {
		inpNotes->value(qrzalert.c_str());
		qrzalert.clear();
	}
	else if (!qrzerror.empty()) {
		inpNotes->value(qrzerror.c_str());
		qrzerror.clear();
	}
}

bool QRZLogin(string& sessionpage)
{
	bool ok = true;
	if (qrzSessionKey.empty()) {
		ok = getSessionKey(sessionpage);
		if (ok) ok = parseSessionKey(sessionpage);
	}
	if (!ok)
		REQ(QRZAlert);

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
	if (ok) {
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

// ----------------------------------------------------------------------------
// Hamcall specific functions
// ----------------------------------------------------------------------------

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

void QRZ_DETAILS_query()
{
	string qrzurl = "http://www.qrz.com/callsign.html?callsign=";
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

	{
		sigset_t usr2;
		sigemptyset(&usr2);
		sigaddset(&usr2, SIGUSR2);
		pthread_sigmask(SIG_UNBLOCK, &usr2, NULL);
	}

	for (;;) {
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
	default:
		LOG_ERROR("Bad query type %d", DB_query);
		return;
	}

	pthread_mutex_lock(&qrz_mutex);
	pthread_cond_signal(&qrz_cond);
	pthread_mutex_unlock(&qrz_mutex);
}
