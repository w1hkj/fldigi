#include <config.h>

#include "fileselect.h"

#include <FL/fl_ask.H>
#include <FL/Fl_Native_File_Chooser.H>


static Fl_Native_File_Chooser* chooser = 0;

static const char* get_file(void)
{
	switch (chooser->show()) {
	case -1:
		fl_alert("%s", chooser->errmsg());
		// fall through
	case 1:
		return NULL;
	default:
		return chooser->filename();
	}
}

const char* file_select(const char* title, const char* filter, const char* def)
{
	if (!chooser)
		chooser = new Fl_Native_File_Chooser;

	chooser->title(title);
	chooser->filter(filter);
	if (def)
		chooser->preset_file(def);
	chooser->options(Fl_Native_File_Chooser::PREVIEW);
	chooser->type(Fl_Native_File_Chooser::BROWSE_FILE);

	return get_file();
}

const char* file_saveas(const char* title, const char* filter, const char* def)
{
	if (!chooser)
		chooser = new Fl_Native_File_Chooser;

	chooser->title(title);
	chooser->filter(filter);
	if (def)
		chooser->preset_file(def);
	chooser->options(Fl_Native_File_Chooser::SAVEAS_CONFIRM |
			 Fl_Native_File_Chooser::NEW_FOLDER |
			 Fl_Native_File_Chooser::PREVIEW);
	chooser->type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);

	return get_file();
}

const char* dir_select(const char* title, const char* filter, const char* def)
{
	if (!chooser)
		chooser = new Fl_Native_File_Chooser;

	chooser->title(title);
	chooser->filter(filter);
	if (def)
		chooser->directory(def);
	chooser->options(Fl_Native_File_Chooser::NEW_FOLDER |
			 Fl_Native_File_Chooser::PREVIEW);
	chooser->type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);

	return get_file();
}
