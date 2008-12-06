#include <config.h>
#include <fstream>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include "qso_db.h"
#include "field_def.h"

using namespace std;

// class cQsoRec

static int compby = COMPDATE;

bool cQsoDb::reverse = false;

cQsoRec::cQsoRec() {
  for (int i=0;i < NUMFIELDS; i++) {
    qsofield[i] = new char [fields[i].size + 1];
    memset (qsofield[i],0, fields[i].size + 1);
  }
}

cQsoRec::~cQsoRec () {
  for (int i = 0; i < NUMFIELDS; i++)
    delete qsofield[i];
}

void cQsoRec::clearRec () {
  for (int i = 0; i < NUMFIELDS; i++)
    memset(qsofield[i], 0, fields[i].size + 1);
}

int cQsoRec::validRec() {
  return 0;
}

void cQsoRec::checkBand() {
	if (strlen(qsofield[FREQ]) == 0 && strlen(qsofield[BAND]) == 0) return;
	if (strlen(qsofield[FREQ]) > 0 && strlen(qsofield[BAND]) > 0) return;
	if (strlen(qsofield[FREQ]) == 0) {
		if (strcmp(qsofield[BAND],"160m")==0) strcpy(qsofield[FREQ],"1.8");
		else if (strcasecmp(qsofield[BAND],"80m")==0) strcpy(qsofield[FREQ],"3.5");
		else if (strcasecmp(qsofield[BAND],"60m")==0) strcpy(qsofield[FREQ],"5.3");
		else if (strcasecmp(qsofield[BAND],"40m")==0) strcpy(qsofield[FREQ],"7.0");
		else if (strcasecmp(qsofield[BAND],"30m")==0) strcpy(qsofield[FREQ],"10.0");
		else if (strcasecmp(qsofield[BAND],"20m")==0) strcpy(qsofield[FREQ],"14.0");
		else if (strcasecmp(qsofield[BAND],"17m")==0) strcpy(qsofield[FREQ],"18.0");
		else if (strcasecmp(qsofield[BAND],"15m")==0) strcpy(qsofield[FREQ],"21.0");
		else if (strcasecmp(qsofield[BAND],"12m")==0) strcpy(qsofield[FREQ],"24.0");
		else if (strcasecmp(qsofield[BAND],"10m")==0) strcpy(qsofield[FREQ],"28.0");
		else if (strcasecmp(qsofield[BAND],"6m")==0) strcpy(qsofield[FREQ],"50.0");
		else if (strcasecmp(qsofield[BAND],"2m")==0) strcpy(qsofield[FREQ],"144.0");
		else if (strcasecmp(qsofield[BAND],"1.25m")==0) strcpy(qsofield[FREQ],"222.0");
		else if (strcasecmp(qsofield[BAND],"70cm")==0) strcpy(qsofield[FREQ],"420.0");
		return;
	} else if (strlen(qsofield[BAND]) == 0) {
		double mhz = atof(qsofield[FREQ]);
		if (mhz < 3.5) strcpy(qsofield[BAND],"160m");
		else if (mhz < 5.3) strcpy(qsofield[BAND],"80m");
		else if (mhz < 7.0) strcpy(qsofield[BAND],"60m");
		else if (mhz < 10.0) strcpy(qsofield[BAND],"40m");
		else if (mhz < 14.0) strcpy(qsofield[BAND],"30m");
		else if (mhz < 18.0) strcpy(qsofield[BAND],"20m");
		else if (mhz < 21.0) strcpy(qsofield[BAND],"17m");
		else if (mhz < 24.0) strcpy(qsofield[BAND],"15m");
		else if (mhz < 28.0) strcpy(qsofield[BAND],"12m");
		else if (mhz < 50.0) strcpy(qsofield[BAND],"10m");
		else if (mhz < 144.0) strcpy(qsofield[BAND],"6m");
		else if (mhz < 222.0) strcpy(qsofield[BAND],"2m");
		else if (mhz < 420.0) strcpy(qsofield[BAND],"1.25m");
		else if (mhz < 900.0) strcpy(qsofield[BAND],"70cm");
		return;
	}
}

void cQsoRec::putField (int n, const char *s){
  if (n < 0 || n >= NUMFIELDS) return;
  strncpy( qsofield[n], s, fields[n].size);
}

void cQsoRec::addtoField (int n, const char *s){
	if (n < 0 || n >= NUMFIELDS) return;
	char *temp = new char[strlen(qsofield[n]) + strlen(s) + 1];
	strcpy(temp, qsofield[n]);
	strcat(temp, s);
	strncpy(qsofield[n], temp, fields[n].size);
}

void cQsoRec::trimFields () {
char *p;
  for (int i = 0; i < NUMFIELDS; i++) {
//right trim string
    p = qsofield[i] + strlen(qsofield[i]) - 1;
    while (*p == ' ' && p >= qsofield[i]) *p-- = 0;
//left trim string
	p = qsofield[i];
	while (*p == ' ') strcpy(p, p+1);
//make all upper case if Callsign or Mode  
    if (i == CALL || i == MODE){
      p = qsofield[i];
      while (*p) {*p = toupper(*p); p++;}
    }
  }
}

char * cQsoRec::getField (int n) {
  if (n < 0 || n >= NUMFIELDS) return 0;
  return (qsofield[n]);
}

const cQsoRec &cQsoRec::operator=(const cQsoRec &right) {
  if (this != &right) {
    for (int i = 0; i < NUMFIELDS; i++)
      strncpy( this->qsofield[i], right.qsofield[i], fields[i].size);
   }
  return *this;
}

int compareTimes (const cQsoRec &r1, const cQsoRec &r2) {
  return  strcmp (r1.qsofield[TIME_OFF], r2.qsofield[TIME_OFF]);
}

int compareDates (const cQsoRec &r1, const cQsoRec &r2) {
int cmp = 0;
  cmp = strcmp (r1.qsofield[QSO_DATE], r2.qsofield[QSO_DATE]);
  if (cmp == 0)
    return compareTimes (r1,r2);
  return cmp;
}

int compareCalls (const cQsoRec &r1, const cQsoRec &r2) {
  int cmp = 0;
  char *s1 = new char[strlen(r1.qsofield[CALL]) + 1];
  char *s2 = new char[strlen(r2.qsofield[CALL]) + 1];
  char *p1, *p2;
  strcpy(s1, r1.qsofield[CALL]);
  strcpy(s2, r2.qsofield[CALL]);
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
    cmp = strcmp (r1.qsofield[CALL], r2.qsofield[CALL]);
  if (cmp == 0)
    return compareDates (r1,r2);
  return cmp;
}

int compareModes (const cQsoRec &r1, const cQsoRec &r2) {
  int cmp = 0;
  cmp = strcmp (r1.qsofield[MODE], r2.qsofield[MODE]);
  if (cmp == 0)
    return compareDates (r1,r2);
  return cmp;
}

int compareFreqs (const cQsoRec &r1, const cQsoRec &r2) {
  int cmp = 0;
  double f1, f2;
  f1 = atof(r1.qsofield[FREQ]);
  f2 = atof(r2.qsofield[FREQ]);
  if (f1 == f2) cmp = 0;
  else if (f1 < f2) cmp = -1;
  else cmp = 1;
  if (cmp == 0)
    return compareDates (r1,r2);
  return cmp;
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
    output << rec.qsofield[i] << delim_out;
  return output;
}

istream &operator>> (istream &input, cQsoRec &rec ) {
char c, *fld;
int i, j, max;
  for (i = 0; i < (isVer3 ? EXPORT : IOTA); i++) {
    j = 0;
    fld = rec.qsofield[i];
    max = fields[i].size;
    c = input.get();
    while (c != delim_in && c != EOF) {
      if (j++ < max) *fld++ = c;
      c = input.get();
    }
    *fld = 0;
  }
  return input;
}

//======================================================================
// class cQsoDb

cQsoDb::cQsoDb() {
  nbrrecs = 0;
  maxrecs = 1;
  qsorec = new cQsoRec[1];
  compby = COMPDATE;
  dirty = 0;
}

cQsoDb::~cQsoDb() {
  delete [] qsorec;
} 

void cQsoDb::deleteRecs() {
  delete [] qsorec;
  qsorec = new cQsoRec[1];
  nbrrecs = 0;
  maxrecs = 1;
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
  nurec->trimFields();
  if (qsoFindRec(nurec) > -1) return;
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
  if (strstr (buff, "_LOGBOOK DB") == 0) {
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
  if (strstr (buff, "_LOGBOOK DB") == 0) {
    inQsoFile.close();
    return 2;
  }
  if (strstr (buff, "_LOGBOOK DBX") == 0) // new file format
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
  outQsoFile << "_LOGBOOK DBX 3.0" << '\n';
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
		const char *xchg1, bool chkxchg1,
		const char *xchg2, bool chkxchg2,
		const char *xchg3, bool chkxchg3 )
{
	int f1, f2;
	f1 = (int)(atof(freq)/1000.0);
	bool b_freqOK = true, b_stateOK = true, b_modeOK = true,
		 b_xchg1OK = true, b_xchg2OK = true, b_xchg3OK = true,
		 b_dtimeOK = true;
	unsigned int datetime = epoch_minutes(szdate, sztime);
	unsigned int qsodatetime;
	
	for (int i = 0; i < nbrrecs; i++) {
		if (strcasecmp(qsorec[i].getField(CALL), callsign) == 0) {
// found callsign duplicate
			if (chkfreq) { // test integer part of frequency
				f2 = (int)(atof(qsorec[i].getField(FREQ))/1000.0);
				b_freqOK = (f1 == f2);
			}
			if (chkstate)
				b_stateOK = (strcasecmp(qsorec[i].getField(STATE), state) == 0);

			if (chkmode)
				b_modeOK = (strcasecmp(qsorec[i].getField(MODE), mode) == 0);

			if (chkxchg1)
				b_xchg1OK = (strcasecmp(qsorec[i].getField(XCHG1), xchg1) == 0);

			if (chkxchg2)
				b_xchg2OK = (strcasecmp(qsorec[i].getField(XCHG2), xchg2) == 0);

			if (chkxchg3)
				b_xchg3OK = (strcasecmp(qsorec[i].getField(XCHG3), xchg3) == 0);

			if (chkdatetime) {
				qsodatetime = epoch_minutes (
								qsorec[i].getField(QSO_DATE),
								qsorec[i].getField(TIME_OFF));
				if ((datetime - qsodatetime) >= interval) b_dtimeOK = false;
			}
// all must be true for a dup.
			if (b_freqOK && b_stateOK && b_modeOK && 
			    b_xchg1OK && b_xchg2OK && b_xchg3OK && 
			    b_dtimeOK)
				return true;
		}
	}
	return false;
}

