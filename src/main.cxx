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
#include <FL/Fl_Shared_Image.H>
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
bool	mailserver = false, mailclient = false;
extern	void start_pskmail();

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
	
	Fl::lock();  // start the gui thread!!	
	Fl::visual(FL_RGB); // insure 24 bit color operation
	fl_register_images();
	
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
	
	if (argc == 2)
		scDevice = argv[1];
	else
		scDevice = progdefaults.SCdevice;
	
	trx_start(scDevice.c_str());

	progStatus.readLastState();
	progStatus.initLastState();
	
	wf->opmode();

	progdefaults.initInterface();

	Fl::set_fonts(0);

	if (mailserver || mailclient) {
		std::cout << "Starting pskmail transport layer" << std::endl; fflush(stdout);
		string PskMailLogName = PskMailDir;

		if (gmfskmail == false)
			PskMailLogName += "mail-io.log";
		else
			PskMailLogName += "gMFSK.log";
			
		Maillogfile = new cLogfile(PskMailLogName.c_str());
		Maillogfile->log_to_file_start();
		Fl::add_timeout(10.0, pskmail_loop);
	}

	fl_digi_main->show();

	return Fl::run();
}

