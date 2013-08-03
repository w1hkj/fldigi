// ----------------------------------------------------------------------------
// log.cxx  --  Received text logging for fldigi
//
// Copyright (C) 2007-2008
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

#ifdef __MINGW32__
#  include "compat.h"
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/time.h>
#include <string>
#include <cstring>

#include "log.h"
#include "trx.h"
#include "fl_digi.h"
#include "timeops.h"

using namespace std;

static const char *lognames[] = { "RX", "TX", "", "" };

cLogfile::cLogfile(const string& fname)
	: retflag(true), logtype(LOG_RX)
{
	if ((logfile = fopen(fname.c_str(), "a"))) {
		setvbuf(logfile, (char*)NULL, _IOLBF, 0);
		set_cloexec(fileno(logfile), 1);
	}
}

cLogfile::~cLogfile()
{
	if (logfile)
		fclose(logfile);
}

void cLogfile::log_to_file(log_t type, const string& s)
{
	if (!logfile || ferror(logfile) || s.empty())
		return;

	char timestr[64];
	struct tm tm;
	time_t t;

	if (type == LOG_RX || type == LOG_TX) {
		if (retflag || type != logtype) {
			if (type != logtype) fprintf(logfile, "\n");
			time(&t);
			gmtime_r(&t, &tm);
			strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%MZ", &tm);
			char freq[20];
			snprintf(freq, sizeof(freq), "%d", 
					static_cast<int>( wf->rfcarrier() + 
										(wf->USB() ? active_modem->get_freq()
										: -active_modem->get_freq() ) ) );
			const char *logmode = mode_info[active_modem->get_mode()].adif_name;

			fprintf(logfile, "%s %s : %s (%s): ", lognames[type], freq, logmode, timestr);
		}
		for (size_t i = 0; i < s.length(); i++)
			if (s[i] == '\n' || (unsigned char)s[i] >= ' ') fprintf(logfile, "%c", s[i]);
		retflag = *s.rbegin() == '\n';
		if (!retflag)
			fflush(logfile);
	}
	else {
		time(&t);
		gmtime_r(&t, &tm);
		strftime(timestr, sizeof(timestr), "%a %b %e %H:%M:%S %Y UTC", &tm);
		fprintf(logfile, "\n--- Logging %s at %s ---\n", s.c_str(), timestr);
	}

	logtype = type;
}



void cLogfile::log_to_file_start()
{
	log_to_file(LOG_START, "started");
}

void cLogfile::log_to_file_stop()
{
	log_to_file(LOG_STOP, "stopped");
}



/* ---------------------------------------------------------------------- */

