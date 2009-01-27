//
//  logger.h Remote Log Interface for fldigi
//
// Copyright W1HKJ, Dave Freese 2006
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "w1hkj@w1hkj.com".
//

#ifndef _LOGGER_H
#define _LOGGER_H

#include <string>

using namespace std;


// IPC interface to Xlog and fl_logbook

#define	LOG_MVERSION	"1"
#define	LOG_MKEY	1238
#define	LOG_MTYPE	88

#define	LOG_MSG_LEN	1024

typedef struct {
	long mtype;
	char mtext[LOG_MSG_LEN];
} msgtype;

extern void submit_log();

extern	char logdate[];
extern  char logtime[];
extern  char adifdate[];
extern	const char *logmode;

#endif
