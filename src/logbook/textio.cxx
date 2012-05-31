// ----------------------------------------------------------------------------
// textio.cxx
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

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
	if (btnSelectQSOdateOn->value()) fprintf (txtFile, "%s", "\"DATE_ON\"");
	if (btnSelectQSOdateOff->value())fprintf (txtFile, "%s", ",\"DATE_OFF\"");
	if (btnSelectTimeON->value())    fprintf (txtFile, "%s", ",\"ON\"");
	if (btnSelectTimeOFF->value())   fprintf (txtFile, "%s", ",\"OFF\"");
	if (btnSelectCall->value())      fprintf (txtFile, "%s", ",\"CALL\"");
	if (btnSelectName->value())      fprintf (txtFile, "%s", ",\"NAME\"");
	if (btnSelectBand->value())      fprintf (txtFile, "%s", ",\"BAND\"");
	if (btnSelectFreq->value())      fprintf (txtFile, "%s", ",\"FREQ\"");
	if (btnSelectMode->value())      fprintf (txtFile, "%s", ",\"MODE\"");
	if (btnSelectTX_pwr->value())    fprintf (txtFile, "%s", ",\"TX_PWR\"");
	if (btnSelectRSTsent->value())   fprintf (txtFile, "%s", ",\"RSTSENT\"");
	if (btnSelectRSTrcvd->value())   fprintf (txtFile, "%s", ",\"RSTRCVD\"");
	if (btnSelectQth->value())       fprintf (txtFile, "%s", ",\"QTH\"");
	if (btnSelectState->value())     fprintf (txtFile, "%s", ",\"ST\"");
	if (btnSelectProvince->value())  fprintf (txtFile, "%s", ",\"PR\"");
	if (btnSelectCNTY->value())      fprintf (txtFile, "%s", ",\"CNTY\"");
	if (btnSelectCountry->value())   fprintf (txtFile, "%s", ",\"CNTRY\"");
	if (btnSelectDXCC->value())      fprintf (txtFile, "%s", ",\"DXCC\"");
	if (btnSelectIOTA->value())      fprintf (txtFile, "%s", ",\"IOTA\"");
	if (btnSelectCONT->value())      fprintf (txtFile, "%s", ",\"CONT\"");
	if (btnSelectITUZ->value())      fprintf (txtFile, "%s", ",\"ITUZ\"");
	if (btnSelectLOC->value())       fprintf (txtFile, "%s", ",\"GRIDSQUARE\"");
	if (btnSelectQSLrcvd->value())   fprintf (txtFile, "%s", ",\"QSL_RCVD\"");
	if (btnSelectQSLsent->value())   fprintf (txtFile, "%s", ",\"QSL_SENT\"");
	if (btnSelectNotes->value())     fprintf (txtFile, "%s", ",\"NOTES\"");
	if (btnSelectSerialIN->value())  fprintf (txtFile, "%s", ",\"SERIAL RCVD\"");
	if (btnSelectSerialOUT->value()) fprintf (txtFile, "%s", ",\"SERIAL_SENT\"");
	if (btnSelectXchgIn->value())    fprintf (txtFile, "%s", ",\"XCHG1\"");
	if (btnSelectMyXchg->value())    fprintf (txtFile, "%s", ",\"MYXCHG\"");
	fprintf (txtFile, "%s", szEOL);
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
				if (btnSelectQSOdateOn->value())
					fprintf (txtFile, "\"%s\"", pRec->getField(QSO_DATE));
				if (btnSelectQSOdateOff->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(QSO_DATE_OFF));
				if (btnSelectTimeON->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(TIME_ON));
				if (btnSelectTimeOFF->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(TIME_OFF));
				if (btnSelectCall->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(CALL));
				if (btnSelectName->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(NAME));
				if (btnSelectBand->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(BAND));
				if (btnSelectFreq->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(FREQ));
				if (btnSelectMode->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(MODE));
				if (btnSelectTX_pwr->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(TX_PWR));
				if (btnSelectRSTsent->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(RST_SENT));
				if (btnSelectRSTrcvd->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(RST_RCVD));
				if (btnSelectQth->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(QTH));
				if (btnSelectState->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(STATE));
				if (btnSelectProvince->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(VE_PROV));
				if (btnSelectCNTY->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(CNTY));
				if (btnSelectCountry->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(COUNTRY));
				if (btnSelectDXCC->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(DXCC));
				if (btnSelectIOTA->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(IOTA));
				if (btnSelectCONT->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(CONT));
				if (btnSelectITUZ->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(ITUZ));
				if (btnSelectLOC->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(GRIDSQUARE));
				if (btnSelectQSLrcvd->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(QSLRDATE));
				if (btnSelectQSLsent->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(QSLSDATE));
				if (btnSelectNotes->value()) {
					string temp = pRec->getField(NOTES);
					for (size_t n = 0; n < temp.length(); n++)
						if (temp[n] == '\r' || temp[n] == '\n') temp[n] = '-';
					fprintf (txtFile, ",\"%s\"", temp.c_str());
				}
				if (btnSelectSerialIN->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(SRX));
				if (btnSelectSerialOUT->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(STX));
				if (btnSelectXchgIn->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(XCHG1));
				if (btnSelectMyXchg->value())
					fprintf (txtFile, ",\"%s\"", pRec->getField(MYXCHG));
				fprintf (txtFile, "%s", szEOL);
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
	if (btnSelectQSOdateOn->value()) fprintf (txtFile, "%-10s", "DATE_ON");
	if (btnSelectQSOdateOff->value())fprintf (txtFile, "%-10s", "DATE_OFF");
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
	if (btnSelectCNTY->value())      fprintf (txtFile, "%-8s",  "CNTY");
	if (btnSelectDXCC->value())      fprintf (txtFile, "%-8s",  "DXCC");
	if (btnSelectIOTA->value())      fprintf (txtFile, "%-8s", "IOTA");
	if (btnSelectCONT->value())      fprintf (txtFile, "%-8s",  "CONT");
	if (btnSelectITUZ->value())      fprintf (txtFile, "%-8s",  "ITUZ");
	if (btnSelectLOC->value())       fprintf (txtFile, "%-15s", "GRIDSQUARE");

	if (btnSelectQSLrcvd->value())   fprintf (txtFile, "%-10s", "QSLR");
	if (btnSelectQSLsent->value())   fprintf (txtFile, "%-10s", "QSLS");
	if (btnSelectNotes->value())     fprintf (txtFile, "%-80s", "NOTES");
	if (btnSelectSerialIN->value())  fprintf (txtFile, "%-7s", "SRX");
	if (btnSelectSerialOUT->value()) fprintf (txtFile, "%-7s", "STX");
	if (btnSelectXchgIn->value())    fprintf (txtFile, "%-15s", "XCHG1");
	if (btnSelectMyXchg->value())    fprintf (txtFile, "%-15s", "MYXCHG");
	fprintf (txtFile, "%s", szEOL);
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
				if (btnSelectQSOdateOn->value())
					fprintf (txtFile, "%-10s", pRec->getField(QSO_DATE));
				if (btnSelectQSOdateOff->value())
					fprintf (txtFile, "%-10s", pRec->getField(QSO_DATE_OFF));
				if (btnSelectTimeON->value())
					fprintf (txtFile, "%-8s", pRec->getField(TIME_ON));
				if (btnSelectTimeOFF->value())
					fprintf (txtFile, "%-8s", pRec->getField(TIME_OFF));
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
				if (btnSelectCNTY->value())
					fprintf (txtFile, "%-8s", pRec->getField(CNTY));
				if (btnSelectDXCC->value())
					fprintf (txtFile, "%-8s", pRec->getField(DXCC));
				if (btnSelectIOTA->value())
					fprintf (txtFile, "%-8s", pRec->getField(IOTA));
				if (btnSelectCONT->value())
					fprintf (txtFile, "%-8s", pRec->getField(CONT));
				if (btnSelectITUZ->value())
					fprintf (txtFile, "%-8s", pRec->getField(ITUZ));
				if (btnSelectLOC->value())
					fprintf (txtFile, "%-15s", pRec->getField(GRIDSQUARE));
				if (btnSelectQSLrcvd->value())
					fprintf (txtFile, "%-10s", pRec->getField(QSLRDATE));
				if (btnSelectQSLsent->value())
					fprintf (txtFile, "%-10s", pRec->getField(QSLSDATE));
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
				fprintf (txtFile, "%s", szEOL);
				pRec->putField(EXPORT,"");
				db->qsoUpdRec(i, pRec);
			}
		}
		fclose (txtFile);
	}
	return 0;
}

