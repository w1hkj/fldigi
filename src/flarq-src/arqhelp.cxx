// ----------------------------------------------------------------------------
// arqhelp.cxx
//
// Copyright 2008-2009
//	Dave Freese, W1HKJ
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

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#include <FL/x.H>
#include <FL/Fl_Help_Dialog.H>

#ifndef __WOE32__
	#include <sys/ipc.h>
	#include <sys/msg.h>
	#include <dirent.h>
#endif

// this tests depends on a modified FL/filename.H in the Fltk-1.3.0
// change
//#  if defined(WIN32) && !defined(__CYGWIN__) && !defined(__WATCOMC__)
// to
//#  if defined(WIN32) && !defined(__CYGWIN__) && !defined(__WATCOMC__) && !defined(__WOE32__)

#include <dirent.h>

#include "flarq.h"
#include "arq.h"
#include "arqdialogs.h"
#include "b64.h"

Fl_Help_Dialog *help_dialog = (Fl_Help_Dialog *)0;

void help_cb() {
  if (!help_dialog) {
    help_dialog = new Fl_Help_Dialog();

    help_dialog->value(
	"<HTML>\n"
	"<HEAD>\n"
	"<TITLE>Flarq Help</TITLE>\n"
	"</HEAD>\n"
	"<BODY BGCOLOR='#ffffff'>\n"

"<H2>Initiating an ARQ connect session</H2>\n"

"<P>Start by sending a 'CQ NBEMS' or some similar unique way of indicating\n"
"that you are seeking to send ARQ traffic.  Do this from the digital modem\n"
"program and not from flarq.  The potential station for receiving your ARQ\n"
"traffic will answer in the clear.  Negotiate what digital mode you will use\n"
"for the ARQ connection; ie: PSK-63, PSK-125, PSK-250, MFKS-16 etc.  Then\n"
"try that mode without ARQ to be sure that QRN and QSB will not seriously\n"
"disrupt the connection.  Ask the responding station to send an ARQ beacon\n"
"using flarq.  You will then see his ARQ callsign appear in the callsign\n"
"window.</P>"

"<P>Click the CONNECT button to connect with that station.  The text next to\n"
"the diamond will change to CONNECTING and remain that way during the connect\n"
"time out period.  During the connection process the CONNECT button will be\n"
"disabled (greyed out).</P>\n"

"<P>After a connection has been established the button label changes to\n"
"'Disconnect' and the text next to the diamond indicator will read CONNECTED.\n"
"Pressing this button will then execute an orderly disconnect from the other\n"
"station and return the program to the CONNECTED state.</P>\n"

"<P>During a file transfer the button's label changes to Abort.  When the\n"
"button says Abort, pressing it will abort the file transfer and the program\n"
"will return to the CONNECTED state.  During the abort text next to the\n"
"diamond indicator will read ABORTING XFR and return to CONNECTED after the\n"
"abort has been fully recognized by both ends of the connection.</P>\n"

"<H2>Beaconing</H2>\n"

"<P>Click the Beacon button to transmit a beacon signal requesting\n"
"assistance with ARQ message forwarding. The small rectangle on the Beacon\n"
"button will turn green when a beacon signal is being sent.  The beacon will\n"
"repeat at the repeat interval (default is 60 seconds).  You should not reduce\n"
"the repeat interval so short as to make it impossible to receive an ARQ\n"
"connection.  This is particularly true on PSK-31.</P>\n"

"<H2>Diamond Indicator</H2>\n"

"<P>The diamond-shaped indicator will be green when ready to transfer messages.\n"
"The ""Not Connected"" label next to the diamond indicator will change to Sending\n"
"when sending, or Connected when connected.</P>\n"

"<H2>Send Menu</H2>\n"

"<P>The Send menu will not be enabled unless a CONNECTION has been established\n"
"with another flarq station.</P>\n"

"<P>This menu accesses four types of files. When selecting any type, the Show: field\n"
"allows you to use the dropdown arrow to choose which type of file to display.</P>\n"

"<P>The area with the question mark is where file content is displayed, if the Preview\n"
"box is checked.</P>\n"

"<P>The Filename field has a row of buttons above it which can be used to quickly\n"
"navigate through the hierarchy of folders shown. Just click the button over the\n"
"folder you want to access.</P>\n"

"<P>When Email is selected, a list of emails waiting to be transferred will be\n"
"displayed. Select an email and click the Send button to start transferring the\n"
"email.</P>\n"

"<P>When Image File is selected, Flarq can send a color, passport photo sized\n"
"picture, in about 10 minutes using PSK250.</P>\n"

"<H2>Config Menu</H2>\n"

"<P>This menu provides a place where you should enter your callsign that Flarq\n"
"will use for transmitting.  Various folders are shown and can be changed, but\n"
"it is recommended that the default folders be used except in special\n"
"circumstances.</P>\n"

"<P>If you are using the Sylpheed mail client you need to check that box.\n"
"Sylpheed uses a different naming convention for storing messages inside of\n"
"it's mail folders.</P>\n"

"<P>The beacon interval is probably the most often changed setting. Use it to\n"
"control how often Flarq sends the beacon text.</P>\n"

"<P>You can enter additional beacon text which will be sent with the each time\n"
"the ARQ beacon is transmited.</P>\n"

"<P>At the bottom left of the Flarq window there is a space on the left side that\n"
"displays messages showing the Flarq status at any given time.</P>\n"

"<P>At the bottom right, there is a space for a progress indicator, which will show\n"
"a moving green bar as a message is transferred. When a transfer is completed, the\n"
"green color will disappear after filling the space, indicating that transfer has\n"
"been completed.</P>\n"

"<H2>Status Bar</H2>\n"

"<P>A notification area in the bar just above the Plain Talk label will show the name\n"
"and size of the file being transferred and how long it took to transfer when the\n"
"transfer is completed. The left and right arrows are for adjusting the number of\n"
"SOH characters preceding each block. Leave it at the default of 10 unless you\n"
"have trouble connecting at high speed, or have too many repeat blocks. Then try\n"
"higher values to reduce the number of repeated blocks.</P>\n"

"<P>Next to the right hand arrow is an area where the quality level of the transfer\n"
"is shown. A transfer without any retries will be shown as 1.00.</P>\n"

"<P>The area next to the Clear button will display a progress indicator, which will\n"
"show the progress of the transfer. When you are sending a message, it will show\n"
"the amount of the message confirmed as being received correctly. When you are on\n"
"the receiving end, it will advance as each message frame is received.</P>\n"

"<P>The Clear button can be used to clear the flarq screen.</P>\n"

"<H2>Plain Talk</H2>\n"

"<P>You can also communicate during, before, or after a file transfer, as long as\n"
"the Connected diamond is green (showing that you are connected to the other\n"
"station), by typing in the box next to the Clear button at the very bottom of\n"
"the flarq window, and pressing Enter. The text you are sending will be shown\n"
"in red in the Plain Talk window and incoming text from the other station will\n"
"be shown in black. Text you type will be sent out at the first opportunity,\n"
"but only after a block completes being sent, so there will be a delay until\n"
"your text appears on the other station's Plain Talk window, and the other\n"
"station responds.  As with most edit controls it is necessary to first put\n"
"the keyboard focus in that box by clicking in it with the mouse.</P>\n"

"<P>The maximum number of characters you can type on the Plain Talk line before\n"
"pressing Enter can be no more than 80 characters. In order to make the speed\n"
"of Plain Talk text exchanges as rapid as possible, Plain Talk uses the current\n"
"mode without any ARQ error checking, so there may be some errors at the\n"
"receiving end that would not occur if ARQ were used.</P>\n"

"<P>The Clear button next to the Plain Talk line can be used to clear the Plain\n"
"Talk display area."
	
"</BODY>\n"
    );
  }

  help_dialog->show();
}

