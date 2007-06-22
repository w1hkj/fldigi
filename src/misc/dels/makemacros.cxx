//#include "macros.h"

//#include "main.h"

//#include "fl_digi.h"
//#include "configation.h"
//#include "Config.h"
//#include "version.h"
//#include "logger.h"

//#include <FL/Fl.H>
//#include "File_Selector.h"

#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

using namespace std;

string mtext = 
"//fldigi macro definition file\n\
// This file defines the macro structe(s) for the digital modem program, fldigi\n\
// It also serves as a basis for any macros that are written by the user\n\
//\n\
// The top line of this file should always be the first line in every macro definition file (.mdf)\n\
// for the fldigi program to recognize it as such.\n\
//\n\
// Macros are short text statements that contain imbedded references to text data used by the\n\
// program fldigi.  The imbedded references are always prefaced by the carat(^) symbol and consist\n\
// of single letter codes.  All references to the local system are in lower case and to the remote\n\
// system or global values are in upper case.\n\
//\n\
//\n\
// <CALL>  remote call\n\
// <LDT>  local date time Zone\n\
//     format : %x %H:%M %Z\n\
//     where %x is preferred short form date ie: MM/DD/YY or DD/MM/YYYY etc\n\
//           %H is ho with leading 0\n\
//           %M is minute with leading 0\n\
//           %Z is abbreviated time zone ie: EDT or GMT\n\
// <ZDT>  GMT date time Zone\n\
//     format : %x %H:%M %Z\n\
// <FREQ>  my frequency\n\
// <ID>  send Mode Idenfier - waterfall script\n\
// <LOC>  remote locator\n\
// <LOG>  submit QSO data to logbook program & clear the QSO data fields\n\
//     Not constrained to a particular position in the macro.\n\
//     Action takes place when macro is expanded, so effect is seen immediately\n\
//     after executing the macro which contains this macro reference.\n\
//\n\
// <MODE>  my mode\n\
// <MYCALL>  my call\n\
// <MYLOC>  my locator\n\
// <MYNAME>  my name\n\
// <MYQTH>  my qth\n\
// <MYRST>  my RST\n\
// <NAME>  remote name\n\
// <QTH>  remote qth\n\
// <RST>  remote RST\n\
// <RX>  retn to receive\n\
// <TX>  start transmit\n\
// <VER>  Fldigi + version\n\
//\n\
// Contest macro definitions:\n\
// <CNTR>  substitute the contest counter - no change in value\n\
// <DECR>  decrement the contest counter - no substitution\n\
// <INCR>  increment the contest counter - no substitution\n\
//\n\
// Autorepeat macro definition:\n\
// <TIMER>NNN autorepeat this macro after NNN seconds\n\
// NNN must be terminated by either a space ' ' or a linefeed\n\
// it can appear anywhere in the macro defintion.\n\
// see macro # 19 for an example of an auto-cq repeating macro\n\
//\n\
//\n\
// Local references are specified ding the program configation and can be changed during \n\
// program operation.\n\
//\n\
// Remote references are all part of the qso log field definitions and are routinely changed \n\
// from contact to contact.\n\
//\n\
// Global references are to things like Greenwich Mean Time\n\
//\n\
// Each new macro begins with a macro specifier line as: /$ nn MACRONAME\n\
//   where 'nn' specifies the macro number.  Macros numbered 0..9 refer to function key 1 to 10\n\
//   respectively.  Macros numbered 10..19 refer to \"alt\" function keys 1 to 10.  Undefined Macros\n\
//   will also contain an empty character string and thus produce no output when invoked.\n\
// MACRONAME can be as long as you want, however only the first 8 characters will be displayed on\n\
//   the associated function key button.\n\
//\n\
// You can put the macro definition on multiple lines.  These lines will be concatenated\n\
// into a single line unless you put the new-line pair \"\\n\" at the end of the line\n\
// that you want to terminate a line ding transmission.\n\
//\n\
// Macro definitions do not need to be in numerical order.\n\
// Macro definitions may be skipped or you can truncate the file to less than\n\
// the full macro set of 20.\n\
//\n\
// I recommend using a copy of this file suitably modified for all of yo macro\n\
// definition files.  Just fill in the appropriate fields and delete those you do not\n\
// need for that macro set.\n\
//\n\
//\n\
// Let the Macros begin!\n\
//\n\
";

static string label[20];
static string text[20];

void newmacros()
{
label[0] = "f1 CQ";
text[0] = "<TX>\\n\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL>\\n\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL> pse k\\n\n\
<RX>";

label[1] = "f2 ANS";
text[1] = "<TX>\\n\n\
<CALL> <CALL> de <MYCALL> <MYCALL> <MYCALL> kn\\n\n\
<RX>";

label[2] ="f3 QSO";
text[2] = "<TX>\\n\n\
<CALL> de <MYCALL> ";

label[3] = "f4 KN";
text[3] = "\\nbtu <NAME> <CALL> de <MYCALL> k\\n\n\
<RX>";

label[4] = "f5 SK";
text[4] = "\\n\n\
tnx fer QSO <NAME>, 73, God bless.\\n\n\
<ZDT> <CALL> de <MYCALL> sk\\n\n\
<RX>";

label[5] = "f6 Me";
text[5] = " my name is <MYNAME> <MYNAME> ";

label[6] = "f7 QTH";
text[6] = " my QTH is <MYQTH>, loc: <MYLOC> ";

label[7] = "f8 Brag";
text[7] = "\\n\n\
(( <MYCALL>, <MYNAME> Info ))\\n\n\
Age:   \\n\n\
Rig:   \\n\n\
Pwr:   \\n\n\
Ant:   \\n\n\
OS:    Linux\\n\n\
Soft:  <VER>\\n\n\
Web:   \\n\n\
Email: \\n";

label[8] = "Tx";
text[8] = "<TX>";

label[9] = "Rx";
text[9] = "<RX>";

label[10] = "C Answer";
text[10] = "<TX>de <MYCALL> <MYCALL><RX>\\n";

label[11] = "C Again";
text[11] = "<TX><DECR><CNTR> <CNTR><INCR> QSL DE <MYCALL> K\\n\n\
<RX>";

label[12] = "C Report";
text[12] = "<TX><CALL> RR NBR <CNTR> <CNTR><INCR> TU DE <MYCALL> K\\n\n\
<RX>";

label[13] = "C Incr";
text[13] = "<INCR>";

label[14] = "C Decr";
text[14] = "<DECR>";

label[15] = "Log QSO";
text[15] = "<LOG>";

label[16] = "CW-CQ";
text[16] = "<TX>CQ CQ CQ DE <MYCALL> <MYCALL> <MYCALL>  CQ CQ CQ DE <MYCALL> K<RX>\\n";

label[17] = "Macro 18";
text[17] = "";

label[18] = "AUTO-CQ";
text[18] = "<TX>\\n\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL>\\n\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL> k<TIMER>15 <RX>";

label[19] = "CQ-ID";
text[19] = "<TX><ID>\\n\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL>\\n\n\
CQ CQ CQ de <MYCALL> <MYCALL> <MYCALL> pse k\\n\n\
<RX>";
}


void createDotFldigi()
{
	string Filename = "";//HomeDir;
//	mkdir(Filename.c_str(), O_CREAT | S_IRUSR|S_IWUSR|S_IXUSR);
	Filename.append("macros.mdf");
	ofstream mfile(Filename.c_str());
	newmacros();
	mfile << mtext;
	for (int i = 0; i < 20; i++) {
		mfile << "//\n// Macro # " << i+1 << "\n";
		mfile << "/$ " << i << " " << label[i].c_str() << "\n";
		mfile << text[i].c_str() << "\n";
	}
	mfile.close();
}

int main (int argc, char *argv[])
{
	createDotFldigi();
}
