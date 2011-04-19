// ----------------------------------------------------------------------------
// flarqenv.cxx
//
// Copyright (C) 2009
//		Stelios Bounanos, M0GLD
// Copyright (C) 2009
//		Dave Freese, W1HKJ
//
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
#include <string>
#include <sstream>

#include <cstdlib>
#include <cstring>
#include <cerrno>

#ifdef __MINGW32__
#  include "compat.h"
#endif

#if HAVE_SYS_UTSNAME_H
#  include <sys/utsname.h>
#endif

#include <signal.h>
#include <getopt.h>

#include <FL/Fl.H>

#include "stacktrace.h"
#include "flarq.h"

using namespace std;

string option_help, version_text, build_text;
extern string arq_address, arq_port;
extern bool ioMPSK;
extern bool SHOWDEBUG;

void generate_option_help(void)
{
	ostringstream help;
	help << "Usage:\n"
	     << "    " << PACKAGE_NAME << " [option...]\n\n";

	help << PACKAGE_NAME << " options:\n\n"
	     << "  --arq-protocol TYPE\n"
	     << "    Set the ARQ protocol\n"
	     << "    May be either ``fldigi'' or ``multipsk''\n"
	     << "    The default is: " << (ioMPSK ? "multipsk" : "fldigi") << "\n\n"

	     << "  --arq-server-address HOSTNAME\n"
	     << "    Set the ARQ TCP server address\n"
	     << "    The default is: " << arq_address << "\n\n"

	     << "  --arq-server-port PORT\n"
	     << "    Set the ARQ TCP server port\n"
	     << "    The default is: " << arq_port << "\n\n"

	     << "  --debug\n"
	     << "    Enable debugging messages\n\n"

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
	     << ':' << FL_NORMAL_SIZE << "\n\n";

	option_help = help.str();
}

int parse_args(int argc, char** argv, int& idx)
{
	// Only handle long options
	if ( !(strlen(argv[idx]) >= 2 && strncmp(argv[idx], "--", 2) == 0) )
		return 0;

        enum { OPT_ZERO,
#ifndef __WOE32__
	       OPT_RX_IPC_KEY, OPT_TX_IPC_KEY,
#endif
	       OPT_ARQ_PROTOCOL, OPT_ARQ_ADDRESS, OPT_ARQ_PORT,

	       OPT_FONT,

	       OPT_DEBUG, OPT_DEPRECATED, OPT_HELP, OPT_VERSION, OPT_BUILD_INFO
	};

	const char shortopts[] = "+";
	struct option longopts[] = {
#ifndef __WOE32__
		{ "rx-ipc-key",	   1, 0, OPT_RX_IPC_KEY },
		{ "tx-ipc-key",	   1, 0, OPT_TX_IPC_KEY },
#endif
		{ "arq-protocol",  1, 0, OPT_ARQ_PROTOCOL },
		{ "arq-server-address", 1, 0, OPT_ARQ_ADDRESS },
		{ "arq-server-port",    1, 0, OPT_ARQ_PORT },

		{ "font",	   1, 0, OPT_FONT },

		{ "debug",         0, 0, OPT_DEBUG },

		{ "help",	   0, 0, OPT_HELP },
		{ "version",	   0, 0, OPT_VERSION },
		{ "build-info",	   0, 0, OPT_BUILD_INFO },
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

		case OPT_ARQ_PROTOCOL:
			if (!strcmp(optarg, "fldigi"))
				ioMPSK = false;
			else if (!strcmp(optarg, "multipsk"))
				ioMPSK = true;
			else {
				cerr << "E: unknown protocol type\n";
				exit(EXIT_FAILURE);
			}
			break;
		case OPT_ARQ_ADDRESS:
			arq_address = optarg;
			break;
		case OPT_ARQ_PORT:
			arq_port = optarg;
			break;

		case OPT_FONT: {
			char *p;
			if ((p = strchr(optarg, ':'))) {
				*p = '\0';
				FL_NORMAL_SIZE = strtol(p + 1, 0, 10);
			}

			Fl::set_font(FL_HELVETICA, optarg);
			break;
		}

		case OPT_DEBUG:
			SHOWDEBUG = true;
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

		case '?': default:
			cerr << "Try `" << PACKAGE_NAME << " --help' for more information.\n";
			exit(EXIT_FAILURE);

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

void set_platform_ui(void)
{
#if defined(__APPLE__)
       FL_NORMAL_SIZE = 12;
#elif defined(__WOE32__)
       Fl::set_font(FL_HELVETICA, "Tahoma");
       FL_NORMAL_SIZE = 11;
#else
       FL_NORMAL_SIZE = 12;
#endif
}

void generate_version_text(void)
{
	version_text.assign(PACKAGE_STRING "\nCopyright (C) 2008, 2009 " PACKAGE_AUTHORS ".\n");
	version_text.append("License GPLv3+: GNU GPL version 3 or later "
			    "<http://www.gnu.org/licenses/gpl.html>\n"
			    "This is free software: you are free to change and redistribute it.\n"
			    "There is NO WARRANTY, to the extent permitted by law.\n");

	ostringstream s;
	s << "Build information:\n";
	s << "  built          : " << BUILD_DATE << " by " << BUILD_USER
	  << '@' << BUILD_HOST << " on " << BUILD_BUILD_PLATFORM
	  << " for " << BUILD_TARGET_PLATFORM << "\n\n"
	  << "  configure flags: " << BUILD_CONFIGURE_ARGS << "\n\n"
	  << "  compiler       : " << BUILD_COMPILER << "\n\n"
	  << "  compiler flags : " << FLARQ_BUILD_CXXFLAGS << "\n\n"
	  << "  linker flags   : " << FLARQ_BUILD_LDFLAGS << "\n\n"

	  << "  libraries      : " "FLTK " FLTK_BUILD_VERSION "\n";

	s << "\nRuntime information:\n";
        struct utsname u;
        if (uname(&u) != -1) {
		s << "  system         : " << u.sysname << ' ' << u.nodename
		  << ' ' << u.release << ' ' << u.version << ' ' << u.machine << "\n\n";
	}

	build_text = s.str();
}

void setup_signal_handlers(void)
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
