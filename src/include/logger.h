// ----------------------------------------------------------------------------
//  logger.h Remote Log Interface for fldigi
//
// Copyright W1HKJ, Dave Freese 2006
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

#ifndef _LOGGER_H
#define _LOGGER_H

// IPC interface to Xlog and fl_logbook

#define	LOG_MVERSION	"1"
#define	LOG_MKEY	1238
#define	LOG_MTYPE	88

#define	LOG_MSG_LEN	1024

#include "qso_db.h"

typedef struct {
	long mtype;
	char mtext[LOG_MSG_LEN];
} msgtype;

extern void submit_log();
extern void submit_record(cQsoRec &);

extern	char logdate[];
extern  char logtime[];
extern  char adifdate[];
extern	const char *logmode;

#endif
