#ifndef QSO_DB
#define QSO_DB

#include <iostream>

#include <stdlib.h>
#include <ctype.h>

#include "adif_def.h"

enum COMPTYPE {COMPTIME, COMPDATE, COMPCALL, COMPFREQ, COMPMODE};

class cQsoRec {

friend int compareCalls (const cQsoRec &, const cQsoRec &);
friend int compareDates (const cQsoRec &, const cQsoRec &);
friend int compareTimes (const cQsoRec &, const cQsoRec &);
friend int compareModes (const cQsoRec &, const cQsoRec &);
friend int compareFreqs (const cQsoRec &, const cQsoRec &);
friend std::ostream &operator<<( std::ostream &, const cQsoRec &);
friend std::istream &operator>>( std::istream &, cQsoRec & );

private:
  char *qsofield[NUMFIELDS];
  bool normal; // sort ordering
public:
  cQsoRec ();
  ~cQsoRec ();
  void putField (int, const char *);
  void addtoField (int, const char *);
  char *getField (int);
  void trimFields();
  void clearRec ();
  int  validRec();
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
public:
  cQsoDb ();
  ~cQsoDb ();
  static bool reverse;
  void deleteRecs();
  void clearDatabase();
  void isdirty(int n) {dirty = n;}
  int  isdirty() {return dirty;}
  void qsoNewRec (cQsoRec *);
  void qsoDelRec (int);
  void qsoUpdRec (int, cQsoRec *);
  int qsoFindRec (cQsoRec *);
  cQsoRec *getRec (int n) {return &qsorec[n];};
  int nbrRecs () {return nbrrecs;};
  bool qsoIsValidFile(const char *);
  int qsoReadFile (const char *);
  int qsoWriteFile (const char *);
  void SortByDate();
  void SortByCall ();
  void SortByMode ();
  void SortByFreq ();
  void sort_reverse(bool rev) { reverse = rev;}
};

#endif
