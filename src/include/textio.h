#ifndef MULTIPSK_H
#define MULTIPSK_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "qso_db.h"

class cTextFile {
private:
#define RECFMT "%-8s|%-5s|%-15s|%-18s|%-12s|%-10s|%-7s|%-7s|%-35s|%-40s\n"
  char header[120];
  void makeHeader();
  char *adif_to_date( char *s);
  char *adif_to_time( char *s);
public:
  cTextFile () {makeHeader();}
  ~cTextFile () {};
  int writeFile (char *, cQsoDb *);
};

#endif

