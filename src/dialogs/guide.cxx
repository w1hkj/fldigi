const char* szBeginner = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\"\n\
    \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n\
<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\">\n\
<head>\n\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n\
<meta name=\"generator\" content=\"AsciiDoc 8.2.2\" />\n\
<style type=\"text/css\">\n\
/* Debug borders */\n\
p, li, dt, dd, div, pre, h1, h2, h3, h4, h5, h6 {\n\
/*\n\
  border: 1px solid red;\n\
*/\n\
}\n\
\n\
body {\n\
  margin: 1em 5% 1em 5%;\n\
}\n\
\n\
a {\n\
  color: blue;\n\
  text-decoration: underline;\n\
}\n\
a:visited {\n\
  color: fuchsia;\n\
}\n\
\n\
em {\n\
  font-style: italic;\n\
}\n\
\n\
strong {\n\
  font-weight: bold;\n\
}\n\
\n\
tt {\n\
  color: navy;\n\
}\n\
\n\
h1, h2, h3, h4, h5, h6 {\n\
  color: #527bbd;\n\
  font-family: sans-serif;\n\
  margin-top: 1.2em;\n\
  margin-bottom: 0.5em;\n\
  line-height: 1.3;\n\
}\n\
\n\
h1 {\n\
  border-bottom: 2px solid silver;\n\
}\n\
h2 {\n\
  border-bottom: 2px solid silver;\n\
  padding-top: 0.5em;\n\
}\n\
\n\
div.sectionbody {\n\
  font-family: serif;\n\
  margin-left: 0;\n\
}\n\
\n\
hr {\n\
  border: 1px solid silver;\n\
}\n\
\n\
p {\n\
  margin-top: 0.5em;\n\
  margin-bottom: 0.5em;\n\
}\n\
\n\
pre {\n\
  padding: 0;\n\
  margin: 0;\n\
}\n\
\n\
span#author {\n\
  color: #527bbd;\n\
  font-family: sans-serif;\n\
  font-weight: bold;\n\
  font-size: 1.1em;\n\
}\n\
span#email {\n\
}\n\
span#revision {\n\
  font-family: sans-serif;\n\
}\n\
\n\
div#footer {\n\
  font-family: sans-serif;\n\
  font-size: small;\n\
  border-top: 2px solid silver;\n\
  padding-top: 0.5em;\n\
  margin-top: 4.0em;\n\
}\n\
div#footer-text {\n\
  float: left;\n\
  padding-bottom: 0.5em;\n\
}\n\
div#footer-badges {\n\
  float: right;\n\
  padding-bottom: 0.5em;\n\
}\n\
\n\
div#preamble,\n\
div.tableblock, div.imageblock, div.exampleblock, div.verseblock,\n\
div.quoteblock, div.literalblock, div.listingblock, div.sidebarblock,\n\
div.admonitionblock {\n\
  margin-right: 10%;\n\
  margin-top: 1.5em;\n\
  margin-bottom: 1.5em;\n\
}\n\
div.admonitionblock {\n\
  margin-top: 2.5em;\n\
  margin-bottom: 2.5em;\n\
}\n\
\n\
div.content { /* Block element content. */\n\
  padding: 0;\n\
}\n\
\n\
/* Block element titles. */\n\
div.title, caption.title {\n\
  font-family: sans-serif;\n\
  font-weight: bold;\n\
  text-align: left;\n\
  margin-top: 1.0em;\n\
  margin-bottom: 0.5em;\n\
}\n\
div.title + * {\n\
  margin-top: 0;\n\
}\n\
\n\
td div.title:first-child {\n\
  margin-top: 0.0em;\n\
}\n\
div.content div.title:first-child {\n\
  margin-top: 0.0em;\n\
}\n\
div.content + div.title {\n\
  margin-top: 0.0em;\n\
}\n\
\n\
div.sidebarblock > div.content {\n\
  background: #ffffee;\n\
  border: 1px solid silver;\n\
  padding: 0.5em;\n\
}\n\
\n\
div.listingblock {\n\
  margin-right: 0%;\n\
}\n\
div.listingblock > div.content {\n\
  border: 1px solid silver;\n\
  background: #f4f4f4;\n\
  padding: 0.5em;\n\
}\n\
\n\
div.quoteblock > div.content {\n\
  padding-left: 2.0em;\n\
}\n\
\n\
div.attribution {\n\
  text-align: right;\n\
}\n\
div.verseblock + div.attribution {\n\
  text-align: left;\n\
}\n\
\n\
div.admonitionblock .icon {\n\
  vertical-align: top;\n\
  font-size: 1.1em;\n\
  font-weight: bold;\n\
  text-decoration: underline;\n\
  color: #527bbd;\n\
  padding-right: 0.5em;\n\
}\n\
div.admonitionblock td.content {\n\
  padding-left: 0.5em;\n\
  border-left: 2px solid silver;\n\
}\n\
\n\
div.exampleblock > div.content {\n\
  border-left: 2px solid silver;\n\
  padding: 0.5em;\n\
}\n\
\n\
div.verseblock div.content {\n\
  white-space: pre;\n\
}\n\
\n\
div.imageblock div.content { padding-left: 0; }\n\
div.imageblock img { border: 1px solid silver; }\n\
span.image img { border-style: none; }\n\
\n\
dl {\n\
  margin-top: 0.8em;\n\
  margin-bottom: 0.8em;\n\
}\n\
dt {\n\
  margin-top: 0.5em;\n\
  margin-bottom: 0;\n\
  font-style: italic;\n\
}\n\
dd > *:first-child {\n\
  margin-top: 0;\n\
}\n\
\n\
ul, ol {\n\
    list-style-position: outside;\n\
}\n\
ol.olist2 {\n\
  list-style-type: lower-alpha;\n\
}\n\
\n\
div.tableblock > table {\n\
  border: 3px solid #527bbd;\n\
}\n\
thead {\n\
  font-family: sans-serif;\n\
  font-weight: bold;\n\
}\n\
tfoot {\n\
  font-weight: bold;\n\
}\n\
\n\
div.hlist {\n\
  margin-top: 0.8em;\n\
  margin-bottom: 0.8em;\n\
}\n\
div.hlist td {\n\
  padding-bottom: 5px;\n\
}\n\
td.hlist1 {\n\
  vertical-align: top;\n\
  font-style: italic;\n\
  padding-right: 0.8em;\n\
}\n\
td.hlist2 {\n\
  vertical-align: top;\n\
}\n\
\n\
@media print {\n\
  div#footer-badges { display: none; }\n\
}\n\
\n\
div#toctitle {\n\
  color: #527bbd;\n\
  font-family: sans-serif;\n\
  font-size: 1.1em;\n\
  font-weight: bold;\n\
  margin-top: 1.0em;\n\
  margin-bottom: 0.1em;\n\
}\n\
\n\
div.toclevel1, div.toclevel2, div.toclevel3, div.toclevel4 {\n\
  margin-top: 0;\n\
  margin-bottom: 0;\n\
}\n\
div.toclevel2 {\n\
  margin-left: 2em;\n\
  font-size: 0.9em;\n\
}\n\
div.toclevel3 {\n\
  margin-left: 4em;\n\
  font-size: 0.9em;\n\
}\n\
div.toclevel4 {\n\
  margin-left: 6em;\n\
  font-size: 0.9em;\n\
}\n\
/* Workarounds for IE6's broken and incomplete CSS2. */\n\
\n\
div.sidebar-content {\n\
  background: #ffffee;\n\
  border: 1px solid silver;\n\
  padding: 0.5em;\n\
}\n\
div.sidebar-title, div.image-title {\n\
  font-family: sans-serif;\n\
  font-weight: bold;\n\
  margin-top: 0.0em;\n\
  margin-bottom: 0.5em;\n\
}\n\
\n\
div.listingblock div.content {\n\
  border: 1px solid silver;\n\
  background: #f4f4f4;\n\
  padding: 0.5em;\n\
}\n\
\n\
div.quoteblock-content {\n\
  padding-left: 2.0em;\n\
}\n\
\n\
div.exampleblock-content {\n\
  border-left: 2px solid silver;\n\
  padding-left: 0.5em;\n\
}\n\
\n\
/* IE6 sets dynamically generated links as visited. */\n\
div#toc a:visited { color: blue; }\n\
</style>\n\
<script type=\"text/javascript\">\n\
/*<![CDATA[*/\n\
window.onload = function(){generateToc(1)}\n\
/* Author: Mihai Bazon, September 2002\n\
 * http://students.infoiasi.ro/~mishoo\n\
 *\n\
 * Table Of Content generator\n\
 * Version: 0.4\n\
 *\n\
 * Feel free to use this script under the terms of the GNU General Public\n\
 * License, as long as you do not remove or alter this notice.\n\
 */\n\
\n\
 /* modified by Troy D. Hanson, September 2006. License: GPL */\n\
 /* modified by Stuart Rackham, October 2006. License: GPL */\n\
\n\
function getText(el) {\n\
  var text = \"\";\n\
  for (var i = el.firstChild; i != null; i = i.nextSibling) {\n\
    if (i.nodeType == 3 /* Node.TEXT_NODE */) // IE doesn't speak constants.\n\
      text += i.data;\n\
    else if (i.firstChild != null)\n\
      text += getText(i);\n\
  }\n\
  return text;\n\
}\n\
\n\
function TocEntry(el, text, toclevel) {\n\
  this.element = el;\n\
  this.text = text;\n\
  this.toclevel = toclevel;\n\
}\n\
\n\
function tocEntries(el, toclevels) {\n\
  var result = new Array;\n\
  var re = new RegExp('[hH]([2-'+(toclevels+1)+'])');\n\
  // Function that scans the DOM tree for header elements (the DOM2\n\
  // nodeIterator API would be a better technique but not supported by all\n\
  // browsers).\n\
  var iterate = function (el) {\n\
    for (var i = el.firstChild; i != null; i = i.nextSibling) {\n\
      if (i.nodeType == 1 /* Node.ELEMENT_NODE */) {\n\
        var mo = re.exec(i.tagName)\n\
        if (mo)\n\
          result[result.length] = new TocEntry(i, getText(i), mo[1]-1);\n\
        iterate(i);\n\
      }\n\
    }\n\
  }\n\
  iterate(el);\n\
  return result;\n\
}\n\
\n\
// This function does the work. toclevels = 1..4.\n\
function generateToc(toclevels) {\n\
  var toc = document.getElementById(\"toc\");\n\
  var entries = tocEntries(document.getElementsByTagName(\"body\")[0], toclevels);\n\
  for (var i = 0; i < entries.length; ++i) {\n\
    var entry = entries[i];\n\
    if (entry.element.id == \"\")\n\
      entry.element.id = \"toc\" + i;\n\
    var a = document.createElement(\"a\");\n\
    a.href = \"#\" + entry.element.id;\n\
    a.appendChild(document.createTextNode(entry.text));\n\
    var div = document.createElement(\"div\");\n\
    div.appendChild(a);\n\
    div.className = \"toclevel\" + entry.toclevel;\n\
    toc.appendChild(div);\n\
  }\n\
}\n\
/*]]>*/\n\
</script>\n\
<title>Beginners' Guide to Fldigi</title>\n\
</head>\n\
<body>\n\
<div id=\"header\">\n\
<h1>Beginners' Guide to Fldigi</h1>\n\
<div id=\"toc\">\n\
  <div id=\"toctitle\">Table of Contents</div>\n\
  <noscript><p><b>JavaScript must be enabled in your browser to display the table of contents.</b></p></noscript>\n\
</div>\n\
</div>\n\
<div id=\"preamble\">\n\
<div class=\"sectionbody\">\n\
<div class=\"sidebarblock\">\n\
<div class=\"sidebar-content\">\n\
<p>Of necessity, this Beginners' Guide contains only as much as you need to know to\n\
get started. You should learn how to make best use of the program by reading the\n\
<a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</a>. You can also access it from within the Fldigi program from the <em>Help</em>\n\
menu item.</p>\n\
</div></div>\n\
</div>\n\
</div>\n\
<h2><a id=\"ref-beginners-q-a\"></a>1. Beginners' Questions Answered</h2>\n\
<div class=\"sectionbody\">\n\
<h3>1.1. What is Fldigi?</h3>\n\
<p><a href=\"http://www.w1hkj.com/Fldigi.html\">Fldigi</a> is a computer program intended for Amateur Radio Digital Modes\n\
operation using a PC (Personal Computer). Fldigi operates (as does most similar\n\
software) in conjunction with a conventional HF SSB radio transceiver, and uses\n\
the PC sound card as the main means of input from the radio, and output to the\n\
radio. These are audio-frequency signals. The software also controls the radio\n\
by means of another connection, typically a serial port.</p>\n\
<p>Fldigi is multi-mode, which means that it is able to operate many popular\n\
digital modes without switching programs, so you only have one program to\n\
learn. Fldigi includes all the popular modes, such as DominoEX, MFSK16, PSK31,\n\
and RTTY.</p>\n\
<p>Unusually, Fldigi is available for multiple computer operating systems;\n\
FreeBSD&#8482;; Linux&#8482;, OS X&#8482; and Windows&#8482;.</p>\n\
<h3>1.2. What is a Digital Mode?</h3>\n\
<p>Digital Modes are a means of operating Amateur radio from the computer\n\
keyboard. The computer acts as <em>modem</em> (modulator - demodulator), as well as\n\
allowing you to type, and see what the other person types. It also controls the\n\
transmitter, changes modes as required, and provides various convenient features\n\
such as easy tuning of signals and prearranged messages.</p>\n\
<p>In this context, we are talking about modes used on the HF (high frequency)\n\
bands, specifically <em>chat</em> modes, those used to have a regular conversation in a\n\
similar way to voice or Morse, where one operator <em>talks</em> for a minute or two,\n\
then another does the same. These chat modes allow multiple operators to take\n\
part in a <em>net</em>.</p>\n\
<p>Because of sophisticated digital signal processing which takes place inside the\n\
computer, digital modes can offer performance that cannot be achieved using\n\
voice (and in some cases even Morse), through reduced bandwidth, improved\n\
signal-to-noise performance and reduced transmitter power requirement. Some\n\
modes also offer built-in automatic error correction.</p>\n\
<p>Digital Mode operating procedure is not unlike Morse operation, and many of the\n\
same abbreviations are used. Software such as Fldigi makes this very simple as\n\
most of the procedural business is set up for you using the Function Keys at the\n\
top of the keyboard. These are easy to learn.</p>\n\
<h3>1.3. Why all the different modes?</h3>\n\
<p>HF propagation is very dependent on the ionosphere, which reflects the signals\n\
back to earth. There are strong interactions between different signals arriving\n\
from different paths. Experience has shown that particular modulation systems,\n\
speeds and bandwidths suit different operating conditions.</p>\n\
<p>Other factors such as available band space, operating speed and convenience,\n\
noise level, signal level and available power also affect the choice of\n\
mode. While in many cases several different modes might be suitable, having a\n\
choice adds to the operating pleasure. It is difficult to advise which mode is\n\
best for each particular occasion, and experience plays an important role.\n\
<br />[To gain a good insight into each mode and its capabilities, you might\n\
consider purchasing <em>Digital Modes for All Occasions</em> (ISBN 1-872309-82-8) by\n\
Murray Greenman ZL1BPU, published by the RSGB and also available from\n\
FUNKAMATEUR and CQ Communications; or the ARRL's <em>HF Digital Handbook</em> (ISBN\n\
0-87259-103-4) by Steve Ford, WB8IMY.]<br /></p>\n\
<h3>1.4. How do I recognise and tune in the signals?</h3>\n\
<p>Recognising the different modes comes with experience. It is a matter of\n\
listening to the signal, and observing the appearance of the signal on the\n\
tuning display. You can also practise transmitting with the transceiver\n\
disconnected, listening to the sound of the signals coming from the\n\
computer. There is also (see later paragraph) an automatic tuning option which\n\
can recognise and tune in most modes for you.</p>\n\
<p>The software provides a tuning display which shows the radio signals that are\n\
receivable within the transceiver passband. Using a <em>point and click</em> technique\n\
with the mouse, you can click on the centre of a signal to select it, and the\n\
software will tune it in for you. Some modes require more care than others, and\n\
of course you need to have the software set for the correct mode first — not\n\
always so easy!</p>\n\
<p>The <a href=\"#ref-rsid\">RSID</a> (automatic mode detection and tuning) feature uses a\n\
special sequence of tones transmitted at the beginning of each transmission to\n\
identify and tune in the signals received. For this feature to work, not only do\n\
you need to enable the feature in the receiver, but in addition the stations you\n\
are wishing to tune in need to have this feature enabled on transmission. Other\n\
programs also offer this RSID feature as an option.</p>\n\
</div>\n\
<h2><a id=\"ref-setting-up\"></a>2. Setting Up</h2>\n\
<div class=\"sectionbody\">\n\
<h3>2.1. Fldigi settings</h3>\n\
<div class=\"title\">Essentials</div><ul>\n\
<li>\n\
<p>\n\
Use the menu <tt>Configure-&gt;Operator</tt> item to set the operator name, callsign,\n\
  locator and so on.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
If you have more than one sound card, use the menu <tt>Configure-&gt;Sound Card</tt>,\n\
  <tt>Audio Devices</tt> tab, to select the sound card you wish to use. You can ignore\n\
  the other tabs for now.\n\
</p>\n\
</li>\n\
</ul>\n\
<div class=\"title\">Rig Control</div><ul>\n\
<li>\n\
<p>\n\
Use the menu <tt>Configure-&gt;Rig Control</tt> item to set how you will control the\n\
  rig. If you will key the rig via a serial port, in the <tt>Hardware PTT</tt> tab\n\
  select <em>Use serial port PTT</em>, the device name you will use, and which line\n\
  controls PTT. If in doubt, check both <em>RTS</em> and <em>DTR</em>. You <strong>must</strong> then press\n\
  the <tt>Initialize</tt> button.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
If you plan to use CAT control of the rig via the COM port, check <em>Use Hamlib</em>\n\
  in the <tt>Hamlib</tt> tab. Select your rig model from the drop-down menu and set the\n\
  serial port device name, baud rate, and RTS/CTS options as needed. If in\n\
  addition you wish to use PTT control via CAT, also check <em>PTT via Hamlib\n\
  command</em>. You <strong>must</strong> then press the <tt>Initialize</tt> button.\n\
</p>\n\
</li>\n\
</ul>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img src=\"/usr/share/asciidoc/images/icons/note.png\" alt=\"Note\" />\n\
</td>\n\
<td class=\"content\">\n\
<p>If your rig is CAT-capable but not yet supported by\n\
<a href=\"http://www.hamlib.org/\">Hamlib</a>, it may still be possible to control it via\n\
Fldigi's <tt>RigCAT</tt> system.  Refer to the <a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</a> for details.</p>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<div class=\"title\">CPU Speed</div><ul>\n\
<li>\n\
<p>\n\
When you start Fldigi for the very first time, it makes a series of\n\
  measurements to determine your computer's processing speed.  Although these\n\
  measurements are usually accurate, if you have a very slow processor (under\n\
  700MHz), you should verify that <em>Slow CPU</em> under <tt>Configure-&gt;Misc-&gt;CPU</tt> has\n\
  been enabled. The receiver decoding strategy of certain modems uses fewer\n\
  processor cycles in this mode.\n\
</p>\n\
</li>\n\
</ul>\n\
<div class=\"title\">Modems</div><ul>\n\
<li>\n\
<p>\n\
Each of the modems can be individually set up from the <tt>Configure-&gt;Modems</tt>\n\
  multi-tabbed dialog. You need not change anything here to start with, although\n\
  it might be a good idea to set the <em>secondary text</em> for DominoEX and THOR to\n\
  something useful, such as your call and locator. <br />[Secondary text is\n\
  transmitted when the text you type does not keep up with the typing speed of\n\
  the mode — this handy text appears in a small window at the very bottom of the\n\
  screen.]<br /> Note that this set of tabs is also where you set the RTTY modem speed\n\
  and shift, although the default values should be fine for normal operation.\n\
</p>\n\
</li>\n\
</ul>\n\
<div class=\"title\">Other settings</div><ul>\n\
<li>\n\
<p>\n\
Use the menu <tt>Configure-&gt;UI</tt>, <tt>Restart</tt> tab, to set the aspect ratio of the\n\
  waterfall display and whether or not you want to dock a second digiscope to\n\
  the main window.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
Use the menu <tt>Configure-&gt;IDs</tt> item to set whether you wish to transmit RSID\n\
  data at the start of each over (this is for the benefit of others and does not\n\
  affect RSID reception). If you plan to regularly use the RSID feature on\n\
  receive, you should deselect the option that starts new modems at the &#8220;sweet\n\
  spot&#8221; frequencies in <tt>Misc-&gt;Sweet Spot</tt>.\n\
</p>\n\
</li>\n\
</ul>\n\
<p>Finally, use the menu item <tt>Configure-&gt;Save Config</tt> to save the new\n\
configuration.</p>\n\
<h3>2.2. Sound Card Mixer</h3>\n\
<ul>\n\
<li>\n\
<p>\n\
Use your sound card <em>Master Volume</em> applet to select the sound card, the Wave\n\
  output and set the transmit audio level. You can check the level using the\n\
  <a href=\"#ref-tune\">Tune</a> button, top right, beyond the Menu.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
On Windows, the <em>Volume</em> applet can usually be opened by clicking\n\
  <tt>Start-&gt;Run…</tt> and entering <tt>sndvol32</tt>, or from the Control Panel.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
Use your sound card <em>Recording Control</em> applet to select the sound card, the\n\
  Line or Mic input and set the receiver audio level. Watch the waterfall\n\
  display for receiver noise when setting the level. If you see any dark blue\n\
  noise, you have the right input and about the right level. The actual setting\n\
  is not very important, provided you see blue noise. If the audio level is too\n\
  high, the little diamond shaped indicator (bottom right) will show red. The\n\
  waterfall may also show red bands. Performance will be degraded if the level\n\
  is too high.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
On Windows, the <em>Record</em> applet can usually be opened by clicking\n\
  <tt>Start-&gt;Run…</tt> and entering <tt>sndvol32</tt>, or from the Control Panel. If opened\n\
  from the Control Panel, you'll end up with the Master Volume applet, and need\n\
  to switch using <tt>Options-&gt;Properties</tt>, and selecting the <tt>Recording</tt> radio\n\
  button.\n\
</p>\n\
</li>\n\
</ul>\n\
</div>\n\
<h2><a id=\"ref-guided-tour\"></a>3. Guided Tour</h2>\n\
<div class=\"sectionbody\">\n\
<p>The main window consists of three main panes.  Study it carefully as you read\n\
these notes. From top to bottom, these are the Receive pane (navajo white), the\n\
Transmit pane (light cyan), and the Waterfall pane (black). At the top is the\n\
collection of entry items which form the Log Data, and at the very top, a\n\
conventional drop-down Menu system, with entries for File, Op Mode, Configure,\n\
View and Help.</p>\n\
<p>Between the Transmit and the Waterfall panes is a line of boxes (buttons) which\n\
represent the Function Keys F1 - F12. This is the Macro group. Below the\n\
Waterfall pane is another line of boxes (buttons), which provide various control\n\
features. This is the Controls group. The program and various buttons can mostly\n\
be operated using the mouse or the keyboard, and users generally find it\n\
convenient to use the mouse while tuning around, and the keyboard and function\n\
keys during a QSO.</p>\n\
<h3><a id=\"ref-receive-pane\"></a>3.1. Receive Pane</h3>\n\
<p>This is where the text from decoded incoming signals is displayed, in black\n\
text. When you transmit, the transmitted text is also displayed here, but in red,\n\
so the Receive pane becomes a complete record of the QSO. The information in\n\
this pane can also be logged to a file.</p>\n\
<p>The line at the bottom of this pane can be dragged up and down with the\n\
mouse. You might prefer to drag it down a bit to enlarge the Receive pane and\n\
reduce the size of the Transmit pane.</p>\n\
<h3>3.2. Transmit Pane</h3>\n\
<p>This is where you type what you want to transmit. The mouse must click in here\n\
before you type (to obtain <em>focus</em>) otherwise your text will go nowhere. You can\n\
type in here while you are receiving, and when you start transmitting, the text\n\
already typed will be sent first. This trick is a cool way to impress others\n\
with your typing speed! As the text is transmitted, the text colour changes from\n\
black to red. At the end of the over, all the transmitted text (and any as yet\n\
not transmitted) will be deleted.</p>\n\
<h3>3.3. Waterfall Pane</h3>\n\
<p>This is the main tuning facility. There are three modes, Waterfall, FFT and\n\
Signal, selected by a button in the Control group. For now, leave it in\n\
Waterfall mode, as this is the easiest to tune with, and gives the best\n\
identification of the signal.</p>\n\
<dl>\n\
<dt>\n\
<strong><tt>WF</tt></strong> (Waterfall)\n\
</dt>\n\
<dd>\n\
<p>\n\
  A spectrogram display of signal strength versus frequency over passing\n\
  time. The receiver passband is analysed and displayed with lower frequencies\n\
  to the left, higher to the right. Weak signals and background noise are dark\n\
  while stronger signals show as brighter colours. As time passes (over a few\n\
  seconds), the historic signals move downwards like a waterfall.\n\
</p>\n\
</dd>\n\
<dt>\n\
<strong><tt>FFT</tt></strong> (Fast Fourier Transform)\n\
</dt>\n\
<dd>\n\
<p>\n\
  A spectrum display of the mean signal strength versus frequency. Again\n\
  frequency is displayed from left to right, but now the vertical direction\n\
  shows signal strength and there is no brightness or historic information.\n\
</p>\n\
</dd>\n\
<dt>\n\
<strong><tt>SIG</tt></strong> (Signal)\n\
</dt>\n\
<dd>\n\
<p>\n\
  An oscilloscope type of display showing the raw audio being captured by the\n\
  sound card.\n\
</p>\n\
</dd>\n\
</dl>\n\
<p>At the top of the pane is a scale of frequency in Hz, which corresponds to the\n\
frequency displayed immediately below it. This scale can be moved around and\n\
zoomed using buttons in the Control group.</p>\n\
<p>As you move the mouse around in this pane you will see a yellow group of tuning\n\
marks following the mouse pointer. Tuning is achieved by left-clicking on a\n\
signal displayed by the waterfall in this pane. Use these yellow marks to\n\
exactly straddle the signal and then left-click on the centre of the signal. The\n\
tuning marks change to red. The red vertical lines will show the approximate\n\
width of the active signal area (the expected signal bandwidth), while a red\n\
horizontal bar above will indicate the receiver software's active decoding\n\
range. When you left-click, the red marks move to where you clicked, and will\n\
attempt to auto-track the signal from there.</p>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img src=\"/usr/share/asciidoc/images/icons/tip.png\" alt=\"Tip\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"title\">Audio history and &#8220;casual tuning&#8221;</div>\n\
<p>You can temporarily &#8220;monitor&#8221; a different signal by right-clicking on it. As\n\
long as you hold the mouse button down, the signal under it will be decoded; as\n\
soon as you release the mouse, decoding will revert to the previously tuned spot\n\
(where the red marks are).  If you also hold the <tt>Control</tt> key down before\n\
right-clicking, Fldigi will first decode all of its buffered audio at that\n\
frequency.</p>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<h3>3.4. Log Data</h3>\n\
<p>Fldigi provides two QSO entry views, one for casual QSO logging and the second\n\
for contesting.  The <tt>View-&gt;Contest fields</tt> menu item switches between the two\n\
modes.</p>\n\
<p>The <em>Frequency</em>, <em>Time Off</em>, and (when in contest mode) <em>#Out</em> fields are filled\n\
by the program.  All the others can be populated by manual keyboard entry or by\n\
selection from the <a href=\"#ref-receive-pane\">Receive pane</a>. The <em>Time Off</em> field is\n\
continuously updated with the current GMT time.  The <em>Time On</em> field will be\n\
filled in when the <em>Call</em> is updated, but can be modified later by the operator.</p>\n\
<p>A right click on the Receive pane brings up a context sensitive menu that will\n\
reflect which of the two QSO capture views you have open.  If you highlight text\n\
in the Receive pane then the menu selection will operate on that text.  If you\n\
simply point to a word of text and right click then the menu selection will\n\
operate on the single word.</p>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img src=\"/usr/share/asciidoc/images/icons/tip.png\" alt=\"Tip\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"title\">Quick log entry</div>\n\
<p>Certain fields (<em>Call</em>, <em>Name</em>, <em>RST In</em>, <em>QTH</em> and <em>Locator</em>) may also be\n\
populated semi-automatically.  Point to a word in the Receive pane and either\n\
double-left-click or hold a Shift key down and left-click.  The program will\n\
then use some simple heuristics to decide which log field will receive the text.</p>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<p>It is generally not possible to distinguish between Operator and QTH names.  For\n\
this reason, Fldigi will use the first non-Call and non-Locator word to fill the\n\
<em>Name</em> field, and subsequent clicks will send text to the <em>QTH</em> field.\n\
Likewise, a text string may be both a valid callsign and a valid\n\
<a href=\"http://en.wikipedia.org/wiki/Maidenhead_Locator_System\">IARU locator</a>.  For best\n\
results, you should attempt to fill the log fields in the order in which they\n\
appear on the main window, and clear the log fields after logging the QSO.  Of\n\
course, text can always be manually typed or pasted into any of the log fields!</p>\n\
<p>You can query online and local (e.g. CD) database systems for data regarding a\n\
callsign.  You make the query by either clicking on the globe button, or\n\
selecting <em>Look up call</em> from the popup menu.  The latter will also move the\n\
call to the <em>Call</em> field.</p>\n\
<p>When the <em>Call</em> field is filled in, the logbook will be searched for the most\n\
recent QSO with that station and, if an entry is found, the <em>Name</em>, <em>QTH</em> and\n\
other fields will be pre-filled.  If the logbook dialog is open, that last QSO\n\
will also be selected for viewing in the logbook.</p>\n\
<p>You open the logbook by selecting from the View menu; <tt>View-&gt;Logbook</tt>.  The\n\
logbook title bar will show you which logbook you currently have open.  Fldigi\n\
can maintain an unlimited (except for disk space) number of logbooks.</p>\n\
<h3>3.5. Menu</h3>\n\
<p>At the very top of the program window is a conventional drop-down menu. If you\n\
click on any of the items, a list of optional functions will appear. Keyboard\n\
menu selection is also provided. Where underscored characters are shown in the\n\
menu, you can select these menu items from the keyboard using the marked\n\
character and <tt>Alt</tt> at the same time, then moving around with the\n\
<tt>up</tt>/<tt>down</tt>/<tt>left</tt>/<tt>right</tt> keys. Press <tt>Esc</tt> to quit from the menu with no\n\
change.</p>\n\
<h4>3.5.1. Menu functions</h4>\n\
<div class=\"title\">File</div>\n\
<p>Allows you to open or save Macros (we won't get into that here), turn on/off\n\
logging to file, record/play audio samples, and exit the program. You can also\n\
exit the program by clicking on the <tt>X</tt> in the top right corner of the window,\n\
in the usual manner.</p>\n\
<div class=\"title\">Op Mode</div>\n\
<p>This is where you select the operating modem used for transmission and\n\
reception. Some modes only have one option. Where more are offered, drag the\n\
mouse down the list and sideways following the arrow to a secondary list, before\n\
releasing it. When you start the program next time, it will remember the last\n\
mode you used.</p>\n\
<p>Not all the modes are widely used, so choose a mode which <em>(a)</em> maximises your\n\
chance of a QSO, and <em>(b)</em> is appropriate for the band, conditions, bandwidth\n\
requirements and permissions relevant to your operating licence.</p>\n\
<p>At the bottom of the list are two &#8220;modes&#8221; which aren't modes at all, and do not\n\
transmit (see <a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</a> for details). <em>WWV</em> mode allows you to receive a\n\
standard time signal so the beeps it transmits can be used for sound card\n\
calibration. <em>Freq Analysis</em> provides just a waterfall display with a very\n\
narrow cursor, and a frequency meter which indicates the received frequency in\n\
Hz to two decimal places. This is useful for on-air frequency measurement.</p>\n\
<div class=\"title\">Configure</div>\n\
<p>This is where you set up the program to suit your computer, yourself and your\n\
operating preferences. The operating settings of the program are grouped into\n\
several categories and there are menu items in which you enter your personal\n\
information, or define your computer sound card, for example. Modems can be\n\
individually changed, each having different adjustments. The Modems dialog has\n\
multiple tabs, so you can edit any one of them. Don't fool with the settings\n\
until you know what you are doing!  The final item, <tt>Save Config</tt> allows you to\n\
save the altered configuration for next time you start the program (otherwise\n\
changes are temporary).</p>\n\
<div class=\"title\">View</div>\n\
<p>This menu item allows you to open extra windows. Most will be greyed out, but\n\
two that are available are the Digiscope, and the PSK Browser. The Digiscope\n\
provides a mode-specific graphical analysis of the received signal, and can have\n\
more than one view (left click in the new window to change the view), or maybe\n\
none at all. The PSK Browser is a rather cool tool that allows you to monitor\n\
several PSK31 signals all at the same time! These windows can be resized to\n\
suit.</p>\n\
<div class=\"title\">Help</div>\n\
<p>Brings up the Online Documentation, the Fldigi Home Page, and various\n\
information about the program.</p>\n\
<h4>3.5.2. Other controls</h4>\n\
<div class=\"title\">RSID</div>\n\
<p><a id=\"ref-rsid\"></a>This button turns on the receive RSID (automatic mode detection and tuning)\n\
feature. When in use, the button turns yellow and no text reception is possible\n\
until a signal is identified, or the feature is turned off again. If you plan to\n\
use the RSID feature on receive, you must leave the <em>Start New Modem at Sweet\n\
Spot</em> item in the menu <tt>Configure-&gt;Defaults-&gt;Misc</tt> tab unchecked.</p>\n\
<div class=\"title\">TUNE</div>\n\
<p><a id=\"ref-tune\"></a>This button transmits a continuous tone at the current audio frequency. The tone\n\
level will be at the maximum signal level for any modem, which makes this\n\
function useful for adjusting your transceiver's output power.</p>\n\
<h3>3.6. Macro buttons</h3>\n\
<p>This line of buttons provides user-editable QSO features. For example, the first\n\
button on the left sends CQ for you. Both the function of these buttons (we call\n\
them Macros) and the label on each button, can be changed.</p>\n\
<p>Select each button to use it by pressing the corresponding Function Key (F1 -\n\
F12, you'll notice the buttons are grouped in patterns four to a group, just as\n\
the Function Keys are). You can also select them with a left-click of the\n\
mouse. If you right-click on the button, you are able to edit the button's label\n\
and its function. A handy dialog pops up to allow this to be done. There are\n\
many standard shortcuts, such as <tt>&lt;MYCALL&gt;</tt>, which you can use within the\n\
Macros. Notice that the buttons also turn the transmitter on and off as\n\
necessary.</p>\n\
<p>You can just about hold a complete QSO using these buttons from left to right\n\
(but please don't!). Notice that at the right are two spare buttons you can set\n\
as you wish, and then a button labelled <tt>1</tt>. Yes, this is the first set of\n\
<em>four</em> sets of Macros, and you can access the others using this button, which\n\
changes to read <tt>2</tt>, <tt>3</tt>, <tt>4</tt> then <tt>1</tt> again (right-click to go backwards), or\n\
by pressing <tt>Alt</tt> and the corresponding number (1-4, not F1-F4) at the same\n\
time.</p>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img src=\"/usr/share/asciidoc/images/icons/note.png\" alt=\"Note\" />\n\
</td>\n\
<td class=\"content\">\n\
<p>If you <em>really</em> mess up the Macros and can't see how to fix them, just close the\n\
program without saving them, and reopen it.</p>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<h3>3.7. Controls</h3>\n\
<p>The line of buttons under the waterfall is used to control the program (as\n\
opposed to the QSO). If you hover the mouse over these buttons, you'll see a\n\
little yellow hint box appear which tells you what each button does.</p>\n\
<p>The first button switches between Waterfall, FFT and Scope modes. The next two\n\
buttons adjust the signal level over which the waterfall works. The default\n\
range is from 0dB downwards 70dB (i.e. to -70dB). Both of these values can be\n\
adjusted to suit your sound card and receiver audio level.</p>\n\
<p>The next button sets the scale zoom factor (visible display width, ×1, ×2 or\n\
×4), and the next three buttons move the visible waterfall area in relation to\n\
the bandwidth cursor.</p>\n\
<p>The next button selects the waterfall speed. NORM or SLOW setting is best unless\n\
you have a very fast computer.</p>\n\
<p>The next four buttons (two on either side of a number, the audio frequency in\n\
Hz) control the receiving frequency (they move the red cursor lines).</p>\n\
<p>The <tt>QSY</tt> button moves the signal under the bandwidth cursor to a preset audio\n\
frequency (typically, the centre of the transceiver's passband). The Store\n\
button allows you to store or recall the current frequency and mode. See the\n\
<a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</a> for details on these functions.</p>\n\
<p>The <tt>Lk</tt> button locks the transmit frequency (fixes the red cursors), and the\n\
<tt>Rv</tt> button turns the signal decoding upside down (some modes are sideband\n\
sensitive, and if they are the wrong way up, can't be received\n\
correctly). Remember to turn this one off when you're done, or you won't receive\n\
anything! If every signal you hear is upside down, check your transceiver\n\
sideband setting.</p>\n\
<p>The <tt>T/R</tt> button forces the transmitter on or off.</p>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img src=\"/usr/share/asciidoc/images/icons/caution.png\" alt=\"Caution\" />\n\
</td>\n\
<td class=\"content\">\n\
<p>Use the <tt>T/R</tt> button with care, as it will stop transmission immediately, losing\n\
whatever is in the buffer (what you have typed in the Transmit pane), or start\n\
it immediately, even if nothing is ready to transmit.</p>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<p>There are two further controls in the bottom right corner of the program, to the\n\
right of the Status line:</p>\n\
<dl>\n\
<dt>\n\
<tt>AFC</tt> (AFC) control\n\
</dt>\n\
<dd>\n\
<p>\n\
  When this button is pressed, an indicator on the button turns yellow, and the\n\
  program will automatically retune to drifting signals. When the button is\n\
  again pressed, AFC is off, and the tuning will stay where you leave it.\n\
</p>\n\
</dd>\n\
<dt>\n\
<tt>SQL</tt> (Squelch) control\n\
</dt>\n\
<dd>\n\
<p>\n\
  When off (no coloured indicator on the button), the receiver displays all\n\
  &#8220;text&#8221; received, even if there is no signal present, and the receiver is\n\
  simply attempting to decode noise. When activated by pressing the button, the\n\
  indicator turns yellow. If the incoming signal strength exceeds that set by\n\
  the adjacent slider control (above the <tt>SQL</tt> button), the indicator turns\n\
  green and the incoming signal is decoded and printed. The signal strength is\n\
  indicated on the green bar beside the Squelch level slider. If nothing seems\n\
  to be printing, the first thing to do is check the Squelch!\n\
</p>\n\
</dd>\n\
</dl>\n\
<h3>3.8. Status Line</h3>\n\
<p>At the very bottom line of the Fldigi window is a row of useful information. At\n\
the left is the current operating mode. Next (some modes) is the measured\n\
signal-to-noise ratio at the receiver, and (in some modes) the measured signal\n\
intermodulation level (IMD).</p>\n\
<p>The larger central box shows (in DominoEX and THOR modes) the received\n\
<em>Secondary Text</em>. This is information (such as station identification) which is\n\
transmitted automatically whenever the transmitter has completed all user text\n\
that is available to send. It is transmitted using special characters, and is\n\
automatically directed to this special window. Secondary text you transmit is\n\
also shown here. This box changes size when you enlarge the program window.</p>\n\
</div>\n\
<h2><a id=\"ref-operating\"></a>4. Operating</h2>\n\
<div class=\"sectionbody\">\n\
<h3>4.1. Procedure</h3>\n\
<p>Operating procedure for digital modes is similar to that for Morse. Some of the\n\
same abbreviations are used. For example, at the beginning of an over, you might\n\
send <tt>VK3XYZ de WB8ABC</tt> or just <tt>RR Jack</tt> and so on. At the end of an over, it\n\
is usual to send <tt>ZL1ABC de AA3AR K</tt>, and at the end of a QSO <tt>73 F3XYZ de 3D2ZZ\n\
SK</tt>. When operating in a group or net it is usual to sign <tt>AA3AE es gp de ZK8WW\n\
K</tt>.</p>\n\
<p>It is also considered a courtesy to send a blank line or two (press <tt>Enter</tt>)\n\
before any text at the start of an over, and following the last text at the end\n\
of an over. You can also place these in the macros. The purpose is to separate\n\
your text from the previous text, and especially from any rubbish that was\n\
printed between overs.</p>\n\
<p>Fldigi does all of this for you. The Function Keys are set up to provide these\n\
start and end of over facilities, and can be edited to suit your preferences. In\n\
order that the other station's callsign can appear when these keys are used, you\n\
need to set the other station's callsign in the log data — it does not matter if\n\
you use the log facility or not.</p>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img src=\"/usr/share/asciidoc/images/icons/note.png\" alt=\"Note\" />\n\
</td>\n\
<td class=\"content\">\n\
<div class=\"title\">Macro symbols</div>\n\
<p>Some Function Key Macro buttons have graphic symbols on them which imply\n\
the following:</p>\n\
<div class=\"hlist\"><table><col width=\"horizontal%\" />\n\
<tr>\n\
<td class=\"hlist1\">\n\
<strong><tt>&gt;&gt;</tt></strong>\n\
</td>\n\
<td class=\"hlist2\">\n\
The transmitter comes on and stays on when you use this button/macro.\n\
</td>\n\
</tr>\n\
<tr>\n\
<td class=\"hlist1\">\n\
<strong><tt>||</tt></strong>\n\
</td>\n\
<td class=\"hlist2\">\n\
The transmitter goes off when the text from this button/macro has been\n\
         sent.\n\
</td>\n\
</tr>\n\
<tr>\n\
<td class=\"hlist1\">\n\
<strong><tt>&gt;|</tt></strong>\n\
</td>\n\
<td class=\"hlist2\">\n\
The transmitter comes on, sends the text from this button/macro, and\n\
         goes off when the text from this button/macro has been sent.\n\
</td>\n\
</tr>\n\
</table></div>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<p>The Macros are set up to control the transmitter as necessary, but you can also\n\
switch the transmitter on at the start of an over with <tt>Ctrl</tt> and <tt>T</tt> or the TX\n\
macro button, and off again with <tt>Ctrl</tt> and <tt>R</tt> or the RX macro button. If you\n\
have Macros copied into or text already typed in the Transmit pane when you\n\
start the transmitter, this is sent first.</p>\n\
<p>Calling another station you have tuned in is as simple as pushing a button. Put\n\
his callsign into the log data (right click, select Call) and press the <tt>ANS</tt>\n\
Macro button (or F2) when you are ready. If he replies, you are in business!\n\
Then press <tt>QSO</tt> (F3) to start each over, and <tt>BTU</tt> (F4) to end it, and <tt>SK</tt>\n\
(F5) to sign off.</p>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img src=\"/usr/share/asciidoc/images/icons/note.png\" alt=\"Note\" />\n\
</td>\n\
<td class=\"content\">\n\
<p>When typing text, the correct use of upper and lower case is important:</p>\n\
<ul>\n\
<li>\n\
<p>\n\
Modes such as RTTY and THROB have no lower case capability.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
In most other modes, excessive use of upper case is considered impolite, like\n\
  SHOUTING!\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
Modes such as PSK31, MFSK16, DominoEX and THOR use character sets which are\n\
  optimised for lower case. You should use lower case as much as possible in\n\
  these modes to achieve maximum text speed. In these modes upper case\n\
  characters are noticeably slower to send and also slightly more prone to\n\
  errors.\n\
</p>\n\
</li>\n\
</ul>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<h3>4.2. Adjustment</h3>\n\
<p>Most digital modes do not require much transmitter power, as the receiver\n\
software is very sensitive. Many modes (PSK31, THROB, MT63) also require very\n\
high transmitter linearity, which is another reason to keep transmitter power\n\
below 30% of maximum. Some modes (Hellschreiber, Morse) have high peak power\n\
output, which may not indicate well on the conventional power meter, another\n\
reason to keep the average transmitted power low to prevent a very broad signal\n\
being transmitted.</p>\n\
<p>Adjust the transmitter output power using the TUNE button, top right, beyond the\n\
Menu. The output will be the same as the peak power in other modes. Adjust the\n\
master Volume applet Wave Out and Master Volume controls to achieve the\n\
appropriate power. Use of excessive drive will result in distortion (signal\n\
difficult to tune in, and often poorer reception) and a very broad signal.</p>\n\
<p>Some multi-carrier modes (MT63 for example) may require individual adjustment as\n\
the average power may be rather low.</p>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img src=\"/usr/share/asciidoc/images/icons/tip.png\" alt=\"Tip\" />\n\
</td>\n\
<td class=\"content\">\n\
<p>Where possible, use the area above 1200Hz on the waterfall.</p>\n\
<ul>\n\
<li>\n\
<p>\n\
Below 1200Hz the second harmonic of the transmitted audio will pass through\n\
  the transmitter filters.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
When using lower frequency tones, adjust the transmitter and audio level with\n\
  great care, as the second (and even third) harmonic will appear in the\n\
  transmitter passband, causing excessive signal width.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
A narrow (CW) filter in the rig is no help in this regard, as it is only used\n\
  on receive. When you do use a narrow filter, this will restrict the area over\n\
  which the receiver and transmitter will operate (without retuning of\n\
  course). Try adjusting the passband tuning (if available).\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
Keep the sound card audio level to a minimum and set the transmitter gain to a\n\
  similar level used for SSB.\n\
</p>\n\
</li>\n\
</ul>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<h3>4.3. Waterfall Tuning</h3>\n\
<p>When using this program, as with most other digital modes programs, tuning is\n\
generally accomplished by leaving the transceiver VFO at a popular spot (for\n\
example 14.070MHz, USB), and performing all the <em>tuning</em> by moving around within\n\
the software.</p>\n\
<p>The Fldigi software has a second &#8220;VFO&#8221; which is tuned by clicking on the\n\
waterfall. On a busy band, you may see many signals at the same time (especially\n\
with PSK31 or Morse), and so you can click with the mouse on any one of these\n\
signals to tune it in, receive it, and if the opportunity allows, reply to the\n\
station.</p>\n\
<p>The software &#8220;VFO&#8221; operates in a transceive mode, so the transmitter signal is\n\
automatically and exactly tuned to the received frequency. If you click\n\
correctly on the signal, your reply will always be in tune with the other\n\
station.</p>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img src=\"/usr/share/asciidoc/images/icons/important.png\" alt=\"Important\" />\n\
</td>\n\
<td class=\"content\">\n\
<p>You <strong>must not</strong> use RIT (Clarifier) when using digital modes.</p>\n\
<ul>\n\
<li>\n\
<p>\n\
With RIT on, you will probably have to retune after every over.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
Use of the RIT will also cause the other station to change frequency, and you\n\
  will chase each other across the band.\n\
</p>\n\
</li>\n\
<li>\n\
<p>\n\
Older transceivers without digital synthesis may have an unwanted offset\n\
  (frequency difference) between transmit and receive frequencies. Such rigs\n\
  should not be used for digital modes.\n\
</p>\n\
</li>\n\
</ul>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<p>Wider digital modes (MT63, Olivia) can be tuned using the rig if necessary, as\n\
tuning is not at all critical. The software tuning still operates, but because\n\
the signal is so wide, there is limited ability to move around in the waterfall\n\
tuning.</p>\n\
</div>\n\
<h2><a id=\"ref-special-keys\"></a>5. Special Keys</h2>\n\
<div class=\"sectionbody\">\n\
<p>Several special keyboard controls are provided to make operating easier.</p>\n\
<div class=\"title\">Start Transmission</div>\n\
<p>Press <tt>Ctrl</tt> and <tt>T</tt> to start transmission if there is text ready in the transmit\n\
buffer.</p>\n\
<div class=\"title\">Pause Transmission</div>\n\
<p>Press <tt>Pause</tt> or <tt>Break</tt> while in receive, and the program will switch to\n\
transmit mode. It will continue with the text in the transmit buffer (the\n\
Transmit pane text) from the current point, i.e. where the red (previously sent)\n\
text ends and the black (yet to be sent) text begins. If the buffer only\n\
contains unsent text, then it will begin at the first character in the\n\
buffer. If the buffer is empty, the program will switch to transmit mode, and\n\
depending on the mode of operation, will send idle characters or nothing at all\n\
until characters are entered into the buffer.</p>\n\
<p>If you press <tt>Pause</tt> or <tt>Break</tt> while in transmit mode, the program will return\n\
to receive mode. There may be a slight delay for some modes like MFSK, PSK and\n\
others, that requires the transmitter to send a postamble at the end of a\n\
transmission. The transmit text buffer stays intact, ready for the\n\
<tt>Pause</tt>/<tt>Break</tt> key to return you to the transmit mode .</p>\n\
<p>Pressing <tt>Alt</tt> or <tt>Meta</tt> and <tt>R</tt> has the same effect as <tt>Pause</tt>/<tt>Break</tt>. You\n\
could think of the <tt>Pause</tt>/<tt>Break</tt> key as a software break-in capability.</p>\n\
<div class=\"title\">Escape</div>\n\
<p>Pressing <tt>Esc</tt> while transmitting will abort the transmission. Transmission\n\
stops as soon as possible, (any necessary postamble is sent), and the program\n\
returns to receive. Any unsent text in the transmit buffer will be lost.</p>\n\
<div class=\"admonitionblock\">\n\
<table><tr>\n\
<td class=\"icon\">\n\
<img src=\"/usr/share/asciidoc/images/icons/tip.png\" alt=\"Tip\" />\n\
</td>\n\
<td class=\"content\">\n\
<p>If you press <tt>Esc Esc</tt> (i.e. twice in quick succession), transmission stops\n\
immediately, without sending any postamble, and the program returns to\n\
receive. Any unsent text in the transmit buffer will be lost. Use this feature\n\
as an <strong>emergency stop</strong>.</p>\n\
</td>\n\
</tr></table>\n\
</div>\n\
<div class=\"title\">Return to Receive</div>\n\
<p>Press <tt>Ctrl</tt> and <tt>R</tt> to insert the <tt>^r</tt> command in the transmit buffer at the\n\
current typing point. When transmission reaches this point, transmission will\n\
stop.</p>\n\
<div class=\"title\">Move Typing Cursor</div>\n\
<p>Press <tt>Tab</tt> to move the cursor (typing insertion point) to the end of the\n\
transmit buffer. This will also pause transmission. A <tt>Tab</tt> press at that\n\
position moves the cursor back to the character following the last one\n\
transmitted.  Morse operation is slightly different. See the <a href=\"http://www.w1hkj.com/FldigiHelp/index.html\">Online Documentation</a> for CW.</p>\n\
<div class=\"title\">Send Any ASCII Character</div>\n\
<p>Press <tt>Ctrl</tt> and (at the same time) any three-digit number (on the numeric\n\
keypad or the normal numeric keys) to insert the ASCII character designated by\n\
that entry value into the transmit buffer. For example, <tt>Ctrl 177</tt> is &#8220;±&#8221;\n\
(plus/minus) and <tt>Ctrl 176</tt> is &#8220;°&#8221; (degree). If you press a key other than the\n\
numeric keypad's 0-9 the sequence will be discarded.</p>\n\
</div>\n\
<h2><a id=\"ref-credits\"></a>6. Credits</h2>\n\
<div class=\"sectionbody\">\n\
<p>Copyright &#169; 2008 Murray Greenman, <tt>ZL1BPU</tt>.</p>\n\
<p>Copyright &#169; 2008-2009 David Freese, <tt>W1HKJ</tt>.</p>\n\
<p>Copyright &#169; 2009 Stelios Bounanos, <tt>M0GLD</tt>.</p>\n\
<p>License GPLv2+: <a href=\"http://www.gnu.org/licenses/gpl-2.0.html\">GNU GPL version 2 or later</a>.</p>\n\
</div>\n\
<div id=\"footer\">\n\
<div id=\"footer-text\">\n\
Version 3.11<br />\n\
Last updated 09-Apr-2009 21:07:28 CDT\n\
</div>\n\
<div id=\"footer-badges\">\n\
<a href=\"http://validator.w3.org/check?uri=referer\">\n\
  <img style=\"border:none; width:88px; height:31px;\"\n\
       src=\"http://www.w3.org/Icons/valid-xhtml11\"\n\
       alt=\"Valid XHTML 1.1!\" />\n\
</a>\n\
<a href=\"http://jigsaw.w3.org/css-validator/check/referer\">\n\
  <img style=\"border:none; width:88px; height:31px;\"\n\
       src=\"http://jigsaw.w3.org/css-validator/images/vcss\"\n\
       alt=\"Valid CSS!\" />\n\
</a>\n\
<a href=\"http://www.mozilla.org/products/firefox/\">\n\
  <img style=\"border:none; width:110px; height:32px;\"\n\
       src=\"http://www.spreadfirefox.com/community/images/affiliates/Buttons/110x32/safer.gif\"\n\
       alt=\"Get Firefox!\" />\n\
</a>\n\
</div>\n\
</div>\n\
</body>\n\
</html>\n\n";
