// ====================================================================
//  logger.cxx Remote Log Interface for fldigi
//
// Copyright W1HKJ, Dave Freese 2006
//
// This library is free software; you can RGBredistribute it and/or
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
// ====================================================================

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef __CYGWIN__
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif
#include <errno.h>
#include <string>

#include "logger.h"
#include "main.h"
#include "modem.h"
#include "debug.h"
#include "macros.h"
#include "status.h"
#include "spot.h"

#include "adif_io.h"

#include "logsupport.h"

#include <FL/fl_ask.H>

//---------------------------------------------------------------------
const char *logmode;
char logdate[32];
char logtime[32];
//---------------------------------------------------------------------

void submit_log(void)
{
	if (progStatus.spot_log)
		spot_log(inpCall->value(), inpLoc->value());

	struct tm *tm;
	time_t t;

	time(&t);
        tm = gmtime(&t);
		strftime(logdate, sizeof(logdate), "%d %b %Y", tm);
		strftime(logtime, sizeof(logtime), "%H%M", tm);
	logmode = mode_info[active_modem->get_mode()].adif_name;

	AddRecord();

}

//---------------------------------------------------------------------

