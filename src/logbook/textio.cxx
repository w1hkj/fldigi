// textio.cxx

#include <config.h>

#include <string>
#include <cstdio>
#include <cstring>

#include "textio.h"
#include "lgbook.h"

using namespace std;

#ifdef __WOE32__
static const char *szEOL = "\r\n";
#else
static const char *szEOL = "\n";
#endif

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
	if (btnSelectTX_pwr->value())    fprintf (txtFile, "TX_PWR\t");
	if (btnSelectRSTsent->value())   fprintf (txtFile, "RSTSENT\t");
	if (btnSelectRSTrcvd->value())   fprintf (txtFile, "RSTRCVD\t");
	if (btnSelectQth->value())       fprintf (txtFile, "QTH\t");
	if (btnSelectState->value())     fprintf (txtFile, "ST\t");
	if (btnSelectProvince->value())  fprintf (txtFile, "PR\t");
	if (btnSelectCountry->value())   fprintf (txtFile, "CNTRY\t");
	if (btnSelectDXCC->value())      fprintf (txtFile, "DXCC\t");
	if (btnSelectIOTA->value())      fprintf (txtFile, "IOTA\t");
	if (btnSelectCONT->value())      fprintf (txtFile, "CONT\t");
	if (btnSelectITUZ->value())      fprintf (txtFile, "ITUZ\t");
	if (btnSelectQSLrcvd->value())   fprintf (txtFile, "QSL_RCVD\t");
	if (btnSelectQSLsent->value())   fprintf (txtFile, "QSL_SENT\t");
	if (btnSelectNotes->value())     fprintf (txtFile, "NOTES\t");
	if (btnSelectSerialIN->value())  fprintf (txtFile, "SERIAL RCVD\t");
	if (btnSelectSerialOUT->value()) fprintf (txtFile, "SERIAL_SENT\t");
	if (btnSelectXchgIn->value())    fprintf (txtFile, "XCHG1\t");
	if (btnSelectMyXchg->value())    fprintf (txtFile, "MYXCHG");
	fprintf (txtFile, szEOL);
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
				if (btnSelectTX_pwr->value())
					fprintf (txtFile, "%s\t", pRec->getField(TX_PWR));
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
				if (btnSelectDXCC->value())
					fprintf (txtFile, "%s\t", pRec->getField(DXCC));
				if (btnSelectIOTA->value())
					fprintf (txtFile, "%s\t", pRec->getField(IOTA));
				if (btnSelectCONT->value())
					fprintf (txtFile, "%s\t", pRec->getField(CONT));
				if (btnSelectITUZ->value())
					fprintf (txtFile, "%s\t", pRec->getField(ITUZ));
				if (btnSelectQSLrcvd->value())
					fprintf (txtFile, "%s\t", pRec->getField(QSL_RCVD));
				if (btnSelectQSLsent->value())
					fprintf (txtFile, "%s\t", pRec->getField(QSL_SENT));
				if (btnSelectNotes->value()) {
					string temp = pRec->getField(NOTES);
    				for (size_t n = 0; n < temp.length(); n++)
	    				if (temp[n] == '\n') temp[n] = ';';
		    			fprintf (txtFile, "%s\t", temp.c_str());
				}
				if (btnSelectSerialIN->value())
					fprintf (txtFile, "%s\t", pRec->getField(SRX));
				if (btnSelectSerialOUT->value())
					fprintf (txtFile, "%s\t", pRec->getField(STX));
				if (btnSelectXchgIn->value())
					fprintf (txtFile, "%s\t", pRec->getField(XCHG1));
				if (btnSelectMyXchg->value())
					fprintf (txtFile, "%s", pRec->getField(MYXCHG));
				fprintf (txtFile, szEOL);
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
	if (btnSelectTX_pwr->value())    fprintf (txtFile, "%-8s", "TX_PWR");
	
	if (btnSelectRSTsent->value())   fprintf (txtFile, "%-6s", "RSTX");
	if (btnSelectRSTrcvd->value())   fprintf (txtFile, "%-6s", "RSTR");
	if (btnSelectQth->value())       fprintf (txtFile, "%-20s", "QTH");
	if (btnSelectState->value())     fprintf (txtFile, "%-5s", "ST");
	if (btnSelectProvince->value())  fprintf (txtFile, "%-5s", "PR");
	if (btnSelectCountry->value())   fprintf (txtFile, "%-15s", "CNTRY");
	if (btnSelectDXCC->value())      fprintf (txtFile, "%-8s",  "DXCC");
	if (btnSelectIOTA->value())      fprintf (txtFile, "%-8s", "IOTA");
	if (btnSelectCONT->value())      fprintf (txtFile, "%-8s",  "CONT");
	if (btnSelectITUZ->value())      fprintf (txtFile, "%-8s",  "ITUZ");
	
	if (btnSelectQSLrcvd->value())   fprintf (txtFile, "%-10s", "QSLR");
	if (btnSelectQSLsent->value())   fprintf (txtFile, "%-10s", "QSLS");
	if (btnSelectNotes->value())     fprintf (txtFile, "%-80s", "NOTES");
	if (btnSelectSerialIN->value())  fprintf (txtFile, "%-7s", "SRX");
	if (btnSelectSerialOUT->value()) fprintf (txtFile, "%-7s", "STX");
	if (btnSelectXchgIn->value())    fprintf (txtFile, "%-15s", "XCHG1");
	if (btnSelectMyXchg->value())    fprintf (txtFile, "%-15s", "MYXCHG");
	fprintf (txtFile, szEOL);
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
				if (btnSelectTX_pwr->value())
					fprintf (txtFile, "%-8s", pRec->getField(TX_PWR));
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
				if (btnSelectDXCC->value())
					fprintf (txtFile, "%-8s", pRec->getField(DXCC));
				if (btnSelectIOTA->value())
					fprintf (txtFile, "%-8s", pRec->getField(IOTA));
				if (btnSelectCONT->value())
					fprintf (txtFile, "%-8s", pRec->getField(CONT));
				if (btnSelectITUZ->value())
					fprintf (txtFile, "%-8s", pRec->getField(ITUZ));
				if (btnSelectQSLrcvd->value())
					fprintf (txtFile, "%-10s", pRec->getField(QSL_RCVD));
				if (btnSelectQSLsent->value())
					fprintf (txtFile, "%-10s", pRec->getField(QSL_SENT));
				if (btnSelectNotes->value()) {
					string temp = pRec->getField(NOTES);
				for (size_t n = 0; n < temp.length(); n++)
					if (temp[n] == '\n') temp[n] = ';';
					fprintf (txtFile, "%-80s", temp.c_str());
				}
				if (btnSelectSerialIN->value())
					fprintf (txtFile, "%-7s", pRec->getField(SRX));
				if (btnSelectSerialOUT->value())
					fprintf (txtFile, "%-7s", pRec->getField(STX));
				if (btnSelectXchgIn->value())
					fprintf (txtFile, "%-15s", pRec->getField(XCHG1));
				if (btnSelectMyXchg->value())
					fprintf (txtFile, "%-15s", pRec->getField(MYXCHG));
				fprintf (txtFile, szEOL);
				pRec->putField(EXPORT,"");
				db->qsoUpdRec(i, pRec);
			}
		}
		fclose (txtFile);
	}
	return 0;
}

