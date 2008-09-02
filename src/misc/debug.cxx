// ----------------------------------------------------------------------------
//      debug.cxx
//
// Copyright (C) 2008
//              Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
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

#include <cstdio>
#include <cstring>
#include <cstdarg>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Slider.H>
#include "FTextView.h"

#include "debug.h"

using namespace std;

#define MAX_LINES 65536

static FILE* wfile;
static FILE* rfile;
static size_t nlines = 0;
static int rfd;
static bool tty;

static Fl_Double_Window* window;
static FTextLog* text;

debug* debug::inst = 0;
debug::level_e debug::level = debug::WARN_LEVEL;

const char* prefix[] = { "Quiet", "Error", "Warning", "Info", "Debug" };

static void slider_cb(Fl_Widget* w, void*);

void debug::start(const char* filename)
{
	if (debug::inst)
		return;
	inst = new debug(filename);

	window = new Fl_Double_Window(512, 256, "Event log");
	window->xclass(PACKAGE_TARNAME);

	int pad = 2;
	Fl_Slider* slider = new Fl_Slider(pad, pad, 128, 20, prefix[level]);
	slider->tooltip("Change log level");
	slider->align(FL_ALIGN_RIGHT);
	slider->type(FL_HOR_NICE_SLIDER);
	slider->range(0.0, LOG_NLEVELS - 1);
	slider->step(1.0);
	slider->value(level);
	slider->callback(slider_cb);

	text = new FTextLog(pad, slider->h()+pad, window->w()-2*pad, window->h()-slider->h()-2*pad, 0);
	text->textfont(FL_SCREEN);
	text->textsize(FL_NORMAL_SIZE);
	window->resizable(text);
	window->end();
}

void debug::stop(void)
{
	delete inst;
	inst = 0;
	delete window;
}

static char fmt[1024];

void debug::log(level_e level, const char* func, const char* srcf, int line, const char* format, ...)
{
	if (!inst)
		return;

	if (unlikely(debug::level == DEBUG_LEVEL)) {
		time_t t = time(NULL);
		struct tm stm;
		(void)localtime_r(&t, &stm);
		snprintf(fmt, sizeof(fmt), "%c: [%02d:%02d:%02d] %s:%d: %s\n",
			 *prefix[level], stm.tm_hour, stm.tm_min, stm.tm_sec, srcf, line, format);
	}
	else
		snprintf(fmt, sizeof(fmt), "%c: %s: %s\n", *prefix[level], func, format);
	va_list args;
	va_start(args, format);
	vfprintf(wfile, fmt, args);
	if (tty && level <= DEBUG_LEVEL && level > QUIET_LEVEL)
		vfprintf(stderr, fmt, args);
	va_end(args);

	Fl::add_timeout(0.0, sync_text, 0);
}

void debug::elog(const char* func, const char* srcf, int line, const char* text)
{
	log(ERROR_LEVEL, func, srcf, line, "%s: %s", strerror(errno));
}

void debug::show(void)
{
	window->show();
}

static char buf[BUFSIZ+1];

void debug::sync_text(void*)
{
	ssize_t n;
	while ((n = read(rfd, buf, sizeof(buf) - 1)) > 0) {
		buf[n] = '\0';
		text->add(buf);
		if (unlikely(++nlines > MAX_LINES)) {
			text->clear();
			nlines = 0;
		}
	}
}

debug::debug(const char* filename)
{
	if ((wfile = fopen(filename, "w")) == NULL)
		throw strerror(errno);
	setlinebuf(wfile);
	set_cloexec(fileno(wfile), 1);

	if ((rfile = fopen(filename, "r")) == NULL)
		throw strerror(errno);
	rfd = fileno(rfile);
	set_cloexec(rfd, 1);
	int f;
	if ((f = fcntl(rfd, F_GETFL)) == -1)
		throw strerror(errno);
	if (fcntl(rfd, F_SETFL, f | O_NONBLOCK) == -1)
		throw strerror(errno);

	tty = isatty(fileno(stderr));
}

debug::~debug()
{
	fclose(wfile);
	fclose(rfile);
}

static void slider_cb(Fl_Widget* w, void*)
{
	Fl_Slider* s = static_cast<Fl_Slider*>(w);
	debug::level = (debug::level_e)s->value();
	s->label(prefix[debug::level]);
	s->parent()->redraw();
}
