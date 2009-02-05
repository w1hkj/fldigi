// ====================================================================
//  speak.cxx rx text data stream to file
//
// Copyright W1HKJ, Dave Freese 2006
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
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
// ====================================================================

#include <iostream>
#include <fstream>
#include <string>

#include "speak.h"
#include "main.h"
#include "status.h"
#include "fl_digi.h"
#include "configuration.h"

using namespace std;

const char *txtTalkInfo = "\
Save all received text, one character at a time to the following file:\n\n\
    fldigi.files\\talk\\textout.txt (Windows)\n\
    ~/.fldigi/talk/textout.txt (Linux, OS X, Free BSD)"; 

string speakfname = "";
ofstream speakout;
bool speakOK = false;

void speak_open()
{
    speakfname = TalkDir;
    speakfname.append("textout.txt");
    speakout.open(speakfname.c_str());
    if (speakout) speakOK = true;
}

void speak_close()
{
    if (!speakOK) return;
    speakout.close();
    remove(speakfname.c_str());
}

void speak(int c)
{
    if (!speakOK) speak_open();
	if (!c) return;
	speakout.put((char)c);
	speakout.flush();
}
