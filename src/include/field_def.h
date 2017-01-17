// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef FIELD_DEFS
#define FIELD_DEFS

// removed unused fields to reduce size of each record and speed up
// record processing

// field position should correspond with fields[] in adif_io.cxx

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
EQSLRDATE,
EQSLSDATE,
LOTWRDATE,
LOTWSDATE,
GRIDSQUARE,
BAND,
CNTY,
COUNTRY,
CQZ,
DXCC,
QSL_VIA,
IOTA,
ITUZ,
CONT,
SRX,
STX,
XCHG1,
MYXCHG,
FDCLASS,
FDSECTION,
TX_PWR,
OP_CALL,
STA_CALL,
MY_GRID,
MY_CITY,
// do not add fields below this line; EXPORT must be last field in struc
EXPORT, // flag used internally in fldigi's logbook
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
