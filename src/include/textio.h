#ifndef MULTIPSK_H
#define MULTIPSK_H

#include "qso_db.h"

class cTextFile {
private:
  char header[120];
  void makeHeader();
  char *adif_to_date( char *s);
  char *adif_to_time( char *s);
public:
  cTextFile () {};
  ~cTextFile () {};
  void writeCSVHeader(FILE *);
  int writeCSVFile (const char *, cQsoDb *);
  void writeTXTHeader(FILE *);
  int writeTXTFile (const char *, cQsoDb *);
};

#endif
