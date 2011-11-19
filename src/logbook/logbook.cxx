#include <config.h>

#include <cstring>

#include <FL/Fl.H>
#include <FL/filename.H>

#include "main.h"
#include "logbook.h"
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

	string label = "Logbook - ";
	label.append(fl_filename_name(logbook_filename.c_str()));
	dlgLogbook->copy_label(label.c_str());

	return;
}

void close_logbook()
{
	if (!qsodb.isdirty()) return;
	if (progdefaults.NagMe)
		if (!fl_choice2(_("Save changed Logbook?"), _("No"), _("Yes"), NULL))
			return;

	cQsoDb::reverse = false;
	qsodb.SortByDate(progdefaults.sort_date_time_off);

	adifFile.writeLog (logbook_filename.c_str(), &qsodb, true);
}

