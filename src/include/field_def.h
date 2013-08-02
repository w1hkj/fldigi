#ifndef FIELD_DEFS
#define FIELD_DEFS

// removed unused fields to reduce size of each record and speed up
// record processing

enum ADIF_FIELD_POS {
FREQ = 0,
CALL,
MODE,
NAME,
QSO_DATE,
QSO_DATE_OFF,
TIME_OFF,
TIME_ON,
QTH,
RST_RCVD,
RST_SENT,
STATE,
VE_PROV,
NOTES,
QSLRDATE,
QSLSDATE,
GRIDSQUARE,
BAND,
CNTY,
COUNTRY,
CQZ,
DXCC,
IOTA,
ITUZ,
CONT,
MYXCHG,
XCHG1,
SRX,
STX,
TX_PWR,
EXPORT, // flag used internally in fldigi's logbook
QSL_VIA,
NUMFIELDS };

#endif
