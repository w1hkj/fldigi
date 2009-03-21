#ifndef FIELD_DEFS
#define FIELD_DEFS

enum ADIF_FIELD_POS {
    ADDRESS = 0, 
    AGE, 
    ARRL_SECT, 
    BAND, 
    CALL,
    CNTY, 
    COMMENT, 
    CONT, 
    CONTEST_ID,
    COUNTRY,
    CQZ, 
    DXCC, 
    EXPORT, // flag used internally in fldigi's logbook
    FREQ, 
    GRIDSQUARE, 
    IOTA,
    ITUZ,
    MODE,
	MYXCHG,  // contest exchange field #3
    NAME, 
    NOTES, 
    OPERATOR,
    PFX,
    PROP_MODE,
    QSLRDATE, 
    QSLSDATE, 
    QSL_MSG,
    QSL_RCVD,
    QSL_SENT, 
    QSL_VIA, 
    QSO_DATE,
    QTH, 
    RST_RCVD, 
    RST_SENT,
    RX_PWR, 
    SAT_MODE,
    SAT_NAME,
    SRX, 
    STATE, 
    STX,
    TEN_TEN, 
    TIME_OFF, 
    TIME_ON, 
    TX_PWR,
    VE_PROV,
	XCHG1,   // contest exchange field
    NUMFIELDS // counter for number of fields in enum
};

#endif
