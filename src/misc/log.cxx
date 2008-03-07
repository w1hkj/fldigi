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

static const char *lognames[] = {"RX", "TX"};

cLogfile::cLogfile(const string& fname) {
	logtype = LOG_RX;
	retflag = true;
	logfilename = fname;
}

void cLogfile::log_to_file(log_t type, const string& s)
{
	char timestr[64];
	struct tm *tm;
	time_t t;
	
	if (_logfile.fail()) {
		return;
	}
	if (s.length() == 0)
		return;

	/* add timestamp to logged data */
	time(&t);
	tm = gmtime(&t);
	memset(timestr, 0, 64);
	strftime(timestr, 63, "%Y-%m-%d %H:%M", tm);

	if (retflag)
		_logfile << lognames[type] << " (" << timestr << "Z): ";
	else if (logtype != type)
		_logfile << "\n" << lognames[type] << " (" << timestr << "Z): ";
	
	_logfile << s;
	
	_logfile.flush();

	if (s[s.length() - 1] == '\n')
		retflag = true;
	else
		retflag = false;

	logtype = type;

}

void cLogfile::log_to_file_start()
{
	time_t t;
	char *str;

	if (!_logfile.is_open())
		_logfile.open(logfilename.c_str(), ios::app);
	if (_logfile.fail())
		return;
	time(&t);
	str = asctime(gmtime(&t));
	str[strlen(str) - 1] = 0;
	_logfile << "\n--- Logging started at " << str << " UTC ---\n";
	_logfile.flush();
}

void cLogfile::log_to_file_stop()
{
	time_t t;
	char *str;
	
	if (_logfile.fail())
		return;

	time(&t);
	str = asctime(gmtime(&t));
	str[strlen(str) - 1] = 0;
	_logfile << "\n--- Logging stopped at " << str << " UTC ---\n";
	_logfile.close();
}



/* ---------------------------------------------------------------------- */

