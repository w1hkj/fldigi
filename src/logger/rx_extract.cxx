// ----------------------------------------------------------------------------
// rx_extract.cxx extract delineated data stream to file
//
// Copyright 2013 W1HKJ, Dave Freese
//           2013 KL4YFD, John Phelps
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
#include "timeops.h"

using namespace std;

static const char *wrap_beg = "[WRAP:beg]";
static const char *wrap_end = "[WRAP:end]";
static const char *flmsg = "<flmsg>";

static const char flamp_beg[] = ">FLAMP";
static const char flamp_end[] = ":EOT}";

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

bool extract_wrap = false;
bool extract_flamp = false;

bool bInit = false;

char dttm[64];

void rx_extract_reset()
{
	rx_buff.clear();
	memset(rx_extract_buff, ' ', bufsize);
	rx_extract_buff[bufsize] = 0;
	extract_wrap = false;
	extract_flamp = false;
	put_status("");
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

void invoke_flmsg()
{
	string cmd = progdefaults.flmsg_pathname;

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

	if ( strstr(rx_extract_buff, wrap_beg) && !extract_flamp) {
		rx_buff.assign(wrap_beg);
		rx_extract_msg = "Extracting WRAP/FLMSG";

		put_status(rx_extract_msg.c_str());

		memset(rx_extract_buff, ' ', bufsize);
		extract_wrap = true;
		REQ(rx_remove_timer);
		REQ(rx_add_timer);
	} else if (extract_wrap) {
		rx_buff += ch;
		REQ(rx_remove_timer);
		REQ(rx_add_timer);
		if (strstr(rx_extract_buff, wrap_end) != NULL) {
			invoke_flmsg();
			rx_extract_reset();
		}
	} else if (strstr(rx_extract_buff, flamp_beg) && ! extract_wrap) {
		extract_flamp = true;
		rx_extract_msg = "Extracting FLAMP";
		put_status(rx_extract_msg.c_str());
	} else if (extract_flamp == true) {
		REQ(rx_remove_timer);
		REQ(rx_add_timer);
		if (strstr(rx_extract_buff, flamp_end) != NULL) {
			rx_extract_reset();
		}
	}
}

void select_flmsg_pathname()
{
	if (NBEMSapps_dir) {
		progdefaults.flmsg_pathname.assign(BaseDir).append("flmsg.exe");
		progdefaults.changed = true;
		txt_flmsg_pathname->value(progdefaults.flmsg_pathname.c_str());
		return;
	}
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

// this only works on Linux and Unix
// not Windoze or
// OS X to find binaries in the /Applications/ directory structure

bool find_pathto_exectable(string &binpath, string executable)
{
	size_t endindex = 0;

	binpath.clear();

// Get the PATH environment variable as pointer to string
// The strings in the environment list are of the form name=value.
// As  typically  implemented, getenv() returns a pointer to a string within 
// the environment list.  The caller must take care not to modify this string,
// since that would  change the environment of the process.
//
// The  implementation of getenv() is not required to be reentrant.  The string
// pointed to by the return value of getenv() may be statically allocated, and 
// can be modified by a  subsequent call to getenv(), putenv(3), setenv(3), or 
// unsetenv(3).

	char *environment = getenv("PATH");

	if (environment == NULL) return false;

	string env = environment;
	string testpath = "";

	char endchar = ':';

	// Parse single PATH string into directories
	while (!env.empty()) {
		endindex = env.find(endchar);
		testpath = env.substr(0, endindex);

		testpath.append("/"); // insert linux, unix, osx OS-correct delimiter
		testpath.append(executable); // append executable name

		// Most portable way to check if a file exists: Try to open it.
		FILE *checkexists = NULL;
		checkexists = fopen( testpath.c_str(), "r" ); // try to open file readonly
		if (checkexists) { // if the file successfully opened, it exists.
			fclose(checkexists);
			binpath = testpath;
			return true;
		}
		if (endindex == string::npos)
			env.clear();
		else
			env.erase(0, endindex + 1);
	}
	return false;
}

string select_binary_pathname(string deffilename)
{
#ifdef __APPLE__
	open_recv_folder("/Applications/");
	return "";
#endif

	string executable;

#ifdef __MINGW32__
	deffilename = "C:\\Program Files\\";
	const char *psz = FSEL::select(_("Locate executable"), _("*.exe"), deffilename.c_str());
#else
	size_t p = deffilename.rfind('/');
	if (p != string::npos) executable = deffilename.substr(p+1);
	else executable = deffilename;

	if (!executable.empty() && find_pathto_exectable(deffilename, executable))
		return deffilename;

	deffilename = "/usr/bin/";
	const char *psz = FSEL::select(_("Locate binary"), _("*"), deffilename.c_str());
#endif

	if (psz) executable = psz;
// do not allow recursion !!
	if (executable.find("fldigi") != string::npos) return "";
	return executable;

}

