//
// Digital Modem Program for the Fast Light Toolkit
//
// Copyright W1HKJ, Dave Freese 2006
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "w1hkj@w1hkj.com".
//

#include <config.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <getopt.h>
#include <sys/types.h>
#ifndef __CYGWIN__
#  include <sys/ipc.h>
#  include <sys/msg.h>
#endif
#include <sys/utsname.h>
#include <unistd.h>
#include <dirent.h>
#include <exception>
#include <signal.h>
#include <locale.h>

#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/x.H>

#include "main.h"
#include "waterfall.h"
#include "fft.h"
#include "soundconf.h"
#include "complex.h"
#include "fl_digi.h"
#include "rigio.h"
#include "globals.h"
#include "psk.h"
#include "cw.h"
#include "mfsk.h"
#include "confdialog.h"
#include "configuration.h"
#include "macros.h"
#include "status.h"

#if USE_HAMLIB
	#include "rigclass.h"
#endif
#include "rigsupport.h"

#include "log.h"

#include "qrunner.h"
#include "stacktrace.h"

using namespace std;

string scDevice[2];
int pa_frames_per_buffer = 0;

char szHomedir[120] = "";
char szPskMailDir[120] = "";
string PskMailDir;
string PskMailFile;
string ArqFilename;
string HomeDir;
string xmlfname;

bool gmfskmail = false;

PTT		*push2talk = (PTT *)0;
#if USE_HAMLIB
Rig		*xcvr = (Rig *)0;
#endif

cLogfile	*logfile = 0;;

cLogfile	*Maillogfile = (cLogfile *)0;
FILE	*server;
FILE	*client;
bool	mailserver = false, mailclient = false, arqmode = false;
extern	void start_pskmail();

RXMSGSTRUC rxmsgst;
int		rxmsgid = -1;

TXMSGSTRUC txmsgst;
int txmsgid = -1;

string option_help, version_text;

qrunner *cbq[NUM_QRUNNER_THREADS];

void arqchecks(void);
void generate_option_help(void);
int parse_args(int argc, char **argv, int& idx);
void generate_version_text(void);
void debug_exec(char** argv);
void string_wrap(std::string& s, unsigned c);

#ifdef __CYGWIN__
void redirect_streams(const std::string& dir);
void restore_streams(void);
#endif

int main(int argc, char ** argv)
{
	debug_exec(argv);
	CREATE_THREAD_ID(); // only call this once
	SET_THREAD_ID(FLMAIN_TID);

	for (int i = 0; i < NUM_QRUNNER_THREADS; i++) {
		cbq[i] = new qrunner(1);
		cbq[i]->attach();
	}

	set_unexpected(handle_unexpected);
	set_terminate(diediedie);
	signal(SIGSEGV, handle_signal);
	signal(SIGILL, handle_signal);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);

	setlocale(LC_TIME, "");

#ifndef __CYGWIN__
	fl_filename_expand(szHomedir, 119, "$HOME/.fldigi/");
#else
	fl_filename_expand(szHomedir, 119, "$APPDATA/fldigi/");
#endif
	HomeDir = szHomedir;

#ifdef __CYGWIN__
	redirect_streams(HomeDir);
	atexit(restore_streams);
#endif

	generate_option_help();
	generate_version_text();
	string_wrap(version_text, 80);
	int arg_idx;
	if (Fl::args(argc, argv, arg_idx, parse_args) != argc) {
	    cerr << PACKAGE_NAME << ": unrecognized option `" << argv[arg_idx]
		 << "'\nTry `" << PACKAGE_NAME
		 << " --help' for more information.\n";
	    exit(EXIT_FAILURE);
	}

	{
		DIR *dir = opendir(HomeDir.c_str());
		if (dir == 0) {
			if ( mkdir(HomeDir.c_str(), 0777) == -1) {
				cerr << "Could not make directory " << HomeDir << ": "
				     << strerror(errno) << endl;
				exit(EXIT_FAILURE);
			}
		} else
			closedir(dir);
	}

	xmlfname = HomeDir; 
	xmlfname.append("rig.xml");
	
	string lfname = HomeDir;
	lfname.append("fldigi.log");
	logfile = new cLogfile(lfname);
	logfile->log_to_file_start();

#ifndef __CYGWIN__
   	txmsgid = msgget( (key_t) progdefaults.tx_msgid, 0666 );
#else
	txmsgid = -1;
#endif
	fl_filename_expand(szPskMailDir, 119, "$HOME/pskmail.files/");
	PskMailDir = szPskMailDir;

	FL_LOCK_E();  // start the gui thread!!	
	Fl::visual(FL_RGB); // insure 24 bit color operation
	
	fl_register_images();
	Fl::set_fonts(0);
	
	rigcontrol = createRigDialog();
	progdefaults.readDefaultsXML();
	progdefaults.testCommPorts();
	
	progStatus.loadLastState();
	
	create_fl_digi_main();

	createConfig();
	inpTTYdev->tooltip(progdefaults.strCommPorts.c_str());
	inpRIGdev->tooltip(progdefaults.strCommPorts.c_str());
	
	macros.loadDefault();

#if USE_HAMLIB
	xcvr = new Rig();
#endif
	
	push2talk = new PTT();

	progdefaults.setDefaults();

	atexit(sound_close);
	sound_init();
	trx_start();

	progdefaults.initInterface();
	
#ifdef __CYGWIN__
	fl_digi_main->icon((char*)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON)));
#endif

	fl_digi_main->show(argc, argv);
	progStatus.initLastState();
	
	Fl::add_timeout(1.0, pskmail_loop);

	int ret = Fl::run();
	for (int i = 0; i < NUM_QRUNNER_THREADS; i++)
		cbq[i]->detach();

	return ret;
}


void generate_option_help(void) {
	// is there a better way of enumerating schemes?
	string schemes = "none";
	const char *possible_schemes[] = { "plastic", "gtk+", 0 };
	const char *old = Fl::scheme();
	const char **s = possible_schemes;
	while (*s) {
		Fl::scheme(*s);
		if (strcasecmp(*s, Fl::scheme()) == 0)
			schemes.append(" ").append(*s);
		s++;
	}
	Fl::scheme(old ? old : "none");


	ostringstream help;
	help << "Usage:\n"
	     << "    " << PACKAGE_NAME << " [option...]\n\n";

	help << PACKAGE_NAME << " options:\n\n"
	     << "  --config-dir DIRECTORY\n"
	     << "    Look for configuration files in DIRECTORY\n"
	     << "    The default is: " << HomeDir << "\n\n"

#ifndef __CYGWIN__
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

	     << "  --resample CONVERTER\n"
	     << "    Set the resampling method\n"
	     << "    CONVERTER can be of `src-sinc-best-quality',\n"
	     << "                        `src-sinc-medium-quality',\n"
		 << "                        `src-sinc-fastest',\n"
		 << "                        `src-zero-order-hold', or\n"
		 << "                        `src-linear'\n"
		 << "    The default is `src-sinc-fastest'\n\n"

	     << "  --version\n"
	     << "    Print version information\n\n"

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
	     << "    ** " << PACKAGE_NAME << " may override this settting **\n\n"

	     << "   -i, -iconic\n"
	     << "    Start " << PACKAGE_NAME << " in iconified state\n\n"

	     << "   -k, -kbd or -nok, -nokbd\n"
	     << "    Enable or disable visible keyboard focus in non-text widgets\n\n"

	     << "   -na CLASSNAME, -name CLASSNAME\n"
	     << "    Set the window class to CLASSNAME\n\n"

	     << "   -s SCHEME, -scheme SCHEME\n"
	     << "    Set the widget scheme\n"
	     << "    SCHEME can be one of: " << schemes << "\n\n"

	     << "   -ti WINDOWTITLE, -title WINDOWTITLE\n"
	     << "    Set the window title\n\n"

	     << "   -to, -tooltips or -not, -notooltips\n"
	     << "    Enable or disable tooltips\n\n";

	help << "Additional UI options:\n\n"

	     << "  --font FONT[:SIZE]\n"
	     << "    Set the widget font and (optionally) size\n"
	     << "    The default is: " << Fl::get_font(FL_HELVETICA)
	     << ':' << FL_NORMAL_SIZE << "\n\n"

//	     << "  --profile PROFILE\n"
//	     << "    If PROFILE is ``emc'', ``emcomm'', or ``minimal'',\n"
//	     << "    widget sizes will be adjusted for a minimal screen footprint\n\n"

	     << "  --twoscopes\n"
	     << "    digiscope adjacent to waterfall and,\n"
	     << "    digiscope in separate resizeable window\n\n"

	     << "  --usechkbtns\n"
	     << "    Use check buttons for AFC / SQL.\n";


	option_help = help.str();
}

void exit_cb(void*) { exit(EXIT_SUCCESS); }

int parse_args(int argc, char **argv, int& idx)
{
	// Only handle long options
	if ( !(strlen(argv[idx]) >= 2 && strncmp(argv[idx], "--", 2) == 0) )
		return 0;

        enum { OPT_ZERO,
#ifndef __CYGWIN__
	       OPT_RX_IPC_KEY, OPT_TX_IPC_KEY,
#endif
	       OPT_CONFIG_DIR,
               OPT_FONT, OPT_WFALL_WIDTH, OPT_WFALL_HEIGHT,
               OPT_WINDOW_WIDTH, OPT_WINDOW_HEIGHT, 
//               OPT_PROFILE, 
               OPT_USE_CHECK,
	       	   OPT_RESAMPLE,
#if USE_PORTAUDIO
               OPT_FRAMES_PER_BUFFER,
#endif
			   OPT_TWO_SCOPES,
               OPT_EXIT_AFTER,
               OPT_HELP, OPT_VERSION };

	const char shortopts[] = "+";
	static struct option longopts[] = {
#ifndef __CYGWIN__
		{ "rx-ipc-key",	   1, 0, OPT_RX_IPC_KEY },
		{ "tx-ipc-key",	   1, 0, OPT_TX_IPC_KEY },
#endif
		{ "config-dir",	   1, 0, OPT_CONFIG_DIR },
		{ "font",	   1, 0, OPT_FONT },

		{ "wfall-width",   1, 0, OPT_WFALL_WIDTH },
		{ "wfall-height",  1, 0, OPT_WFALL_HEIGHT },
		{ "window-width",  1, 0, OPT_WINDOW_WIDTH },
		{ "window-height", 1, 0, OPT_WINDOW_HEIGHT },
//		{ "profile",	   1, 0, OPT_PROFILE },
		{ "twoscopes",     0, 0, OPT_TWO_SCOPES },
		{ "usechkbtns",    0, 0, OPT_USE_CHECK },

		{ "resample",      1, 0, OPT_RESAMPLE },

#if USE_PORTAUDIO
		{ "frames-per-buf",1, 0, OPT_FRAMES_PER_BUFFER },
#endif
		{ "exit-after",    1, 0, OPT_EXIT_AFTER },

		{ "help",	   0, 0, OPT_HELP },
		{ "version",	   0, 0, OPT_VERSION },
		{ 0 }
	};

	int longindex;
	optind = idx;
	int c = getopt_long(argc, argv, shortopts, longopts, &longindex);

	switch (c) {
		case -1:
			return 0;
		case 0:
			// handle options with non-0 flag here
			return 0;

#ifndef __CYGWIN__
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

		case OPT_CONFIG_DIR:
			HomeDir = optarg;
			if (*HomeDir.rbegin() != '/')
				HomeDir += '/';
			break;

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

		case OPT_WFALL_WIDTH:
			IMAGE_WIDTH = strtol(optarg, NULL, 10);
			break;

		case OPT_WFALL_HEIGHT:
			Hwfall = strtol(optarg, NULL, 10);
			break;

		case OPT_WINDOW_WIDTH:
			WNOM = strtol(optarg, NULL, 10);
			break;

		case OPT_WINDOW_HEIGHT:
			HNOM = strtol(optarg, NULL, 10);
			break;

//		case OPT_PROFILE:
//			if (!strcasecmp(optarg, "emcomm") || !strcasecmp(optarg, "emc") ||
//       	            !strcasecmp(optarg, "minimal")) {
//				IMAGE_WIDTH = DEFAULT_IMAGE_WIDTH;
//				Hwfall = EMC_HWFALL;
//				HNOM = EMC_HNOM;
//				WNOM = EMC_WNOM;
//				FL_NORMAL_SIZE = 11;//12;
//			}
//			break;

		case OPT_TWO_SCOPES:
			twoscopes = true;
			break;

		case OPT_USE_CHECK:
			useCheckButtons = true;
			break;

		case OPT_RESAMPLE:
			if (SoundBase::get_converter(optarg) == INT_MIN) {
				cerr << "Unknown converter `" << optarg << "'\n";
				goto help;
			}
			progdefaults.sample_converter = optarg;
			break;

#if USE_PORTAUDIO
		case OPT_FRAMES_PER_BUFFER:
			pa_frames_per_buffer = strtol(optarg, 0, 10);
			break;
#endif // USE_PORTAUDIO

		case OPT_EXIT_AFTER:
			Fl::add_timeout(strtod(optarg, 0), exit_cb);
			break;

		case OPT_HELP:
			cout << option_help;
			exit(EXIT_SUCCESS);

		case OPT_VERSION:
			cout << version_text;
			exit(EXIT_SUCCESS);

	help:
		case '?':
			cerr << "Try `" << PACKAGE_NAME << " --help' for more information.\n";
			exit(EXIT_FAILURE);

		default:
			cerr << option_help;
			exit(EXIT_FAILURE);
	}

	// Increment idx by the number of args we used and return that number.
	// We must check whether the option argument is in the same argv element
	// as the option name itself, i.e., --opt=arg.
        c = longopts[longindex].has_arg ? 2 : 1;
        if (c == 2) {
                string arg = argv[idx];
                string::size_type p;
                if ((p = arg.rfind(optarg)) != string::npos && arg.substr(p) == optarg)
                        c = 1;
        }
	idx += c;
	return c;
}

void generate_version_text(void)
{
	ostringstream s;
	s << PACKAGE_NAME << ' ' << PACKAGE_VERSION << "\n\nSystem: ";
        struct utsname u;
        if (uname(&u) != -1) {
		s << u.sysname << ' ' << u.nodename
		  << ' ' << u.release << ' ' << u.version << ' '
		  << u.machine << '\n';
	}

#include "versions.h"
#ifdef HAVE_VERSIONS_H
	s /*<< "\nConfigured with: " << COMPILE_CFG*/ << '\n'
	    << "Built on " << COMPILE_DATE << " by " << COMPILE_USER
	    << '@' << COMPILE_HOST << " with:\n"
	    << COMPILER << '\n'
	    << "CFLAGS=" << CFLAGS << '\n'
	    << "LDFLAGS=" << LDFLAGS << '\n';
#endif // HAVE_VERSIONS_H

	s << "Libraries:\n"
	  << "FLTK " << FL_MAJOR_VERSION << '.' << FL_MINOR_VERSION << '.'
	  << FL_PATCH_VERSION << '\n';

	s << src_get_version() << '\n';

#if USE_HAMLIB
	s << hamlib_version << '\n';
#endif

#if USE_PORTAUDIO
	s << Pa_GetVersionText() << ' ' << Pa_GetVersion() << '\n';
#endif

#if USE_SNDFILE
	char sndfile_version[32];
	sf_command(NULL, SFC_GET_LIB_VERSION, sndfile_version, sizeof(sndfile_version));
	s << sndfile_version << '\n';
#endif

	version_text = s.str();
}

// When debugging is enabled, reexec with malloc debugging hooks enabled, unless
// the env var FLDIGI_NO_EXEC is set, or our parent process is gdb.
void debug_exec(char** argv)
{
#if !defined(NDEBUG) && !defined(__CYGWIN__)
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

void string_wrap(std::string& s, unsigned c)
{
	string::size_type spos = s.find(' '), prev = spos, line = 0;

	while ((spos = s.find_first_of(" \n", spos+1)) != string::npos) {
		if (spos - line > c) {
			s[prev] = '\n';
			line = prev + 1;
		}
		if (s[spos] == '\n')
			line = spos + 1;
		prev = spos;
	}
	if (s.length() - line > c)
		s[prev] = '\n';
}

#ifdef __CYGWIN__
static ofstream outlogfile;
static ostringstream outlogstring;
static streambuf* streambufs[3];

void redirect_streams(const std::string& dir)
{
	string log = dir;
	if (*log.rbegin() != '/')
		log += '/';
	log += "status_log.txt";
	outlogfile.open(log.c_str());

	if (!isatty(STDOUT_FILENO)) {
		streambufs[0] = cout.rdbuf();
		if (outlogfile)
			cout.rdbuf(outlogfile.rdbuf());
		else
			cout.rdbuf(outlogstring.rdbuf());
	}
	if (!isatty(STDERR_FILENO)) {
		streambufs[1] = cerr.rdbuf();
		streambufs[2] = clog.rdbuf();
		if (outlogfile) {
			cerr.rdbuf(outlogfile.rdbuf());
			clog.rdbuf(outlogfile.rdbuf());
		}
		else {
			cerr.rdbuf(outlogstring.rdbuf());
			clog.rdbuf(outlogstring.rdbuf());
		}
	}
}

void restore_streams(void)
{
	cout.rdbuf(streambufs[0]);
	cerr.rdbuf(streambufs[1]);
	clog.rdbuf(streambufs[2]);
}
#endif // __CYGWIN__
