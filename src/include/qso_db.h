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
	string qsofield[NUMFIELDS];
	bool normal; // sort ordering
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

};

#endif
