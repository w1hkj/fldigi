#ifndef ADIFIO
#define ADIFIO

#include <cstdio>

#include "qso_db.h"

#define ADIF_VERS "2.2.3"

class cAdifIO {
private:
	bool write_all;
	cQsoRec adifqso;
	FILE *adiFile;
	void fillfield(int, char *);
public:
	cAdifIO ();
	~cAdifIO () {};
	int readAdifRec () {return 0;};
	int writeAdifRec () {return 0;};
	void readFile (const char *, cQsoDb *);
	int writeFile (const char *, cQsoDb *);
	int writeLog (const char *, cQsoDb *);
};


#endif
