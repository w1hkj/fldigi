// ----------------------------------------------------------------------------
//
// fileselect.cxx -- file selector front end
//
// Copyright (C) 2008-2009
//		Stelios Bounanos, M0GLD
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

#include <string>
#include <cstdlib>
#include <libgen.h>

#include "fileselect.h"
#include "icons.h"
#include "debug.h"

#include <FL/fl_ask.H>
#include <FL/Fl_Native_File_Chooser.H>

using namespace std;

FSEL* FSEL::inst = 0;
static std::string filename;

void FSEL::create(void)
{
	if (inst)
		return;
	inst = new FSEL;
}

void FSEL::destroy(void)
{
	delete inst;
	inst = 0;
}

FSEL::FSEL()
	: chooser(new Fl_Native_File_Chooser) { }
FSEL::~FSEL() { delete chooser; }

// this method is called by FSEL internal methods only
const char* FSEL::get_file(void)
{
// Show native chooser and select file
	switch ( inst->chooser->show() ) {
		case -1: 
			fl_alert2("ERROR: %s\n", chooser->errmsg());  // ERROR
			// fall through
		case  1: break;  // CANCEL
		default: 
			filename = inst->chooser->filename();
			string::size_type i = filename.rfind('/');
			if (i != string::npos)
				inst->chooser->directory(filename.substr(0, i).c_str());
			return filename.c_str();
	}
	return NULL; // same as CANCELLED or ERROR
}

// example from logsupport.cxx
// const char* p = FSEL::select(_("Open logbook file"), "ADIF\t*." ADIF_SUFFIX, logbook_filename.c_str());
//
const char* FSEL::select(const char* title, const char* filter, const char* def, int* fsel)
{
	inst->chooser->title(title);
	inst->chooser->filter(filter);
	if (def) {
		char *s = strdup(def), *dir = dirname(s);
		if (strcmp(".", dir))
			inst->chooser->directory(dir);
		free(s);
		s = strdup(def);
		inst->chooser->preset_file(basename(s));
		free(s);
	} else {
		inst->chooser->directory(NULL);
	}
	inst->chooser->options(Fl_Native_File_Chooser::PREVIEW);
	inst->chooser->type(Fl_Native_File_Chooser::BROWSE_FILE);

	const char* fn = inst->get_file();
	if (fsel)
		*fsel = inst->chooser->filter_value();
	return fn;
}

// example from logsupport.cxx
// const char* p = FSEL::saveas(_("Export to CSV file"), filters.c_str(), "export." "csv");
//
const char* FSEL::saveas(const char* title, const char* filter, const char* def, int* fsel)
{
	inst->chooser->title(title);
	inst->chooser->filter(filter);
	if (def) {
		char *s = strdup(def), *dir = dirname(s);
		if (strcmp(".", dir))
			inst->chooser->directory(dir);
		free(s);
		s = strdup(def);
		inst->chooser->preset_file(basename(s));
		free(s);
	} else {
		inst->chooser->directory(NULL);
	}
	inst->chooser->options(
		Fl_Native_File_Chooser::SAVEAS_CONFIRM |
		Fl_Native_File_Chooser::NEW_FOLDER |
		Fl_Native_File_Chooser::PREVIEW);
	inst->chooser->type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);

	const char* fn = inst->get_file();
	if (fsel)
		*fsel = inst->chooser->filter_value();
	return fn;
}

// not currently called by any fldigi /flarq functions or methods

const char* FSEL::dir_select(const char* title, const char* filter, const char* def)
{
	inst->chooser->title(title);
	inst->chooser->filter(filter);
	if (def)
		inst->chooser->directory(def);
	inst->chooser->options(Fl_Native_File_Chooser::NEW_FOLDER |
			       Fl_Native_File_Chooser::PREVIEW);
	inst->chooser->type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);

	return inst->get_file();
}
