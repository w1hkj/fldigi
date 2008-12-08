// textio.cxx

#include <config.h>
#include <string>
#include <iostream>

using namespace std;

#include "textio.h"
#include "lgbook.h"

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

void cTextFile::writeCSVHeader(FILE *txtFile)
{
	if (btnSelectQSOdate->value())   fprintf (txtFile, "DATE\t");
	if (btnSelectTimeON->value())    fprintf (txtFile, "ON\t");
	if (btnSelectTimeOFF->value())   fprintf (txtFile, "OFF\t");
	if (btnSelectCall->value())      fprintf (txtFile, "CALL\t");
	if (btnSelectName->value())      fprintf (txtFile, "NAME\t");
	if (btnSelectBand->value())      fprintf (txtFile, "BAND\t");
	if (btnSelectFreq->value())      fprintf (txtFile, "FREQ\t");
	if (btnSelectMode->value())      fprintf (txtFile, "MODE\t");
	if (btnSelectRSTsent->value())   fprintf (txtFile, "RSTSENT\t");
	if (btnSelectRSTrcvd->value())   fprintf (txtFile, "RSTRCVD\t");
	if (btnSelectQth->value())       fprintf (txtFile, "QTH\t");
	if (btnSelectState->value())     fprintf (txtFile, "ST\t");
	if (btnSelectProvince->value())  fprintf (txtFile, "PR\t");
	if (btnSelectCountry->value())   fprintf (txtFile, "CNTRY\t");
	if (btnSelectQSLrcvd->value())   fprintf (txtFile, "QSL_RCVD\t");
	if (btnSelectQSLsent->value())   fprintf (txtFile, "QSL_SENT\t");
	if (btnSelectComment->value())   fprintf (txtFile, "COMMENT\t");
	if (btnSelectSerialIN->value())  fprintf (txtFile, "SERIAL RCVD\t");
	if (btnSelectSerialOUT->value()) fprintf (txtFile, "SERIAL_SENT\t");
	if (btnSelectXchg1->value())     fprintf (txtFile, "XCHG1\t");
	if (btnSelectXchg2->value())     fprintf (txtFile, "XCHG2\t");
	if (btnSelectXchg3->value())     fprintf (txtFile, "XCHG3");
	fprintf (txtFile, "\n");
}

int cTextFile::writeCSVFile (const char *fname, cQsoDb *db) {
	cQsoRec *pRec = (cQsoRec *)0;
	FILE *txtFile = fopen (fname, "w");
	if (!txtFile) return 1;
  
	if (txtFile) {
		writeCSVHeader(txtFile);
		for (int i = 0; i < db->nbrRecs(); i++) {
			pRec = db->getRec(i);
			if (pRec->getField(EXPORT)[0] == 'E') {
				if (btnSelectQSOdate->value())
					fprintf (txtFile, "%s\t", pRec->getField(QSO_DATE));
				if (btnSelectTimeON->value())
					fprintf (txtFile, "%s\t", pRec->getField(TIME_ON));
				if (btnSelectTimeOFF->value())
					fprintf (txtFile, "%s\t", pRec->getField(TIME_OFF));
				if (btnSelectCall->value())
					fprintf (txtFile, "%s\t", pRec->getField(CALL));
				if (btnSelectName->value())
					fprintf (txtFile, "%s\t", pRec->getField(NAME));
				if (btnSelectBand->value())
					fprintf (txtFile, "%s\t", pRec->getField(BAND));
				if (btnSelectFreq->value())
					fprintf (txtFile, "%s\t", pRec->getField(FREQ));
				if (btnSelectMode->value())
					fprintf (txtFile, "%s\t", pRec->getField(MODE));
				if (btnSelectRSTsent->value())
					fprintf (txtFile, "%s\t", pRec->getField(RST_SENT));
				if (btnSelectRSTrcvd->value())
					fprintf (txtFile, "%s\t", pRec->getField(RST_RCVD));
				if (btnSelectQth->value())
					fprintf (txtFile, "%s\t", pRec->getField(QTH));
				if (btnSelectState->value())
					fprintf (txtFile, "%s\t", pRec->getField(STATE));
				if (btnSelectProvince->value())
					fprintf (txtFile, "%s\t", pRec->getField(VE_PROV));
				if (btnSelectCountry->value())
					fprintf (txtFile, "%s\t", pRec->getField(COUNTRY));
				if (btnSelectQSLrcvd->value())
					fprintf (txtFile, "%s\t", pRec->getField(QSL_RCVD));
				if (btnSelectQSLsent->value())
					fprintf (txtFile, "%s\t", pRec->getField(QSL_SENT));
				if (btnSelectComment->value()) {
					string temp = pRec->getField(COMMENT);
				for (size_t n = 0; n < temp.length(); n++)
					if (temp[n] == '\n') temp[n] = ';';
					fprintf (txtFile, "%s\t", temp.c_str());
				}
				if (btnSelectSerialIN->value())
					fprintf (txtFile, "%s\t", pRec->getField(SRX));
				if (btnSelectSerialOUT->value())
					fprintf (txtFile, "%s\t", pRec->getField(STX));
				if (btnSelectXchg1->value())
					fprintf (txtFile, "%s\t", pRec->getField(XCHG1));
				if (btnSelectXchg2->value())
					fprintf (txtFile, "%s\t", pRec->getField(XCHG2));
				if (btnSelectXchg3->value())
					fprintf (txtFile, "%s", pRec->getField(XCHG3));
				fprintf (txtFile, "\n");
				pRec->putField(EXPORT,"");
				db->qsoUpdRec(i, pRec);
			}
		}
		fclose (txtFile);
	}
	return 0;
}

// text file in fixed fields
void cTextFile::writeTXTHeader(FILE *txtFile)
{
	if (btnSelectQSOdate->value())   fprintf (txtFile, "%-10s", "DATE");
	if (btnSelectTimeON->value())    fprintf (txtFile, "%-6s", "ON");
	if (btnSelectTimeOFF->value())   fprintf (txtFile, "%-6s", "OFF");
	if (btnSelectCall->value())      fprintf (txtFile, "%-10s", "CALL");
	if (btnSelectName->value())      fprintf (txtFile, "%-15s", "NAME");
	if (btnSelectBand->value())      fprintf (txtFile, "%-7s", "BAND");
	if (btnSelectFreq->value())      fprintf (txtFile, "%-12s", "FREQ");
	if (btnSelectMode->value())      fprintf (txtFile, "%-8s", "MODE");
	if (btnSelectRSTsent->value())   fprintf (txtFile, "%-6s", "RSTX");
	if (btnSelectRSTrcvd->value())   fprintf (txtFile, "%-6s", "RSTR");
	if (btnSelectQth->value())       fprintf (txtFile, "%-20s", "QTH");
	if (btnSelectState->value())     fprintf (txtFile, "%-5s", "ST");
	if (btnSelectProvince->value())  fprintf (txtFile, "%-5s", "PR");
	if (btnSelectCountry->value())   fprintf (txtFile, "%-15s", "CNTRY");
	if (btnSelectQSLrcvd->value())   fprintf (txtFile, "%-10s", "QSLR");
	if (btnSelectQSLsent->value())   fprintf (txtFile, "%-10s", "QSLS");
	if (btnSelectComment->value())   fprintf (txtFile, "%-30s", "COMMENT");
	if (btnSelectSerialIN->value())  fprintf (txtFile, "%-7s", "SRX");
	if (btnSelectSerialOUT->value()) fprintf (txtFile, "%-7s", "STX");
	if (btnSelectXchg1->value())     fprintf (txtFile, "%-15s", "XCHG1");
	if (btnSelectXchg2->value())     fprintf (txtFile, "%-15s", "XCHG2");
	if (btnSelectXchg3->value())     fprintf (txtFile, "%-15s", "XCHG3");
	fprintf (txtFile, "\n");
}

int cTextFile::writeTXTFile (const char *fname, cQsoDb *db) {
	cQsoRec *pRec = (cQsoRec *)0;
	FILE *txtFile = fopen (fname, "w");
	if (!txtFile) return 1;
  
	if (txtFile) {
		writeTXTHeader(txtFile);
		for (int i = 0; i < db->nbrRecs(); i++) {
			pRec = db->getRec(i);
			if (pRec->getField(EXPORT)[0] == 'E') {
				if (btnSelectQSOdate->value())
					fprintf (txtFile, "%-10s", pRec->getField(QSO_DATE));
				if (btnSelectTimeON->value())
					fprintf (txtFile, "%-6s", pRec->getField(TIME_ON));
				if (btnSelectTimeOFF->value())
					fprintf (txtFile, "%-6s", pRec->getField(TIME_OFF));
				if (btnSelectCall->value())
					fprintf (txtFile, "%-10s", pRec->getField(CALL));
				if (btnSelectName->value())
					fprintf (txtFile, "%-15s", pRec->getField(NAME));
				if (btnSelectBand->value())
					fprintf (txtFile, "%-7s", pRec->getField(BAND));
				if (btnSelectFreq->value())
					fprintf (txtFile, "%-12s", pRec->getField(FREQ));
				if (btnSelectMode->value())
					fprintf (txtFile, "%-8s", pRec->getField(MODE));
				if (btnSelectRSTsent->value())
					fprintf (txtFile, "%-6s", pRec->getField(RST_SENT));
				if (btnSelectRSTrcvd->value())
					fprintf (txtFile, "%-6s", pRec->getField(RST_RCVD));
				if (btnSelectQth->value())
					fprintf (txtFile, "%-20s", pRec->getField(QTH));
				if (btnSelectState->value())
					fprintf (txtFile, "%-5s", pRec->getField(STATE));
				if (btnSelectProvince->value())
					fprintf (txtFile, "%-5s", pRec->getField(VE_PROV));
				if (btnSelectCountry->value())
					fprintf (txtFile, "%-15s", pRec->getField(COUNTRY));
				if (btnSelectQSLrcvd->value())
					fprintf (txtFile, "%-10s", pRec->getField(QSL_RCVD));
				if (btnSelectQSLsent->value())
					fprintf (txtFile, "%-10s", pRec->getField(QSL_SENT));
				if (btnSelectComment->value()) {
					string temp = pRec->getField(COMMENT);
				for (size_t n = 0; n < temp.length(); n++)
					if (temp[n] == '\n') temp[n] = ';';
					fprintf (txtFile, "-30%s", temp.c_str());
				}
				if (btnSelectSerialIN->value())
					fprintf (txtFile, "%-7s", pRec->getField(SRX));
				if (btnSelectSerialOUT->value())
					fprintf (txtFile, "%-7s", pRec->getField(STX));
				if (btnSelectXchg1->value())
					fprintf (txtFile, "%-15s", pRec->getField(XCHG1));
				if (btnSelectXchg2->value())
					fprintf (txtFile, "%-15s", pRec->getField(XCHG2));
				if (btnSelectXchg3->value())
					fprintf (txtFile, "%-15s", pRec->getField(XCHG3));
				fprintf (txtFile, "\n");
				pRec->putField(EXPORT,"");
				db->qsoUpdRec(i, pRec);
			}
		}
		fclose (txtFile);
	}
	return 0;
}

