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
    FREQ, 
    GRIDSQUARE, 
    MODE,
    NAME, 
    NOTES, 
    QSLRDATE, 
    QSLSDATE, 
    QSL_RCVD,
    QSL_SENT, 
    QSO_DATE,
    QTH, 
    RST_RCVD, 
    RST_SENT,
    STATE, 
    STX,
    TIME_OFF, 
    TIME_ON, 
    TX_PWR,
// additional for 2.0
    IOTA,
    ITUZ,
    OPERATOR,
    PFX,
    PROP_MODE,
    QSL_MSG,
    QSL_VIA, 
    RX_PWR, 
    SAT_MODE,
    SAT_NAME,
    SRX, 
    TEN_TEN, 
    VE_PROV,
    EXPORT, // internal use in fl_logbook
    NUMFIELDS // counter for number of fields in enum
};

#endif
