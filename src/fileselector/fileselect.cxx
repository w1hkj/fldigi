#include <config.h>

#include <string>
#include <cstdlib>
#include <libgen.h>

#include "fileselect.h"
#include "icons.h"
#include "debug.h"

#include <FL/fl_ask.H>
#include <FL/Fl_Native_File_Chooser.H>

#if FSEL_THREAD
#    include <FL/Fl.H>
#    include <semaphore.h>
#    include "threads.h"
#endif

using namespace std;


FSEL* FSEL::inst = 0;
static std::string filename;
#if FSEL_THREAD
static pthread_t fsel_thread;
sem_t fsel_sem;
#endif

void FSEL::create(void)
{
	if (inst)
		return;
#if FSEL_THREAD
	if (sem_init(&fsel_sem, 0, 0) == -1) {
		LOG_PERROR("sem_init");
		return;
	}
#endif
	inst = new FSEL;
}

void FSEL::destroy(void)
{
#if FSEL_THREAD
	sem_destroy(&fsel_sem);
#endif
	delete inst;
	inst = 0;
}


FSEL::FSEL()
	: chooser(new Fl_Native_File_Chooser) { }
FSEL::~FSEL() { delete chooser; }


#if FSEL_THREAD
void* FSEL::thread_func(void* arg)
{
	FSEL* fsel = reinterpret_cast<FSEL*>(arg);
	fsel->result = fsel->chooser->show();
	sem_post(&fsel_sem);
	return NULL;
}
#endif

const char* FSEL::get_file(void)
{
	// Calling directory() is apparently not enough on Linux
#ifndef __WOE32__
	const char* preset = chooser->preset_file();
	if (preset && *preset != '/' && chooser->directory()) {
		filename = chooser->directory();
		filename.append("/").append(preset);
		chooser->preset_file(filename.c_str());
	}
#endif

#if FSEL_THREAD
	if (pthread_create(&fsel_thread, NULL, thread_func, this) != 0) {
		fl_alert2("could not create file selector thread");
		return NULL;
	}
	for (;;) {
		if (sem_trywait(&fsel_sem) == 0)
			break;
		Fl::wait(0.1);
	}
#else
	result = chooser->show();
#endif

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
	inst->chooser->options(Fl_Native_File_Chooser::PREVIEW);
	inst->chooser->type(Fl_Native_File_Chooser::BROWSE_FILE);

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
	inst->chooser->options(Fl_Native_File_Chooser::SAVEAS_CONFIRM |
			       Fl_Native_File_Chooser::NEW_FOLDER |
			       Fl_Native_File_Chooser::PREVIEW);
	inst->chooser->type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);

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
	inst->chooser->options(Fl_Native_File_Chooser::NEW_FOLDER |
			       Fl_Native_File_Chooser::PREVIEW);
	inst->chooser->type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);

	return inst->get_file();
}
