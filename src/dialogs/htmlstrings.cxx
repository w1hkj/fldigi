// ----------------------------------------------------------------------------
//
//	htmlstrings.cxx
//
// Copyright (C) 2008
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ----------------------------------------------------------------------------

#include <config.h>

char szBeginner[] = "\
<HTML>\n\
<HEAD>\n\
<TITLE>Beginners' Guide to Fldigi</TITLE>\n\
</HEAD>\n\
<BODY BGCOLOR=FFFFCC TEXT=404040 LINK=CC0088 VLINK=995544>\n\
<font size=\"+0\" face=\"Verdana, Arial, Helvetica\">\n\
<CENTER>\n\
<H1>Beginners' Guide to Fldigi</H1>\n\
<P>\n\
<A HREF=\"#Wotuneed\">Requirements</A>&nbsp;\n\
<A HREF=\"#Install\">Installation</A>&nbsp;\n\
<A HREF=\"#Start\">Getting Started</A>&nbsp;\n\
<A HREF=\"#Oper\">Operating</A>&nbsp;\n\
<A HREF=\"#Keys\">Special Keys</A>&nbsp;\n\
\n\
<HR>\n\
</CENTER>\n\
<H2>Beginners' Questions Answered</H2>\n\
<H3><I>What is Fldigi?</I></H3>\n\
<BLOCKQUOTE>\n\
Fldigi is a computer program intended for Amateur Radio Digital Modes operation using a PC (Personal Computer).  \n\
Fldigi operates (as does most similar software) in conjunction with a conventional HF SSB radio transceiver, \n\
and uses the \n\
PC sound card as the main means of input from the radio, and output to the radio. These are audio-frequency \n\
signals. The software also controls the radio by means of another connection, typically a serial port.  \n\
<P>\n\
Fldigi is multi-mode, which means that it is able to operate many popular digital modes without switching \n\
programs, so you only have one program to learn. Fldigi includes all the popular modes, such as DominoEX, MFSK16, \n\
PSK31, and RTTY.  \n\
<P>\n\
Unusually, Fldigi is available for multiple computer operating systems; Linux&trade;, FreeBSD&trade;; OS X&trade; and Windows&trade;.  \n\
</BLOCKQUOTE>\n\
<H3><I>What is a Digital Mode?</I></H3>\n\
<BLOCKQUOTE>\n\
Digital Modes are a means of operating Amateur radio from the computer keyboard. The computer acts as 'modem' (modulator - demodulator), \n\
as well as allowing you to type, and see what the other person types. It also controls the transmitter, changes modes as required, \n\
and provides various convenient features such as easy tuning of signals and prearranged messages.  \n\
<P>\n\
In this context, we are talking \n\
about modes used on the HF (high frequency) bands, specifically 'chat' modes, those used to have a regular \n\
conversation in a similar way to voice or Morse, where one operator 'talks' for a minute or two, then another \n\
does the same. These chat modes allow multiple operators to take part in a 'net'.  \n\
<P>\n\
Because of sophisticated digital signal processing which takes place inside the computer, digital modes can \n\
offer performance that cannot be achieved using voice (and in some cases even Morse), through reduced \n\
bandwidth, improved signal-to-noise performance and reduced transmitter power requirement. Some modes also \n\
offer built-in automatic error correction.  \n\
<P>\n\
Digital Mode operating procedure is not unlike Morse operation, and many of the same abbreviations are used. \n\
Software such as Fldigi makes this very simple as most of the procedural business is set up for you using \n\
the Function Keys at the top of the keyboard. These are easy to learn.  \n\
</BLOCKQUOTE>\n\
<H3><I>Why all the different modes?</I></H3>\n\
<BLOCKQUOTE>\n\
HF propagation is very dependent on the ionosphere, which reflects the signals back to earth. There are strong \n\
interactions between different signals arriving from different paths. Experience has shown that particular \n\
modulation systems, speeds and bandwidths suit different operating conditions.  \n\
<P>\n\
Other factors such as available band space, operating speed and convenience, noise level, signal level and \n\
available power also affect the choice of mode. While in many cases several different modes might be suitable, \n\
having a choice adds to the operating pleasure. It is difficult to advise which mode is best for each particular \n\
occasion, and experience plays an important role. \n\
<P>\n\
You might consider purchasing '<I>Digital Modes for All Occasions</I>' \n\
(ISBN 1-872309-82-8) by Murray Greenman ZL1BPU, published by the <A HREF=\"http://www.rsgbshop.org/\">RSGB</A>, \n\
as this gives a good insight into each mode and its capabilities. The book is also available from FUNKAMATEUR and CQ Communications.  \n\
You might also want to purchase the ARRL's <I>HF Digital Handbook</I>(ISBN 0-87259-103-4) by Steve Ford, WB8IMY.  \n\
<P>\n\
</BLOCKQUOTE>\n\
<H3><I>How do I recognise and tune in the signals?</I></H3>\n\
<BLOCKQUOTE>\n\
Recognising the different modes comes with experience. It is a matter of listening to the signal, and observing \n\
the appearance of the signal on the tuning display. You can also practice transmitting with the transceiver \n\
disconnected, listening to the sound of the signals coming from the computer. There is also (see later paragraph) \n\
an automatic tuning option which can recognise and tune in most modes for you.  \n\
<P>\n\
The software provides a tuning display which shows the radio signals that are receivable within the transceiver \n\
passband. Using a 'point and click' technique with the mouse, you can click on the centre of a signal to select it, \n\
and the software will tune it in for you. Some modes require more care than others, and of course you need to have \n\
the software set for the correct mode first - not always so easy!  \n\
<P>\n\
The 'RSID' (automatic mode detection and tuning) feature uses a special sequence of tones transmitted at the beginning of each transmission to identify \n\
and tune in the signals received. For this feature to work, not only do you need to enable the feature in the receiver, \n\
but in addition the stations you are wishing to tune in need to have this feature enabled on transmission. Other \n\
programs also offer this RSID feature as an option.  \n\
</BLOCKQUOTE>\n\
<H3><I>Where can I find detailed instructions for Fldigi?</I></H3>\n\
<BLOCKQUOTE>\n\
Of necessity, this Beginner's Guide contains only as much as you need to know to get started. You can always read the \n\
details and learn how to make best use of the program by reading the <A HREF=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</A>. You can also access it from within the Fldigi program from the HELP Menu item. \n\
</BLOCKQUOTE>\n\
<A NAME=\"Wotuneed\">\n\
<HR>\n\
<H2>Requirements</H2>\n\
<BLOCKQUOTE>\n\
<DL>\n\
<DT><B>Computer</B></DT>\n\
<DD>Pentium&trade; 3 or 4, Celeron&trade;, or equivalent, 750MHz or better, 256MB RAM or more.  Faster computers will give \n\
better performance.</DD> \n\
<DT><B>Operating System</B></DT>\n\
<DD>Linux distributions such as Debian&trade;, Ubuntu&trade;, Fedora&trade;, openSUSE&trade;, Mandriva&trade;, \n\
Puppy Linux&trade;; FreeBSD&trade;; OS X&trade;; and Windows&trade; XP SP2 (Home or Pro), Windows Vista&trade;. Fldigi can be home compiled for other distributions. Fldigi does not support \n\
Windows 98&trade;.</DD> \n\
<DT><B>Other Requirements</B></DT> \n\
<DD><LI>Computer serial port (or USB serial port) for rig control \n\
<LI>Optional serial CAT (Computer Aided Tuning) computer control \n\
<LI>About 5MB of drive space is required for program files \n\
<LI>Internet connection will allow direct connection to the online help files and your online callsign server subscription.  \n\
<LI>A radio interface to the sound card and serial port, such as the RigBlaster&trade; or similar. You can make one yourself.  \n\
<LI>A modern HF SSB transceiver, with or without CAT control. For some modes, frequency stability is very important \n\
(preferably less than 1Hz drift per over), and the difference between transmit and receive frequencies should also \n\
be low (less than 1Hz). Tuning steps should be 100Hz or less. Most commercial synthesized transceivers made in the last \n\
20 years will meet these requirements.  \n\
</DD></DL>\n\
</BLOCKQUOTE>\n\
<A NAME=\"Install\">\n\
<HR>\n\
<H2>Installation</H2>\n\
<H3><I>Installing</I></H3>\n\
<BLOCKQUOTE>\n\
<UL><LI>Locate and download the correct archive for your operating system from the <A HREF=\"http://www.w1hkj.com/Fldigi.html\"> \n\
Fldigi web site</A>.  \n\
<LI>Create a suitable folder on your hard drive (for example Programs/Fldigi) and unzip the archive into this folder.  \n\
<LI>Right-click on the executable <B>fldigi.exe</B> (Windows), <B>fldigi</B> (Linux), select 'Create Shortcut', and drag the resulting shortcut either to \n\
the desktop or to a suitable place in the menu structure.  \n\
<LI>Double-click on this new shortcut to start the program. You're in business!  \n\
</UL>\n\
</BLOCKQUOTE>\n\
<H3><I>Updating the Program</I></H3>\n\
<BLOCKQUOTE>\n\
<UL><LI>Delete the shortcut you made during installation \n\
<LI>Repeat the installation process described above, using the same folder. The new program will over-write the old.  \n\
<LI>Make a new shortcut as described above.  \n\
</UL>\n\
</BLOCKQUOTE>\n\
<H3><I>Removing the Program</I></H3>\n\
<BLOCKQUOTE>\n\
<UL><LI>Delete the shortcut you made during installation \n\
<LI>Locate the program folder you made during installation, delete the contents, and then delete the folder.  \n\
</UL>\n\
</BLOCKQUOTE>\n\
<H3><I>Setting Up</I></H3>\n\
<BLOCKQUOTE>\n\
<UL><LI>Use the menu <B>Configure/Sound Card</B>, <B>Audio/Devices</B> tab, to select the sound card you wish to use.  \n\
You can ignore the other tabs for now.  \n\
<LI>Use the menu <B>Configure/Operator</B> item to set the user name, callsign, locator and so on.  \n\
<LI>Use the menu <B>Configure/Rig Control</B> item to set how you will control the rig.  \n\
If you will key the rig via a serial port, in the <B>Hardware PTT</B> tab select 'Use serial port PTT', the device name you will use, \n\
and which line controls PTT.  If in doubt, check both RTS and DTR. You MUST then press the <B>Initialize</B> button.  \n\
<LI>If you plan to use CAT control of the rig via the COM port, check 'Use rigCAT' in the <B>RigCAT</B> tab. If in addition you wish to use \n\
PTT control via CAT, also then check 'PTT via CAT command'. You MUST then press the <B>Initialize</B> button.  \n\
<LI>Use the menu <B>Configure/UI</B>, <B>Restart</B> tab, to set the aspect ratio of the waterfall display and whether or not you want \n\
to dock a second digiscope to the main window.\n\
<LI>Use the menu <B>Configure/IDs</B> item to set whether you wish to transmit RSID data at the start of each \n\
over (this is for the benefit of others, this setting does not affect RSID reception). \n\
If you plan to regularly use the RSID feature on receive, you should deselect the option that starts new modems \n\
at the 'Sweet Spot' frequencies in <B>Misc/Sweet Spot</B>.  \n\
<LI>If you have a slow computer (under 700MHz), select 'Slow CPU' under <B>Configure/Misc/CPU</B>. \n\
The receiver decoding strategy of certain modems uses fewer processor cycles in this mode. \n\
<LI>Each of the modems can be individually set up from the <B>Configure/Modems</B> multi-tabbed dialog.  \n\
You need not change anything here to start with, although it might be a good idea to set the 'secondary text' for DominoEX \n\
and THOR to something useful, such as your call and locator. (Secondary text is transmitted when the text \n\
you type does not keep up with the typing speed of the mode - this handy text appears in a small window at the very bottom of the screen).  \n\
Note that this set of tabs is also where you set the RTTY modem speed and shift, although the default values should be fine \n\
for normal operation.  \n\
<LI>Use the menu <B>Configure/Save Config</B> item to save the new configuration.  \n\
<LI>Use your sound card 'Master Volume' applet to select the sound card, the Wave output and set the transmit audio level.  \n\
You can check the level using the <B>TUNE</B> button, top right, beyond the Menu.  \n\
<LI>On Windows, the 'Volume' applet can usually be opened by <B>START/Run...</B> and enter '<B>sndvol32'</B>, or from the Control Panel.  \n \
<LI>Use your sound card 'Recording Control' applet to select the sound card, the line or mic input and set the receiver audio level.  \n\
Watch the waterfall display for receiver noise when setting the level. If you see any dark blue noise, you have the right input \n\
and about the right level. The actual setting is not very important, provided you see blue noise. If the audio level is too high, \n\
the little diamond shaped indicator (bottom right) will show <B><FONT COLOR=RED>red</FONT></B>. The waterfall may also show red bands.  \n\
Performance will be degraded if the level is too high.  \n\
<LI>On Windows, the 'Record' applet can usually be opened by <B>START/Run...</B> and enter '<B>sndvol32 /r</B>', or from the Control Panel.  \n \
If opened from the Control Panel, you'll end up with the Master Volume applet, and need to switch using Options/Properties, and selecting \n\
the 'Recording' radio button.  \n\
</UL>\n\
</BLOCKQUOTE>\n\
<A NAME=\"Start\">\n\
<HR>\n\
<H2>Getting Started</H2>\n\
<H3><I>Guided Tour</I></H3>\n\
<BLOCKQUOTE>\n\
Double-click on the Fldigi shortcut to start the program. A window with three main panes will appear.  \n\
Study it carefully as you read these notes. From top to bottom, these are the <B>RECEIVE</B> pane \n\
(<B>navaho white</B>), the <B>TRANSMIT</B> pane (<B>light cyan</B>), and the <B>WATERFALL</B> pane (<B>black</B>). At the top is \n\
the collection of entry items which form the <B>LOG DATA</B>, and at the very top, a \n\
conventional drop-down <B>MENU</B> system, with entries for <U>F</U>ile, Op <U>M</U>ode, Configure, View and Help.  \n\
<P>\n\
Between the RECEIVE and TRANSMIT panes is a line of boxes (buttons), which represent the Function Keys F1 - F12.  \n\
We call this the <B>FUNCTIONS</B> group.  \n\
Below the WATERFALL pane is another line of boxes (buttons), which provide various control features. We call this the \n\
<B>CONTROLS</B> group. The program and various buttons can mostly be operated using the mouse or the keyboard, and users \n\
generally find it convenient \n\
to use the mouse while tuning around, and the keyboard and function keys during a QSO.  \n\
</BLOCKQUOTE>\n\
<H3><I>RECEIVE Pane</I></H3>\n\
<BLOCKQUOTE>\n\
This is where the text from decoded incoming signals is displayed, in <B><FONT COLOR=black>black</FONT></B> text.  \n\
When you transmit, the transmitted text is also displayed here, but in <B><FONT COLOR=red>red</FONT></B>, so \n\
the RECEIVE pane becomes a complete record of the QSO. The information in this pane can also be logged to a file.  \n\
<P>\n\
The line at the bottom of this pane can be dragged up and down with the mouse. You might prefer to drag it down a bit \n\
to enlarge the RECEIVE pane and reduce the size of the TRANSMIT pane.  \n\
</BLOCKQUOTE>\n\
<H3><I>TRANSMIT Pane</I></H3>\n\
<BLOCKQUOTE>\n\
This is where you type what you want to transmit. The mouse must click in here before you type (to obtain 'focus') otherwise \n\
your text will go nowhere. You can type in here while you are receiving, and when you start transmitting, the text already typed \n\
will be sent first. This trick is a cool way to impress others with your typing speed! As the text is transmitted, the text colour \n\
changes from <B><FONT COLOR=black>black</FONT></B> to <B><FONT COLOR=red>red</FONT></B>. At the end of the over, all the transmitted \n\
text (and any as yet not transmitted) will be deleted.  \n\
</BLOCKQUOTE>\n\
<H3><I>WATERFALL Pane</I></H3>\n\
<BLOCKQUOTE>\n\
This is the main tuning facility. There are two modes, 'Waterfall' and 'FFT', selected by a button in the CONTROL group. For now, \n\
leave it in 'Waterfall' mode, as this is the easiest to tune with, and gives the best identification of the signal.  \n\
<UL><LI>'Waterfall' is a spectrogram display, of signal strength versus frequency, over passing time. The receiver passband \n\
is analysed and displayed with lower frequencies to the left, higher to the right. Weak signals and background noise \n\
are dark while stronger signals show as brighter colours. As time passes (over a few seconds), the historic signals move downwards like a waterfall.  \n\
<LI>'FFT' is a spectrum display, simply the mean signal strength versus frequency. Again frequency is displayed from left to right, \n\
but now the vertical direction shows signal strength and there is no brightness or historic information.</UL> \n\
<P>\n\
At the top of the pane is a scale of frequency in Hz, which corresponds to the frequency displayed immediately below it. This \n\
scale can be moved around and zoomed using buttons in the CONTROL group.  \n\
<P>\n\
As you move the mouse around in this pane you will see a <B><FONT COLOR=CCCC00>yellow</FONT></B> group of tuning marks following the mouse pointer.  \n\
Tuning is achieved by left-clicking on a signal displayed by the waterfall in this pane. Use these yellow marks to \n\
exactly straddle the signal and then left-click on the centre of the signal. The tuning marks change to \n\
<B><FONT COLOR=red>red</FONT></B>. The <B><FONT COLOR=red>red</FONT></B> vertical lines will show the approximate \n\
width of the active signal area (the expected signal bandwidth), while a <B><FONT COLOR=red>red</FONT></B> \n\
horizontal bar above will indicate the receiver software's active decoding range. When you left-click, the red \n\
marks move to where you clicked, and will attempt to auto-track the signal from there.  \n\
<P>\n\
You can temporarily 'monitor' a different signal by right-clicking on it. As long as you hold the mouse button down, the signal under \n\
it will be decoded; as soon as you release the mouse, decoding will revert to the previously tuned spot (where the red marks are).  \n\
</BLOCKQUOTE>\n\
<H3><I>LOG Data</I></H3>\n\
<BLOCKQUOTE>\n\
Using this group of entry boxes, you can keep a log of your QSOs. At the left are two '<B>Frequency</B>' boxes.  \n\
If you use CAT control to operate your transceiver, the dial frequency is recorded automatically in the lower box; \n\
otherwise you can type it manually or select (button to the right) from a list of common frequencies. The audio frequency from the waterfall \n\
is added to this value and displayed in the upper box - assuming your rig is calibrated correctly, this is the true \n\
centre frequency of the station you are in QSO with. You can't type in the upper box.  \n\
<P>\n\
The program is able to switch sidebands (the '<B>U</B>' or '<B>L</B>' button under the '<B>Time</B>' display), \n\
so if your rig is in LSB mode, the signals are inverted for you, and when you \n\
add the frequency to the log information, the audio frequency is subtracted rather than added, so again the true transmit \n\
frequency is displayed and logged.  \n\
<P>\n\
The '<B>Time</B>' box can be typed in, or you can push the adjacent button to insert the current time. All times are in UTC, \n\
and the computer knows how to calculate this from the PC local civil time, time zone and summer-time setting. Clever stuff!  \n\
<P>\n\
You can type the other station's callsign and name in the '<B>Call</B>' and '<B>Name</B>' boxes, or right-click \n\
on the appropriate word in the RECEIVE pane to insert them automatically. The same applies to the '<B>QTH</B>', \n\
'<B>LOC</B>' (locator) and received signal report '<B>RST In</B>'. The other entries must be added manually.  \n\
If you have access to an appropriate Callbook on-line subscription or CD, pressing the '<B>QRZ</B>' button will fetch \n\
this information for you automatically.  \n\
<P>\n\
The '<B>Clear</B>' button clears all the log data; the '<B>Save</B>' button sends the logged information to the log file.  \n\
</BLOCKQUOTE>\n\
<H3><I>MENU</I></H3>\n\
<BLOCKQUOTE>\n\
At the very top of the program window is a conventional drop-down menu. If you click on any of the items, a list of optional \n\
functions will appear. Keyboard menu selection is also provided.  \n\
Where <U>underscored</U> characters are shown in the menu, you can select these menu items from the keyboard \n\
using the marked character and &lt;<B>Alt</B>&gt; at the same time, then moving around with the up/down/left/right keys.  \n\
Use &lt;<B>Esc</B>&gt; to quit from the menu with no change.  \n\
These menu functions are: \n\
</BLOCKQUOTE>\n\
<DL><DT><B><U>F</U>ile</B></DT>\n\
<DD>Allows you to open or save Macros (we won't get into that here), turn on/off logging to file, record/play audio samples, and exit the program. You can also exit the program by clicking on the 'X' in the top right corner of the window, in the usual manner.</DD> \n\
<DT><B>Op <U>M</U>ode</B></DT>\n\
<DD>This is where you select the operating modem used for transmission and reception. Some modes only have one option. Where more are \n\
offered, drag the mouse down the list and sideways following the arrow to a secondary list, before releasing it. When you start the \n\
program next time, it will remember the last mode you used.  \n\
<P>\n\
Not all the modes are widely used, so choose a mode which (a) maximizes your chance of a QSO, and (b) is appropriate for \n\
the band, conditions, bandwidth requirements and permissions relevant to your operating licence.  \n\
<P>\n\
At the bottom of the list are two 'modes' which aren't modes at all, and do not transmit (see \n\
<A HREF=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</A> for details). <B>WWV</B> mode allows you to receive a standard time \n\
signal so the beeps it transmits can be used for sound card calibration. <B>Freq Analysis</B> provides just a waterfall display with a very narrow cursor, and a frequency meter \n\
which indicates the received frequency in Hz to two decimal places. This is useful for on-air frequency measurement.</DD>  \n\
<DT><B>Configure</B></DT>\n\
<DD>This is where you set up the program to suit your computer, yourself and your operating preferences. The operating settings of the program are grouped into several categories and there are menu items in which you enter your personal information, or define your computer sound card, for example.  \n\
<B>Modems</B> can be individually changed, each having \n\
different adjustments. The <B>Modems</B> dialog has multiple tabs, so you can edit any one of them. Don't fool with \n\
the settings until you know what you are doing! The final item, <B>Save Config</B> allows you to save the altered configuration \n\
for next time you start the program (otherwise changes are temporary).  \n\
<DT><B>View</B></DT>\n\
<DD>This menu item allows you to open extra windows. Most will be greyed out, but two that are available are the <B>Digiscope</B>, \n\
and the <B>PSK Browser</B>. The <B>Digiscope</B> provides a mode-specific graphical analysis of the received signal, and \n\
can have more than one view (left click in the new window to change the view), or maybe none at all. The <B>PSK Browser</B> \n\
is a rather cool tool that allows you to monitor several PSK31 signals all at the same time! These windows can be resized to suit.</DD> \n\
<DT><B>Help</B></DT>\n\
<DD>Brings up the <A HREF=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</A>, the Fldigi \n\
<A HREF=\"http://www.w1hkj.com/Fldigi.html\">Home Page</A>, and various information about the program.</DD> \n\
</DL>\n\
The two non-menu functions are:\n\
<DL><DT><B>RSID</B></DT>\n\
<DD>This button turns on the receive RSID (automatic mode detection and tuning) feature. When in use, the button turns yellow and no text reception is possible until \n\
a signal is identified, or the feature is turned off again. If you plan to use the RSID feature on receive, \n\
you must leave the 'Start New Modem at Sweet Spot' item in the Menu Configure/Defaults/Mics tab unchecked.</DD>\n\
<DT><B>TUNE</B></DT>\n\
<DD>This button transmits a continuous tone at the current audio frequency. The tone level will be at the maximum signal level for any modem, \n\
which makes this function useful for adjusting your transceiver's output power.</DD>\n\
</DL>\n\
<H3><I>FUNCTIONS</I></H3>\n\
<BLOCKQUOTE>\n\
This line of buttons provides user-editable QSO features. For example, the first button on the left sends CQ for you. Both the \n\
function of these buttons (we call them <B><I>MACROS</I></B>) and the label on each button, can be changed. Select each button \n\
to use it by pressing the corresponding Function Key (F1 - F12, you'll notice the buttons are grouped in patterns four to a group, \n\
just as the Function Keys are). You can also select them with a left-click of the mouse. If you right-click on the button, you \n\
are able to edit the buttons label and its function. A handy dialog pops up to allow this to be done. There are many standard \n\
shortcuts, such as &lt;MYCALL&gt; which you can use within the Macros. Notice that the buttons also turn the transmitter on \n\
and off as necessary.  \n\
<P>\n\
You can just about hold a complete QSO using these buttons from left to right (but please don't!). Notice that at the right are \n\
two spare buttons you can set as you wish, and then a button labelled '1'. Yes, this is the first set of FOUR sets of Macros, \n\
and you can access the others using this button, which changes to read '2', '3', '4' then '1' again (right-click to go backwards), \n\
or by pressing &lt;<B>Alt</B>&gt; and the corresponding number (1-4, not F1-F4) at the same time.  \n\
<P>\n\
If you REALLY mess up the Macros and can't see how to fix them, just close the program without saving them, and reopen it.  \n\
</BLOCKQUOTE>\n\
<H3><I>CONTROLS</I></H3>\n\
<BLOCKQUOTE>\n\
The line of buttons under the waterfall is used to control the program (as opposed to the QSO). If you hover the mouse over \n\
these buttons, you'll see a little yellow hint box appear which tells you what each button does.  \n\
<P>\n\
The first button switches between Waterfall and FFT modes. The next two buttons adjust the signal level over which the waterfall works. \n\
The default range is from 0dB downwards 70dB (i.e. to -70dB). Both of these values can be adjusted to suit your sound card and receiver audio level.\n\
<P>\n\
The next button sets the scale zoom factor (visible display width, x1, x2 or x4), and the next three buttons move the visible waterfall \n\
area in relation to the bandwidth cursor.\n\
<P>\n\
The next button selects the waterfall speed. NORM or SLOW setting is best unless you have a very fast computer.\n\
<P>\n\
The next four buttons (two on either side of a number, the audio frequency in Hz) control the receiving frequency (they move the red cursor lines).\n\
<P>\n\
The <B>QSY</B> button moves the signal under the bandwidth cursor to a preset audio frequency (typically, the centre of the transceiver's passband). \n\
The <B>Store</B> button allows you to store or recall the current frequency and mode. See the \n\
<A HREF=\"http://www.w1hkj.com/FldigiHelp/OperatingControls.html\">Online Documentation</A> for details on these functions.\n\
<P>\n\
The <B>Lk</B> button locks the transmit frequency (fixes the red cursors), and the <B>Rv</B> button turns \n\
the signal decoding upside down (some modes are sideband sensitive, and if they are the wrong way up, can't be received \n\
correctly). Remember to turn this one off when you're done, or you won't receive anything! If every signal you hear is upside \n\
down, check your transceiver sideband setting.  \n\
<P>\n\
The <B>T/R</B> button forces the transmitter on or off - use this with care, as it will stop transmission immediately, \n\
losing whatever is in the buffer (what you have typed in the Transmit pane), or start it immediately, even if nothing is ready to transmit.  \n\
<P>\n\
There are two further controls in the bottom right corner of the program, to the right of the Status line: \n\
</BLOCKQUOTE>\n\
<DL><DT><B>AFC</B> - The AFC control</DT>\n\
<DD>When this button is pressed, an indicator on the button turns yellow, and the program will automatically retune to drifting signals.  \n\
When the button is again pressed, AFC is off, and the tuning will stay where you leave it.</DD>  \n\
<DT><B>SQL</B> - The Squelch control</DT>  \n\
<DD>When off (no coloured indicator on the button, the receiver displays all 'text' received, even if there is no signal present, \n\
and the receiver is simply attempting to decode noise. When activated by pressing the button, the indicator turns yellow.  \n\
If the incoming signal strength exceeds that set by the adjacent slider control (above the <B>SQL</B> button), the indicator \n\
turns green and the incoming signal is decoded and printed. The signal strength is indicated on the green bar beside the \n\
Squelch level slider. If nothing seems to be printing, the first thing to do is check the Squelch!</DD>  \n\
</DL>\n\
</BLOCKQUOTE>\n\
<H3><I>STATUS Line</I></H3>\n\
<BLOCKQUOTE>\n\
At the very bottom line of the Fldigi window is a row of useful information. At the left is the current operating mode. Next (some modes) \n\
is the measured signal-to-noise ratio at the receiver, and (in some modes) the measured signal intermodulation level (IMD).  \n\
<P>\n\
The larger central box shows (in DominoEX and THOR modes) the received 'Secondary Text'. This is information (such as \n\
station identification) which is transmitted automatically whenever the transmitter has completed all user text that is \n\
available to send. It is transmitted using special characters, and is automatically directed to this special window. Secondary text \n\
you transmit is also shown here. This box changes size when you enlarge the program window.  \n\
</BLOCKQUOTE>\n\
<A NAME=\"Oper\">\n\
<HR>\n\
<H2>Operating</H2>\n\
<H3><I>Procedure</I></H3>\n\
<BLOCKQUOTE>\n\
Operating procedure for digital modes is similar to that for Morse. Some of the same abbreviations are used. For example, at the beginning \n\
of an over, you might send 'VK3XYZ de WB8ABC' or just 'RR Jack' and so on. At the end of an over, it is usual to send 'ZL1ABC de AA3AR K', \n\
and at the end of a QSO '73 F3XYZ de 3D2ZZ SK'. When operating in a group or net it is usual to sign 'AA3AE es gp de ZK8WW K'.  \n\
<P>\n\
It is also considered a courtesy to send a blank line or two (press &lt;<B>Enter</B>&gt;) \n\
before any text at the start of an over, and following the last text at the end of an over. You can also place these in the macros.  \n\
The purpose is to separate your text from the previous text, and especially from any rubbish that was printed between overs.  \n\
<P>\n\
Fldigi does all of this for you. The Function Keys are set up to provide these start and end of over facilities, and can be edited \n\
to suit your preferences. In \n\
order that the other station's callsign can appear when these keys are used, you need to set the other station's callsign in the log \n\
data - it does not matter if you use the log facility or not.  \n\
<DL>\n\
<DT><B>Hint:</B> Some Function Key Macro buttons have graphic symbols on them which imply the following:<BR> \n\
<DD><B>&gt;&gt;</B>&nbsp;&nbsp;The transmitter comes on and stays on when you use this button/macro.  \n\
<DD><B>||</B>&nbsp;&nbsp;&nbsp;The transmitter goes off when the text from this button/macro has been sent.  \n\
<DD><B>&gt;|</B>&nbsp;&nbsp;The transmitter comes on, sends the text from this button/macro, and goes off when the text from this button/macro has been sent.  \n\
</DL>\n\
\n\
The Macros are set up to control the transmitter as necessary, but you can also switch the transmitter on at the start of an over with &lt;<B>Ctrl</B>&gt; \n\
and <B>T</B> or the <B>TX</B> macro button, and off again with &lt;<B>Ctrl</B>&gt; and <B>R</B> or the <B>RX</B> macro button. If you \n\
have Macros copied into or text already typed in the Transmit pane when you start the transmitter, this is sent first. \n\
<P>\n\
Calling another station you have tuned in is as simple as pushing a button. Put his callsign into the log data (right click, select Call) \n\
and press the <B>ANS</B> Macro button (or <B>F2</B>) when you are ready. If he replies, you are in business! Then press <B>QSO</B> (<B>F3</B>) \n\
to start each over, and <B>BTU</B> (<B>F4</B>) to end it, and <B>SK</B> (<B>F5</B>) to sign off.  \n\
<DL>\n\
<DT><B>Hint:</B> When typing text, the correct use of upper and lower case is important:<BR> \n\
<DD><LI>Modes such as RTTY and THROB have no lower case capability.  \n\
<DD><LI>In most other modes, excessive use of upper case is considered impolite, like SHOUTING!  \n\
<DD><LI>Modes such as PSK31, MFSK16, DominoEX and THOR use character sets which are optimized for lower case.  \n\
You should use lower case as much as possible in these modes to achieve maximum text speed. In these modes upper case \n\
characters are noticeably slower to send and also slightly more prone to errors.  \n\
</DL>\n\
</BLOCKQUOTE>\n\
<H3><I>Adjustment</I></H3>\n\
<BLOCKQUOTE>\n\
Most digital modes do not require much transmitter power, as the receiver software is very sensitive. Many modes (PSK31, THROB, \n\
MT63) also require very high transmitter linearity, which is another reason to keep transmitter power below 30% of maximum.  \n\
Some modes (Hellschreiber, Morse) have high peak power output, which may not indicate well on the conventional power meter, \n\
another reason to keep the average transmitted power low to prevent a very broad signal being transmitted.  \n\
<P>\n\
Adjust the transmitter output power using the <B>TUNE</B> button, top right, beyond the Menu. The output will be the same as the peak \n\
power in other modes. Adjust the master Volume applet Wave Out and Master Volume controls to achieve the appropriate power.  \n\
Use of excessive drive will result in distortion (signal difficult to tune in, and often poorer reception) and a very broad signal.  \n\
<P>Some multi-carrier modes (MT63 for example) may require individual adjustment as the average power may be rather low.  \n\
<DL>\n\
<DT><B>Hint:</B> Where possible, use the area above 1200Hz on the waterfall.  \n\
<DD><LI>Below 1200Hz the second harmonic of the transmitted audio will pass through the transmitter filters.  \n\
<DD><LI>When using lower frequency tones, adjust the transmitter and audio level with great \n\
care, as the second (and even third) harmonic will appear in the transmitter passband, causing excessive signal width. \n\
<DD><LI>A narrow (CW) filter in the rig is no help in this regard, as it is only used on receive. When you do use a narrow filter, this will restrict the area over which the receiver and transmitter will operate (without retuning of course). Try adjusting the passband tuning (if available).  \n\
<DD><LI>Keep the sound card audio level to a minimum and set the transmitter gain to a similar level used for SSB.\n\
</DL>\n\
</BLOCKQUOTE>\n\
<H3><I>Waterfall Tuning</I></H3>\n\
<BLOCKQUOTE>\n\
When using this program, as with most other digital modes programs, tuning is generally accomplished by leaving the \n\
transceiver VFO at a popular spot (for example 14.070MHz, USB), and performing all the 'tuning' by moving around within \n\
the software.  \n\
<P>\n\
The Fldigi software has a second 'VFO' which is tuned by clicking on the waterfall. On a busy band, you \n\
may see many signals at the same time (especially with PSK31 or Morse), and so you can click with the mouse on \n\
any one of these signals to tune it in, receive it, and if the opportunity allows, reply to the station.  \n\
<P>\n\
The software 'VFO' operates in a transceive mode, so the transmitter signal is automatically and exactly tuned to the \n\
received frequency. If you click correctly on the signal, your reply will always be in tune with the other station.  \n\
<DL>\n\
<DT><B>Hint:</B> You <B>MUST NOT</B> use RIT (Clarifier) when using digital modes.  \n\
<DD><LI>With RIT on, you will probably have to retune after every over.  \n\
<DD><LI>Use of the RIT will also cause the other station to change frequency, and you will chase each other \n\
across the band.  \n\
<DD><LI>Older transceivers without digital synthesis may have an unwanted offset (frequency difference) \n\
between transmit and receive frequencies. Such rigs should not be used for digital modes.  \n\
</DL>\n\
Wider digital modes (MT63, Olivia) can be tuned using the rig if necessary, as tuning is not at all critical.  \n\
The software tuning still operates, but because the signal is so wide, there is limited ability to move \n\
around in the waterfall tuning.  \n\
</BLOCKQUOTE>\n\
<A NAME=\"Keys\">\n\
<H3><I>Special Keys</I></H3>\n\
<BLOCKQUOTE>\n\
Several special keyboard controls are provided to make operating easier. \n\
</BLOCKQUOTE>\n\
<DL><DT><B>Pause Transmission</B></DT>\n\
<DD>\n\
Press &lt;<B>Pause/Break</B>&gt; while in receive, and the program will switch to transmit mode. It will continue \n\
with the text in the transmit buffer (the Transmit pane text) from the current point, i.e. where the red (previously sent) text ends and \n\
the black (yet to be sent) text begins. If the buffer only contains unsent text, then it will begin at the first \n\
character in the buffer.  If the buffer is empty, the program will switch to transmit mode, and depending on the \n\
mode of operation, will send idle characters or nothing at all until characters are entered into the buffer.  \n\
<P>\n\
If you press &lt;<B>Pause/Break</B>&gt; while in transmit mode, the program will return to receive mode. There \n\
may be a slight delay for some modes like MFSK, PSK and others, that requires the transmitter to send a postamble \n\
at the end of a transmission.  The transmit text buffer stays intact, ready for the &lt;<B>Pause/Break</B>&gt; \n\
key to return you to the transmit mode .  \n\
<P>\n\
Pressing &lt;<B>Alt/Meta</B>&gt; and <B>R</B> has the same effect as &lt;<B>Pause/Break</B>&gt;.  \n\
You could think of the &lt;<B>Pause/Break</B>&gt; key as a software break-in capability.</DD> \n\
<P>\n\
<DT><B>ESCAPE</B></DT>\n\
<DD>\n\
Pressing &lt;<B>Esc</B>&gt; while transmitting will abort the transmission. Transmission stops as soon as possible, \n\
(any necessary postamble is sent), and the program returns to receive. Any unsent text in the transmit \n\
buffer will be lost.  \n\
<P>\n\
If you press &lt;<B>Esc</B>&gt; &lt;<B>Esc</B>&gt; (i.e. twice in quick succession), \n\
transmission stops immediately, (without sending any postamble), and the program returns to receive. Any unsent \n\
text in the transmit buffer will be lost. Use this feature as an <FONT COLOR=RED><B>EMERGENCY STOP</B></FONT>.  \n\
<P>\n\
<DT><B>RETURN to Receive</B></DT>\n\
<DD>Press &lt;<B>Ctrl</B>&gt; and <B>R</B> to insert the <B>^r</B> command in the transmit buffer at the current typing \n\
point. When transmission reaches this point, transmission will stop. The transmission does not stop immediately.  \n\
<P>\n\
<DT><B>START Transmission</B></DT>\n\
<DD>Press &lt;<B>Ctrl</B>&gt; and <B>T</B> to start transmission if there is text ready in the transmit buffer.  \n\
</DD><P>\n\
<P>\n\
<DT><B>MOVE Typing Cursor</B></DT>\n\
<DD>Press &lt;<B>Tab</B>&gt; to move the cursor (typing insertion point) to the end of the transmit buffer.  \n\
This will also pause transmission. A &lt;<B>Tab</B>&gt; press at that position moves the cursor back to the \n\
character following the last one transmitted. Morse operation is slightly different. See the on-line help for \n\
<A HREF=\"http://www.w1hkj.com/FldigiHelp/CW.html\">CW</A>.  \n\
</DD><P>\n\
<P>\n\
<DT><B>SEND any ASCII character</B></DT>\n\
<DD>Press &lt;<B>Ctl</B>&gt; and (at the same time) any three-digit number (on the numeric keypad) to \n\
insert the ASCII character designated by that entry value into the transmit buffer. For example, \n\
&lt;<B>Ctl</B>&gt;<B>177</B> is &plusmn; (plus/minus) and &lt;<B>Ctl</B>&gt;<B>176</B> is &deg; (degree).  If you press a key other than the numeric keypad's 0 - 9 the sequence will be discarded.  \n\
You can also use the <B>Ctl</B> with the normal numeric keys.\n\
</DL>\n\
<HR>\n\
<SMALL><CENTER><B>\n\
Version " PACKAGE_VERSION "<BR>\n\
Copyright &copy 2008 M. Greenman.\n\
</CENTER>\n\
\n\
</BODY>\n\
</HTML>\n\
<html><body></body></html>\n\
";

char szAbout[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n\
<html>\n\
<head>\n\
  <title>About</title>\n\
</head>\n\
<BODY BGCOLOR=FFFFCO TEXT=101010>\n\
<font size=\"0\" face=\"Verdana, Arial, Helvetica\">\n\
<CENTER>\n\
<H1><I>Fldigi " PACKAGE_VERSION "</I></H1>\n\
</CENTER>\n\
<P>\n\
<H4>Digital modem program for:</H4>\n\
&nbsp; &nbsp; &nbsp;Linux<br>\n\
&nbsp; &nbsp; &nbsp;FreeBSD<br>\n\
&nbsp; &nbsp; &nbsp;OS X<br>\n\
&nbsp; &nbsp; &nbsp;Windows<br>\n\
<H4>Programmers:</H4>\n\
&nbsp; &nbsp; &nbsp;Dave Freese, W1HKJ<br>\n\
&nbsp; &nbsp; &nbsp;Stelios Bounanos, M0GLD<br>\n\
&nbsp; &nbsp; &nbsp;Leigh Klotz, WA5ZNU<br>\n\
<H4>Beginners' Guide:</H4>\n\
&nbsp; &nbsp; &nbsp;Murray Greenman, ZL1BPU<br>\n\
<br>\n\
<P>\n\
Distributed under the GNU General Public License version 2 or later.<br>\n\
This is free software: you are free to change and redistribute it.<br>\n\
There is NO WARRANTY, to the extent permitted by law.\n\
</body>\n\
</html>\n\
";

