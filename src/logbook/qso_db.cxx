// ----------------------------------------------------------------------------
// qso_db.cxx
//
// Copyright (C) 2006-2009
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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include "qso_db.h"
#include "field_def.h"
#include "globals.h"

using namespace std;

// class cQsoRec

static int compby = COMPDATE;

bool cQsoDb::reverse = false;

cQsoRec::cQsoRec() {
	for (int i=0;i < NUMFIELDS; i++)
		qsofield[i] = "";
}

cQsoRec::~cQsoRec () {
}

void cQsoRec::clearRec () {
	for (int i = 0; i < NUMFIELDS; i++)
		qsofield[i].clear();
}

int cQsoRec::validRec() {
	return 0;
}

void cQsoRec::checkBand() {
	size_t flen = qsofield[FREQ].length(), blen = qsofield[BAND].length();
	if (flen == 0 && blen != 0)
		qsofield[FREQ] =  band_freq(qsofield[BAND].c_str());
	else if (blen == 0 && flen != 0)
		qsofield[BAND] =  band_name(qsofield[FREQ].c_str());
}

void cQsoRec::putField (int n, const char *s){
	if (n < 0 || n >= NUMFIELDS) return;
	qsofield[n] = s;
}

void cQsoRec::putField (int n, const char *s, int len) {
	if (n < 0 || n >= NUMFIELDS) return;
	qsofield[n].clear();
	qsofield[n].append(s, len);
}

void cQsoRec::addtoField (int n, const char *s){
	if (n < 0 || n >= NUMFIELDS) return;
	qsofield[n].append(s);
}

void cQsoRec::trimFields () {
	size_t p;
	string s;
	for (int i = 0; i < NUMFIELDS; i++) {
		s = qsofield[i];
//right trim string
		p = s.length();
		while (p && s[p-1] == ' ') {
			s.erase(p - 1);
			p = s.length();
		}
//left trim string
		p = s.length();
		while (p && s[0] == ' ') {
			s.erase(0,1);
			p = s.length();
		}
//make all upper case if Callsign or Mode  
		if (i == CALL || i == MODE) {
			for (p = 0; p < s.length(); p++)
				s[p] = toupper(s[p]);
		}
		qsofield[i] = s;
	}
}

const char * cQsoRec::getField (int n) {
	if (n < 0 || n >= NUMFIELDS) return 0;
	return (qsofield[n].c_str());
}

const cQsoRec &cQsoRec::operator=(const cQsoRec &right) {
	if (this != &right) {
		for (int i = 0; i < NUMFIELDS; i++)
			this->qsofield[i] = right.qsofield[i];
	}
	return *this;
}

int compareTimes (const cQsoRec &r1, const cQsoRec &r2) {
	if (r1.qsofield[TIME_OFF] < r2.qsofield[TIME_OFF])
		return -1;
	if (r1.qsofield[TIME_OFF] > r2.qsofield[TIME_OFF])
		return 1;
	return 0;
}

int compareDates (const cQsoRec &r1, const cQsoRec &r2) {
	if (r1.qsofield[QSO_DATE] < r2.qsofield[QSO_DATE])
		return -1;
	if (r1.qsofield[QSO_DATE] > r2.qsofield[QSO_DATE])
		return 1;
	return compareTimes (r1,r2);
}

int compareCalls (const cQsoRec &r1, const cQsoRec &r2) {
	int cmp = 0;
	char *s1 = new char[r1.qsofield[CALL].length() + 1];
	char *s2 = new char[r2.qsofield[CALL].length() + 1];
	char *p1, *p2;
	strcpy(s1, r1.qsofield[CALL].c_str());
	strcpy(s2, r2.qsofield[CALL].c_str());
	p1 = strpbrk (&s1[1], "0123456789");
	p2 = strpbrk (&s2[1], "0123456789");
	if (p1 && p2) {
		cmp = (*p1 < *p2) ? -1 :(*p1 > *p2) ? 1 : 0;
		if (cmp == 0) {
			*p1 = 0; *p2 = 0;
			cmp = strcmp (s1, s2);
			if (cmp == 0)
				cmp = strcmp(p1+1, p2+1);
		}
	} else
		cmp = (r1.qsofield[CALL] == r2.qsofield[CALL]);
	delete [] s1;
	delete [] s2;
	if (cmp == 0)
		return compareDates (r1,r2);
	return cmp;
}

int compareModes (const cQsoRec &r1, const cQsoRec &r2) {
	if (r1.qsofield[MODE] < r2.qsofield[MODE])
		return -1;
	if (r1.qsofield[MODE] > r2.qsofield[MODE])
		return 1;
	return compareDates (r1,r2);
}

int compareFreqs (const cQsoRec &r1, const cQsoRec &r2) {
	int cmp = 0;
	double f1, f2;
	f1 = atof(r1.qsofield[FREQ].c_str());
	f2 = atof(r2.qsofield[FREQ].c_str());
	if (f1 == f2) cmp = 0;
	else if (f1 < f2) return -1;
	else if (f1 > f2) return 1;
	return compareDates (r1,r2);
}

int compareqsos (const void *p1, const void *p2) {
	cQsoRec *r1, *r2;
	if (cQsoDb::reverse) {
		r2 = (cQsoRec *)p1;
		r1 = (cQsoRec *)p2;
	} else {
		r1 = (cQsoRec *)p1;
		r2 = (cQsoRec *)p2;
	}

	switch (compby) {
		case COMPCALL :
			return compareCalls (*r1, *r2);
		case COMPMODE :
			return compareModes (*r1, *r2);
		case COMPFREQ :
			return compareFreqs (*r1, *r2);
		case COMPDATE :
		default :
			return compareDates (*r1, *r2);
	}
}

bool cQsoRec::operator==(const cQsoRec &right) const {
	if (compareDates (*this, right) != 0) return false;
	if (compareTimes (*this, right) != 0) return false;
	if (compareCalls (*this, right) != 0) return false;
	if (compareFreqs (*this, right) != 0) return false;
	return true;
}

bool cQsoRec::operator<(const cQsoRec &right) const {
	if (compareDates (*this, right) > -1) return false;
	if (compareTimes (*this, right) > -1) return false;
	if (compareCalls (*this, right) > -1) return false;
	if (compareFreqs (*this, right) > -1) return false;
	return true;
}

static char delim_in = '\t';
static char delim_out = '\t';
static bool isVer3 = false;

ostream &operator<< (ostream &output, const cQsoRec &rec) {
	for (int i = 0; i < EXPORT; i++)
		output << rec.qsofield[i].c_str() << delim_out;
	return output;
}

istream &operator>> (istream &input, cQsoRec &rec ) {
	char c;
	int i;
	for (i = 0; i < NUMFIELDS; i++) {
		rec.qsofield[i].clear();
		c = input.get();
		while (c != delim_in && c != EOF) {
			rec.qsofield[i] += c;
			c = input.get();
		}
	}
	return input;
}

//======================================================================
// class cQsoDb

#define MAXRECS 8192

cQsoDb::cQsoDb() {
  nbrrecs = 0;
  maxrecs = MAXRECS;
  qsorec = new cQsoRec[maxrecs];
  compby = COMPDATE;
  dirty = 0;
}

cQsoDb::~cQsoDb() {
  delete [] qsorec;
} 

void cQsoDb::deleteRecs() {
  delete [] qsorec;
  nbrrecs = 0;
  maxrecs = MAXRECS;
  qsorec = new cQsoRec[maxrecs];
  dirty = 0;
}

void cQsoDb::clearDatabase() {
  deleteRecs();
}

int cQsoDb::qsoFindRec(cQsoRec *rec) {
  for (int i = 0; i < nbrrecs; i++)
    if (qsorec[i] == *rec)
      return i;
  return -1;
}

void cQsoDb::qsoNewRec (cQsoRec *nurec) {
  if (nbrrecs == maxrecs) {
    maxrecs *= 2;
    cQsoRec *atemp = new cQsoRec[maxrecs];
    for (int i = 0; i < nbrrecs; i++)
        atemp[i] = qsorec[i];
    delete [] qsorec;
    qsorec = atemp;
  }
  qsorec[nbrrecs] = *nurec;
  qsorec[nbrrecs].checkBand();
  nbrrecs++;
  dirty = 1;
}

void cQsoDb::qsoDelRec (int rnbr) {
  if (rnbr < 0 || rnbr > (nbrrecs - 1)) 
    return;
  for (int i = rnbr; i < nbrrecs - 1; i++)
    qsorec[i] = qsorec[i+1];
  nbrrecs--;
  qsorec[nbrrecs].clearRec();
}
  
void cQsoDb::qsoUpdRec (int rnbr, cQsoRec *updrec) {
  if (rnbr < 0 || rnbr > (nbrrecs - 1))
    return;
  qsorec[rnbr] = *updrec;
  qsorec[rnbr].checkBand();
  return;
}

void cQsoDb::SortByDate () {
  compby = COMPDATE;
  qsort (qsorec, nbrrecs, sizeof (cQsoRec), compareqsos);
}

void cQsoDb::SortByCall () {
  compby = COMPCALL;
  qsort (qsorec, nbrrecs, sizeof (cQsoRec), compareqsos);
}

void cQsoDb::SortByMode () {
  compby = COMPMODE;
  qsort (qsorec, nbrrecs, sizeof (cQsoRec), compareqsos);
}

void cQsoDb::SortByFreq () {
	compby = COMPFREQ;
	qsort (qsorec, nbrrecs, sizeof (cQsoRec), compareqsos);
}

bool cQsoDb::qsoIsValidFile(const char *fname) {
  char buff[256];
  ifstream inQsoFile (fname, ios::in);
  if (!inQsoFile)
    return false;
  inQsoFile.getline (buff, 256);
  if (strstr (buff, "_LOGBODUP DB") == 0) {
    inQsoFile.close();
    return false;
  }
  inQsoFile.close();
  return true;
}

int cQsoDb::qsoReadFile (const char *fname) {
char buff[256];
  ifstream inQsoFile (fname, ios::in);
  if (!inQsoFile)
    return 1;
  inQsoFile.getline (buff, 256);
  if (strstr (buff, "_LOGBODUP DB") == 0) {
    inQsoFile.close();
    return 2;
  }
  if (strstr (buff, "_LOGBODUP DBX") == 0) // new file format
    delim_in = '\n';
  if (strstr (buff, "3.0") != 0)
	isVer3 = true;    
  
  cQsoRec inprec;
  while (inQsoFile >> inprec) {
    qsoNewRec (&inprec);
    inprec.clearRec();
  }
  inQsoFile.close();
  SortByDate();
  return 0;
}

int cQsoDb::qsoWriteFile (const char *fname) {
  ofstream outQsoFile (fname, ios::out);
  if (!outQsoFile) {
  	printf("write failure: %s\n", fname);
    return 1;
  }
  outQsoFile << "_LOGBODUP DBX 3.0" << '\n';
  for (int i = 0; i < nbrrecs; i++)
    outQsoFile << qsorec[i];
  outQsoFile.close();
  return 0;
}

const int cQsoDb::jdays[2][13] = {
  { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
  { 0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 }
};

bool cQsoDb::isleapyear( int y )
{
  if( y % 400 == 0 || ( y % 100 != 0 && y % 4 == 0 ) )
    return true;
  return false;
}

int cQsoDb::dayofyear (int year, int mon, int mday)
{
  return mday + jdays[isleapyear (year) ? 1 : 0][mon];
}

unsigned int cQsoDb::epoch_minutes (const char *szdate, const char *sztime)
{
  unsigned int  doe;
  int  era, cent, quad, rest;
  int year, mon, mday;
  int mins;
  
  year = ((szdate[0]*10 + szdate[1])*10 + szdate[2])*10 + szdate[3];
  mon  = szdate[4]*10 + szdate[5];
  mday = szdate[6]*10 + szdate[7];
  
  mins = (sztime[0]*10 + sztime[1])*60 + sztime[2]*10 + sztime[3];
  
  /* break down the year into 400, 100, 4, and 1 year multiples */
  rest = year - 1;
  quad = rest / 4;        rest %= 4;
  cent = quad / 25;       quad %= 25;
  era = cent / 4;         cent %= 4;
  
  /* set up doe */
  doe = dayofyear (year, mon, mday);
  doe += era * (400 * 365 + 97);
  doe += cent * (100 * 365 + 24);
  doe += quad * (4 * 365 + 1);
  doe += rest * 365;
  
  return doe*60*24 + mins;
}

bool cQsoDb::duplicate(
		const char *callsign, 
		const char *szdate, const char *sztime, unsigned int interval, bool chkdatetime,
		const char *freq, bool chkfreq,
		const char *state, bool chkstate,
		const char *mode, bool chkmode,
		const char *xchg1, bool chkxchg1 )
{
	int f1, f2 = 0;
	f1 = (int)(atof(freq)/1000.0);
	bool b_freqDUP = true, b_stateDUP = true, b_modeDUP = true,
		 b_xchg1DUP = true,
		 b_dtimeDUP = true;
	unsigned int datetime = epoch_minutes(szdate, sztime);
	unsigned int qsodatetime;
	
	for (int i = 0; i < nbrrecs; i++) {
		if (strcasecmp(qsorec[i].getField(CALL), callsign) == 0) {
// found callsign duplicate
			b_freqDUP = b_stateDUP = b_modeDUP = 
				   	   b_xchg1DUP = b_dtimeDUP = false;
			if (chkfreq) {
				f2 = (int)atof(qsorec[i].getField(FREQ));
				b_freqDUP = (f1 == f2);
			}
			if (chkstate)
				b_stateDUP = (qsorec[i].getField(STATE)[0] == 0 && state[0] == 0) ||
							 (strcasestr(qsorec[i].getField(STATE), state) != 0);
			if (chkmode)
				b_modeDUP  = (qsorec[i].getField(MODE)[0] == 0 && mode[0] == 0) ||
							 (strcasestr(qsorec[i].getField(MODE), mode) != 0);
			if (chkxchg1)
				b_xchg1DUP = (qsorec[i].getField(XCHG1)[0] == 0 && xchg1[0] == 0) ||
							 (strcasestr(qsorec[i].getField(XCHG1), xchg1) != 0);

			if (chkdatetime) {
				qsodatetime = epoch_minutes (
								qsorec[i].getField(QSO_DATE),
								qsorec[i].getField(TIME_OFF));
				if ((datetime - qsodatetime) < interval) b_dtimeDUP = true;
			}
 			if ( (!chkfreq     || (chkfreq     && b_freqDUP)) &&
			     (!chkstate    || (chkstate    && b_stateDUP)) &&
			     (!chkmode     || (chkmode     && b_modeDUP)) &&
			     (!chkxchg1    || (chkxchg1    && b_xchg1DUP)) &&
			     (!chkdatetime || (chkdatetime && b_dtimeDUP))) {
			     return true;
			 }
		}
	}
	return false;
}

