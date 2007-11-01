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

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <dirent.h>

#include <FL/Fl_Shared_Image.H>
#ifdef PORTAUDIO
	#include <portaudiocpp/PortAudioCpp.hxx>
#endif
#include "main.h"
#include "waterfall.h"
#include "fft.h"
#include "sound.h"
#include "complex.h"
#include "fl_digi.h"
#include "rigio.h"
#include "globals.h"
#include "psk.h"
#include "cw.h"
#include "mfsk.h"
#include "Config.h"
#include "configuration.h"
#include "macros.h"
#include "status.h"

#ifndef NOHAMLIB
	#include "rigclass.h"
#endif
#include "rigsupport.h"

#include "log.h"

#include "qrunner.h"

using namespace std;

string scDevice = "/dev/dsp1";
char szHomedir[120] = "";
char szPskMailDir[120] = "";
string PskMailDir;
string PskMailFile;
string ArqFilename;
string HomeDir;
string xmlfname;

bool gmfskmail = false;

PTT		*push2talk = (PTT *)0;
#ifndef NOHAMLIB
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

string option_help;


void arqchecks(void);
void generate_option_help(void);
int parse_args(int argc, char **argv, int& idx);
void print_versions(void);

int main(int argc, char ** argv)
{
	CREATE_THREAD_ID(); // only call this once
	SET_THREAD_ID(FLMAIN_TID);

	for (int i = 0; i < NUM_QRUNNER_THREADS; i++) {
		cbq[i] = new qrunner(1);
		cbq[i]->attach();
	}

	fl_filename_expand(szHomedir, 119, "$HOME/.fldigi/");
	HomeDir = szHomedir;
cout << HomeDir.c_str() << endl;

	generate_option_help();
	int arg_idx;
	
	if (Fl::args(argc, argv, arg_idx, parse_args) != argc) {
	    cerr << FLDIGI_NAME << ": unrecognized option `" << argv[arg_idx]
		 << "'\nTry `" << FLDIGI_NAME
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

   	txmsgid = msgget( (key_t) progdefaults.tx_msgid, 0666 );
	fl_filename_expand(szPskMailDir, 119, "$HOME/pskmail.files/");
	PskMailDir = szPskMailDir;

	FL_LOCK_E();  // start the gui thread!!	
	Fl::visual(FL_RGB); // insure 24 bit color operation
	
	fl_register_images();
	Fl::set_fonts(0);
	
	rigcontrol = createRigDialog();
	progdefaults.readDefaultsXML();
	progdefaults.testCommPorts();
	
	create_fl_digi_main();

	createConfig();
	inpTTYdev->tooltip(progdefaults.strCommPorts.c_str());
	inpRIGdev->tooltip(progdefaults.strCommPorts.c_str());
	
	macros.loadDefault();

#ifndef NOHAMLIB
	xcvr = new Rig();
#endif
	
	push2talk = new PTT();

	progdefaults.openDefaults();
	
	glob_t gbuf;
	glob("/dev/dsp*", 0, NULL, &gbuf);
	for (size_t i = 0; i < gbuf.gl_pathc; i++)
		menuOSSDev->add(gbuf.gl_pathv[i]);
	if (progdefaults.OSSdevice == "")
		progdefaults.OSSdevice = gbuf.gl_pathv[0];
	menuOSSDev->value(progdefaults.OSSdevice.c_str());
	globfree(&gbuf);
	
#ifdef PORTAUDIO
	portaudio::AutoSystem autoSys;
	portaudio::System &sys = portaudio::System::instance();
	for (portaudio::System::DeviceIterator idev = sys.devicesBegin();
	     idev != sys.devicesEnd(); ++idev) {
		string s;
		s.append(idev->hostApi().name()).append("/").append(idev->name());
		string::size_type i = s.find('/') + 1;
// backslash-escape any slashes in the device name
		while ((i = s.find('/', i)) != string::npos) {
			s.insert(i, 1, '\\');
			i += 2;
		}
		menuPADev->add(s.c_str());
// set the initial value in the configuration structure
		if (progdefaults.PAdevice == "" && idev == sys.devicesBegin())
			progdefaults.PAdevice = idev->name();
	}
	menuPADev->value(progdefaults.PAdevice.c_str());
	
	btnAudioIO[1]->activate();
#endif

	glob("/dev/mixer*", 0, NULL, &gbuf);
	for (size_t i = 0; i < gbuf.gl_pathc; i++) 
		menuMix->add(gbuf.gl_pathv[i]);
	if (progdefaults.MXdevice == "") 
		progdefaults.MXdevice = gbuf.gl_pathv[0];
	globfree(&gbuf);
	menuMix->value(progdefaults.MXdevice.c_str());
	
// set the Sound Card configuration tab to the correct initial values
	if (progdefaults.btnAudioIOis == 0) {
		scDevice = progdefaults.OSSdevice;
		btnAudioIO[0]->value(1);
		btnAudioIO[1]->value(0);
		menuOSSDev->activate();
		menuPADev->deactivate();
	} else {
		scDevice = progdefaults.PAdevice;
		btnAudioIO[0]->value(0);
		btnAudioIO[1]->value(1);
		menuOSSDev->deactivate();
		menuPADev->activate();
	}
	resetMixerControls();

	trx_start(scDevice.c_str());

	progdefaults.initInterface();
	progStatus.initLastState();
	
	Fl::add_timeout(1.0, pskmail_loop);

	fl_digi_main->show(argc, argv);

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
	int width = 37;
	help << "Usage:\n " << FLDIGI_NAME << " [option...]\n";

	help << '\n' << FLDIGI_NAME << " options:\n"

	     << setw(width) << setiosflags(ios::left)
	     << " --config-dir DIRECTORY"
	     << "Look for configuration files in DIRECTORY\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "The default is: " << HomeDir << '\n'

	     << setw(width) << setiosflags(ios::left)
	     << " --rx-ipc-key KEY" << "Set the receive message queue key\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "May be given in hex if prefixed with \"0x\"\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "The default is: " << progdefaults.rx_msgid
	     << " or 0x" << hex << progdefaults.rx_msgid << dec << '\n'

	     << setw(width) << setiosflags(ios::left)
	     << " --tx-ipc-key KEY" << "Set the transmit message queue key\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "May be given in hex if prefixed with \"0x\"\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "The default is: " << progdefaults.tx_msgid
	     << " or 0x" << hex << progdefaults.tx_msgid << dec << '\n'

	     << setw(width) << setiosflags(ios::left)
	     << " --version" << "Print version information\n"

	     << setw(width) << setiosflags(ios::left)
	     << " --help" << "Print this option help\n";


	// Fl::help looks ugly so we'll write our own

	help << "\nStandard FLTK options:\n"

	     << setw(width) << setiosflags(ios::left)
	     << " -bg COLOR, -background COLOR"
	     << "Set the background color\n"

	     << setw(width) << setiosflags(ios::left)
	     << " -bg2 COLOR, -background2 COLOR"
	     << "Set the secondary (text) background color\n"

	     << setw(width) << setiosflags(ios::left)
	     << " -di, -display DISPLAY"
	     << "Set the X display to use\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "DISPLAY format is ``host:n.n''\n"

	     << setw(width) << setiosflags(ios::left)
	     << " -dn, -dnd or -nodn, -nodnd"
	     << "Enable or disable drag and drop copy and\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "paste in text fields\n"

	     << setw(width) << setiosflags(ios::left)
	     << " -fg COLOR, -foreground COLOR"
	     << "Set the foreground color\n"

	     << setw(width) << setiosflags(ios::left)
	     << " -g GEOMETRY, -geometry GEOMETRY"
	     << "Set the initial window size and position\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "GEOMETRY format is ``WxH+X+Y''\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "** " << FLDIGI_NAME << " may override this settting **\n"

	     << setw(width) << setiosflags(ios::left)
	     << " -i, -iconic"
	     << "Start " << FLDIGI_NAME << " in iconified state\n"

	     << setw(width) << setiosflags(ios::left)
	     << " -k, -kbd or -nok, -nokbd"
	     << "Enable or disable visible keyboard focus\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "in non-text widgets\n"

	     << setw(width) << setiosflags(ios::left)
	     << " -na CLASSNAME, -name CLASSNAME"
	     << "Set the window class to CLASSNAME\n"

	     << setw(width) << setiosflags(ios::left)
	     << " -s SCHEME, -scheme SCHEME"
	     << "Set the widget scheme\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "SCHEME can be one of: " << schemes << '\n'

	     << setw(width) << setiosflags(ios::left)
	     << " -ti WINDOWTITLE, -title WINDOWTITLE"
	     << "Set the window title\n"

	     << setw(width) << setiosflags(ios::left)
	     << " -to, -tooltips or -not, -notooltips"
	     << "Enable or disable tooltips\n";

	help << "\nAdditional UI options:\n"

	     << setw(width) << setiosflags(ios::left)
	     << " --fast-text" << "Use fast text widgets\n"

	     << setw(width) << setiosflags(ios::left)
	     << " --font FONT[:SIZE]"
	     << "Set the widget font and (optional) size\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "The default is: " << Fl::get_font(FL_HELVETICA)
	     << ':' << FL_NORMAL_SIZE << '\n'

	     << setw(width) << setiosflags(ios::left)
	     << " --profile PROFILE"
	     << "If PROFILE is ``emc'', ``emcomm'', or\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "``minimal'', widget sizes will be adjusted\n"
	     << setw(width) << setiosflags(ios::left)
	     << "" << "for a minimal screen footprint.\n"

	     << setw(width) << setiosflags(ios::left)
	     << " --usechkbtns"
	     << "Use check buttons for AFC / SQL\n";


	option_help = help.str();
}

int parse_args(int argc, char **argv, int& idx)
{
	// Only handle long options
	if ( !(strlen(argv[idx]) >= 2 && strncmp(argv[idx], "--", 2) == 0) )
		return 0;

        enum { ZERO, RX_IPC_KEY, TX_IPC_KEY, CONFIG_DIR, FAST_TEXT, FONT,
               WFALL_WIDTH, WFALL_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT, PROFILE,
               USE_CHECK, HELP, VERSION };

	const char shortopts[] = "+";
	static struct option longopts[] = {
		{ "rx-ipc-key",	   1, 0, RX_IPC_KEY },
		{ "tx-ipc-key",	   1, 0, TX_IPC_KEY },
		{ "config-dir",	   1, 0, CONFIG_DIR },
		{ "fast-text",	   0, 0, FAST_TEXT },
		{ "font",	   	   1, 0, FONT },

		{ "wfall-width",   1, 0, WFALL_WIDTH },
		{ "wfall-height",  1, 0, WFALL_HEIGHT },
		{ "window-width",  1, 0, WINDOW_WIDTH },
		{ "window-height", 1, 0, WINDOW_HEIGHT },
		{ "profile",	   1, 0, PROFILE },
		{ "usechkbtns",    0, 0, USE_CHECK },

		{ "help",	       0, 0, HELP },
		{ "version",	   0, 0, VERSION },
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

		case RX_IPC_KEY: case TX_IPC_KEY:
		{
			errno = 0;
			int key = strtol(optarg, NULL, (strncasecmp(optarg, "0x", 2) ? 10 : 16));
			if (errno || key <= 0)
				cerr << "Hmm, " << key << " doesn't look like a valid IPC key\n";
			if (c == 'r')
				progdefaults.rx_msgid = key;
			else
				progdefaults.tx_msgid = key;
		}
			idx += 2;
			return 2;

		case CONFIG_DIR:
			HomeDir = optarg;
			if (*HomeDir.rbegin() != '/')
				HomeDir += '/';
			idx += 2;
			return 2;

		case FAST_TEXT:
			progdefaults.alt_text_widgets = false;
			idx += 1;
			return 1;

		case FONT:
		{
			char *p;
			if ((p = strchr(optarg, ':'))) {
				*p = '\0';
				extern int FL_NORMAL_SIZE;
				FL_NORMAL_SIZE = strtol(p + 1, 0, 10);
			}
		}
			Fl::set_font(FL_HELVETICA, optarg);
			idx += 2;
			return 2;

		case WFALL_WIDTH:
			IMAGE_WIDTH = strtol(optarg, NULL, 10);
			idx += 2;
			return 2;

		case WFALL_HEIGHT:
			Hwfall = strtol(optarg, NULL, 10);
			idx += 2;
			return 2;

		case WINDOW_WIDTH:
			WNOM = strtol(optarg, NULL, 10);
			idx += 2;
			return 2;

		case WINDOW_HEIGHT:
			HNOM = strtol(optarg, NULL, 10);
			idx += 2;
			return 2;

		case PROFILE:
			if (!strcasecmp(optarg, "emcomm") || !strcasecmp(optarg, "emc") ||
        	            !strcasecmp(optarg, "minimal")) {
				IMAGE_WIDTH = DEFAULT_IMAGE_WIDTH;
				Hwfall = EMC_HWFALL;
				HNOM = EMC_HNOM;
				WNOM = EMC_WNOM;
				FL_NORMAL_SIZE = 12;
			}
			idx += 2;
			return 2;

		case USE_CHECK:
			useCheckButtons = true;
			idx += 1;
			return 1;

		case HELP:
			cerr << option_help;
			exit(EXIT_SUCCESS);

		case VERSION:
			print_versions();
			exit(EXIT_SUCCESS);

		case '?':
			cerr << "Try `" << FLDIGI_NAME << " --help' for more information.\n";
			exit(EXIT_FAILURE);

		default:
			cerr << option_help;
			exit(EXIT_FAILURE);
		}
	return 0;
}

void print_versions(void)
{
	cerr << FLDIGI_NAME << ' ' << FLDIGI_VERSION << "\n\nSystem: ";
        struct utsname u;
        if (uname(&u) != -1) {
                cerr << u.sysname << ' ' << u.nodename
                     << ' ' << u.release << ' ' << u.version << ' '
                     << u.machine << '\n';
        }

#include "versions.h"
#ifdef HAVE_VERSIONS_H
	cerr << "\nConfigured with: " << COMPILE_CFG << '\n'
	     << "Built on " << COMPILE_DATE << " by " << COMPILE_USER
	     << '@' << COMPILE_HOST << " with:\n"
	     << ' ' << COMPILER << '\n'
	     << " CFLAGS=" << CFLAGS << '\n'
	     << " LDFLAGS=" << LDFLAGS << '\n';
#endif // HAVE_VERSIONS_H

	cerr << "Libraries:\n"
	     << " FLTK " << FL_MAJOR_VERSION << '.' << FL_MINOR_VERSION << '.'
	     << FL_PATCH_VERSION << '\n';

#ifndef NOHAMLIB
	cerr << ' ' << hamlib_version << '\n';
#endif

#ifdef PORTAUDIO
	cerr << ' ' << portaudio::System::versionText() << ' '
	     << portaudio::System::version() << '\n';
#endif

        char sndfile_version[32];
        sf_command(NULL, SFC_GET_LIB_VERSION, sndfile_version, sizeof(sndfile_version));
        cerr << ' ' << sndfile_version << endl;
}
