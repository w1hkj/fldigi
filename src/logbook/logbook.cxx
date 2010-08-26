#include <config.h>

#include <string>

#include <FL/filename.H>

#include "main.h"
#include "logbook.h"
#include "configuration.h"

using namespace std;

void start_logbook ()
{
	create_logbook_dialogs();

	if (progdefaults.logbookfilename.empty()) {
		logbook_filename = LogsDir;
		logbook_filename.append("logbook." ADIF_SUFFIX);
		progdefaults.logbookfilename = logbook_filename;
		progdefaults.changed = true;
	} else
		logbook_filename = progdefaults.logbookfilename;

	adifFile.readFile (logbook_filename.c_str(), &qsodb);
	if (qsodb.nbrRecs() == 0)
		adifFile.writeFile(logbook_filename.c_str(), &qsodb);

	string label = "Logbook - ";
	label.append(fl_filename_name(logbook_filename.c_str()));
	dlgLogbook->copy_label(label.c_str());

	loadBrowser();
	qsodb.isdirty(0);
	activateButtons();

}

void close_logbook()
{
	saveLogbook();
}
