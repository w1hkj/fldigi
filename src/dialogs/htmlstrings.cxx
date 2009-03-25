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
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n\
<html><head><title>Beginners' Guide to Fldigi</title></head>\n\
<body>\n\
<center>\n\
<h1>Beginners' Guide to Fldigi</h1>\n\
<p>\n\
<a href=\"#Setting_Up\">Setting Up</a> &nbsp; <a href=\"#Start\">Quick Tour</a> <a href=\"#Logbook\"></a></p><p><a href=\"#Oper\">Operating</a>&nbsp;\n\
&nbsp;<a href=\"#Logbook\">Logbook</a> &nbsp;&nbsp;<a href=\"#Keys\">Special Keys</a>&nbsp;\n\
</p>\n\
<hr></center>\n\
<h2>Beginners' Questions Answered</h2>\n\
<h3><i>Where can I find detailed instructions for Fldigi?</i></h3>\n\
<blockquote>\n\
Of necessity, this Beginner's Guide contains only as much as you need to\n\
know to get started. You should&nbsp;learn how to\n\
make best use of the program by reading the <a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online\n\
Documentation</a>. You can also access it from within the Fldigi\n\
program from the HELP Menu item.</blockquote>\n\
<h3><i>What is Fldigi?</i></h3>\n\
<blockquote>\n\
Fldigi is a computer program intended for Amateur Radio Digital Modes\n\
operation using a PC (Personal Computer). Fldigi operates (as does most\n\
similar software) in conjunction with a conventional HF SSB radio\n\
transceiver, and uses the PC sound card as the main means of input from\n\
the radio, and output to the radio. These are audio-frequency signals.\n\
The software also controls the radio by means of another connection,\n\
typically a serial port.\n\
<p>Fldigi is multi-mode, which means that it is able to operate many popular\n\
digital modes without switching programs, so you only have one program\n\
to learn. Fldigi includes all the popular modes, such as DominoEX,\n\
MFSK16, PSK31, and RTTY. </p>\n\
<p>Unusually,Fldigi is available for multiple computer operating systems; Linux&#8482;,\n\
FreeBSD&#8482;; OS X&#8482; and Windows&#8482;. </p>\n\
</blockquote>\n\
<h3><i>What is a Digital Mode?</i></h3>\n\
<blockquote>\n\
Digital Modes are a means of operating Amateur radio from the computer\n\
keyboard. The computer acts as 'modem' (modulator - demodulator), as\n\
well as allowing you to type, and see what the other person types. It\n\
also controls the transmitter, changes modes as required, and provides\n\
various convenient features such as easy tuning of signals and\n\
prearranged messages.\n\
<p>In this context, we are talking about modes used on the HF (high\n\
frequency) bands, specifically 'chat' modes, those used to have a\n\
regular conversation in a similar way to voice or Morse, where one\n\
operator 'talks' for a minute or two, then another does the same. These\n\
chat modes allow multiple operators to take part in a 'net'. </p>\n\
<p>Because of sophisticated digital signal processing which takes place inside the\n\
computer, digital modes can offer performance that cannot be achieved\n\
using voice (and in some cases even Morse), through reduced bandwidth,\n\
improved signal-to-noise performance and reduced transmitter power\n\
requirement. Some modes also offer built-in automatic error correction.\n\
</p>\n\
<p>Digital Mode operating procedure is not unlike Morse operation, and many of the\n\
same abbreviations are used. Software such as Fldigi makes this very\n\
simple as most of the procedural business is set up for you using the\n\
Function Keys at the top of the keyboard. These are easy to learn. </p>\n\
</blockquote>\n\
<h3><i>Why all the different modes?</i></h3>\n\
<blockquote>\n\
HF propagation is very dependent on the ionosphere, which reflects the\n\
signals back to earth. There are strong interactions between different\n\
signals arriving from different paths. Experience has shown that\n\
particular modulation systems, speeds and bandwidths suit different\n\
operating conditions.\n\
<p>Other factors such as available band space, operating speed and convenience,\n\
noise level, signal level and available power also affect the choice of\n\
mode. While in many cases several different modes might be suitable,\n\
having a choice adds to the operating pleasure. It is difficult to\n\
advise which mode is best for each particular occasion, and experience\n\
plays an important role.</p>\n\
</blockquote>\n\
<h3><i>How do I recognise and tune in the signals?</i></h3>\n\
<blockquote>\n\
Recognising the different modes comes with experience. It is a matter of listening\n\
to the signal, and observing the appearance of the signal on the tuning\n\
display. You can also practice transmitting with the transceiver\n\
disconnected, listening to the sound of the signals coming from the\n\
computer. There is also (see later paragraph) an automatic tuning\n\
option which can recognise and tune in most modes for you.\n\
<p>The software provides a tuning display which shows the radio signals that\n\
are receivable within the transceiver passband. Using a 'point and\n\
click' technique with the mouse, you can click on the centre of a\n\
signal to select it, and the software will tune it in for you. Some\n\
modes require more care than others, and of course you need to have the\n\
software set for the correct mode first - not always so easy! </p>\n\
<p>The RSID (automatic mode detection and tuning) feature uses a special\n\
sequence of tones transmitted at the beginning of each transmission to\n\
identify and tune in the signals received. For this feature to work,\n\
not only do you need to enable the feature in the receiver, but in\n\
addition the stations you are wishing to tune in need to have this\n\
feature enabled on transmission. Other programs also offer this RSID\n\
feature as an option. </p>\n\
</blockquote>\n\
<h2><i><a name=\"Setting_Up\"></a></i>Setting\n\
Up</h2>\n\
<blockquote>\n\
<ul>\n\
<li>Use the menu <b>Configure/Sound Card</b>, <b>Audio/Devices</b>\n\
tab, to select the sound card you wish to use. You can ignore the other\n\
tabs for now. </li>\n\
<li>Use the menu <b>Configure/Operator</b> item to set the user\n\
name, callsign, locator and so on. </li>\n\
<li>Use the menu <b>Configure/Rig Control</b> item to set how you\n\
will control the rig. If you will key the rig via a serial port, in the\n\
<b>Hardware PTT</b> tab select 'Use serial port PTT',\n\
the device name you will use, and which line controls PTT. If in doubt,\n\
check both RTS and DTR. You MUST then press the <b>Initialize</b>\n\
button. </li>\n\
<li>If you plan to use CAT control of the rig via the COM port, check 'Use\n\
rigCAT' in the <b>RigCAT</b> tab. If in addition you wish\n\
to use PTT control via CAT, also then check 'PTT via CAT command'. You\n\
MUST then press the <b>Initialize</b> button. </li>\n\
<li>Use the menu <b>Configure/UI</b>, <b>Restart</b>\n\
tab, to set the aspect ratio of the waterfall display and whether or\n\
not you want to dock a second digiscope to the main window.\n\
</li>\n\
<li>Use the menu <b>Configure/IDs</b> item to set whether you wish\n\
to transmit RSID data at the start of each over (this is for the\n\
benefit of others, this setting does not affect RSID reception). If you\n\
plan to regularly use the RSID feature on receive, you should deselect\n\
the option that starts new modems at the 'Sweet Spot' frequencies in <b>Misc/Sweet\n\
Spot</b>. </li>\n\
<li>The first time fldigi is started it will make a series of\n\
measurements to determine the computer's clock rate. &nbsp;It's\n\
performance measurment is usually accurate. &nbsp;You may find\n\
however that you need to compensate for inadequate cpu performance.\n\
&nbsp;If you have a slow computer (under 700MHz), select 'Slow CPU' under <b>Configure/Misc/CPU</b>.\n\
The receiver decoding strategy of certain modems uses fewer processor\n\
cycles in this mode. </li>\n\
<li>Each of the modems can be individually set up from the <b>Configure/Modems</b>\n\
multi-tabbed dialog. You need not change anything here to start with,\n\
although it might be a good idea to set the 'secondary text' for\n\
DominoEX and THOR to something useful, such as your call and locator.\n\
(Secondary text is transmitted when the text you type does not keep up\n\
with the typing speed of the mode - this handy text appears in a small\n\
window at the very bottom of the screen). Note that this set of tabs is\n\
also where you set the RTTY modem speed and shift, although the default\n\
values should be fine for normal operation. </li>\n\
<li>Use the menu <b>Configure/Save Config</b> item to save the new\n\
configuration. </li>\n\
<li>Use your sound card 'Master Volume' applet to select the sound card, the\n\
Wave output and set the transmit audio level. You can check the level\n\
using the <b>TUNE</b> button, top right, beyond the Menu. </li>\n\
<li>On Windows, the 'Volume' applet can usually be opened by <b>START/Run...</b>\n\
and enter '<b>sndvol32'</b>, or from the Control Panel. </li>\n\
<li>Use your sound card 'Recording Control' applet to select the sound card,\n\
the line or mic input and set the receiver audio level. Watch the\n\
waterfall display for receiver noise when setting the level. If you see\n\
any dark blue noise, you have the right input and about the right\n\
level. The actual setting is not very important, provided you see blue\n\
noise. If the audio level is too high, the little diamond shaped\n\
indicator (bottom right) will show <b>red</b>.\n\
The waterfall may also show red bands. Performance will be degraded if\n\
the level is too high. </li>\n\
<li>On Windows, the 'Record' applet can usually be opened by <b>START/Run...</b>\n\
and enter '<b>sndvol32</b>', or from the Control Panel.\n\
If opened from the Control Panel, you'll end up with the Master Volume\n\
applet, and need to switch using Options/Properties, and selecting the\n\
'Recording' radio button.</li>\n\
</ul>\n\
</blockquote>\n\
<hr>\n\
<h2><a name=\"Start\"></a>Guided\n\
Tour</h2>\n\
<blockquote>\n\
The main window consists of three main panes. &nbsp;Study it\n\
carefully as you read these notes. From top to bottom, these are the <b>RECEIVE</b>\n\
pane (<b>navaho white</b>), the <b>TRANSMIT</b>\n\
pane (<b>light cyan</b>), and the <b>WATERFALL</b>\n\
pane (<b>black</b>). At the top is the collection of entry\n\
items which form the <b>LOG DATA</b>, and at the very top,\n\
a conventional drop-down <b>MENU</b> system, with entries\n\
for <u>F</u>ile, Op <u>M</u>ode, Configure,\n\
View and Help.\n\
<p>Between the&nbsp;TRANSMIT and the WATERFALL panes is a\n\
line of boxes (buttons), which represent the Function Keys F1 - F12.\n\
This is the <span style=\"font-weight: bold;\">MACRO</span>\n\
group. Below the WATERFALL pane is another line of boxes (buttons), which provide\n\
various control features. This is the <b>CONTROLS</b> group. The\n\
program and various buttons can mostly be operated using the mouse or\n\
the keyboard, and users generally find it convenient to use the mouse\n\
while tuning around, and the keyboard and function keys during a QSO. </p>\n\
</blockquote>\n\
<h3><i>RECEIVE\n\
Pane</i></h3>\n\
<blockquote>\n\
This is where the text from decoded incoming\n\
signals is displayed, in <b>black</b>\n\
text. When you transmit, the\n\
transmitted text is also displayed here,\n\
but in <b style=\"color: black;\">red</b>\n\
, so the RECEIVE pane becomes a complete record of the QSO. The\n\
information in this pane can also be logged to a file.\n\
<p>The line at the bottom of this pane can be\n\
dragged up and down with the mouse. You might prefer to drag it down a\n\
bit to enlarge the RECEIVE pane and reduce the size of the TRANSMIT\n\
pane. </p>\n\
</blockquote>\n\
<h3><i>TRANSMIT\n\
Pane</i></h3>\n\
<blockquote>\n\
This is where you type what you want to\n\
transmit. The mouse must click in here before you type (to obtain\n\
'focus') otherwise your text will go nowhere. You can type in here\n\
while you are receiving, and when you start transmitting, the text\n\
already typed will be sent first. This trick is a cool way to impress\n\
others with your typing speed! As the text is transmitted, the text\n\
colour changes from <b>black</b>\n\
to <b style=\"color: black;\">red</b>.\n\
At the end of the over, all the transmitted text (and any as yet not\n\
transmitted) will be deleted. </blockquote>\n\
<h3><i>WATERFALL\n\
Pane</i></h3>\n\
<blockquote>\n\
This is the main tuning facility. There are three\n\
modes,&nbsp;Waterfall,&nbsp;FFT and Signal, selected by a\n\
button in the CONTROL group. For now, leave it in&nbsp;Waterfall mode, as this is the\n\
easiest to tune with, and gives the best identification of the signal.\n\
<ul>\n\
<li>WF - or waterfall is a spectrogram display, of signal\n\
strength versus frequency, over passing time. The receiver passband is\n\
analysed and displayed with lower frequencies to the left, higher to\n\
the right. Weak signals and background noise are dark while stronger\n\
signals show as brighter colours. As time passes (over a few seconds),\n\
the historic signals move downwards like a waterfall. </li>\n\
<li>FFT - or Fast Fourier Transform, is a spectrum display, simply the mean\n\
signal strength versus frequency. Again frequency is displayed from\n\
left to right, but now the vertical direction shows signal strength and\n\
there is no brightness or historic information.</li>\n\
<li>SIG - or signal is an oscilloscope type of display\n\
showing the raw audio being captured by the sound card.</li>\n\
</ul>\n\
<p>At the top of the pane is a scale of frequency\n\
in Hz, which corresponds to the frequency displayed immediately below\n\
it. This scale can be moved around and zoomed using buttons in the\n\
CONTROL group. </p>\n\
<p>As you move the\n\
mouse around in this pane you\n\
will see a <b style=\"color: black;\">yellow</b>\n\
group of tuning marks following\n\
the mouse pointer. Tuning is achieved\n\
by left-clicking on a signal displayed by the waterfall in this pane.\n\
Use these yellow marks to exactly straddle the signal and then\n\
left-click on the centre of the signal. The tuning marks change to <b>red</b>.\n\
The <b>red</b> vertical lines will\n\
show the approximate width of the active signal area (the expected\n\
signal bandwidth), while a <b>red</b>\n\
horizontal bar above will indicate the\n\
receiver software's active\n\
decoding range. When you left-click, the red marks move to where you\n\
clicked, and will attempt to auto-track the signal from there. </p>\n\
<p>You can temporarily\n\
'monitor' a different\n\
signal by right-clicking on it. As long as you hold the mouse button\n\
down, the signal under it will be decoded; as soon as you release the\n\
mouse, decoding will revert to the previously tuned spot (where the red\n\
marks are). </p>\n\
</blockquote>\n\
<h2><i><a name=\"Logbook\"></a></i>Logbook Data</h2><p>Fldigi provides two QSO entry views. &nbsp;One for casual QSO logging and the second&nbsp;for contesting.<br><br>The\n\
frequency, Off (time off), and #Out are filled by the program.\n\
&nbsp;All the others can be populated by manual keyboard entry or by\n\
selection from the Rx panel. &nbsp;The time off, Off, is continuously\n\
update with the current GMT. &nbsp;The time on, On, will be filled in\n\
when the Call is updated, but can be modified later by the operator.<br><br>A right click on the Rx panel brings\n\
up a context sensitive menu that will reflect which of the two QSO\n\
capture views you have open. &nbsp;<br></p>If\n\
you highlight text in the Rx pane then the menu selection will operate\n\
on that text. &nbsp;If you simply point to a word of text and right\n\
click then the menu selection will operate on the single word.<br><br>Certain\n\
fields may also be populated with automatic parsing, Call, Name, Qth\n\
and Loc. &nbsp; You point to the Rx pane word and then either\n\
double-left-click or hold a shift key down and left click. &nbsp;The\n\
program will attempt to parse the word as a regular expression to\n\
populate the Call, Name, Qth, and Loc fields in that order. &nbsp;It\n\
may place some non standard calls into the Loc field if they qualify as\n\
a proper Maidenhead Grid Square, such as MM55CQ. &nbsp;That may be a\n\
special event station, but it also looks like a grid square locator\n\
value. &nbsp;You need to decide when that occurs and use the pop up\n\
menu for those special cases. &nbsp;The first non-Call non-Loc word\n\
will fill the Name field and subsequent qualify words will go into the\n\
Qth field.<br><br>A highlighted section of text, can always be copied\n\
to the clipboard for subsequent pasting elsewhere. &nbsp;The Copy menu\n\
item will be active when text in the Rx pane has been highlighted.\n\
&nbsp;That text can also be saved to a file. &nbsp;Use the \"Save <span style=\"text-decoration: underline;\">a</span>s...\"\n\
menu item for that purpose. &nbsp;All data fields in fldigi share a\n\
common set of keyboard shortcuts. &nbsp;Linux users will recognize\n\
these as familiar Emacs shortcuts. &nbsp;There is also a small popup\n\
menu that can be opened for each field by right clicking the contents with the\n\
mouse. &nbsp;Highlighted\n\
text will be overwritten when a paste is selected. &nbsp;Otherwise the\n\
clipboard will be pasted at the current cursor position.<br><br>You\n\
can query on-line and local CD based data base systems for data\n\
regarding a Call. &nbsp;You make the query by either clicking on the\n\
globe button, or\n\
selecing \"Look up call\" from the popup menu. &nbsp;The latter will also\n\
move\n\
the call to the Call field and make the query.<br><br>If you have\n\
previously worked a station the logbook will be searched for the most\n\
recent qso and fill the Name, Qth and other fields from the logbook.\n\
&nbsp;If the logbook dialog is open that last qso will be selected for\n\
viewing in the logbook.<br>&nbsp;&nbsp;<br>You open the logbook by\n\
selecting from the View menu; View/Logbook. &nbsp;The logbook title bar\n\
will show you which logbook you currently have open. &nbsp;fldigi can\n\
maintain an unlimited (except for disk space) number of logbooks.<br>\n\
\n\
<h3><i>MENU</i></h3>\n\
<blockquote>\n\
At the very top of\n\
the program window is a\n\
conventional drop-down menu. If you click on any of the items, a list\n\
of optional functions will appear. Keyboard menu selection is also\n\
provided. Where <u>underscored</u> characters are shown in\n\
the menu, you can select these menu items from the keyboard using the\n\
marked character and &lt;<b>Alt</b>&gt; at the same\n\
time, then moving around with the up/down/left/right keys. Use &lt;<b>Esc</b>&gt;\n\
to quit from the menu with no change. These menu functions are: </blockquote>\n\
<dl>\n\
<dt><b><u>F</u>ile</b>\n\
</dt>\n\
<dd>Allows you to open\n\
or save Macros (we won't get\n\
into that here), turn on/off logging to file, record/play audio\n\
samples, and exit the program. You can also exit the program by\n\
clicking on the 'X' in the top right corner of the window, in the usual\n\
manner. </dd>\n\
<dt><b>Op <u>M</u>ode</b>\n\
</dt>\n\
<dd>This is where you\n\
select the operating modem\n\
used for transmission and reception. Some modes only have one option.\n\
Where more are offered, drag the mouse down the list and sideways\n\
following the arrow to a secondary list, before releasing it. When you\n\
start the program next time, it will remember the last mode you used.\n\
<p>Not all the modes\n\
are widely used, so choose a\n\
mode which (a) maximizes your chance of a QSO, and (b) is appropriate\n\
for the band, conditions, bandwidth requirements and permissions\n\
relevant to your operating licence. </p>\n\
<p>At the bottom of\n\
the list are two 'modes' which\n\
aren't modes at all, and do not transmit (see <a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online\n\
Documentation</a> for details). <b>WWV</b> mode\n\
allows you to receive a standard time signal so the beeps it transmits\n\
can be used for sound card calibration. <b>Freq Analysis</b>\n\
provides just a waterfall display with a very narrow cursor, and a\n\
frequency meter which indicates the received frequency in Hz to two\n\
decimal places. This is useful for on-air frequency measurement. </p>\n\
</dd>\n\
<dt><b>Configure</b>\n\
</dt>\n\
<dd>This\n\
is where you set up the program to suit your computer, yourself and\n\
your operating preferences. The operating settings of the program are\n\
grouped into several categories and there are menu items in which you\n\
enter your personal information, or define your computer sound card,\n\
for example. <b>Modems</b> can be individually changed,\n\
each having different adjustments. The <b>Modems</b>\n\
dialog has multiple tabs, so you can edit any one of them. Don't fool\n\
with the settings until you know what you are doing!&nbsp; The final item, <b>Save\n\
Config</b> allows you to save the altered configuration for next\n\
time you start the program (otherwise changes are temporary).</dd><dt></dt>\n\
<dt><b>View</b>\n\
</dt>\n\
<dd>This\n\
menu item allows you to open extra windows. Most will be greyed out,\n\
but two that are available are the <b>Digiscope</b>, and\n\
the <b>PSK Browser</b>. The <b>Digiscope</b>\n\
provides a mode-specific graphical analysis of the received signal, and\n\
can have more than one view (left click in the new window to change the\n\
view), or maybe none at all. The <b>PSK Browser</b> is a\n\
rather cool tool that allows you to monitor several PSK31 signals all\n\
at the same time! These windows can be resized to suit.</dd><dt></dt>\n\
<dt><b>Help</b>\n\
</dt>\n\
<dd>Brings\n\
up the <a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online\n\
Documentation</a>, the Fldigi <a href=\"http://www.w1hkj.com/Fldigi.html\">Home Page</a>,\n\
and various information about the program. </dd>\n\
</dl>\n\
The\n\
two non-menu functions are:\n\
<dl>\n\
<dt><b>RSID</b>\n\
</dt>\n\
<dd>This\n\
button turns on the receive RSID (automatic mode detection and tuning)\n\
feature. When in use, the button turns yellow and no text reception is\n\
possible until a signal is identified, or the feature is turned off\n\
again. If you plan to use the RSID feature on receive, you must leave\n\
the 'Start New Modem at Sweet Spot' item in the Menu\n\
Configure/Defaults/Mics tab unchecked.</dd><dt></dt>\n\
<dt><b>TUNE</b>\n\
</dt>\n\
<dd>This\n\
button transmits a continuous tone at the current audio frequency. The\n\
tone level will be at the maximum signal level for any modem, which\n\
makes this function useful for adjusting your transceiver's output\n\
power.\n\
</dd>\n\
</dl>\n\
<h3><i>FUNCTIONS</i></h3>\n\
<blockquote>\n\
This\n\
line of buttons provides user-editable QSO features. For example, the\n\
first button on the left sends CQ for you. Both the function of these\n\
buttons (we call them <b><i>MACROS</i></b>)\n\
and the label on each button, can be changed. Select each button to use\n\
it by pressing the corresponding Function Key (F1 - F12, you'll notice\n\
the buttons are grouped in patterns four to a group, just as the\n\
Function Keys are). You can also select them with a left-click of the\n\
mouse. If you right-click on the button, you are able to edit the\n\
buttons label and its function. A handy dialog pops up to allow this to\n\
be done. There are many standard shortcuts, such as\n\
&lt;MYCALL&gt; which you can use within the Macros. Notice that\n\
the buttons also turn the transmitter on and off as necessary.\n\
<p>You\n\
can just about hold a complete QSO using these buttons from left to\n\
right (but please don't!). Notice that at the right are two spare\n\
buttons you can set as you wish, and then a button labelled '1'. Yes,\n\
this is the first set of FOUR sets of Macros, and you can access the\n\
others using this button, which changes to read '2', '3', '4' then '1'\n\
again (right-click to go backwards), or by pressing &lt;<b>Alt</b>&gt;\n\
and the corresponding number (1-4, not F1-F4) at the same time. </p>\n\
<p>If\n\
you REALLY mess up the Macros and can't see how to fix them, just close\n\
the program without saving them, and reopen it. </p>\n\
</blockquote>\n\
<h3><i>CONTROLS</i></h3>\n\
<blockquote>\n\
The\n\
line of buttons under the waterfall is used to control the program (as\n\
opposed to the QSO). If you hover the mouse over these buttons, you'll\n\
see a little yellow hint box appear which tells you what each button\n\
does.\n\
<p>The\n\
first button switches between Waterfall and FFT modes. The next two\n\
buttons adjust the signal level over which the waterfall works. The\n\
default range is from 0dB downwards 70dB (i.e. to -70dB). Both of these\n\
values can be adjusted to suit your sound card and receiver audio\n\
level.\n\
</p>\n\
<p>The\n\
next button sets the scale zoom factor (visible display width, x1, x2\n\
or x4), and the next three buttons move the visible waterfall area in\n\
relation to the bandwidth cursor.\n\
</p>\n\
<p>The\n\
next button selects the waterfall speed. NORM or SLOW setting is best\n\
unless you have a very fast computer.\n\
</p>\n\
<p>The\n\
next four buttons (two on either side of a number, the audio frequency\n\
in Hz) control the receiving frequency (they move the red cursor\n\
lines).\n\
</p>\n\
<p>The\n\
<b>QSY</b> button moves the signal under the bandwidth\n\
cursor to a preset audio frequency (typically, the centre of the\n\
transceiver's passband). The <b>Store</b> button allows\n\
you to store or recall the current frequency and mode. \n\
See the <a href=\"http://www.w1hkj.com/FldigiHelp/OperatingControls.html\">Online\n\
Documentation</a> for details on these functions.\n\
</p>\n\
<p>The\n\
<b>Lk</b> button locks the transmit frequency (fixes the\n\
red cursors), and the <b>Rv</b> button turns the signal\n\
decoding upside down (some modes are sideband sensitive, and if they\n\
are the wrong way up, can't be received correctly). Remember to turn\n\
this one off when you're done, or you won't receive anything! If every\n\
signal you hear is upside down, check your transceiver sideband\n\
setting. </p>\n\
<p>The\n\
<b>T/R</b> button forces the transmitter on or off - use\n\
this with care, as it will stop transmission immediately, losing\n\
whatever is in the buffer (what you have typed in the Transmit pane),\n\
or start it immediately, even if nothing is ready to transmit. </p>\n\
<p>There\n\
are two further controls in the bottom right corner of the program, to\n\
the right of the Status line: </p>\n\
</blockquote>\n\
<dl>\n\
<dt><b>AFC</b>\n\
- The AFC control\n\
</dt>\n\
<dd>When\n\
this button is pressed, an indicator on the button turns yellow, and\n\
the program will automatically retune to drifting signals. When the\n\
button is again pressed, AFC is off, and the tuning will stay where you\n\
leave it. </dd>\n\
<dt><b>SQL</b>\n\
- The Squelch control </dt>\n\
<dd>When\n\
off (no coloured indicator on the button, the receiver displays all\n\
'text' received, even if there is no signal present, and the receiver\n\
is simply attempting to decode noise. When activated by pressing the\n\
button, the indicator turns yellow. If the incoming signal strength\n\
exceeds that set by the adjacent slider control (above the <b>SQL</b>\n\
button), the indicator turns green and the incoming signal is decoded\n\
and printed. The signal strength is indicated on the green bar beside\n\
the Squelch level slider. If nothing seems to be printing, the first\n\
thing to do is check the Squelch! </dd>\n\
</dl>\n\
<h3><i>STATUS\n\
Line</i></h3>\n\
<blockquote>\n\
At\n\
the very bottom line of the Fldigi window is a row of useful\n\
information. At the left is the current operating mode. Next (some\n\
modes) is the measured signal-to-noise ratio at the receiver, and (in\n\
some modes) the measured signal intermodulation level (IMD).\n\
<p>The\n\
larger central box shows (in DominoEX and THOR modes) the received\n\
'Secondary Text'. This is information (such as station identification)\n\
which is transmitted automatically whenever the transmitter has\n\
completed all user text that is available to send. It is transmitted\n\
using special characters, and is automatically directed to this special\n\
window. Secondary text you transmit is also shown here. This box\n\
changes size when you enlarge the program window. </p>\n\
</blockquote>\n\
<h2><a name=\"Oper\">Operating</a></h2>\n\
<h3><i>Procedure</i></h3>\n\
<blockquote>\n\
Operating procedure\n\
for digital modes is similar\n\
to that for Morse. Some of the same abbreviations are used. For\n\
example, at the beginning of an over, you might send 'VK3XYZ de WB8ABC'\n\
or just 'RR Jack' and so on. At the end of an over, it is usual to send\n\
'ZL1ABC de AA3AR K', and at the end of a QSO '73 F3XYZ de 3D2ZZ SK'.\n\
When operating in a group or net it is usual to sign 'AA3AE es gp de\n\
ZK8WW K'.\n\
<p>It is also\n\
considered a courtesy to send a blank\n\
line or two (press &lt;<b>Enter</b>&gt;) before any\n\
text at the start of an over, and following the last text at the end of\n\
an over. You can also place these in the macros. The purpose is to\n\
separate your text from the previous text, and especially from any\n\
rubbish that was printed between overs. </p>\n\
<p>Fldigi does all of\n\
this for you. The Function\n\
Keys are set up to provide these start and end of over facilities, and\n\
can be edited to suit your preferences. In order that the other\n\
station's callsign can appear when these keys are used, you need to set\n\
the other station's callsign in the log data - it does not matter if\n\
you use the log facility or not. </p>\n\
<dl>\n\
<dt><b>Hint:</b>\n\
Some Function Key Macro\n\
buttons have graphic symbols on them which imply the following:<br>\n\
</dt>\n\
</dl>\n\
<ul>\n\
<li><b>&gt;&gt;</b>&nbsp;&nbsp;The\n\
transmitter comes on and stays on when you use this button/macro. </li>\n\
<li><b>||</b>&nbsp;&nbsp;&nbsp;The\n\
transmitter goes off when the text from this button/macro has been\n\
sent. </li>\n\
<li><b>&gt;|</b>&nbsp;&nbsp;The\n\
transmitter comes on, sends the text from this button/macro, and goes\n\
off when the text from this button/macro has been sent. </li>\n\
</ul>\n\
<dl>\n\
</dl>\n\
The Macros are set\n\
up to control the transmitter\n\
as necessary, but you can also switch the transmitter on at the start\n\
of an over with &lt;<b>Ctrl</b>&gt; and <b>T</b>\n\
or the <b>TX</b> macro button, and off again with &lt;<b>Ctrl</b>&gt;\n\
and <b>R</b> or the <b>RX</b> macro button.\n\
If you have Macros copied into or text already typed in the Transmit\n\
pane when you start the transmitter, this is sent first.\n\
<p>Calling another\n\
station you have tuned in is as\n\
simple as pushing a button. Put his callsign into the log data (right\n\
click, select Call) and press the <b>ANS</b> Macro button\n\
(or <b>F2</b>) when you are ready. If he replies, you are\n\
in business! Then press <b>QSO</b> (<b>F3</b>)\n\
to start each over, and <b>BTU</b> (<b>F4</b>)\n\
to end it, and <b>SK</b> (<b>F5</b>) to sign\n\
off. </p>\n\
<dl>\n\
<dt><b>Hint:</b>\n\
When typing text, the\n\
correct use of upper and lower case is important:<br>\n\
</dt>\n\
</dl>\n\
<ul>\n\
<li></li>\n\
<li>Modes\n\
such as RTTY and\n\
THROB have no lower case capability. </li>\n\
<li></li>\n\
<li>In\n\
most other modes,\n\
excessive use of upper case is considered impolite, like SHOUTING! </li>\n\
<li>Modes\n\
such as PSK31,\n\
MFSK16, DominoEX and THOR use character sets which are optimized for\n\
lower case. You should use lower case as much as possible in these\n\
modes to achieve maximum text speed. In these modes upper case\n\
characters are noticeably slower to send and also slightly more prone\n\
to errors. </li>\n\
</ul>\n\
</blockquote>\n\
<h3><i>Adjustment</i></h3>\n\
<blockquote>\n\
Most digital modes\n\
do not require much\n\
transmitter power, as the receiver software is very sensitive. Many\n\
modes (PSK31, THROB, MT63) also require very high transmitter\n\
linearity, which is another reason to keep transmitter power below 30%\n\
of maximum. Some modes (Hellschreiber, Morse) have high peak power\n\
output, which may not indicate well on the conventional power meter,\n\
another reason to keep the average transmitted power low to prevent a\n\
very broad signal being transmitted.\n\
<p>Adjust the\n\
transmitter output power using the <b>TUNE</b>\n\
button, top right, beyond the Menu. The output will be the same as the\n\
peak power in other modes. Adjust the master Volume applet Wave Out and\n\
Master Volume controls to achieve the appropriate power. Use of\n\
excessive drive will result in distortion (signal difficult to tune in,\n\
and often poorer reception) and a very broad signal. </p>\n\
<p>Some multi-carrier\n\
modes (MT63 for example) may\n\
require individual adjustment as the average power may be rather low. </p>\n\
<dl>\n\
<dt><b>Hint:</b>\n\
Where possible, use the\n\
area above 1200Hz on the waterfall. </dt>\n\
</dl>\n\
<ul>\n\
<li>Below\n\
1200Hz the\n\
second harmonic of the transmitted audio will pass through the\n\
transmitter filters. </li>\n\
<li>When\n\
using lower\n\
frequency tones, adjust the transmitter and audio level with great\n\
care, as the second (and even third) harmonic will appear in the\n\
transmitter passband, causing excessive signal width. </li>\n\
<li>A\n\
narrow (CW) filter\n\
in the rig is no help in this regard, as it is only used on receive.\n\
When you do use a narrow filter, this will restrict the area over which\n\
the receiver and transmitter will operate (without retuning of course).\n\
Try adjusting the passband tuning (if available). </li>\n\
<li>Keep\n\
the sound card\n\
audio level to a minimum and set the transmitter gain to a similar\n\
level used for SSB.</li>\n\
</ul>\n\
</blockquote>\n\
<h3><i>Waterfall\n\
Tuning</i></h3>\n\
<blockquote>\n\
When using this\n\
program, as with most other\n\
digital modes programs, tuning is generally accomplished by leaving the\n\
transceiver VFO at a popular spot (for example 14.070MHz, USB), and\n\
performing all the 'tuning' by moving around within the software.\n\
<p>The Fldigi software\n\
has a second 'VFO' which is\n\
tuned by clicking on the waterfall. On a busy band, you may see many\n\
signals at the same time (especially with PSK31 or Morse), and so you\n\
can click with the mouse on any one of these signals to tune it in,\n\
receive it, and if the opportunity allows, reply to the station. </p>\n\
<p>The software 'VFO'\n\
operates in a transceive\n\
mode, so the transmitter signal is automatically and exactly tuned to\n\
the received frequency. If you click correctly on the signal, your\n\
reply will always be in tune with the other station. </p>\n\
<dl>\n\
<dt><b>Hint:</b>\n\
You <b>MUST NOT</b>\n\
use RIT (Clarifier) when using digital modes. </dt>\n\
</dl>\n\
<ul>\n\
<li>With\n\
RIT on, you will\n\
probably have to retune after every over. </li>\n\
<li>Use\n\
of the RIT will\n\
also cause the other station to change frequency, and you will chase\n\
each other across the band. </li>\n\
<li>Older\n\
transceivers\n\
without digital synthesis may have an unwanted offset (frequency\n\
difference) between transmit and receive frequencies. Such rigs should\n\
not be used for digital modes. </li>\n\
</ul>\n\
Wider digital modes\n\
(MT63, Olivia) can be tuned\n\
using the rig if necessary, as tuning is not at all critical. The\n\
software tuning still operates, but because the signal is so wide,\n\
there is limited ability to move around in the waterfall tuning. </blockquote>\n\
<h3><a name=\"Keys\"><i>Special Keys</i></a></h3>\n\
<blockquote>\n\
Several special\n\
keyboard controls are provided\n\
to make operating easier. </blockquote>\n\
<dl>\n\
<dt><b>Pause\n\
Transmission</b>\n\
</dt>\n\
<dd>Press &lt;<b>Pause/Break</b>&gt;\n\
while in receive, and the program will switch to transmit mode. It will\n\
continue with the text in the transmit buffer (the Transmit pane text)\n\
from the current point, i.e. where the red (previously sent) text ends\n\
and the black (yet to be sent) text begins. If the buffer only contains\n\
unsent text, then it will begin at the first character in the buffer.\n\
If the buffer is empty, the program will switch to transmit mode, and\n\
depending on the mode of operation, will send idle characters or\n\
nothing at all until characters are entered into the buffer.\n\
<p>If you press &lt;<b>Pause/Break</b>&gt;\n\
while in transmit mode, the program will return to receive mode. There\n\
may be a slight delay for some modes like MFSK, PSK and others, that\n\
requires the transmitter to send a postamble at the end of a\n\
transmission. The transmit text buffer stays intact, ready for the\n\
&lt;<b>Pause/Break</b>&gt; key to return you to the\n\
transmit mode . </p>\n\
<p>Pressing &lt;<b>Alt/Meta</b>&gt;\n\
and <b>R</b> has the same effect as &lt;<b>Pause/Break</b>&gt;.\n\
You could think of the &lt;<b>Pause/Break</b>&gt;\n\
key as a software break-in capability. </p>\n\
</dd>\n\
<dt><b>ESCAPE</b>\n\
</dt>\n\
<dd>Pressing &lt;<b>Esc</b>&gt;\n\
while transmitting will abort the transmission. Transmission stops as\n\
soon as possible, (any necessary postamble is sent), and the program\n\
returns to receive. Any unsent text in the transmit buffer will be\n\
lost.\n\
<p>If you press &lt;<b>Esc</b>&gt;\n\
&lt;<b>Esc</b>&gt; (i.e. twice in quick\n\
succession), transmission stops immediately, (without sending any\n\
postamble), and the program returns to receive. Any unsent text in the\n\
transmit buffer will be lost. Use this feature as an <b>EMERGENCY\n\
STOP</b>. </p>\n\
</dd>\n\
<dt><b>RETURN\n\
to Receive</b>\n\
</dt>\n\
<dd>Press\n\
&lt;<b>Ctrl</b>&gt;\n\
and <b>R</b> to insert the <b>^r</b> command\n\
in the transmit buffer at the current typing point. When transmission\n\
reaches this point, transmission will stop. The transmission does not\n\
stop immediately.</dd></dl><dl>\n\
<dt><b>START Transmission</b>\n\
</dt>\n\
<dd>Press\n\
&lt;<b>Ctrl</b>&gt;\n\
and <b>T</b> to start transmission if there is text ready\n\
in the transmit buffer.</dd></dl><dl>\n\
<dt><b>MOVE Typing Cursor</b>\n\
</dt>\n\
<dd>Press\n\
&lt;<b>Tab</b>&gt; to\n\
move the cursor (typing insertion point) to the end of the transmit\n\
buffer. This will also pause transmission. A &lt;<b>Tab</b>&gt;\n\
press at that position moves the cursor back to the character following\n\
the last one transmitted.&nbsp; Morse operation is slightly\n\
different. See\n\
the on-line help for <a href=\"http://www.w1hkj.com/FldigiHelp/CW.html\">CW</a>.</dd></dl><dl>\n\
<dt><b>SEND any ASCII character</b>\n\
</dt>\n\
<dd>Press\n\
&lt;<b>Ctl</b>&gt; and (at the same time) any\n\
three-digit number (on the numeric keypad) to insert the ASCII\n\
character designated by that entry value into the transmit buffer. For\n\
example, &lt;<b>Ctl</b>&gt;<b>177</b>\n\
is ± (plus/minus) and &lt;<b>Ctl</b>&gt;<b>176</b>\n\
is ° (degree). If you press a key other than the numeric keypad's 0 - 9\n\
the sequence will be discarded. You can also use the <b>Ctl</b>\n\
with the normal numeric keys.</dd><dt></dt>\n\
</dl>\n\
<hr>\n\
<div style=\"text-align: center;\"><small><b>Copyright\n\
W1HKJ, M0GLD, and others\n\
</b></small></div>\n\
</body></html>";

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

