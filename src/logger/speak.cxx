// ----------------------------------------------------------------------------
//  speak.cxx rx text data stream to file
//            rx text data stream to client socket on MS
//
// Copyright 2009 W1HKJ, Dave Freese
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
#include <fstream>
#include <string>

#include "gettext.h"
#include "speak.h"
#include "main.h"
#include "status.h"
#include "fl_digi.h"
#include "configuration.h"
#include "debug.h"

#include "socket.h"
#include "icons.h"

using namespace std;

const char *txtTalkInfo = _("\
Save all received text, one character at a time to the following file:\n\n\
    fldigi.files\\talk\\textout.txt (Windows)\n\
    ~/.fldigi/talk/textout.txt (Linux, OS X, Free BSD)");

string speakfname = "";
ofstream speakout;
string speakbuffer = "";

#ifndef __WIN32__
void open_talker() {}
void close_talker() {}
void toggle_talker() {}
#endif

#ifdef __WIN32__
#include "confdialog.h"

string talkbuffer = "";
Socket *talker_tcpip = 0;
string talker_address =  "127.0.0.1";
string talker_port = "1111";
bool   can_talk = true;

void open_talker()
{
	try {
		talker_tcpip = new Socket(Address(talker_address.c_str(), talker_port.c_str()));
		talker_tcpip->set_timeout(0.01);
		talker_tcpip->connect();
		btnConnectTalker->value(1);
		btnConnectTalker->redraw();
		can_talk = true;
	}
	catch (const SocketException& e) {
		LOG_INFO("Talker Server not available");
		btnConnectTalker->value(0);
		btnConnectTalker->redraw();
		can_talk = false;
	}
}

void close_talker()
{
	talker_tcpip->close();
	btnConnectTalker->value(0);
	btnConnectTalker->redraw();
	can_talk = false;
}

void toggle_talker()
{
	if (can_talk)
		close_talker();
	else
		open_talker();
}

void speak(int c)
{
	if (progdefaults.speak) {
		if (speakfname.empty()) {
			speakfname = TalkDir;
			speakfname.append("textout.txt");
		}

		speakbuffer += (char)c;
// Windows is not able to handle continuously open/close of the append file
// the file might or might not be written to.
		if (!speakout) speakout.open(speakfname.c_str(), ios::app);
		if (speakout) {
			for (size_t i = 0; i < speakbuffer.length(); i++)
				speakout.put(speakbuffer[i]);
			speakbuffer.clear();
		}
	}

	if (can_talk && !talker_tcpip) open_talker();
	if (!talker_tcpip)
		return;
	if (c < ' ' || c > 'z') c = ',';
	talkbuffer += c;
	try {
		talker_tcpip->send(talkbuffer);
	}
	catch (const SocketException& e) {
		LOG_INFO("Talker server not available");
		delete talker_tcpip;
		talker_tcpip = 0;
		btnConnectTalker->labelcolor(FL_RED);
		btnConnectTalker->redraw_label();
		can_talk = false;
	}
	talkbuffer.clear();
}
#else

void speak(int c)
{
	if (!progdefaults.speak)
		return;

	if (speakfname.empty()) {
		speakfname = TalkDir;
		speakfname.append("textout.txt");
	}

	speakbuffer += c;

	speakout.open(speakfname.c_str(), ios::app);
	if (!speakout) return;

	for (size_t i = 0; i < speakbuffer.length(); i++)
		speakout.put(speakbuffer[i]);
	speakbuffer.clear();
	speakout.flush();
	speakout.close();
}

#endif
