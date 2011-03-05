// ----------------------------------------------------------------------------
// rx_extract.cxx extract delineated data stream to file
//
// Copyright 2009 W1HKJ, Dave Freese
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

#include <iostream>
#include <fstream>
#include <string>

#include <FL/filename.H>
#include "fileselect.h"

#include "gettext.h"
#include "rx_extract.h"
#include "main.h"
#include "status.h"
#include "fl_digi.h"
#include "configuration.h"
#include "confdialog.h"
#include "debug.h"
#include "icons.h"

using namespace std;

const char *beg = "[WRAP:beg]";
const char *end = "[WRAP:end]";
const char *flmsg = "<flmsg>";

#ifdef __WIN32__
const char *txtWrapInfo = _("\
Detect the occurance of [WRAP:beg] and [WRAP:end]\n\
Save tags and all enclosed text to date-time stamped file, ie:\n\
    NBEMS.files\\WRAP\\recv\\extract-20090127-092515.wrap");
#else
const char *txtWrapInfo = _("\
Detect the occurance of [WRAP:beg] and [WRAP:end]\n\
Save tags and all enclosed text to date-time stamped file, ie:\n\
    ~/.nbems/WRAP/recv/extract-20090127-092515.wrap");
#endif

#define   bufsize  16
char  rx_extract_buff[bufsize + 1];
string rx_buff;
string rx_extract_msg;
bool extracting = false;
bool bInit = false;

char dttm[64];

void rx_extract_reset()
{
	rx_buff.clear();
	memset(rx_extract_buff, ' ', bufsize);
	rx_extract_buff[bufsize] = 0;
	extracting = false;
}

void rx_extract_add(int c)
{
	if (!c) return;
	check_nbems_dirs();

	if (!bInit) {
		rx_extract_reset();
		bInit = true;
	}
	char ch = (char)c;

	memmove(rx_extract_buff, &rx_extract_buff[1], bufsize - 1);
	rx_extract_buff[bufsize - 1] = ch;

	if ( strstr(rx_extract_buff, beg) != NULL ) {
		rx_buff = beg;
		rx_extract_msg = "Extracting";

		put_status(rx_extract_msg.c_str(), 60, STATUS_CLEAR);

		memset(rx_extract_buff, ' ', bufsize);
		extracting = true;
	} else if (extracting) {
		rx_buff += ch;
		if (strstr(rx_extract_buff, end) != NULL) {
			struct tm tim;
			time_t t;
			time(&t);
	        gmtime_r(&t, &tim);
			strftime(dttm, sizeof(dttm), "%Y%m%d-%H%M%S", &tim);

			string outfilename = WRAP_recv_dir;
			outfilename.append("extract-");
			outfilename.append(dttm);
			outfilename.append(".wrap");
			ofstream extractstream(outfilename.c_str(), ios::binary);
			if (extractstream) {
				extractstream << rx_buff;
				extractstream.close();
			}
			rx_extract_msg = "File saved in ";
			rx_extract_msg.append(WRAP_recv_dir);
			put_status(rx_extract_msg.c_str(), 20, STATUS_CLEAR);

			if (progdefaults.open_nbems_folder)
				open_recv_folder(WRAP_recv_dir.c_str());

			if (progdefaults.open_flmsg && 
				(rx_buff.find(flmsg) != string::npos) &&
				!progdefaults.flmsg_pathname.empty()) {
				string cmd = progdefaults.flmsg_pathname;
#ifdef __MINGW32__
				cmd.append(" \"").append(outfilename).append("\"");
				char *cmdstr = strdup(cmd.c_str());
				STARTUPINFO si;
				PROCESS_INFORMATION pi;
				memset(&si, 0, sizeof(si));
				si.cb = sizeof(si);
				memset(&pi, 0, sizeof(pi));
				if (!CreateProcess( NULL, cmdstr, 
					NULL, NULL, FALSE, 
					CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
					LOG_ERROR("CreateProcess failed with error code %ld", GetLastError());
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				free (cmdstr);
#else
				switch (fork()) {
				case 0:
#  ifndef NDEBUG
					unsetenv("MALLOC_CHECK_");
					unsetenv("MALLOC_PERTURB_");
#  endif
					execlp(
						(char*)cmd.c_str(), 
						(char*)cmd.c_str(), 
						(char*)outfilename.c_str(), 
						(char*)0);
					exit(EXIT_FAILURE);
				case -1:
					fl_alert2(_("Could not start flmsg"));
				}
#endif
			}
			rx_extract_reset();
		} else if (rx_buff.length() > 16384) {
			rx_extract_msg = "Extract length exceeded 16384 bytes";
			put_status(rx_extract_msg.c_str(), 20, STATUS_CLEAR);
			rx_extract_reset();
		}
	}
}

void select_flmsg_pathname()
{
#ifdef __APPLE__
	open_recv_folder("/Applications/");
	return;
#else
	string deffilename = progdefaults.flmsg_pathname;
	if (deffilename.empty())
#  ifdef __MINGW32__
		deffilename = "C:\\Program Files\\";
		const char *p = FSEL::select(_("Locate flmsg executable"), _("flmsg.exe\t*.exe"), deffilename.c_str());
#  else
		deffilename = "/usr/local/bin/";
		const char *p = FSEL::select(_("Locate flmsg executable"), _("flmsg\t*"), deffilename.c_str());
# endif
	if (p) {
		progdefaults.flmsg_pathname = p;
		progdefaults.changed = true;
		txt_flmsg_pathname->value(p);
	}
#endif
}
