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

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

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

void arqchecks()
{
   	txmsgid = msgget( (key_t) 6789, 0666 );
	if (txmsgid != -1)
		return;

	fl_filename_expand(szPskMailDir, 119, "$HOME/pskmail.files/");
	PskMailDir = szPskMailDir;
	PskMailFile = PskMailDir;
	PskMailFile += "PSKmailserver";
	ifstream testFile;
	testFile.open(PskMailFile.c_str());
	if (testFile.is_open()) {
		mailserver = true;
		testFile.close();
	} else {
		PskMailFile = PskMailDir;
		PskMailFile += "PSKmailclient";
		testFile.open(PskMailFile.c_str());
		if (testFile.is_open()) {
			mailclient = true;
			testFile.close();
		} else {
			PskMailDir = "./";
			PskMailFile = PskMailDir;
			PskMailFile += "PSKmailserver";
			testFile.open(PskMailFile.c_str());
			if (testFile.is_open()) {
				mailserver = true;
				testFile.close();
				gmfskmail = true;
			} else {
				PskMailFile = PskMailDir;
				PskMailFile += "PSKmailclient";
				testFile.open(PskMailFile.c_str());
				if (testFile.is_open()) {
					mailclient = true;
					testFile.close();
					gmfskmail = true;
				}
			}
		}
	}
}

int main(int argc, char ** argv) {

	fl_filename_expand(szHomedir, 119, "$HOME/.fldigi/");
	if (fl_filename_isdir(szHomedir) == 0)
		HomeDir = "./";
	else
		HomeDir = szHomedir;
	xmlfname = HomeDir; 
	xmlfname.append("rig.xml");
	
	string lfname = HomeDir;
	lfname.append("fldigi.log");
	logfile = new cLogfile(lfname);
	logfile->log_to_file_start();

	arqchecks();
	
	Fl::lock();  // start the gui thread!!	
	Fl::visual(FL_RGB); // insure 24 bit color operation
	
//	Fl::visual(FL_DOUBLE|FL_INDEX| FL_RGB);
	
	fl_register_images();
	Fl::set_fonts(0);
	
	rigcontrol = createRigDialog();
	create_fl_digi_main();

	createConfig();
	macros.loadDefault();

#ifndef NOHAMLIB
	xcvr = new Rig();
#endif
	
	push2talk = new PTT();

	if (progdefaults.openDefaults())
		push2talk->reset(	progdefaults.btnPTTis,
							progdefaults.btnRTSDTRis,
							progdefaults.btnPTTREVis );
	
#ifndef PORTAUDIO
	scDevice = progdefaults.SCdevice;
#else
	if (progdefaults.btnAudioIOis == 0)
		scDevice = progdefaults.OSSdevice;
	else if (progdefaults.btnAudioIOis == 1)
		scDevice = progdefaults.PAdevice;
#endif

	glob_t gbuf;
	glob("/dev/dsp*", 0, NULL, &gbuf);
	for (size_t i = 0; i < gbuf.gl_pathc; i++)
		menuOSSDev->add(gbuf.gl_pathv[i]);
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
	}
	btnAudioIO[1]->activate();
#endif

	glob("/dev/mixer*", 0, NULL, &gbuf);
	for (size_t i = 0; i < gbuf.gl_pathc; i++)
		menuMix->add(gbuf.gl_pathv[i]);
	globfree(&gbuf);

	if (progdefaults.MXdevice == "") {
		int n = 0;
		progdefaults.MXdevice = "/dev/mixer";
		if (sscanf(progdefaults.SCdevice.c_str(), "/dev/dsp%d", &n) == 1)
			progdefaults.MXdevice += n;
		menuMix->value(progdefaults.MXdevice.c_str());
	}

	resetMixerControls();

	trx_start(scDevice.c_str());

	progdefaults.initInterface();

	fl_digi_main->show();

	progStatus.initLastState();
	wf->opmode();
	
	if (mailserver || mailclient) {
		std::cout << "Starting pskmail transport layer" << std::endl; fflush(stdout);
		string PskMailLogName = PskMailDir;

		if (gmfskmail == true)
			PskMailLogName += "gMFSK.log";
		else
			PskMailLogName += "mail-io.log";
			
		Maillogfile = new cLogfile(PskMailLogName.c_str());
		Maillogfile->log_to_file_start();
	}
	Fl::add_timeout(1.0, pskmail_loop);

	return Fl::run();
}

