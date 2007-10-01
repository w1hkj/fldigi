#include "rigCAT.h"

#include "rigsupport.h"
#include "FreqControl.h"
#include "rigdialog.h"
#include "serial.h"
#include "threads.h"
#include "File_Selector.h"

void MilliSleep(long msecs) {
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = msecs * 1000L;
	select (0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &tv);
}


Fl_Double_Window *window;
char homedir[FL_PATH_MAX];
string xmlfname;

int main (int argc, char *argv[])
{	
	ifstream testfile;
	string fname;
	char *home[] = { "$HOME/.fldigi/", "$HOME/", "./"};

	if (argc == 2)
		fname.append(argv[1]);
	else
		fname.append("rig.xml");

	for (int i = 0; i < 3; i++) {
		fl_filename_expand(homedir, FL_PATH_MAX, home[i]);
		xmlfname = homedir; xmlfname.append(fname);
		testfile.open(xmlfname.c_str(), ios::in);
		if (testfile.is_open())
			break;
	}
	if (!testfile.is_open()) {
		std::cout << "No rig definition file found!\n";
		return (1);
	}
	testfile.close();

    Fl::lock();
	window = createRigDialog();
	window->show ();

    return Fl::run();
}
