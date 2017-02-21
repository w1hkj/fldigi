// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef QSO_DB
#define QSO_DB

#include <iosfwd>
#include <string>
#include <cstring>

#include "adif_def.h"

using namespace std;

enum COMPTYPE {COMPTIME, COMPDATE, COMPCALL, COMPFREQ, COMPMODE};

class cQsoDb;
class cQsoRec;

class cQsoRec {

friend int compareCalls (const cQsoRec &, const cQsoRec &);
friend int compareDates (const cQsoRec &, const cQsoRec &);
friend int compareTimes (const cQsoRec &, const cQsoRec &);
friend int compareModes (const cQsoRec &, const cQsoRec &);
friend int compareFreqs (const cQsoRec &, const cQsoRec &);
friend std::ostream &operator<<( std::ostream &, const cQsoRec &);
friend std::istream &operator>>( std::istream &, cQsoRec & );

private:
	string *qsofield[NUMFIELDS];
	//bool normal; // sort ordering
public:
	cQsoRec ();
	~cQsoRec ();
	void putField (int, const char *);
	void putField (int, const char *, int);
	void addtoField (int, const char *);
	const char *getField (int) const;
	void trimFields();
	void clearRec ();
	int  validRec();
	void checkBand();
	void checkDateTimes();
	void setDateTime(bool dtOn);
	void setFrequency(long long freq);
// operator overloads
	const cQsoRec &operator=(const cQsoRec &);
	bool operator==(const cQsoRec &) const;
	bool operator<(const cQsoRec &) const;
	bool operator!=(const cQsoRec &right) const {
		return !( *this == right);
	}
	bool operator<=(const cQsoRec &right) const {
		if (*this < right || *this == right)
			return true;
		return false;
	}
	bool operator>(const cQsoRec &right) const {
		return !(*this <= right);
	}
};

class cQsoDb {
private:
	cQsoRec * qsorec;
	int maxrecs;
	int nbrrecs;
	int dirty;

	static const int jdays[][13];
	bool isleapyear( int y ) const;
	int dayofyear (int year, int mon, int mday);
	unsigned long epoch_dt (const char *szdate, const char *sztime);
public:
	cQsoDb ();
	cQsoDb (cQsoDb *);
	~cQsoDb ();
	static bool reverse;
	void deleteRecs();
	void clearDatabase();
	void isdirty(int n) {dirty = n;}
	int  isdirty() const {return dirty;}
	void qsoNewRec (cQsoRec *);
	cQsoRec *newrec();
	void qsoDelRec (int);
	void qsoUpdRec (int, cQsoRec *);
	int qsoFindRec (cQsoRec *);
	cQsoRec *getRec (int n) {return &qsorec[n];};
	int nbrRecs () const {return nbrrecs;};
	bool qsoIsValidFile(const char *);
	int qsoReadFile (const char *);
	int qsoWriteFile (const char *);
	void SortByDate(bool);
	void SortByCall ();
	void SortByMode ();
	void SortByFreq ();
	void sort_reverse(bool rev) { reverse = rev;}
	const cQsoRec *recarray() { return qsorec; }

	bool duplicate(
		const char *callsign,
		const char *date, const char *time, unsigned int interval, bool chkdatetime,
		const char *freq, bool chkfreq,
		const char *state, bool chkstate,
		const char *mode, bool chkmode,
		const char *xchg1, bool chkxchg1 );

	int matched( cQsoRec *rec );
};

extern int comparebydate(const void *p1, const void *p2);
extern int comparebymode(const void *p1, const void *p2);
extern int comparebycall(const void *p1, const void *p2);
extern int comparebyfreq(const void *p1, const void *p2);


#endif
