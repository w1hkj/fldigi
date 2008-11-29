// textio.cxx

#include <config.h>

#include "textio.h"

char * cTextFile::adif_to_date( char *s) {
static char date[9];
	strcpy(date, "  /  /  ");
	for (int i = 0; i < 2; i++) {
		date[i+6] = s[i+2];
		date[i] = s[i+4];
		date[i+3] = s[i+6];
	}
	return date;
}

char * cTextFile::adif_to_time( char *s) {
static char time[6];
	strcpy(time, "  :  ");
	for (int i = 0; i < 2; i++) {
		time[i] = s[i];
		time[i+3] = s[i+2];
	}
	return time;
}


void cTextFile::makeHeader()
{
	snprintf(header, sizeof(header), RECFMT,
	"DATE", "GMT", "CALL", "NAME", "FREQ (MHZ)", "MODE", "RST OUT", "RST IN", "QTH", "COMMENT");	
} 


int cTextFile::writeFile (const char *fname, cQsoDb *db) {
	cQsoRec *pRec = (cQsoRec *)0;
	FILE *txtFile = fopen (fname, "w");
	if (!txtFile) return 1;
  
	if (txtFile) {
		fprintf (txtFile, header);
		for (int i = 0; i < db->nbrRecs(); i++) {
			pRec = db->getRec(i);
			if (pRec->getField(EXPORT)[0] == 'E') {
				fprintf (txtFile, RECFMT,
					adif_to_date (pRec->getField(QSO_DATE)),
					adif_to_time (pRec->getField(TIME_ON)),
					pRec->getField(CALL),
					pRec->getField(NAME),
					pRec->getField(FREQ),
					pRec->getField(MODE),
					pRec->getField(RST_SENT),
					pRec->getField(RST_RCVD),
					pRec->getField(QTH),
					pRec->getField(COMMENT));
				pRec->putField(EXPORT,"");
				db->qsoUpdRec(i, pRec);
			}
		}
		fclose (txtFile);
	}
	return 0;
}

