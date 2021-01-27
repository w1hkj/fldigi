// ----------------------------------------------------------------------------
//      debug.cxx
//
// Copyright (C) 2008-2010
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

#ifdef __MINGW32__
#  include "compat.h"
#endif

#include <sstream>
#include <fstream>

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
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Button.H>

#include <FL/Fl_Browser.H>

#include "debug.h"
#include "debug_dialog.h"
#include "status.h"

#include "timeops.h"
#include "icons.h"
#include "gettext.h"

#include "threads.h"

#ifndef FLARQ_VERSION
#	include "status.h"
#	include "fl_digi.h"
#endif

static pthread_mutex_t debug_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t debug_hd_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t rotate_mutex = PTHREAD_MUTEX_INITIALIZER;

extern Fl_Double_Window *fl_digi_main;
extern void update_main_title();

using namespace std;

#define MAX_LINES 65536

static FILE* wfile = 0;
static FILE* rfile = 0;
static int rfd;
static bool tty;

static Fl_Double_Window* window;
static string linebuf;

debug* debug::inst = 0;
debug::level_e debug::level = debug::INFO_LEVEL;

static const char* prefix[] = {
	_("Quiet"), _("Error"), _("Warning"), _("Info"), _("Verbose"), _("Debug")
};

unsigned int debug::mask = 0;

#include <iostream>

void fcopy(std::string from, std::string to)
{
	char buffer[65536];
	FILE *fp_from, *fp_to;
	size_t n;
	if ((fp_from = fopen(from.c_str(), "rb")) != NULL) {
		if ((fp_to = fopen(to.c_str(), "wb")) != NULL) {
			while(1) {
				if (feof(fp_from))
					break;
//				memset(buffer, 0, sizeof(buffer));
				if ((n = fread(buffer, 1, sizeof(buffer), fp_from)) > 0)
					n = fwrite(buffer, 1, n, fp_to);
				else
					break;
			}
			fflush(fp_to);
			fclose(fp_to);
		}
		fclose(fp_from);
	}
}

void rotate_log(std::string filename)
{
	guard_lock rlock(&rotate_mutex);

	std::string oldfn, newfn;
	const char *ext[] = {".1", ".2", ".3", ".4", ".5"};

	for (int i = 4; i > 0; i--) {
		newfn.assign(filename).append(ext[i]);
		oldfn.assign(filename).append(ext[i - 1]);
		fcopy(oldfn, newfn);
	}
	newfn.assign(filename).append(ext[0]);
	fcopy(filename, newfn);
}


void debug::start(const char* filename)
{
	if (debug::inst)
		return;
	rotate_log(filename);
	inst = new debug(filename);

	window = debug_dialog();
}

void debug::stop(void)
{
	if (window) {
		window->hide();
		delete window;
		window = 0;
//std::cout << "debug window deleted" << std::endl;
	}
//	if (inst) {
//		delete inst;
//		inst = 0;
//std::cout << "instance deleted" << std::endl;
//	}
//std::cout << "debug stopped" << std::endl;
}

static char fmt[2048];
static char dtext[32768];

void debug::log(level_e level, const char* func, const char* srcf, int line, const char* format, ...)
{
	guard_lock debug_lock(&debug_mutex);

	if (!debug::inst)
		return;

// always annotate with date/time & line number

	struct tm tm;
	time_t t_temp;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	t_temp=(time_t)tv.tv_sec;

//	gmtime_r(&t_temp, &tm);
	localtime_r(&t_temp, &tm);

	static int _zmsec = 0;
	static int _zsec = 0;
	static int _zmin = 0;
	static int _zhr = 0;

	_zmsec = tv.tv_usec / 1000;
	_zsec = tm.tm_sec;
	_zmin = tm.tm_min;
	_zhr  = tm.tm_hour;

	snprintf(fmt, sizeof(fmt), "%c: [%02d:%02d:%02d.%03d] %s : %d : %s\n    %s\n",
		*prefix[level], 
		_zhr, _zmin, _zsec, _zmsec, 
		srcf, line, func, format);

	va_list args;
	va_start(args, format);
	intptr_t nt = vsnprintf(dtext, sizeof(dtext), fmt, args);
	va_end(args);

	fprintf(wfile, "%s", dtext);
	if (tty) {
		if (level <= DEBUG_LEVEL && level > QUIET_LEVEL) {
			fprintf(stderr, "%s", dtext);
		}
	}

#ifdef __MINGW32__
	fflush(wfile);
#endif

	linebuf.append(dtext);
	Fl::awake(sync_text, (void*)nt);
}

void debug::hex_dump(const char* func, const char * data, int length)
{
	guard_lock debug_lock(&debug_hd_mutex);

	char cbuff[32];
	char hbuff[64];
	char tbuff[32];
	int index = 0;
	int data_index = 0;
	int count = length;
	unsigned int c = 0;
	int hi = 0;
	int step = 16;

	if(!func)
		func = "Unassigned";

	if(!data || length < 0) return;

	while(count > 0) {
		memset(cbuff, 0, sizeof(cbuff));
		memset(hbuff, 0, sizeof(hbuff));
		memset(tbuff, 0, sizeof(tbuff));
		hi = 0;
		for(index = 0; index < step; index++) {
			if(data_index < length) {
				c = ((unsigned int) data[data_index]) & 0xFF;
				if(c >= ' ' && c <= 0xff) {
					cbuff[index] = c;
				} else {
					cbuff[index] = '.';
				}
				snprintf(tbuff, sizeof(tbuff) - 1, "%02X", c);
				hbuff[hi++] = tbuff[0];
				hbuff[hi++] = tbuff[1];
				hbuff[hi++] = ' ';
			} else {
				break;
			}
			data_index++;
		}
		if (debug::inst) LOG_DEBUG("%s: %s %s", func, cbuff, hbuff);
		count -= step;
	}
}

void debug::elog(const char* func, const char* srcf, int line, const char* text)
{
	if (debug::inst)
		log(ERROR_LEVEL, func, srcf, line, "%s: %s", text, strerror(errno));
}

void debug::show(void)
{
	btext->bottomline(btext->size());
	window->show();
}

void debug::sync_text(void* arg)
{
	guard_lock debug_lock(&debug_mutex);

	if (!window) return;

	size_t p1 = 0, p2 = linebuf.find("\n");
	while (p2 != string::npos) {
		btext->add(linebuf.substr(p1, p2 - p1).c_str());
		p1 = p2 + 1;
		p2 = linebuf.find("\n", p1);
	}
	btext->redraw();
	btext->bottomline(btext->size());
	linebuf.clear();
	return;
}

debug::debug(const char* filename)
{
	if ((wfile = fl_fopen(filename, "w")) == NULL)
		throw strerror(errno);
	setvbuf(wfile, (char*)NULL, _IOLBF, 0);
	set_cloexec(fileno(wfile), 1);

	if ((rfile = fl_fopen(filename, "r")) == NULL)
		throw strerror(errno);
	rfd = fileno(rfile);
	set_cloexec(rfd, 1);
#ifndef __MINGW32__
	int f;
	if ((f = fcntl(rfd, F_GETFL)) == -1)
		throw strerror(errno);
	if (fcntl(rfd, F_SETFL, f | O_NONBLOCK) == -1)
		throw strerror(errno);
#endif
	tty = isatty(fileno(stderr));
	linebuf.clear();
}

debug::~debug()
{
	if (wfile) fclose(wfile);
	if (rfile) fclose(rfile);
//	if (window) {
//		window->hide();
//		delete window;
//		window = 0;
//	}
//	if (inst) {
//		delete inst;
//		inst = 0;
//	}
}

void mnu_debug_level_cb()
{
	debug::level = (debug::level_e)(mnu_debug_level->value());
}

void btn_debug_source_cb(int n)
{
	int mask = 1 << n;
	if (source_code[n]->value())
		debug::mask |= mask;
	else
		debug::mask &= ~mask;
}

void clear_debug()
{
	guard_lock debug_lock(&debug_mutex);
	btext->clear();
	linebuf.clear();
}

void set_debug_mask(int mask)
{
	debug::mask = mask;
	for (int n = 0; n < 15; n++) {
		source_code[n]->value( (mask & (1 << n)) ? 1 : 0);
		source_code[n]->redraw();
	}
	mnu_debug_level->value(progStatus.debug_level);
	debug::level = debug::level_e(progStatus.debug_level);
}
