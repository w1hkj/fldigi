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

#include <config.h>

#include <cstring>
#include <iostream>
#include <fstream>

#include <FL/Fl.H>
#include <FL/filename.H>

#include "main.h"
#include "logbook.h"
#include "logsupport.h"
#include "configuration.h"
#include "debug.h"
#include "qrunner.h"
#include "gettext.h"
#include "icons.h"

using namespace std;

void start_logbook ()
{
	if (progdefaults.logbookfilename.empty()) {
		logbook_filename = LogsDir;
		logbook_filename.append("logbook." ADIF_SUFFIX);
		progdefaults.logbookfilename = logbook_filename;
		progdefaults.changed = true;
	} else
		logbook_filename = progdefaults.logbookfilename;

	qsodb.deleteRecs();

	adifFile.readFile (logbook_filename.c_str(), &qsodb);
	rotate_log(logbook_filename);

	string label = "Logbook - ";
	label.append(fl_filename_name(logbook_filename.c_str()));
	dlgLogbook->copy_label(label.c_str());
	txtLogFile->value(logbook_filename.c_str());
	txtLogFile->redraw();

	restore_sort();
	loadBrowser();

	qsodb.isdirty(0);
	MilliSleep(100);

	return;
}

void close_logbook()
{
/* DISABLE CONSTRAINTS - ATTEMPT TO THWART WINDOWS FAILURE TO WRITE LOG ON EXIT
	if (!qsodb.isdirty()) return;
	if (progdefaults.NagMe)
		if (!fl_choice2(_("Save changed Logbook?"), _("No"), _("Yes"), NULL))
			return;
	cQsoDb::reverse = false;
	qsodb.SortByDate(progdefaults.sort_date_time_off);
*/

// force immediate write to logbook adif file

	adifFile.writeLog (logbook_filename.c_str(), &qsodb, true);

}

