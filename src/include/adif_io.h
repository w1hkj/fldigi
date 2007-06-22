#ifndef ADIFIO
#define ADIFIO

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "qso_db.h"

#define ADIF_VERS "2.0"

class cAdifIO {
private:
  cQsoRec adifqso;
  FILE *adiFile;
  void fillfield(int, char *);
public:
  cAdifIO () {};
  ~cAdifIO () {};
  int readAdifRec () {return 0;};
  int writeAdifRec () {return 0;};
  int readFile (char *, cQsoDb *);
  int writeFile (char *, cQsoDb *);
  
};

#endif
