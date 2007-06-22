#include "adif_io.h"
#include "version.h"

#include <FL/Fl.H>
#include <FL/filename.H>

// These ADIF fields are a part of the QSO database
  
FIELD fields[] = {
//  NAME,  SIZE
  {"ADDRESS",   40},  // 0 - contacted stations mailing address
  {"AGE",        3},  // 1 - contacted operators age in years
  {"ARRL_SECT", 12},  // 2 - contacted stations ARRL section
  {"BAND",       6},  // 3 - QSO band
  {"CALL",      10},  // 4 - contacted stations CALLSIGN
  {"CNTY",      20},  // 5 - secondary political subdivision, ie: STATE
  {"COMMENT",   80},  // 6 - comment field for QSO
  {"CONT",      10},  // 7 - contacted stations continent
  {"CONTEST_ID", 6},  // 8 - QSO contest identifier
  {"COUNTRY",   20},  // 9 - contacted stations DXCC entity name
  {"CQZ",        8},  // 10 - contacted stations CQ Zone
  {"DXCC",       8},  // 11 - contacted stations Country Code
  {"FREQ",      10},  // 12 - QSO frequency in Mhz
  {"GRIDSQUARE", 6},  // 13 - contacted stations Maidenhead Grid Square
  {"MODE",       8},  // 14 - QSO mode
  {"NAME",      18},  // 15 - contacted operators NAME
  {"NOTES",     80},  // 16 - QSO notes
  {"QSLRDATE",   8},  // 17 - QSL received date
  {"QSLSDATE",   8},  // 18 - QSL sent date
  {"QSL_RCVD",   1},  // 19 - QSL received status
  {"QSL_SENT",   1},  // 20 - QSL sent status
  {"QSO_DATE",   8},  // 21 - QSO data
  {"QTH",       30},  // 22 - contacted stations city
  {"RST_RCVD",   3},  // 23 - received signal report
  {"RST_SENT",   3},  // 24 - sent signal report
  {"STATE",      2},  // 25 - contacted stations STATE
  {"STX",        8},  // 26 - QSO transmitted serial number
  {"TIME_OFF",   4},  // 27 - HHMM or HHMMSS in UTC
  {"TIME_ON",    4},  // 28 - HHMM or HHMMSS in UTC
  {"TX_PWR",     4}   // 29 - power transmitted by this station
};

int fieldnbr (char *s) {
  for (int i = 0; i < NUMFIELDS; i++)
    if (strncasecmp( fields[i].name, s, fields[i].size) == 0)
      return i;
  return -1;
}

int findfield (char *p) {
  for (int i=0; i < NUMFIELDS; i++)
    if (strncasecmp (p, fields[i].name, strlen(fields[i].name)) == 0)
      return i;
  if (strncasecmp (p, "EOR>", 4) == 0)
    return -1;
  return -2;
}

void cAdifIO::fillfield (int fieldnum, char *buff){
char *p = buff;
char numeral[8];
int n, fldsize;
  memset (numeral, 0, 8);
  n = 0;
  while (*p != ':' && n < 10) {p++; n++;}
  if (n == 10) return; // bad ADIF specifier ---> no ':' after field name
// found first ':'
  p++;
  n = 0;
  while (*p >= '0' && *p <= '9' && n < 8) {
    numeral[n++] = *p;
    p++;
  }
  fldsize = atoi(numeral);
  p = strchr(buff,'>'); // end of specifier +1 == > start of data
  if (!p) return;
  p++;
  char *flddata = new char[fldsize+1];
  memset (flddata, 0, fldsize + 1);
  strncpy (flddata, p, fldsize);
  adifqso.putField (fieldnum, (const char *)flddata); 
}

int cAdifIO::readFile (char *fname, cQsoDb *db) {
  long filesize = 0;
  char *buff;
  int found;
// open the adif file
  adiFile = fopen (fname, "r");
  if (!adiFile)
    return 1;
// determine its size for buffer creation
  fseek (adiFile, 0, SEEK_END);
  filesize = ftell (adiFile);
  buff = new char[filesize + 1];
// read the entire file into the buffer
  fseek (adiFile, 0, SEEK_SET);
  fread (buff, filesize, 1, adiFile);
  fclose (adiFile);
  
  char *p1 = buff, *p2;
// is there a header?
  if (*p1 != '<') { // yes find the start of the records
    p1 = strchr(buff, '<');
    while (strncasecmp (p1+1,"EOH>", 4) != 0) {
      p1 = strchr(p1+1, '<'); // find next <> field
    }
    if (!p1) return 1;   // must not be an ADIF compliant file
    p1 += 1;
  }
  
  p2 = strchr(p1,'<'); // find first ADIF specifier
  adifqso.clearRec();
  while (p2) {
    found = findfield(p2+1);
    if (found > -1)
      fillfield (found, p2+1);
    else if (found == -1) { // <eor> reached; add this record to db
      db->qsoNewRec (&adifqso);
      adifqso.clearRec();
    }
    else
      return 1; // corrupt file or end of records
    p1 = p2 + 1;
    p2 = strchr(p1,'<');
  }
  return 0;

}

char *ADIFHEADER = "\
ADIF Export from fldigi\n\
w1hkj@w1hkj.com\n\
File: %s\n\
<ADIF_VERS:%d>%s\n\
<PROGRAMID:7>logbook\n\
<PROGRAMVERSION:%d>%s\n\
<EOH>\n\n";


int cAdifIO::writeFile (char *fname, cQsoDb *db) {
// open the adif file
  char *szFld;
  cQsoRec *rec;
  adiFile = fopen (fname, "r");
  if (!adiFile) {
  	adiFile = fopen(fname,"w");
	if (!adiFile)
		return 1;
	fprintf (adiFile, ADIFHEADER,
           fl_filename_name(fname),
           strlen(ADIF_VERS), ADIF_VERS,
           strlen(VERSION), VERSION);
	fclose(adiFile);
  }
  adiFile = fopen (fname, "a");
  if (!adiFile)
    return 1;
// write the current record to the file  
//  for (int i = 0; i < db->nbrRecs(); i++) {
//    rec = db->getRec(i);
//    for (int j=0; j < NUMFIELDS; j++) {
//      szFld = rec->getField(j);
//      if (strlen(szFld))
//        fprintf(adiFile,
//          "<%s:%d>%s ", fields[j].name, strlen(szFld), szFld);
//    }
//    fprintf(adiFile, "<EOR>\n");
//  }  
  fclose (adiFile);
  return 0;
}

