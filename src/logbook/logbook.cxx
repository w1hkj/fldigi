#include <config.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/filename.H>
#include <FL/x.H>
#include <FL/fl_ask.H>
#include <FL/filename.H>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "main.h"
#include "logbook.h"
#include "configuration.h"

void start_logbook ()
{
	create_logbook_dialogs();

	if (progdefaults.logbookfilename.empty()) {
		logbook_filename = HomeDir;
		logbook_filename.append("logbook." ADIF_SUFFIX);
		progdefaults.logbookfilename = logbook_filename;
		progdefaults.changed = true;
	} else
		logbook_filename = progdefaults.logbookfilename;

	adifFile.readFile (logbook_filename.c_str(), &qsodb);
	dlgLogbook->copy_label(fl_filename_name(logbook_filename.c_str()));
	loadBrowser();
	qsodb.isdirty(0);

}

void close_logbook()
{
	saveLogbook();
}
