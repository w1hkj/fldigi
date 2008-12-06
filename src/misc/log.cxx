/*
 *    log.cxx  --  Received text logging for fldigi
 */


#include <config.h>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/time.h>
#include <string>
#include <cstring>

#include "log.h"

using namespace std;

static const char *lognames[] = { "RX", "TX", "", "" };

cLogfile::cLogfile(const string& fname)
	: retflag(true), logtype(LOG_RX)
{
	if ((logfile = fopen(fname.c_str(), "a"))) {
		setlinebuf(logfile);
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
			fprintf(logfile, "%s (%s): ", lognames[type], timestr);
		}
		for (size_t i = 0; i < s.length(); i++)
			if (s[i] == '\n' || s[i] >= ' ') fprintf(logfile, "%c", s[i]);
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

