#include <config.h>

#include <string>

#include "fileselect.h"

#include <FL/fl_ask.H>
#include <FL/Fl_Native_File_Chooser.H>

using namespace std;

static Fl_Native_File_Chooser chooser;
static std::string filename;

static const char* get_file(void)
{
	const char* preset = chooser.preset_file();
	if (preset && *preset != '/' && chooser.directory()) {
		filename = chooser.directory();
		filename.append("/").append(preset);
		chooser.preset_file(filename.c_str());
	}

	switch (chooser.show()) {
	case -1:
		fl_alert("%s", chooser.errmsg());
		// fall through
	case 1:
		return NULL;
	default:
		filename = chooser.filename();
		string::size_type i = filename.rfind('/');
		if (i != string::npos)
			chooser.directory(filename.substr(0, i).c_str());
		return filename.c_str();
	}
}

const char* file_select(const char* title, const char* filter, const char* def, int* fsel)
{
	chooser.title(title);
	chooser.filter(filter);
	if (def)
		chooser.preset_file(def);
	chooser.options(Fl_Native_File_Chooser::PREVIEW);
	chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);

	const char* fn = get_file();
	if (fsel)
	    *fsel = chooser.filter_value();
	return fn;
}

const char* file_saveas(const char* title, const char* filter, const char* def, int* fsel)
{
	chooser.title(title);
	chooser.filter(filter);
	if (def)
		chooser.preset_file(def);
	chooser.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM |
			 Fl_Native_File_Chooser::NEW_FOLDER |
			 Fl_Native_File_Chooser::PREVIEW);
	chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);

	const char* fn = get_file();
	if (fsel)
	    *fsel = chooser.filter_value();
	return fn;
}

const char* dir_select(const char* title, const char* filter, const char* def)
{
	chooser.title(title);
	chooser.filter(filter);
	if (def)
		chooser.directory(def);
	chooser.options(Fl_Native_File_Chooser::NEW_FOLDER |
			 Fl_Native_File_Chooser::PREVIEW);
	chooser.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);

	return get_file();
}
