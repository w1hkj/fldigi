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

// ADIF multiline string is a sequence of Characters and line-breaks,
// where a line break is an ASCII CR (code 13) followed immediately by an ASCII  LF (code 10)
// Not sure fldigi is completely conformant with this.
// #define ADIF_EOL "\r\n"
#define ADIF_EOL "\n"

// Forward declaration for QsoHelper.
class cQsoRec ;

// Helps for creating a new ADIF record.
class QsoHelper {
	cQsoRec * qso_rec ;
	QsoHelper();
	QsoHelper(const QsoHelper &);
	QsoHelper & operator=(const QsoHelper &);
public:
	QsoHelper(int mode);
	~QsoHelper();
	// Inserts a key-value pair.
	void Push( ADIF_FIELD_POS pos, const std::string & value );
};

#endif
