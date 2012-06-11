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

#include <string>
#include <cstdlib>
#include <libgen.h>

#include "fileselect.h"
#include "icons.h"
#include "debug.h"

#include <FL/fl_ask.H>
#include "FL/Native_File_Chooser.H"

#if FSEL_THREAD
#    include <FL/Fl.H>
#    include <semaphore.h>
#    include "threads.h"
#endif

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

#ifdef __APPLE__
FSEL::FSEL()
	: chooser(new MAC_chooser) { }
FSEL::~FSEL() { delete chooser; }
#else
FSEL::FSEL()
	: chooser(new Fl_Native_File_Chooser) { }
FSEL::~FSEL() { delete chooser; }
#endif

const char* FSEL::get_file(void)
{
	// Calling directory() is apparently not enough on Linux
#if !defined(__WOE32__) && !defined(__APPLE__)
	const char* preset = chooser->preset_file();
	if (preset && *preset != '/' && chooser->directory()) {
		filename = chooser->directory();
		filename.append("/").append(preset);
		chooser->preset_file(filename.c_str());
	}
#endif

	result = chooser->show();

	switch (result) {
	case -1:
		fl_alert2("%s", chooser->errmsg());
		// fall through
	case 1:
		return NULL;
	default:
		filename = chooser->filename();
		string::size_type i = filename.rfind('/');
		if (i != string::npos)
			chooser->directory(filename.substr(0, i).c_str());
		return filename.c_str();
	}
}

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
	}
#ifdef __APPLE__
	inst->chooser->options(MAC_chooser::PREVIEW);
	inst->chooser->type(MAC_chooser::BROWSE_FILE);
#else
	inst->chooser->options(Fl_Native_File_Chooser::PREVIEW);
	inst->chooser->type(Fl_Native_File_Chooser::BROWSE_FILE);
#endif
	const char* fn = inst->get_file();
	if (fsel)
		*fsel = inst->chooser->filter_value();
	return fn;
}

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
	}
#ifdef __APPLE__
	inst->chooser->options( MAC_chooser::SAVEAS_CONFIRM |
							MAC_chooser::NEW_FOLDER |
							MAC_chooser::PREVIEW);
	inst->chooser->type(MAC_chooser::BROWSE_SAVE_FILE);
#else
	inst->chooser->options(Fl_Native_File_Chooser::SAVEAS_CONFIRM |
			       Fl_Native_File_Chooser::NEW_FOLDER |
			       Fl_Native_File_Chooser::PREVIEW);
	inst->chooser->type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
#endif
	const char* fn = inst->get_file();
	if (fsel)
		*fsel = inst->chooser->filter_value();
	return fn;
}

const char* FSEL::dir_select(const char* title, const char* filter, const char* def)
{
	inst->chooser->title(title);
	inst->chooser->filter(filter);
	if (def)
		inst->chooser->directory(def);
#ifdef __APPLE__
	inst->chooser->options(	MAC_chooser::NEW_FOLDER |
							MAC_chooser::PREVIEW);
	inst->chooser->type(MAC_chooser::BROWSE_DIRECTORY);
#else
	inst->chooser->options(Fl_Native_File_Chooser::NEW_FOLDER |
			       Fl_Native_File_Chooser::PREVIEW);
	inst->chooser->type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
#endif
	return inst->get_file();
}
