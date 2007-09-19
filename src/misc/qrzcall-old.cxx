// ----------------------------------------------------------------------------
// qrzcall.cxx  -- a part of fldigi
//
// Copyright (C) 2006
//		Dave Freese, W1HKJ
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <math.h>
#include <ctype.h>

#include <FL/fl_ask.H>

#include "threads.h"

#include "misc.h"
#include "configuration.h"

#include "qrzcall.h"
#include "main.h"
#include "Config.h"
#include "fl_digi.h"
#include "qrzlib.h"

using namespace std;

#define BEGIN_NAME		"Name:</td><td><b>"
#define BEGIN_ADDR1		"Addr1:</td><td><b>"
#define BEGIN_ADDR2		"Addr2:</td><td><b>"
#define BEGIN_COUNTRY	"Country:</td><td><b>"
#define BEGIN_GRID		"Grid:</td><td><b>"
#define BEGIN_BORN		"Born:</td><td><b>"
#define NOT_FOUND		"<b class=red>"
#define snip_end_RECORD		"</b>"

static string htmlpage = "";
static string host = "www.qrz.com";
static string detail;
static string callsign = "";
static string qrzname;
static string qrzaddr1;
static string qrzaddr2;
static string qrzcountry;
static string qrzborn;
static string qrzfname;
static string qrzqth;
static string qrzgrid;
static string qrznotes;

static const char *error[] = {
"OK",								// err 0
"Host not found", 					// err 1
"Not an IP host!", 					// err 2
"No http service",					// err 3
"Cannot open socket",				// err 4
"Cannot Connect to www.qrz.com",	// err 5
"Socket write error",				// err 6
"Socket timeout",					// err 7
"Socket select error"				// err 8
};

static char rbuffer[32768];

static fd_set readfds, testfds;
static struct timeval timeout;
static int sockfd = -1;
static int result;
static struct sockaddr_in address;
static struct hostent *hostinfo;
static struct servent *servinfo;

static int qrzdummy;
static Fl_Thread QRZ_thread;
static bool QRZ_exit = false;
static bool QRZ_enabled = false;
static bool QRZ_query = false;
static void *QRZloop(void *args);

QRZ *qCall;

bool parse_html()
{
	size_t snip, snip_end;

	qrzname = "";
	qrzfname = "";
	qrzqth = "";
	qrzaddr1 = "";
	qrzaddr2 = "";
	qrzcountry = "";
	qrzgrid = "";
	qrzborn = "";
	qrznotes = "";
	
	if (htmlpage.find(NOT_FOUND) != string::npos) {
		qrznotes = "NOT FOUND";
		return false;
	}


	snip = htmlpage.find(BEGIN_NAME);
	if (snip != string::npos) {
		snip += strlen(BEGIN_NAME);
		snip_end  = htmlpage.find(snip_end_RECORD, snip);
		qrzname = htmlpage.substr(snip, snip_end - snip);
		snip = qrzname.find(' ');
		qrzfname = qrzname.substr(0, snip);
		for (size_t i = 0; i < qrzfname.length(); i++)
			if (qrzfname[i] < ' ' || qrzfname[i] > 'z')
				qrzfname[i] = ' ';
		while ((snip = qrzfname.find(' ')) != string::npos)
			qrzfname.erase(snip, 1);
	}	
	
	snip = htmlpage.find(BEGIN_ADDR1);
	if (snip != string::npos) {
		snip += strlen(BEGIN_ADDR1);
		snip_end  = htmlpage.find(snip_end_RECORD, snip);
		qrzaddr1 = htmlpage.substr(snip, snip_end - snip);
	}	

	snip = htmlpage.find(BEGIN_ADDR2);
	if (snip != string::npos) {
		snip += strlen(BEGIN_ADDR2);
		snip_end  = htmlpage.find(snip_end_RECORD, snip);
		qrzaddr2 = htmlpage.substr(snip, snip_end - snip);
		qrzqth += qrzaddr2;
	}	

	snip = htmlpage.find(BEGIN_COUNTRY);
	if (snip != string::npos) {
		qrzqth += ", ";
		snip += strlen(BEGIN_COUNTRY);
		snip_end  = htmlpage.find(snip_end_RECORD, snip);
		qrzcountry = htmlpage.substr(snip, snip_end - snip);
		qrzqth += qrzcountry;
	}	

	snip = htmlpage.find(BEGIN_GRID);
	if (snip != string::npos) {
		snip += strlen(BEGIN_GRID);
		snip_end  = htmlpage.find(snip_end_RECORD, snip);
		qrzgrid = htmlpage.substr(snip, snip_end - snip);
	}	
	
	snip = htmlpage.find(BEGIN_BORN);
	if (snip != string::npos) {
		snip += strlen(BEGIN_BORN);
		snip_end  = htmlpage.find(snip_end_RECORD, snip);
		qrzborn = htmlpage.substr(snip, snip_end - snip);
		qrznotes = "Born: " + qrzborn;
	}	
	return true;
} 


int getRecord()
{
    hostinfo = gethostbyname(host.c_str());
    if(!hostinfo)
    	return 1;
    if(hostinfo->h_addrtype != AF_INET)
    	return 2;
    servinfo = getservbyname("http", "tcp");
    if(!servinfo)
    	return 3;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sockfd == -1)
		return 4;
    address.sin_family = AF_INET;
    address.sin_port = servinfo->s_port;
    address.sin_addr = *(struct in_addr *)*hostinfo->h_addr_list;

    result = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
    if(result == -1) {
		close(sockfd);
    	return 5;
	}

	detail = "GET /detail/";
	detail += callsign;
	detail += "\r\n";
	
	result = write(sockfd, detail.c_str() , detail.length());
	if (result != (int)detail.length()) {
		close(sockfd);
		return 6;
	}

	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);
	
	while (htmlpage.find("</html") == string::npos) {
		testfds = readfds;
		timeout.tv_sec = 5;		// timeout = 5 seconds
		timeout.tv_usec = 0;
		result = select(FD_SETSIZE, &testfds, (fd_set *)0, (fd_set *)0, &timeout);
		if (result == 0) {
			close(sockfd);
			return 7;
		}
		if (result == -1) {
			close(sockfd);
			return 8;
		}
		if (FD_ISSET(sockfd, &testfds)) {
			memset(rbuffer, 0, 32768);
			result = read(sockfd, rbuffer, sizeof(rbuffer));
			htmlpage += rbuffer;
		}
    }
    
    close(sockfd);
    return 0;
}

// code submitted by WA5ZNU
//#define MY_QRA ("CM87wk")
int bearing(const char *, const char *);
void qra(const char *, double &, double &);

int bearing(const char *myqra, const char *dxqra) {
	double	lat1, lat1r, lon1;
	double	lat2, lat2r, lon2;
	double	dlong, arg1, arg2a, arg2b, arg2, bearingr, bearing;
	double	k=180.0/M_PI;

	qra(dxqra, lat2, lon2);
	qra(myqra, lat1, lon1);
//std::cout << lat1 << " " << lon1 << std::endl << lat2 << " " << lon2 << std::endl; cout.flush();

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

    
// code submitted by WA5ZNU
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

void QRZ_disp_result()
{
	FL_LOCK();
		inpName->value(qrzfname.c_str());
		inpQth->value(qrzqth.c_str());
		inpLoc->value(qrzgrid.c_str());
// code provided by WA5ZNU
		if (!progdefaults.myLocator.empty()) {
			char buf[10];
			buf[0] = '\0';
			if (!qrzgrid.empty()) {
				int b = bearing( progdefaults.myLocator.c_str(), qrzgrid.c_str() );
				int br = (b + 180) % 360;
				snprintf(buf, 256, "%03d / %03d", b, br);
		  }
		  inpAZ->value(buf);
		}
		inpNotes->value(qrznotes.c_str());
	FL_UNLOCK();
}

void QRZ_COM_query()
{
	if (!QRZ_enabled) {
		QRZinit();
		if (!QRZ_enabled)
			return;
	}

	htmlpage = "";
	QRZ_query = true;
	FL_LOCK();
	inpNotes->value(" *** Request sent to qrz.com ***");
	FL_UNLOCK();
}

void QRZ_CD_query()
{
	char srch[20];
	size_t snip;
	
	memset( srch, 0, 20 );
	strncpy( srch, callsign.c_str(), 6 );
	for (size_t i = 0; i < strlen(srch); i ++ )
		srch[i] = toupper(srch[i]);

	if( qCall->FindRecord( srch ) == 1) {
		qrzfname = qCall->GetFname();
		snip = qrzfname.find(' ');
		if (snip != string::npos)
			qrzfname.erase(snip, qrzfname.length() - snip);
		qrzqth = qCall->GetCity();
		qrzqth.append(" ");
		qrzqth.append(qCall->GetState());
		qrzqth.append(" ");
		qrzqth.append(qCall->GetZIP());
		qrzgrid = "";
		qrznotes = "";
	} else {
		qrzfname = "";
		qrzqth = "";
		qrzgrid = "";
		qrzborn = "";
		qrznotes = "Not found in CD database!";
	}
	QRZ_disp_result();
}

void QRZquery()
{
	FL_LOCK();
		callsign = inpCall->value();
	FL_UNLOCK();
	
	if (callsign.length() == 0)
		return;
	if (progdefaults.QRZ == 0)
		return;
	
	if (progdefaults.QRZ == 2) {
		if (!qCall)
			qCall = new QRZ( "callbkc" );
		if (qCall && qCall->getQRZvalid())
			QRZ_CD_query();
	} else
		QRZ_COM_query();
}	

void QRZinit(void)
{
	QRZ_enabled = false;
	if (fl_create_thread(QRZ_thread, QRZloop, &qrzdummy) < 0) {
		fl_message("QRZ init: pthread_create failed");
		return;
	} 
	QRZ_enabled = true;
}

void QRZclose(void)
{
	if (!QRZ_enabled) return;
// tell the QRZ thread to kill it self
	QRZ_exit = true;
// and then wait for it to die
	fl_join(QRZ_thread);
//std::cout <<"QRZ down\n"; fflush(stdout);
	QRZ_enabled = false;
	QRZ_exit = false;
}

static void *QRZloop(void *args)
{
	SET_THREAD_ID(QRZ_TID);

	int err;
	for (;;) {
// see if we are being canceled
		if (QRZ_exit)
			break;
		if (QRZ_query) {
			err = getRecord();
			if (!err) {
				parse_html();
				QRZ_disp_result();
			} else {
				FL_LOCK();
				inpNotes->value(error[err]);
				FL_UNLOCK();
			}
			QRZ_query = false;
		}
		MilliSleep(100);
	}
	return NULL;
}

/* ---------------------------------------------------------------------- */

