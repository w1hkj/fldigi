// ----------------------------------------------------------------------------
// Digital Modem Program for the Fast Light Toolkit
//
// Copyright 2006-2010, Dave Freese, W1HKJ
// Copyright 2007-2010, Stelios Bounanos, M0GLD
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
//
// Please report all bugs and problems to fldigi-devel@lists.sourceforge.net.
// ----------------------------------------------------------------------------

#include <config.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <getopt.h>
#include <sys/types.h>

#if !defined(__WOE32__) && !defined(__APPLE__)
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif

#ifdef __MINGW32__
#  include "compat.h"
#endif

#include <sys/stat.h>

#if HAVE_SYS_UTSNAME_H
#  include <sys/utsname.h>
#endif

#include <unistd.h>

#include <exception>
#include <signal.h>
#include <locale.h>

#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/x.H>
#ifdef __MINGW32__
#  define dirent fl_dirent_no_thanks
#endif
#include <FL/filename.H>

#ifdef __WOE32__
#	if FLDIGI_FLTK_API_MAJOR == 1 && FLDIGI_FLTK_API_MINOR < 3
#		undef dirent
#		include <dirent.h>
#	endif
#else
#	include <dirent.h>
#endif


#include "gettext.h"
#include "main.h"
#include "waterfall.h"
#include "trx.h"
#include "soundconf.h"
#include "fl_digi.h"
#include "rigio.h"
#include "globals.h"
#include "confdialog.h"
#include "configuration.h"
#include "macros.h"
#include "status.h"
#include "fileselect.h"
#include "timeops.h"
#include "debug.h"
#include "pskrep.h"
#include "notify.h"
#include "logbook.h"
#include "dxcc.h"
#include "newinstall.h"
#include "Viewer.h"
#include "kmlserver.h"
#include "data_io.h"
#include "maclogger.h"
#include "psm/psm.h"
#include "fd_logger.h"

#if USE_HAMLIB
	#include "rigclass.h"
#endif
#include "rigsupport.h"

#include "log.h"

#include "qrunner.h"
#include "stacktrace.h"

#include "xmlrpc.h"

#if BENCHMARK_MODE
	#include "benchmark.h"
#endif

#include "icons.h"

#include "nullmodem.h"

using namespace std;

string appname;

string scDevice[2];

string BaseDir = "";
string HomeDir = "";
string RigsDir = "";
string ScriptsDir = "";
string PalettesDir = "";
string LogsDir = "";
string PicsDir = "";
string AvatarDir = "";
string HelpDir = "";
string MacrosDir = "";
string WrapDir = "";
string TalkDir = "";
string TempDir = "";
string KmlDir = "";
string PskMailDir = "";

string NBEMS_dir = "";
string DATA_dir = "";
string ARQ_dir = "";
string ARQ_files_dir = "";
string ARQ_recv_dir = "";
string ARQ_send = "";
string WRAP_dir = "";
string WRAP_recv_dir = "";
string WRAP_send_dir = "";
string WRAP_auto_dir = "";
string ICS_dir = "";
string ICS_msg_dir = "";
string ICS_tmp_dir = "";

string FLMSG_dir = "";
string FLMSG_dir_default = "";
string FLMSG_WRAP_dir = "";
string FLMSG_WRAP_recv_dir = "";
string FLMSG_WRAP_send_dir = "";
string FLMSG_WRAP_auto_dir = "";
string FLMSG_ICS_dir = "";
string FLMSG_ICS_msg_dir = "";
string FLMSG_ICS_tmp_dir = "";

string PskMailFile;
string ArqFilename;
string xmlfname;

PTT		*push2talk = (PTT *)0;
#if USE_HAMLIB
Rig		*xcvr = (Rig *)0;
#endif

bool tlfio = false;
cLogfile	*logfile = 0;
cLogfile	*Maillogfile = (cLogfile *)0;
FILE	*server;
FILE	*client;
bool	mailserver = false, mailclient = false, arqmode = false;
static bool show_cpucheck = false;
static bool iconified = false;
bool	bMOREINFO = false;

string option_help, version_text, build_text;

qrunner *cbq[NUM_QRUNNER_THREADS];

void arqchecks(void);
void generate_option_help(void);
int parse_args(int argc, char **argv, int& idx);
void generate_version_text(void);
void debug_exec(char** argv);
void set_platform_ui(void);
double speed_test(int converter, unsigned repeat);
static void setup_signal_handlers(void);
static void checkdirectories(void);

static void arg_error(const char* name, const char* arg, bool missing);
static void fatal_error(string);

// TODO: find out why fldigi crashes on OS X if the wizard window is
// shown before fldigi_main.
#ifndef __APPLE__
#  define SHOW_WIZARD_BEFORE_MAIN_WINDOW 1
#else
#  define SHOW_WIZARD_BEFORE_MAIN_WINDOW 0
#endif

void start_process(string executable)
{
	if (!executable.empty()) {
#ifdef __MINGW32__
		char* cmd = strdup(executable.c_str());
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);
		memset(&pi, 0, sizeof(pi));
		if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
			LOG_ERROR("CreateProcess failed with error code %ld", GetLastError());
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		free(cmd);
#else
#ifdef __APPLE__
	if (executable.find(".app") == (executable.length() - 4)) {
		std::string progname = executable;
		size_t p = progname.find("/", 1);
		if (p != std::string::npos) progname.erase(0,p+1);
		p = progname.find("-");
		if (p != std::string::npos) progname.erase(p);
		executable.append("/Contents/MacOS/").append(progname);
	}
#endif
		switch (fork()) {
			case -1:
				LOG_PERROR("fork");
				// fall through
			default:
				break;

			case 0:
				execl("/bin/sh", "sh", "-c", executable.c_str(), (char *)NULL);
				perror("execl");
				exit(EXIT_FAILURE);
		}
#endif
	}
}

void toggle_io_port_selection(int io_mode)
{

	switch(io_mode) {
		case ARQ_IO:
			enable_arq();
			progdefaults.changed = false;
		break;

		case KISS_IO:

			enable_kiss();

			if(progdefaults.tcp_udp_auto_connect) {
				btn_connect_kiss_io->value(1);
				btn_connect_kiss_io->do_callback();
			}

			if(progdefaults.kpsql_enabled && progdefaults.show_psm_btn) {
				btnPSQL->value(progdefaults.kpsql_enabled);
				btnPSQL->do_callback();
			}

			progdefaults.changed = false;
			break;

		default:
			LOG_INFO("Unknown data io mode");
	}
}

static void auto_start()
{
	bool run_flamp = false;

	// Make sure we are in ARQ_IO mode if executing FLAMP
	if (!progdefaults.auto_flamp_pathname.empty() &&
		 progdefaults.flamp_auto_enable) {
		 toggle_io_port_selection(ARQ_IO);
		 run_flamp = true;
	}

	// A general wait to ensure FLDIGI initialization of
	// io ports. 1/4 to 3/4 second delay.
	int nloops = 0;
	while(nloops++ < 3) {
		MilliSleep(250);
		if(arq_state() && data_io_enabled == ARQ_IO)
			break; // Exit early if verified.
	}

	if (!progdefaults.auto_flrig_pathname.empty() &&
		 progdefaults.flrig_auto_enable)
		start_process(progdefaults.auto_flrig_pathname);

	if (run_flamp)
		start_process(progdefaults.auto_flamp_pathname);

	if (!progdefaults.auto_fllog_pathname.empty() &&
		 progdefaults.fllog_auto_enable)
		start_process(progdefaults.auto_fllog_pathname);

	if (!progdefaults.auto_flnet_pathname.empty() &&
		 progdefaults.flnet_auto_enable)
		start_process(progdefaults.auto_flnet_pathname);

	if (!progdefaults.auto_prog1_pathname.empty() &&
		 progdefaults.prog1_auto_enable)
		start_process(progdefaults.auto_prog1_pathname);

	if (!progdefaults.auto_prog2_pathname.empty() &&
		 progdefaults.prog2_auto_enable)
		start_process(progdefaults.auto_prog2_pathname);

	if (!progdefaults.auto_prog3_pathname.empty() &&
		 progdefaults.prog3_auto_enable)
		start_process(progdefaults.auto_prog3_pathname);
}

// reset those default values that have been overriden by a command line parameter
void check_overrides()
{
	if (xmlrpc_address_override_flag)
		progdefaults.xmlrpc_address = override_xmlrpc_address;
	if (xmlrpc_port_override_flag)
		progdefaults.xmlrpc_port = override_xmlrpc_port;
	if (arq_address_override_flag)
		progdefaults.arq_address = override_arq_address;
	if (arq_port_override_flag)
		progdefaults.arq_port = override_arq_port;
}

// these functions are all started after Fl::run() is executing
void delayed_startup(void *)
{
	macros.loadDefault();

	connect_to_log_server();

#ifdef __WIN32__
	if (progdefaults.auto_talk) open_talker();
#else
	grpTalker->hide();
#endif

	XML_RPC_Server::start(progdefaults.xmlrpc_address.c_str(), progdefaults.xmlrpc_port.c_str());

	FLRIG_start_flrig_thread();

	data_io_enabled = DISABLED_IO;

	arq_init();
	FD_init();

	start_psm_thread();

	if (progdefaults.connect_to_maclogger) maclogger_init();
	data_io_enabled = progStatus.data_io_enabled;

	toggle_io_port_selection(data_io_enabled);
	disable_config_p2p_io_widgets();

	notify_start();

	if (progdefaults.usepskrep)
		if (!pskrep_start())
			LOG_ERROR("Could not start PSK reporter: %s", pskrep_error());

	auto_start();

	if (progdefaults.check_for_updates)
		cb_mnuCheckUpdate((Fl_Widget *)0, NULL);

#if USE_PORTAUDIO
	LOG_INFO("%s", str_pa_devices.c_str());
#endif
}

int main(int argc, char ** argv)
{
	// for KISS_IO status information
	program_start_time = time(0);

	// ztimer must be run by FLTK's timeout handler
	ztimer((void*)true);

	active_modem = new NULLMODEM;

	string appdir = appname = argv[0];
	string test_file_name;

	BaseDir.clear();
	HomeDir.clear();
	NBEMS_dir.clear();
	FLMSG_dir.clear();

#ifdef __WOE32__
	size_t p = appdir.rfind("fldigi.exe");
	appdir.erase(p);
	p = appdir.find("FL_APPS\\");
	if (p != string::npos) {
		BaseDir.assign(appdir.substr(0, p + 8));
		progdefaults.flmsg_pathname.assign(BaseDir).append("flmsg.exe");
	} else {
		BaseDir.clear();
		HomeDir.clear();
		NBEMS_dir.clear();
		FLMSG_dir.clear();
	}
#else
	char apptemp[FL_PATH_MAX + 1];
	fl_filename_absolute(apptemp, sizeof(apptemp), argv[0]);
	appdir.assign(apptemp);
	size_t p = appdir.rfind("fldigi");
	if (p != string::npos)
		appdir.erase(p);
	p = appdir.find("FL_APPS/");
	if (p != string::npos) {
		BaseDir.assign(appdir.substr(0, p + 8));
		progdefaults.flmsg_pathname.assign(BaseDir).append("flmsg");
		string test_dir;
		test_dir.assign(BaseDir).append("fldigi.files/");
		DIR *isdir = opendir(test_dir.c_str());
		if (isdir) {
			HomeDir = test_dir;
			closedir(isdir);
		} else {
			test_dir.assign(BaseDir).append(".fldigi/");
			isdir = opendir(test_dir.c_str());
			if (isdir) {
				HomeDir = test_dir;
			} else {
				HomeDir.clear();
			}
		}
		if (!HomeDir.empty()) {
			test_dir.assign(BaseDir).append("NBEMS.files/");
			isdir = opendir(test_dir.c_str());
			if (isdir) {
				NBEMS_dir = test_dir;
				FLMSG_dir = test_dir;
				closedir(isdir);
			} else {
				test_dir.assign(BaseDir).append(".nbems/");
				isdir = opendir(test_dir.c_str());
				if (isdir) {
					NBEMS_dir = test_dir;
					FLMSG_dir = test_dir;
				} else {
					NBEMS_dir.clear();
					FLMSG_dir.clear();
				}
			}
		}
	} else {
		BaseDir.clear();
		HomeDir.clear();
		NBEMS_dir.clear();
		FLMSG_dir.clear();
	}
#endif

	debug_exec(argv);
	CREATE_THREAD_ID(); // only call this once
	SET_THREAD_ID(FLMAIN_TID);

	for (int i = 0; i < NUM_QRUNNER_THREADS; i++) {
		cbq[i] = new qrunner;
		switch(i) {
			case TRX_TID:
				cbq[i]->attach(i, "TRX_TID");
				break;

			case QRZ_TID:
				cbq[i]->attach(i, "QRZ_TID");
				break;

			case RIGCTL_TID:
				cbq[i]->attach(i, "RIGCTL_TID");
				break;

			case NORIGCTL_TID:
				cbq[i]->attach(i, "NORIGCTL_TID");
				break;

			case EQSL_TID:
				cbq[i]->attach(i, "EQSL_TID");
				break;

			case ADIF_RW_TID:
				cbq[i]->attach(i, "ADIF_RW_TID");
				break;

			case XMLRPC_TID:
				cbq[i]->attach(i, "XMLRPC_TID");
				break;

			case ARQ_TID:
				cbq[i]->attach(i, "ARQ_TID");
				break;

			case ARQSOCKET_TID:
				cbq[i]->attach(i, "ARQSOCKET_TID");
				break;

			case KISS_TID:
				cbq[i]->attach(i, "KISS_TID");
				break;

			case KISSSOCKET_TID:
				cbq[i]->attach(i, "KISSSOCKET_TID");
				break;

			case MACLOGGER_TID:
				cbq[i]->attach(i, "MACLOGGER_TID");
				break;

			case PSM_TID:
				cbq[i]->attach(i, "PSM_TID");
				break;

			case FD_TID:
				cbq[i]->attach(i, "FD_TID");
				break;

			case FLMAIN_TID:
				cbq[i]->attach(i, "FLMAIN_TID");
				break;

			default:
				break;
		}
	}

	set_unexpected(handle_unexpected);
	set_terminate(diediedie);
	setup_signal_handlers();

#ifndef ENABLE_NLS
	setlocale(LC_TIME, "");
#endif

	set_platform_ui();

	generate_version_text();
	{
		char dirbuf[FL_PATH_MAX + 1];
#ifdef __WOE32__
		if (BaseDir.empty()) {
			fl_filename_expand(dirbuf, sizeof(dirbuf) -1, "$USERPROFILE/");
			BaseDir = dirbuf;
		}
#else
		if (BaseDir.empty()) {
			fl_filename_expand(dirbuf, sizeof(dirbuf) -1, "$HOME/");
			BaseDir = dirbuf;
		}
#endif
	}

	generate_option_help();

	int arg_idx;
	if (Fl::args(argc, argv, arg_idx, parse_args) != argc)
		arg_error(argv[0], NULL, false);

	if (argv_window_title.empty())
		argv_window_title.assign(PACKAGE_TARNAME);

#ifdef __WOE32__
	if (HomeDir.empty()) HomeDir.assign(BaseDir).append("fldigi.files/");
	if (PskMailDir.empty()) PskMailDir = BaseDir;
	if (DATA_dir.empty()) DATA_dir.assign(BaseDir).append("DATA.files/");
	if (NBEMS_dir.empty()) NBEMS_dir.assign(BaseDir).append("NBEMS.files/");
	if (FLMSG_dir.empty()) FLMSG_dir = FLMSG_dir_default = NBEMS_dir;
#else
	if (HomeDir.empty()) HomeDir.assign(BaseDir).append(".fldigi/");
	if (PskMailDir.empty()) PskMailDir = BaseDir;
	if (DATA_dir.empty()) DATA_dir.assign(BaseDir).append("DATA.files/");
	if (NBEMS_dir.empty()) NBEMS_dir.assign(BaseDir).append(".nbems/");
	if (FLMSG_dir.empty()) FLMSG_dir = FLMSG_dir_default = NBEMS_dir;
#endif

	if (!FLMSG_dir_default.empty()) {
		char dirbuf[FL_PATH_MAX + 1];
		if (FLMSG_dir_default[FLMSG_dir_default.length()-1] != '/')
			FLMSG_dir_default += '/';
		fl_filename_expand(dirbuf, sizeof(dirbuf) - 1, FLMSG_dir_default.c_str());
		FLMSG_dir = dirbuf;
	}
	checkdirectories();
	check_nbems_dirs();
	check_data_dir();

	try {
		debug::start(string(HomeDir).append("status_log.txt").c_str());
		time_t t = time(NULL);
		LOG(debug::QUIET_LEVEL, debug::LOG_OTHER, _("%s log started on %s"), PACKAGE_STRING, ctime(&t));
		LOG_THREAD_ID();
	}
	catch (const char* error) {
		cerr << error << '\n';
		debug::stop();
	}

	LOG_INFO("appname: %s", appname.c_str());
	LOG_INFO("HomeDir: %s", HomeDir.c_str());
	LOG_INFO("RigsDir: %s", RigsDir.c_str());
	LOG_INFO("ScriptsDir: %s", ScriptsDir.c_str());
	LOG_INFO("PalettesDir: %s", PalettesDir.c_str());
	LOG_INFO("LogsDir: %s", LogsDir.c_str());
	LOG_INFO("PicsDir: %s", PicsDir.c_str());
	LOG_INFO("HelpDir: %s", HelpDir.c_str());
	LOG_INFO("MacrosDir: %s", MacrosDir.c_str());
	LOG_INFO("WrapDir: %s", WrapDir.c_str());
	LOG_INFO("TalkDir: %s", TalkDir.c_str());
	LOG_INFO("TempDir: %s", TempDir.c_str());
	LOG_INFO("KmlDir: %s", KmlDir.c_str());
	LOG_INFO("PskMailDir: %s", PskMailDir.c_str());

	LOG_INFO("DATA_dir: %s", DATA_dir.c_str());
	LOG_INFO("NBEMS_dir: %s", NBEMS_dir.c_str());
	LOG_INFO("ARQ_dir: %s", ARQ_dir.c_str());
	LOG_INFO("ARQ_files_dir: %s", ARQ_files_dir.c_str());
	LOG_INFO("ARQ_recv_dir: %s", ARQ_recv_dir.c_str());
	LOG_INFO("ARQ_send: %s", ARQ_send.c_str());
	LOG_INFO("WRAP_dir: %s", WRAP_dir.c_str());
	LOG_INFO("WRAP_recv_dir: %s", WRAP_recv_dir.c_str());
	LOG_INFO("WRAP_send_dir: %s", WRAP_send_dir.c_str());
	LOG_INFO("WRAP_auto_dir: %s", WRAP_auto_dir.c_str());
	LOG_INFO("ICS_dir: %s", ICS_dir.c_str());
	LOG_INFO("ICS_msg_dir: %s", ICS_msg_dir.c_str());
	LOG_INFO("ICS_tmp_dir: %s", ICS_tmp_dir.c_str());

	LOG_INFO("FLMSG_dir: %s", FLMSG_dir.c_str());
	LOG_INFO("FLMSG_dir_default: %s", FLMSG_dir_default.c_str());
	LOG_INFO("FLMSG_WRAP_dir: %s", FLMSG_WRAP_dir.c_str());
	LOG_INFO("FLMSG_WRAP_recv_dir: %s", FLMSG_WRAP_recv_dir.c_str());
	LOG_INFO("FLMSG_WRAP_send_dir: %s", FLMSG_WRAP_send_dir.c_str());
	LOG_INFO("FLMSG_WRAP_auto_dir: %s", FLMSG_WRAP_auto_dir.c_str());
	LOG_INFO("FLMSG_ICS_dir: %s", FLMSG_ICS_dir.c_str());
	LOG_INFO("FLMSG_ICS_msg_dir: %s", FLMSG_ICS_msg_dir.c_str());
	LOG_INFO("FLMSG_ICS_tmp_dir: %s", FLMSG_ICS_tmp_dir.c_str());

	bool have_config = progdefaults.readDefaultsXML();
	check_overrides();

	xmlfname = HomeDir;
	xmlfname.append(DEFAULT_RIGXML_FILENAME);

	checkTLF();

	Fl::lock();  // start the gui thread!!
	Fl::visual(FL_RGB); // insure 24 bit color operation

	fl_register_images();
	Fl::set_fonts(0);

	Fl::scheme(progdefaults.ui_scheme.c_str());
	progdefaults.initFonts();

	if (progdefaults.cty_dat_pathname.empty())
		progdefaults.cty_dat_pathname = HomeDir;

	dxcc_open(string(progdefaults.cty_dat_pathname).append("cty.dat").c_str());
	qsl_open(string(progdefaults.cty_dat_pathname).append("lotw1.txt").c_str(), QSL_LOTW);
	if (!qsl_open(string(progdefaults.cty_dat_pathname).append("eqsl.txt").c_str(), QSL_EQSL))
		qsl_open(string(progdefaults.cty_dat_pathname).append("AGMemberList.txt").c_str(), QSL_EQSL);

	progStatus.loadLastState();
	create_fl_digi_main(argc, argv);

	if (!have_config || show_cpucheck) {
		double speed = speed_test(SRC_SINC_FASTEST, 8);

		if (speed > 150.0) {      // fast
			progdefaults.slowcpu = false;
			progdefaults.sample_converter = SRC_SINC_BEST_QUALITY;
		}
		else if (speed > 60.0) {  // ok
			progdefaults.slowcpu = false;
			progdefaults.sample_converter = SRC_SINC_MEDIUM_QUALITY;
		}
		else if (speed > 15.0) { // slow
			progdefaults.slowcpu = true;
			progdefaults.sample_converter = SRC_SINC_FASTEST;
		}
		else {                   // recycle me
			progdefaults.slowcpu = true;
			progdefaults.sample_converter = SRC_LINEAR;
		}

		LOG_INFO("CPU speed factor=%f: setting slowcpu=%s, sample_converter=\"%s\"", speed,
			 progdefaults.slowcpu ? "true" : "false",
			 src_get_name(progdefaults.sample_converter));
	}
	if (progdefaults.XmlRigFilename.empty())
		progdefaults.XmlRigFilename = xmlfname;

#if BENCHMARK_MODE
	return setup_benchmark();
#endif

	FSEL::create();

#if FLDIGI_FLTK_API_MAJOR == 1 && FLDIGI_FLTK_API_MINOR < 3
		listbox_charset_status->hide();
#else
		listbox_charset_status->show();
#endif
	populate_charset_listbox();
	set_default_charset();
	setTabColors();

	progdefaults.testCommPorts();

#if USE_HAMLIB
	xcvr = new Rig();
#endif

	push2talk = new PTT();

	progdefaults.setDefaults();

	atexit(sound_close);
	sound_init();

	progdefaults.initInterface();
	trx_start();

#if SHOW_WIZARD_BEFORE_MAIN_WINDOW
	if (!have_config) {
		show_wizard(argc, argv);
		Fl_Window* w;
		while ((w = Fl::first_window()) && w->visible())
			Fl::wait();
	}
#endif

	dlgViewer = createViewer();
	create_logbook_dialogs();
	LOGBOOK_colors_font();

	if( progdefaults.kml_save_dir.empty() ) {
		progdefaults.kml_save_dir = KmlDir ;
	}
	kml_init(true);

// OS X will prevent the main window from being resized if we change its
// size *after* it has been shown. With some X11 window managers, OTOH,
// the main window will not be restored at its exact saved position if
// we move it *after* it has been shown.
#ifndef __APPLE__
	fl_digi_main->show(argc, argv);
	progStatus.initLastState();
#else
	progStatus.initLastState();
	fl_digi_main->show(argc, argv);
#endif

	if (iconified)
		for (Fl_Window* w = Fl::first_window(); w; w = Fl::next_window(w))
			w->iconize();
	update_main_title();

	mode_browser = new Mode_Browser;

#if !SHOW_WIZARD_BEFORE_MAIN_WINDOW
	if (!have_config)
		show_wizard();
#endif

	Fl::add_timeout(.05, delayed_startup);

	int ret = Fl::run();

	return ret;
}

void exit_process() {

	if (progdefaults.kml_enabled)
		KmlServer::Exit();

	stop_psm_thread();
	arq_close();
	FD_close();
	kiss_close(false);
	maclogger_close();
	XML_RPC_Server::stop();

	if (progdefaults.usepskrep)
		pskrep_stop();

LOG_INFO("Detach/delete qrunner threads");
	for (int i = 0; i < NUM_QRUNNER_THREADS; i++) {
LOG_INFO("thread %d", i);
		cbq[i]->detach();
		delete cbq[i];
	}
LOG_INFO("FSEL::destroy()");
	FSEL::destroy();
}

void generate_option_help(void) {
	ostringstream help;
	string disp_base_dir = BaseDir;
#ifdef __WOE32__
	size_t p = 0;
	while ((p = disp_base_dir.find("/")) != string::npos)
		disp_base_dir[p] = '\\';
#endif

	help << "Usage:\n"
		 << "    " << PACKAGE_NAME << " [option...]\n\n";

	help << PACKAGE_NAME << " options:\n\n"
#if !defined(__WOE32__)
		 << "  --home-dir DIRECTORY\n"
		 << "    Set the home directory to full pathname of DIRECTORY\n"
		 << "    fldigi will put the file stores\n"
		 << "      .fldigi.files, and .nbems.files\n"
		 << "    in this directory\n"
		 << "    The default is: " << disp_base_dir << "\n\n"

		 << "  --config-dir DIRECTORY\n"
		 << "    Look for configuration files in DIRECTORY\n"
		 << "    The default is: " << disp_base_dir << ".fldigi/\n\n"
#else
		 << "  --home-dir FOLDER\n"
		 << "    Set the home folder to full pathname of FOLDER\n"
		 << "    fldigi will put the file stores\n"
		 << "       fldigi.files, and nbems.files\n"
		 << "    in this folder\n"
		 << "    The default is: " << disp_base_dir << "\n\n"

		 << "  --config-dir FOLDER\n"
		 << "    Look for configuration files in FOLDER\n"
		 << "    The default is: " << disp_base_dir << "fldigi.files\\\n\n"
#endif

#if !defined(__WOE32__) && !defined(__APPLE__)
		 << "  --rx-ipc-key KEY\n"
		 << "    Set the receive message queue key\n"
		 << "    May be given in hex if prefixed with \"0x\"\n"
		 << "    The default is: " << progdefaults.rx_msgid
		 << " or 0x" << hex << progdefaults.rx_msgid << dec << "\n\n"

		 << "  --tx-ipc-key KEY\n"
		 << "    Set the transmit message queue key\n"
		 << "    May be given in hex if prefixed with \"0x\"\n"
		 << "    The default is: " << progdefaults.tx_msgid
		 << " or 0x" << hex << progdefaults.tx_msgid << dec << "\n\n"
#endif

		 << "  --enable-io-port <" << ARQ_IO << "|" << KISS_IO << "> ARQ=" << ARQ_IO << " KISS=" << KISS_IO << "\n"
		 << "    Select the active IO Port\n"
		 << "    The default is: " << progdefaults.data_io_enabled << "\n\n"

		 << "  --kiss-server-address HOSTNAME\n"
		 << "    Set the KISS TCP/UDP server address\n"
		 << "    The default is: " << progdefaults.kiss_address << "\n\n"
		 << "  --kiss-server-port-io I/O PORT\n"
		 << "    Set the KISS TCP/UDP server I/O port\n"
		 << "    The default is: " << progdefaults.kiss_io_port << "\n\n"
		 << "  --kiss-server-port-o Output PORT\n"
		 << "    Set the KISS UDP server output port\n"
		 << "    The default is: " << progdefaults.kiss_out_port << "\n\n"
		 << "  --kiss-server-dual-port Dual Port Use (0=disable / 1=enable)\n"
		 << "    Set the KISS UDP server dual port flag\n"
		 << "    The default is: " << progdefaults.kiss_dual_port_enabled << "\n\n"

		 << "  --arq-server-address HOSTNAME\n"
		 << "    Set the ARQ TCP server address\n"
		 << "    The default is: " << progdefaults.arq_address << "\n\n"
		 << "  --arq-server-port PORT\n"
		 << "    Set the ARQ TCP server port\n"
		 << "    The default is: " << progdefaults.arq_port << "\n\n"
		 << "  --flmsg-dir DIRECTORY\n"
		 << "    Look for flmsg files in DIRECTORY\n"
		 << "    The default is " << FLMSG_dir_default << "\n\n"
		 << "  --auto-dir DIRECTORY\n"
		 << "    Look for auto-send files in DIRECTORY\n"
		 << "    The default is " << HomeDir << "/autosend" << "\n\n"

		 << "  --xmlrpc-server-address HOSTNAME\n"
		 << "    Set the XML-RPC server address\n"
		 << "    The default is: " << progdefaults.xmlrpc_address << "\n\n"
		 << "  --xmlrpc-server-port PORT\n"
		 << "    Set the XML-RPC server port\n"
		 << "    The default is: " << progdefaults.xmlrpc_port << "\n\n"
		 << "  --xmlrpc-allow REGEX\n"
		 << "    Allow only the methods whose names match REGEX\n\n"
		 << "  --xmlrpc-deny REGEX\n"
		 << "    Allow only the methods whose names don't match REGEX\n\n"
		 << "  --xmlrpc-list\n"
		 << "    List all available methods\n\n"

#if BENCHMARK_MODE
		 << "  --benchmark-modem ID\n"
		 << "    Specify the modem\n"
		 << "    Default: " << mode_info[benchmark.modem].sname << "\n\n"
		 << "  --benchmark-frequency FREQ\n"
		 << "    Specify the modem frequency\n"
		 << "    Default: " << benchmark.freq << "\n\n"
		 << "  --benchmark-afc BOOLEAN\n"
		 << "    Set modem AFC\n"
		 << "    Default: " << benchmark.afc
		 << " (" << boolalpha << benchmark.afc << noboolalpha << ")\n\n"
		 << "  --benchmark-squelch BOOLEAN\n"
		 << "    Set modem squelch\n"
		 << "    Default: " << benchmark.sql
		 << " (" << boolalpha << benchmark.sql << noboolalpha << ")\n\n"
		 << "  --benchmark-squelch-level LEVEL\n"
		 << "    Set modem squelch level\n"
		 << "    Default: " << benchmark.sqlevel << " (%)\n\n"
		 << "  --benchmark-input INPUT\n"
		 << "    Specify the input\n"
		 << "    Must be a positive integer indicating the number of samples\n"
		"    of silence to generate as the input"
#  if USE_SNDFILE
		", or a filename containing\n"
		"    non-digit characters"
#endif
		"\n\n"

		 << "  --benchmark-output FILE\n"
		 << "    Specify the output data file\n"
		 << "    Default: decoder output is discarded\n\n"
		 << "  --benchmark-src-ratio RATIO\n"
		 << "    Specify the sample rate conversion ratio\n"
		 << "    Default: 1.0 (input is not resampled)\n\n"
		 << "  --benchmark-src-type TYPE\n"
		 << "    Specify the sample rate conversion type\n"
		 << "    Default: " << benchmark.src_type << " (" << src_get_name(benchmark.src_type) << ")\n\n"
#endif

		 << "  --cpu-speed-test\n"
		 << "    Perform the CPU speed test, show results in the event log\n"
		 << "    and possibly change options.\n\n"

		 << "  --noise\n"
		 << "    Unhide controls for noise tests\n\n"

		 << "  --wfall-only\n"
		 << "    Hide all controls but the waterfall\n\n"

		 << "  --debug-level LEVEL\n"
		 << "    Set the event log verbosity\n\n"

		 << "  --debug-pskmail\n"
		 << "    Enable logging for pskmail / arq events\n\n"

		 << "  --debug-audio\n"
		 << "    Enable logging for sound-card events\n\n"

		 << "  --version\n"
		 << "    Print version information\n\n"

		 << "  --build-info\n"
		 << "    Print build information\n\n"

		 << "  --help\n"
		 << "    Print this option help\n\n";

// Fl::help looks ugly so we'll write our own

	help << "Standard FLTK options:\n\n"

		 << "   -bg COLOR, -background COLOR\n"
		 << "    Set the background color\n"

		 << "   -bg2 COLOR, -background2 COLOR\n"
		 << "    Set the secondary (text) background color\n\n"

		 << "   -di DISPLAY, -display DISPLAY\n"
		 << "    Set the X display to use DISPLAY,\n"
		 << "    format is ``host:n.n''\n\n"

		 << "   -dn, -dnd or -nodn, -nodnd\n"
		 << "    Enable or disable drag and drop copy and paste in text fields\n\n"

		 << "   -fg COLOR, -foreground COLOR\n"
		 << "    Set the foreground color\n\n"

		 << "   -g GEOMETRY, -geometry GEOMETRY\n"
		 << "    Set the initial window size and position\n"
		 << "    GEOMETRY format is ``WxH+X+Y''\n"
		 << "    ** " << PACKAGE_NAME << " may override this setting **\n\n"

		 << "   -i, -iconic\n"
		 << "    Start " << PACKAGE_NAME << " in iconified state\n\n"

		 << "   -k, -kbd or -nok, -nokbd\n"
		 << "    Enable or disable visible keyboard focus in non-text widgets\n\n"

		 << "   -na CLASSNAME, -name CLASSNAME\n"
		 << "    Set the window class to CLASSNAME\n\n"

		 << "   -ti WINDOWTITLE, -title WINDOWTITLE\n"
		 << "    Set the window title\n\n";

	help << "Additional UI options:\n\n"

		 << "  --font FONT[:SIZE]\n"
		 << "    Set the widget font and (optionally) size\n"
		 << "    The default is: " << Fl::get_font(FL_HELVETICA)
		 << ':' << FL_NORMAL_SIZE << "\n\n"

		;

	option_help = help.str();
}

void exit_cb(void*) { fl_digi_main->do_callback(); }

int parse_args(int argc, char **argv, int& idx)
{
	// Only handle long options
	if (!(strlen(argv[idx]) >= 2 && strncmp(argv[idx], "--", 2) == 0)) {
		// Store the window title. We may need this early in the initialisation
		// process, before FLTK uses it to set the main window title.
		if (argv_window_title.empty() && argc > idx &&
		    (!strcmp(argv[idx], "-ti") || !strcmp(argv[idx], "-title")))
			argv_window_title = argv[idx + 1];
		else if (!strcmp(argv[idx], "-i") || !strcmp(argv[idx], "-iconic"))
			iconified = true;
		return 0;
	}

		enum { OPT_ZERO,
#ifndef __WOE32__
		   OPT_RX_IPC_KEY, OPT_TX_IPC_KEY,
#endif
		   OPT_HOME_DIR,
		   OPT_CONFIG_DIR,
		   OPT_ARQ_ADDRESS, OPT_ARQ_PORT,
		   OPT_SHOW_CPU_CHECK,
		   OPT_FLMSG_DIR,
		   OPT_AUTOSEND_DIR,

		   OPT_CONFIG_XMLRPC_ADDRESS, OPT_CONFIG_XMLRPC_PORT,
		   OPT_CONFIG_XMLRPC_ALLOW, OPT_CONFIG_XMLRPC_DENY, OPT_CONFIG_XMLRPC_LIST,
		   OPT_CONFIG_KISS_ADDRESS, OPT_CONFIG_KISS_PORT_IO, OPT_CONFIG_KISS_PORT_O,
		   OPT_CONFIG_KISS_DUAL_PORT, OPT_ENABLE_IO_PORT,

#if BENCHMARK_MODE
		   OPT_BENCHMARK_MODEM, OPT_BENCHMARK_AFC, OPT_BENCHMARK_SQL, OPT_BENCHMARK_SQLEVEL,
		   OPT_BENCHMARK_FREQ, OPT_BENCHMARK_INPUT, OPT_BENCHMARK_OUTPUT,
		   OPT_BENCHMARK_SRC_RATIO, OPT_BENCHMARK_SRC_TYPE,
#endif

			   OPT_FONT, OPT_WFALL_HEIGHT,
			   OPT_WINDOW_WIDTH, OPT_WINDOW_HEIGHT, OPT_WFALL_ONLY,
			   OPT_RX_ONLY,
#if USE_PORTAUDIO
			   OPT_FRAMES_PER_BUFFER,
#endif
		   OPT_MORE_INFO,
		   OPT_NOISE, OPT_DEBUG_LEVEL, OPT_DEBUG_PSKMAIL, OPT_DEBUG_AUDIO,
			   OPT_EXIT_AFTER,
			   OPT_DEPRECATED, OPT_HELP, OPT_VERSION, OPT_BUILD_INFO };

	static const char shortopts[] = ":";
	static const struct option longopts[] = {
#ifndef __WOE32__
		{ "rx-ipc-key",	   1, 0, OPT_RX_IPC_KEY },
		{ "tx-ipc-key",	   1, 0, OPT_TX_IPC_KEY },
#endif
		{ "home-dir",	   1, 0, OPT_HOME_DIR },
		{ "config-dir",	   1, 0, OPT_CONFIG_DIR },

		{ "arq-server-address", 1, 0, OPT_ARQ_ADDRESS },
		{ "arq-server-port",    1, 0, OPT_ARQ_PORT },
		{ "flmsg-dir", 1, 0, OPT_FLMSG_DIR },
		{ "auto-dir", 1, 0, OPT_AUTOSEND_DIR },

		{ "cpu-speed-test", 0, 0, OPT_SHOW_CPU_CHECK },

		{ "enable-io-port",   1, 0, OPT_ENABLE_IO_PORT },

		{ "kiss-server-address",   1, 0, OPT_CONFIG_KISS_ADDRESS },
		{ "kiss-server-port-io",   1, 0, OPT_CONFIG_KISS_PORT_IO },
		{ "kiss-server-port-o",    1, 0, OPT_CONFIG_KISS_PORT_O },
		{ "kiss-server-dual-port", 1, 0, OPT_CONFIG_KISS_DUAL_PORT },

		{ "xmlrpc-server-address", 1, 0, OPT_CONFIG_XMLRPC_ADDRESS },
		{ "xmlrpc-server-port",    1, 0, OPT_CONFIG_XMLRPC_PORT },
		{ "xmlrpc-allow",          1, 0, OPT_CONFIG_XMLRPC_ALLOW },
		{ "xmlrpc-deny",           1, 0, OPT_CONFIG_XMLRPC_DENY },
		{ "xmlrpc-list",           0, 0, OPT_CONFIG_XMLRPC_LIST },

#if BENCHMARK_MODE
		{ "benchmark-modem", 1, 0, OPT_BENCHMARK_MODEM },
		{ "benchmark-frequency", 1, 0, OPT_BENCHMARK_FREQ },
		{ "benchmark-afc", 1, 0, OPT_BENCHMARK_AFC },
		{ "benchmark-squelch", 1, 0, OPT_BENCHMARK_SQL },
		{ "benchmark-squelch-level", 1, 0, OPT_BENCHMARK_SQLEVEL },
		{ "benchmark-input", 1, 0, OPT_BENCHMARK_INPUT },
		{ "benchmark-output", 1, 0, OPT_BENCHMARK_OUTPUT },
		{ "benchmark-src-ratio", 1, 0, OPT_BENCHMARK_SRC_RATIO },
		{ "benchmark-src-type", 1, 0, OPT_BENCHMARK_SRC_TYPE },
#endif

		{ "font",	   1, 0, OPT_FONT },

		{ "wfall-height",  1, 0, OPT_WFALL_HEIGHT },
		{ "window-width",  1, 0, OPT_WINDOW_WIDTH },
		{ "window-height", 1, 0, OPT_WINDOW_HEIGHT },
		{ "wfall-only",    0, 0, OPT_WFALL_ONLY },
		{ "wo",            0, 0, OPT_WFALL_ONLY },
		{ "rx-only",       0, 0, OPT_RX_ONLY },
		{ "ro",            0, 0, OPT_RX_ONLY },

#if USE_PORTAUDIO
		{ "frames-per-buffer",1, 0, OPT_FRAMES_PER_BUFFER },
#endif
		{ "more-info",     1, 0, OPT_MORE_INFO },
		{ "exit-after",    1, 0, OPT_EXIT_AFTER },

		{ "noise", 0, 0, OPT_NOISE },
		{ "debug-level",   1, 0, OPT_DEBUG_LEVEL },
		{ "debug-pskmail", 0, 0, OPT_DEBUG_PSKMAIL },
		{ "debug-audio", 0, 0, OPT_DEBUG_AUDIO },

		{ "help",	   0, 0, OPT_HELP },
		{ "version",	   0, 0, OPT_VERSION },
		{ "build-info",	   0, 0, OPT_BUILD_INFO },
		{ 0 }
	};

	int longindex;
	optind = idx;
	opterr = 0;
	int c = getopt_long(argc, argv, shortopts, longopts, &longindex);

	switch (c) {
		case -1:
			return 0;
		case 0:
			// handle options with non-0 flag here
			return 0;

#if !defined(__WOE32__) && !defined(__APPLE__)
		case OPT_RX_IPC_KEY: case OPT_TX_IPC_KEY:
		{
			errno = 0;
			int key = strtol(optarg, NULL, (strncasecmp(optarg, "0x", 2) ? 10 : 16));
			if (errno || key <= 0)
				cerr << "Hmm, " << key << " doesn't look like a valid IPC key\n";
			if (c == OPT_RX_IPC_KEY)
				progdefaults.rx_msgid = key;
			else
				progdefaults.tx_msgid = key;
		}
			break;
#endif

		case OPT_HOME_DIR: {
			char buf[FL_PATH_MAX + 1];
			fl_filename_absolute(buf, sizeof(buf) - 1, optarg);
			BaseDir = buf;
		}
			if (*BaseDir.rbegin() != '/')
				   BaseDir += '/';
			break;

		case OPT_CONFIG_DIR: {
			char buf[FL_PATH_MAX + 1];
			fl_filename_absolute(buf, sizeof(buf) - 1, optarg);
			HomeDir = buf;
		}
			if (*HomeDir.rbegin() != '/')
				   HomeDir += '/';
			break;

		case OPT_ARQ_ADDRESS:
			override_arq_address = optarg;
			arq_address_override_flag = true;
			break;
		case OPT_ARQ_PORT:
			override_arq_port = optarg;
			arq_address_override_flag = true;
			break;

		case OPT_FLMSG_DIR:
			FLMSG_dir_default = optarg;
			break;

		case OPT_AUTOSEND_DIR:
			FLMSG_WRAP_auto_dir = optarg;
			break;

		case OPT_ENABLE_IO_PORT:
			if(optarg) {
				switch(atoi(optarg)) {
					case ARQ_IO:
						progdefaults.data_io_enabled = ARQ_IO;
						override_data_io_enabled = ARQ_IO;
						arq_address_override_flag = true;
						break;

					case KISS_IO:
						progdefaults.data_io_enabled = KISS_IO;
						override_data_io_enabled = KISS_IO;
						kiss_address_override_flag = true;
						break;
				}
			}
			break;

		case OPT_CONFIG_KISS_ADDRESS:
			progdefaults.kiss_address = optarg;
			override_kiss_address = optarg;
			kiss_address_override_flag = true;
			break;
		case OPT_CONFIG_KISS_PORT_IO:
			progdefaults.kiss_io_port = optarg;
			override_kiss_io_port = optarg;
			kiss_address_override_flag = true;
			break;
		case OPT_CONFIG_KISS_PORT_O:
			progdefaults.kiss_out_port = optarg;
			override_kiss_out_port = optarg;
			kiss_address_override_flag = true;
			break;
		case OPT_CONFIG_KISS_DUAL_PORT:
			if((optarg) && atoi(optarg)) {
				progdefaults.kiss_dual_port_enabled = true;
				override_kiss_dual_port_enabled = true;
				kiss_address_override_flag = true;
			} else {
				progdefaults.kiss_dual_port_enabled = false;
				override_kiss_dual_port_enabled = false;
				kiss_address_override_flag = true;
			}
			break;

		case OPT_CONFIG_XMLRPC_ADDRESS:
			override_xmlrpc_address = optarg;
			xmlrpc_address_override_flag = true;
			break;
		case OPT_CONFIG_XMLRPC_PORT:
			override_xmlrpc_port = optarg;
			xmlrpc_port_override_flag = true;
			break;
		case OPT_CONFIG_XMLRPC_ALLOW:
			progdefaults.xmlrpc_allow = optarg;
			break;
		case OPT_CONFIG_XMLRPC_DENY:
			if (!progdefaults.xmlrpc_allow.empty())
				cerr << "W: --" << longopts[longindex].name
					 << " cannot be used together with --"
					 << longopts[OPT_CONFIG_XMLRPC_ALLOW-1].name
					 << " and will be ignored\n";
			else
				progdefaults.xmlrpc_deny = optarg;
			break;
		case OPT_CONFIG_XMLRPC_LIST:
			XML_RPC_Server::list_methods(cout);
			exit(EXIT_SUCCESS);

#if BENCHMARK_MODE
		case OPT_BENCHMARK_MODEM:
			benchmark.modem = strtol(optarg, NULL, 10);
			if (!(benchmark.modem >= 0 && benchmark.modem < NUM_MODES)) {
				fatal_error(_("Bad modem id"));
			}
			break;

		case OPT_BENCHMARK_FREQ:
			benchmark.freq = strtol(optarg, NULL, 10);
			if (benchmark.freq < 0) {
				fatal_error(_("Bad frequency"));
			}
			break;

		case OPT_BENCHMARK_AFC:
			benchmark.afc = strtol(optarg, NULL, 10);
			break;

		case OPT_BENCHMARK_SQL:
			benchmark.sql = strtol(optarg, NULL, 10);
			break;

		case OPT_BENCHMARK_SQLEVEL:
			benchmark.sqlevel = strtod(optarg, NULL);
			break;

		case OPT_BENCHMARK_INPUT:
			benchmark.input = optarg;
			break;

		case OPT_BENCHMARK_OUTPUT:
			benchmark.output = optarg;
			break;

		case OPT_BENCHMARK_SRC_RATIO:
			benchmark.src_ratio = strtod(optarg, NULL);
			break;

		case OPT_BENCHMARK_SRC_TYPE:
			benchmark.src_type = strtol(optarg, NULL, 10);
			break;
#endif

		case OPT_FONT:
		{
			char *p;
			if ((p = strchr(optarg, ':'))) {
				*p = '\0';
				FL_NORMAL_SIZE = strtol(p + 1, 0, 10);
			}
		}
			Fl::set_font(FL_HELVETICA, optarg);
			break;

//		case OPT_WFALL_HEIGHT:
//			progdefaults.wfheight = strtol(optarg, NULL, 10);
//			break;

//		case OPT_WINDOW_WIDTH:
//			WNOM = strtol(optarg, NULL, 10);
//			break;

//		case OPT_WINDOW_HEIGHT:
//			HNOM = strtol(optarg, NULL, 10);
//			break;

#if USE_PORTAUDIO
		case OPT_FRAMES_PER_BUFFER:
			progdefaults.PortFramesPerBuffer = strtol(optarg, 0, 10);
			break;
#endif // USE_PORTAUDIO

		case OPT_MORE_INFO:
			bMOREINFO = true;
			break;

		case OPT_EXIT_AFTER:
			Fl::add_timeout(strtod(optarg, 0), exit_cb);
			break;

		case OPT_WFALL_ONLY:
			bWF_only = true;
			break;

		case OPT_RX_ONLY:
			rx_only = true;
			break;

		case OPT_NOISE:
			withnoise = true;
			break;

		case OPT_SHOW_CPU_CHECK:
			show_cpucheck = true;
			break;

		case OPT_DEBUG_LEVEL:
		{
			int v = strtol(optarg, 0, 10);
			debug::level = (debug::level_e)CLAMP(v, 0, debug::LOG_NLEVELS-1);
		}
			break;

		case OPT_DEBUG_PSKMAIL:
			debug_pskmail = true;
			break;

		case OPT_DEBUG_AUDIO:
			debug_audio = true;
			break;

		case OPT_DEPRECATED:
			cerr << "W: the --" << longopts[longindex].name
				 << " option has been deprecated and will be removed in a future version\n";
			break;

		case OPT_HELP:
			cout << option_help;
			exit(EXIT_SUCCESS);

		case OPT_VERSION:
			cout << version_text;
			exit(EXIT_SUCCESS);

		case OPT_BUILD_INFO:
			cout << build_text;
			exit(EXIT_SUCCESS);

		case '?': case ':': default:
			arg_error(argv[0], argv[idx], (c == ':'));
	}

	// Increment idx by the number of args we used and return that number.
	// We must check whether the option argument is in the same argv element
	// as the option name itself, i.e., --opt=arg.
		c = longopts[longindex].has_arg ? 2 : 1;
		if (c == 2) {
				string arg = argv[idx];
				string::size_type p;
				if ((p = arg.rfind(optarg)) != string::npos && arg[p-1] == '=')
						c = 1;
		}
	idx += c;
	return c;
}

void generate_version_text(void)
{
	version_text.assign(PACKAGE_STRING "\nCopyright (C) 2007-2010 " PACKAGE_AUTHORS ".\n");
	version_text.append(_("License GPLv3+: GNU GPL version 3 or later "
				  "<http://www.gnu.org/licenses/gpl-3.0.html>\n"
				  "This is free software: you are free to change and redistribute it.\n"
				  "There is NO WARRANTY, to the extent permitted by law.\n"));

	ostringstream s;
	s << "Build information:\n";
	s << "  built          : " << BUILD_DATE << " by " << BUILD_USER
	  << '@' << BUILD_HOST << " on " << BUILD_BUILD_PLATFORM
	  << " for " << BUILD_TARGET_PLATFORM << "\n\n"
	  << "  configure flags: " << BUILD_CONFIGURE_ARGS << "\n\n"
	  << "  compiler       : " << BUILD_COMPILER << "\n\n"
	  << "  compiler flags : " << FLDIGI_BUILD_CXXFLAGS << "\n\n"
	  << "  linker flags   : " << FLDIGI_BUILD_LDFLAGS << "\n\n"

	  << "  libraries      : " "FLTK " FLTK_BUILD_VERSION "\n"
	  << "                   " "libsamplerate " << SAMPLERATE_BUILD_VERSION "\n";
#if USE_SNDFILE
	s << "                   " "libsndfile " << SNDFILE_BUILD_VERSION "\n";
#endif
#if USE_PORTAUDIO
	s << "                   " "PortAudio " << PORTAUDIO_BUILD_VERSION "\n";
#endif
#if USE_PULSEAUDIO
	s << "                   " "PulseAudio " << PULSEAUDIO_BUILD_VERSION "\n";
#endif
#if USE_HAMLIB
	s << "                   " "Hamlib " << HAMLIB_BUILD_VERSION "\n";
#endif

	s << "\nRuntime information:\n";
		struct utsname u;
		if (uname(&u) != -1) {
		s << "  system         : " << u.sysname << ' ' << u.nodename
		  << ' ' << u.release << ' ' << u.version << ' ' << u.machine << "\n\n";
	}

	s << "  libraries      : " << src_get_version() << '\n';
#if USE_SNDFILE
	char sndfile_version[32];
	sf_command(NULL, SFC_GET_LIB_VERSION, sndfile_version, sizeof(sndfile_version));
	s << "                   " << sndfile_version << '\n';
#endif
#if USE_PORTAUDIO
	s << "                   " << Pa_GetVersionText() << ' ' << Pa_GetVersion() << '\n';
#endif
#if USE_PULSEAUDIO
	s << "                   " << "Pulseaudio " << pa_get_library_version() << '\n';
#endif
#if USE_HAMLIB
	s << "                   " << hamlib_version << '\n';
#endif

	build_text = s.str();
}

// When debugging is enabled, reexec with malloc debugging hooks enabled, unless
// the env var FLDIGI_NO_EXEC is set, or our parent process is gdb.
void debug_exec(char** argv)
{
#if !defined(NDEBUG) && defined(__GLIBC__)
		if (getenv("FLDIGI_NO_EXEC"))
				return;

	char ppath[32], lname[32];
	ssize_t n;
	snprintf(ppath, sizeof(ppath), "/proc/%u/exe", getppid());
	if ((n = readlink(ppath, lname, sizeof(lname))) > 0) {
		lname[n] = '\0';
		if (strstr(lname, "gdb")) {
						cerr << "Not using malloc debugging hooks\n";
						return;
				}
	}

		setenv("FLDIGI_NO_EXEC", "1", 0);
		setenv("MALLOC_CHECK_", "3", 0);
		setenv("MALLOC_PERTURB_", "42", 0);
		if (execvp(*argv, argv) == -1)
				perror("execvp");
#endif
}

void set_platform_ui(void)
{
#if defined(__APPLE__)
	   FL_NORMAL_SIZE = 12;
	   progdefaults.WaterfallFontsize = 12;
	   progdefaults.RxFontsize = 12;
	   progdefaults.TxFontsize = 12;
#elif defined(__WOE32__)
	   Fl::set_font(FL_HELVETICA, "Tahoma");
	   FL_NORMAL_SIZE = 11;
	   progdefaults.WaterfallFontnbr = FL_HELVETICA;
	   progdefaults.WaterfallFontsize = 12;
	   progdefaults.RxFontsize = 12;
	   progdefaults.TxFontsize = 12;
#else
	   FL_NORMAL_SIZE = 12;
#endif
}

// Convert 1 second of 1-channel silence from IN_RATE Hz to OUT_RATE Hz,
// Repeat test "repeat" times. Return (repeat / elapsed_time),
// the faster-than-realtime factor averaged over "repeat" runs.
// Some figures for SRC_SINC_FASTEST:
// Pentium 4 2.8GHz:     70
// Pentium 3 550MHz:     13
// UltraSparc II 270MHz: 3.5
// Atom N280 1.66GHz:    17.7
#define IN_RATE 48000
#define OUT_RATE 8000
double speed_test(int converter, unsigned repeat)
{
	SRC_DATA src;
	src.src_ratio = (double)OUT_RATE / IN_RATE;
	src.input_frames = IN_RATE;
	src.output_frames = OUT_RATE;
	src.data_in = new float[src.input_frames];
	src.data_out = new float[src.output_frames];

	memset(src.data_in, 0, src.input_frames * sizeof(float));

	// warm up
	src_simple(&src, converter, 1);

	struct timespec t0, t1;
#ifdef _POSIX_MONOTONIC_CLOCK
	clock_gettime(CLOCK_MONOTONIC, &t0);
#else
	clock_gettime(CLOCK_REALTIME, &t0);
#endif
	for (unsigned i = 0; i < repeat; i++)
		src_simple(&src, converter, 1);
#ifdef _POSIX_MONOTONIC_CLOCK
	clock_gettime(CLOCK_MONOTONIC, &t1);
#else
	clock_gettime(CLOCK_REALTIME, &t1);
#endif

	delete [] src.data_in;
	delete [] src.data_out;

	t0 = t1 - t0;
	return repeat / (t0.tv_sec + t0.tv_nsec/1e9);
}

static void setup_signal_handlers(void)
{
#ifndef __WOE32__
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));

	// no child stopped notifications, no zombies
	action.sa_handler = SIG_DFL;
	action.sa_flags = SA_NOCLDSTOP;
#ifdef SA_NOCLDWAIT
	action.sa_flags |= SA_NOCLDWAIT;
#endif
	sigaction(SIGCHLD, &action, NULL);
	action.sa_flags = 0;

	action.sa_handler = handle_signal;
	sigaction(SIGSEGV, &action, NULL);
	sigaction(SIGILL, &action, NULL);
	sigaction(SIGABRT, &action, NULL);
	sigaction(SIGUSR2, &action, NULL);

	action.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &action, NULL);

	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGUSR2);
	pthread_sigmask(SIG_BLOCK, &action.sa_mask, NULL);
#else
	signal(SIGSEGV, handle_signal);
	signal(SIGILL, handle_signal);
	signal(SIGABRT, handle_signal);
#endif
}

// Show an error dialog and print to cerr if available.
// On win32 Fl::fatal displays its own error window.
static void fatal_error(string sz_error)
{
	string s = "Fatal error!\n";
	s.append(sz_error).append("\n").append(strerror(errno));

// Win32 will display a MessageBox error message
#if !defined(__WOE32__)
	fl_message_font(FL_HELVETICA, FL_NORMAL_SIZE);
	fl_alert2("%s", s.c_str());
#endif
	Fl::fatal(s.c_str());
}

static void checkdirectories(void)
{
	struct DIRS {
		string& dir;
		const char* suffix;
		void (*new_dir_func)(void);
	};
	DIRS fldigi_dirs[] = {
		{ HomeDir, 0, 0 },
		{ RigsDir, "rigs", 0 },
		{ ScriptsDir, "scripts", 0 },
		{ PalettesDir, "palettes", create_new_palettes },
		{ LogsDir, "logs", 0 },
		{ PicsDir, "images", 0 },
		{ AvatarDir, "avatars", 0},
		{ HelpDir, "help", 0 },
		{ MacrosDir, "macros", create_new_macros },
		{ WrapDir, "wrap", 0 },
		{ TalkDir, "talk", 0 },
		{ TempDir, "temp", 0 },
		{ KmlDir, "kml", 0 },
		{ DATA_dir, "data", 0 },
	};

	int r;
	for (size_t i = 0; i < sizeof(fldigi_dirs)/sizeof(*fldigi_dirs); i++) {
		if (fldigi_dirs[i].suffix)
			fldigi_dirs[i].dir.assign(HomeDir).append(fldigi_dirs[i].suffix).append(PATH_SEP);
		r  = mkdir(fldigi_dirs[i].dir.c_str(), 0777);
		if (r == -1 && errno != EEXIST) {
			string s = _("Could not make directory ");
			s.append(fldigi_dirs[i].dir);
			fatal_error(s);
		}
		else if (r == 0 && fldigi_dirs[i].new_dir_func)
			fldigi_dirs[i].new_dir_func();
	}

}

bool nbems_dirs_checked = false;

void check_nbems_dirs(void)
{
	if (nbems_dirs_checked) return;

	struct DIRS {
		string& dir;
		const char* suffix;
		void (*new_dir_func)(void);
	};
	DIRS NBEMS_dirs[] = {
		{ NBEMS_dir,     0, 0 },
		{ ARQ_dir,       "ARQ", 0 },
		{ ARQ_files_dir, "ARQ/files", 0 },
		{ ARQ_recv_dir,  "ARQ/recv", 0 },
		{ ARQ_send,      "ARQ/send", 0 },
		{ WRAP_dir,      "WRAP", 0 },
		{ WRAP_recv_dir, "WRAP/recv", 0 },
		{ WRAP_send_dir, "WRAP/send", 0 },
		{ WRAP_auto_dir, "WRAP/auto", 0 },
		{ ICS_dir,       "ICS", 0 },
		{ ICS_msg_dir,   "ICS/messages", 0 },
		{ ICS_tmp_dir,   "ICS/templates", 0 },
	};

	int r;
	for (size_t i = 0; i < sizeof(NBEMS_dirs)/sizeof(*NBEMS_dirs); i++) {
		if (NBEMS_dirs[i].suffix)
			NBEMS_dirs[i].dir.assign(NBEMS_dir).append(NBEMS_dirs[i].suffix).append(PATH_SEP);

		if ((r = mkdir(NBEMS_dirs[i].dir.c_str(), 0777)) == -1 && errno != EEXIST) {
			string s = _("Could not make directory ");
			s.append(NBEMS_dirs[i].dir).append(", ").append(strerror(errno));
			fatal_error(s);
		}
		else if (r == 0 && NBEMS_dirs[i].new_dir_func)
			NBEMS_dirs[i].new_dir_func();
	}

	DIRS FLMSG_dirs[] = {
		{ FLMSG_dir,               0, 0 },
		{ FLMSG_WRAP_dir,          "WRAP", 0 },
		{ FLMSG_WRAP_recv_dir,     "WRAP/recv", 0 },
		{ FLMSG_WRAP_send_dir,     "WRAP/send", 0 },
		{ FLMSG_WRAP_auto_dir,     "WRAP/auto", 0 },
		{ FLMSG_ICS_dir,           "ICS", 0 },
		{ FLMSG_ICS_msg_dir,       "ICS/messages", 0 },
		{ FLMSG_ICS_tmp_dir,       "ICS/templates", 0 },
	};

	for (size_t i = 0; i < sizeof(FLMSG_dirs)/sizeof(*FLMSG_dirs); i++) {
		if (FLMSG_dirs[i].dir.empty() && FLMSG_dirs[i].suffix)
			FLMSG_dirs[i].dir.assign(FLMSG_dir).append(FLMSG_dirs[i].suffix).append("/");

		if ((r = mkdir(FLMSG_dirs[i].dir.c_str(), 0777)) == -1 && errno != EEXIST) {
			string s = _("Could not make directory ");
			s.append(FLMSG_dirs[i].dir);
			fatal_error(s);
		}
		else if (r == 0 && FLMSG_dirs[i].new_dir_func)
			FLMSG_dirs[i].new_dir_func();
	}

	nbems_dirs_checked = true;
}

void check_data_dir(void)
{
	if (mkdir(DATA_dir.c_str(), 0777) == -1 && errno != EEXIST) {
		string s = _("Could not make directory ");
		s.append(DATA_dir);
		fatal_error(s);
	}
}

// Print an error message and exit.
static void arg_error(const char* name, const char* arg, bool missing)
{
	ostringstream msg;
	msg << name << ": ";
	if (arg && *arg) {
		if (missing)
			msg << "option '" << arg << "' requires an argument\n";
		else
			msg << "unrecognized option '" << arg << "'\n";
	}
	else
		msg << "error while parsing command line\n";

	msg << "See command line help for more information.";

	fatal_error(msg.str());
}

/// Sets or resets the KML parameters, and loads existing files.
void kml_init(bool load_files)
{
	if (progdefaults.kml_enabled == false) return; // disabled kml service

	KmlServer::GetInstance()->InitParams(
			progdefaults.kml_command,
			progdefaults.kml_save_dir,
			(double)progdefaults.kml_merge_distance,
			progdefaults.kml_retention_time,
			progdefaults.kml_refresh_interval,
			progdefaults.kml_balloon_style);

	if(load_files) {
		KmlServer::GetInstance()->ReloadKmlFiles();
	}

	/// TODO: Should do this only when the locator has changed.
	try {
		/// One special KML object for the user.
		CoordinateT::Pair myCoo( progdefaults.myLocator );

		/// TODO: Fix this: It does not seem to create a polyline when changing the locator.
		KmlServer::CustomDataT custData ;
		custData.Push( "QTH", progdefaults.myQth );
		custData.Push( "Locator", progdefaults.myLocator );
		custData.Push( "Antenna", progdefaults.myAntenna );
		custData.Push( "Name", progdefaults.myName );

		KmlServer::GetInstance()->Broadcast(
			"User",
			KmlServer::UniqueEvent,
			myCoo,
			0.0, // Altitude.
			progdefaults.myCall,
			progdefaults.myLocator,
			progdefaults.myQth,
			custData );
	}
	catch( const std::exception & exc ) {

;//		LOG_WARN("Cannot publish user position:%s", exc.what() );
	}
}

/// Tests if a directory exists.
int directory_is_created( const char * strdir )
{
	DIR *dir = opendir(strdir);
	if (dir) {
		closedir(dir);
		return true;
	}
	return false;
}

