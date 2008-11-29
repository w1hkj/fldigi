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

void start_logbook ()
{
	create_logbook_dialogs();

	logbook_filename = HomeDir;
	logbook_filename.append("logbook." ADIF_SUFFIX);

	adifFile.readFile (logbook_filename.c_str(), &qsodb);
	loadBrowser();
	qsodb.isdirty(0);

}

void close_logbook()
{
	saveLogbook();
}
