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

#include <unistd.h>
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
#include "qrunner.h"

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

void rx_extract_timer(void *)
{
	rx_extract_msg = "Extract timed out";
	put_status(rx_extract_msg.c_str(), 20, STATUS_CLEAR);
	rx_extract_reset();
}

void rx_add_timer()
{
	Fl::add_timeout(progdefaults.extract_timeout, rx_extract_timer, NULL);
}

void rx_remove_timer()
{
	Fl::remove_timeout(rx_extract_timer);
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
		REQ(rx_remove_timer);
		REQ(rx_add_timer);
	} else if (extracting) {
		rx_buff += ch;
		REQ(rx_remove_timer);
		REQ(rx_add_timer);
		if (strstr(rx_extract_buff, end) != NULL) {
			REQ(rx_remove_timer);
			struct tm tim;
			time_t t;
			time(&t);
	        gmtime_r(&t, &tim);
			strftime(dttm, sizeof(dttm), "%Y%m%d-%H%M%S", &tim);

			string outfilename = FLMSG_WRAP_recv_dir;
			outfilename.append("extract-");
			outfilename.append(dttm);
			outfilename.append(".wrap");
			ofstream extractstream(outfilename.c_str(), ios::binary);
			if (extractstream) {
				extractstream << rx_buff;
				extractstream.close();
			}
			rx_extract_msg = "File saved in ";
			rx_extract_msg.append(FLMSG_WRAP_recv_dir);
			put_status(rx_extract_msg.c_str(), 20, STATUS_CLEAR);

			if (progdefaults.open_nbems_folder)
				open_recv_folder(FLMSG_WRAP_recv_dir.c_str());

			if ((progdefaults.open_flmsg || progdefaults.open_flmsg_print) && 
				(rx_buff.find(flmsg) != string::npos) &&
				!progdefaults.flmsg_pathname.empty()) {
				string cmd = progdefaults.flmsg_pathname;
#ifdef __MINGW32__
				cmd.append(" -title ").append(dttm);
				cmd.append(" --flmsg-dir ").append("\"").append(FLMSG_dir).append("\"");

				if (progdefaults.open_flmsg_print && progdefaults.open_flmsg)
					cmd.append(" --b");
				else if (progdefaults.open_flmsg_print)
					cmd.append(" --p");
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
				string params = "";
				static string ap[10];// = cmd;//"";
				string param = "";

				size_t p = cmd.find(" -");
				if (p != string::npos) {
					param.assign(cmd.substr(p));
					cmd = cmd.substr(0,p);
				}
				for (int i = 0; i < 10; i++) ap[i].clear();

				int n = 0;
				ap[n++] = "-title"; ap[n++] = dttm;
				ap[n++] = "--flmsg-dir"; ap[n++] = FLMSG_dir;
				if (progdefaults.open_flmsg_print && progdefaults.open_flmsg)
					ap[n++] = " --b";//params = " --b";
				else if (progdefaults.open_flmsg_print)
					ap[n++] = " --p";//params = " --p";
				ap[n++] = outfilename; 

				switch (fork()) {
				case 0:
#  ifndef NDEBUG
					unsetenv("MALLOC_CHECK_");
					unsetenv("MALLOC_PERTURB_");
#  endif
					switch (n) {
						case 1:
							execlp(
								(char*)cmd.c_str(), (char*)cmd.c_str(),
								(char*)ap[0].c_str(),
								(char*)0);
							break;
						case 2:
							execlp(
								(char*)cmd.c_str(), (char*)cmd.c_str(),
								(char*)ap[0].c_str(), (char*)ap[1].c_str(),
								(char*)0);
							break;
						case 3:
							execlp(
								(char*)cmd.c_str(), (char*)cmd.c_str(),
								(char*)ap[0].c_str(), (char*)ap[1].c_str(),
								(char*)ap[2].c_str(),
								(char*)0);
							break;
						case 4:
							execlp(
								(char*)cmd.c_str(), (char*)cmd.c_str(),
								(char*)ap[0].c_str(), (char*)ap[1].c_str(),
								(char*)ap[2].c_str(), (char*)ap[3].c_str(),
								(char*)0);
							break;
						case 5:
							execlp(
								(char*)cmd.c_str(), (char*)cmd.c_str(),
								(char*)ap[0].c_str(), (char*)ap[1].c_str(),
								(char*)ap[2].c_str(), (char*)ap[3].c_str(),
								(char*)ap[4].c_str(),
								(char*)0);
							break;
						case 6:
							execlp(
								(char*)cmd.c_str(), (char*)cmd.c_str(),
								(char*)ap[0].c_str(), (char*)ap[1].c_str(),
								(char*)ap[2].c_str(), (char*)ap[3].c_str(),
								(char*)ap[4].c_str(), (char*)ap[5].c_str(),
								(char*)0);
							break;
						case 7:
							execlp(
								(char*)cmd.c_str(), (char*)cmd.c_str(),
								(char*)ap[0].c_str(), (char*)ap[1].c_str(),
								(char*)ap[2].c_str(), (char*)ap[3].c_str(),
								(char*)ap[4].c_str(), (char*)ap[5].c_str(),
								(char*)ap[6].c_str(),
								(char*)0);
							break;
						case 8:
							execlp(
								(char*)cmd.c_str(), (char*)cmd.c_str(),
								(char*)ap[0].c_str(), (char*)ap[1].c_str(),
								(char*)ap[2].c_str(), (char*)ap[3].c_str(),
								(char*)ap[4].c_str(), (char*)ap[5].c_str(),
								(char*)ap[6].c_str(), (char*)ap[7].c_str(),
								(char*)0);
							break;
						case 9:
							execlp(
								(char*)cmd.c_str(), (char*)cmd.c_str(),
								(char*)ap[0].c_str(), (char*)ap[1].c_str(),
								(char*)ap[2].c_str(), (char*)ap[3].c_str(),
								(char*)ap[4].c_str(), (char*)ap[5].c_str(),
								(char*)ap[6].c_str(), (char*)ap[7].c_str(),
								(char*)ap[8].c_str(),
								(char*)0);
							break;
						case 10:
							execlp(
								(char*)cmd.c_str(), (char*)cmd.c_str(),
								(char*)ap[0].c_str(), (char*)ap[1].c_str(),
								(char*)ap[2].c_str(), (char*)ap[3].c_str(),
								(char*)ap[4].c_str(), (char*)ap[5].c_str(),
								(char*)ap[6].c_str(), (char*)ap[7].c_str(),
								(char*)ap[8].c_str(), (char*)ap[9].c_str(),
								(char*)0);
							break;
						default : ;
					}
					exit(EXIT_FAILURE);
				case -1:
					fl_alert2(_("Could not start flmsg"));
				}
#endif
			}
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
