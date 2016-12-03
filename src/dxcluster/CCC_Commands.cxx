std::string ccc_commands = "\n\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 5//EN\">\n\
<html>\n\
<head>\n\
<meta http-equiv=\"Content-Type\" content=\"text/html\">\n\
<title>CCC Commands</title>\n\
</head>\n\
<body>\n\
<div align=\"left\">\n\
<table height=\"20\" border=\"0\" cellpadding=\"0\" cellspacing=\"0\"\n\
width=\"100%\" align=\"center\">\n\
<tbody>\n\
<tr>\n\
<td bgcolor=\"#ffffff\">\n\
<h2 align=\"center\">CC Cluster Commands</h2>\n\
</td>\n\
</tr>\n\
</tbody>\n\
</table>\n\
</div>\n\
<div align=\"left\">\n\
<table bgcolor=\"#FFFFFF\" border=\"0\"\n\
cellpadding=\"5\" cellspacing=\"0\" width=\"100%\">\n\
<tbody>\n\
<tr>\n\
<td>\n\
<div align=\"center\">\n\
<table border=\"1\" cellpadding=\"3\"\n\
 cellspacing=\"0\" width=\"100%\">\n\
<tbody>\n\
<tr>\n\
<td width=\"150\">ANnounce</td>\n\
<td width=\"600\">Send an announcement to local USERs.&nbsp; (AN&lt;Text Message&gt;)</td>\n\
</tr>\n\
<tr>\n\
<td>ANnounce/Full</td>\n\
<td>Send an announcement to all nodes and USERs.&nbsp; (AN/F &lt;Text Message&gt;)</td>\n\
</tr>\n\
<tr>\n\
<td>BYE</td>\n\
<td>Disconnect from the node. (BYE) or (B)</td>\n\
</tr>\n\
<tr>\n\
<td>DX</td>\n\
<td>Send a DX spot.&nbsp; (DX &lt;Callsign&gt; &lt;Frequency&gt; or DX &lt;Frequency&gt; &lt;Callsign&gt;)</td>\n\
</tr>\n\
<tr>\n\
<td>DXTest</td>\n\
<td>Returns&nbsp; to USER only. (DXT P5NOW 14006.06) Good for testing RES 1 &amp; RES 2</td>\n\
</tr>\n\
<tr>\n\
<td>DIR</td>\n\
<td>Shows mail messages on the node</td>\n\
</tr>\n\
<tr>\n\
<td>DIR/BULLETIN</td>\n\
<td>Shows mail messages to ALL, BULLETIN and anything not to a call</td>\n\
</tr>\n\
<tr>\n\
<td>DIR/NEW</td>\n\
<td>Shows only mail messages you haven't seen since your last DIR</td>\n\
</tr>\n\
<tr>\n\
<td>DIR/OWN</td>\n\
<td>Shows only mail messages to you including messages to ALL &amp; ones you sent</td>\n\
</tr>\n\
<tr>\n\
<td>DIR/SUBJECT</td>\n\
<td>Shows mail messages with subject you enter.&nbsp; (DIR/SUBJECT ARL)</td>\n\
</tr>\n\
<tr>\n\
<td>DELete</td>\n\
<td>Delete mail messages. (DEL (Msg #) (DEL 1-99) Deletes your messages from 1 to 99</td>\n\
</tr>\n\
<tr>\n\
<td>Kill</td>\n\
<td>Delete mail messages.&nbsp; (K (Msg #) (K 1-99) Deletes your messages from 1 to 99</td>\n\
</tr>\n\
<tr>\n\
<td>List</td>\n\
<td>Shows mail messages on the node</td>\n\
</tr>\n\
<tr>\n\
<td>List/NEW</td>\n\
<td>Shows only mail messages you haven't seen since your last DIR or L</td>\n\
</tr>\n\
<tr>\n\
<td>List/OWN</td>\n\
<td>Shows only mail messages to you including messages to ALL &amp; ones you sent</td>\n\
</tr>\n\
<tr>\n\
<td>QUIT</td>\n\
<td>Disconnect from the node</td>\n\
</tr>\n\
<tr>\n\
<td>READ</td>\n\
<td>Read cluster mail.&nbsp; (READ &lt;Message #&gt;)&nbsp; See Mail Send/Receive below</td>\n\
</tr>\n\
<tr>\n\
<td>REply</td>\n\
<td>REply without a number following replies to the last mail message you read.&nbsp;\n\
 REply &lt;#&gt; replies to the message with that number given.&nbsp;\n\
 REply/DELete replies to message and deletes it.&nbsp; REply/DELete/RR replies\n\
 to message, delets message and gets a return receipt.&nbsp; REply/RR replies\n\
 to message and gets a return receipt.&nbsp;</td>\n\
</tr>\n\
<tr>\n\
<td>SEND</td>\n\
<td>(SEND &lt;Callsign&gt;) Sends mail&nbsp; to that callsign.&nbsp;SEND &lt;LOCAL&gt;\n\
 to just send a message to local node USERs.&nbsp; SEND &lt;ALL&gt;, SEND\n\
 &lt;FORSALE&gt; and SEND &lt;DXNEWS&gt; will be passed to all nodes for all USERs.</td>\n\
</tr>\n\
<tr>\n\
<td>SET/ANN</td>\n\
<td>Turn on announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SET/BEACON</td>\n\
<td>Turn on beacon spots.&nbsp; These are spots ending in \"/B\" or \"BCN\"</td>\n\
</tr>\n\
<tr>\n\
<td>SET/BEEP</td>\n\
<td>Turn on a beep for DX and Announcement spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/BOB</td>\n\
<td>Turn on bottom of band DX spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/DX</td>\n\
<td>Turn on DX spot announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SET/DXCQ</td>\n\
<td>Turn on CQ Zone in DX info for DX spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/DXITU</td>\n\
<td>Turn on ITU Zone in DX info for DX spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/DXS</td>\n\
<td>Turn on US state/province or country in DX info for DX spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/USSTATE</td>\n\
<td>Turn on US\n\
state or Canadian province spotter in DX info for DX spots</td>\n\
</tr> \n\
<tr>\n\
<td>SET/FILTER</td>\n\
<td>See Band\n\
&amp; Mode Filtering Below</td>\n\
</tr>\n\
<tr>\n\
<td>SET/GRID</td>\n\
<td>Turns on DX\n\
Grid, toggles CQ Zone, ITU Zone, &amp; US State to off</td>\n\
</tr>\n\
<tr>\n\
<td>SET/HOME</td>\n\
<td>Tell cluster\n\
your home node.&nbsp;(SET/HOME &lt;Node Call&gt;) If you normally connect to\n\
K8SMC then it would be (SET/HOME K8SMC)&nbsp;</td>\n\
</tr>\n\
<tr>\n\
<td>SET/LOCATION</td>\n\
<td>Set your location (lat/lon) of your station.&nbsp; (SET/LOCATION 42 17 N 84 21 W)</td>\n\
</tr>\n\
<tr>\n\
<td>SET/LOGIN</td>\n\
<td>Tells cluster to send USER connects and disconnects.</td> \n\
</tr>\n\
<tr>\n\
<td>SET/NAME</td>\n\
<td>Set your name<o:p></o:p> (SET/NAME &lt;First Name&gt;)</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOANN</td>\n\
<td>Turn off announcements.</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOBEACON</td>\n\
<td>Turn off beacon spots.&nbsp; These are spots ending in \"/B\" or \"BCN\"</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOBEEP</td>\n\
<td>Turn off a beep for DX and Announcement spots<o:p>\n\
</o:p> .</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOBOB</td>\n\
<td>Turn off bottom of band DX spots.</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOCQ</td>\n\
<td>Turn off CQ Zone in spot announcements.</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NODX</td>\n\
<td>Turn off DX spot announcements.</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NODXCQ</td>\n\
<td>Turn off CQ Zone in DX info for DX spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NODXITU</td>\n\
<td>Turn off ITU Zone in DX info for DX spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NODXS</td>\n\
<td>Turn off US state/province or country in DX info for DX spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOUSSTATE</td>\n\
<td>Turn off US state or Canadian province spotter in DX info for DX spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOGRID</td>\n\
<td>Turn off DX Grids in spot announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOITU</td>\n\
<td>Turn off ITU Zone in spot announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOLOGIN</td>\n\
<td>Stops cluster from sending USER connects and disconnects</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOOWN</td>\n\
<td>Turn off skimmer spots for your own call</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOSELF</td>\n\
<td>Turn off self spots by other users</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOSKIMMER</td>\n\
<td>Turn off Skimmer spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOTALK</td>\n\
<td>Turn off the display of talk messages<o:p></o:p></td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOWCY</td>\n\
<td>Turn off the display of WCY spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOWWV</td>\n\
<td>Turn off the display of WWV spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOWX</td>\n\
<td>Turn off the display of weather announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SET/OWN</td>\n\
<td>Turn on Skimmer spots for own call</td>\n\
</tr>\n\
<tr>\n\
<td>SET/NOLOGIN</td>\n\
<td>Stops cluster from sending USER connects and disconnects</td>\n\
</tr>\n\
<tr>\n\
<td>SET/QRA</td>\n\
<td>Input your Grid Square.&nbsp;(SET/QRA EN72)</td>\n\
</tr>\n\
<tr>\n\
<td>SET/QTH</td>\n\
<td>Set your city and state. (SET/QTH &lt;City, State&gt;) DX &lt;City, Country&gt;</td>\n\
</tr>\n\
<tr>\n\
<td>SET/RES 1</td>\n\
<td>Tells CC-Cluster to give you 1 decimal point rounding in DX spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/RES 2</td>\n\
<td>Tells CC-Cluster to give you 2 decimal point rounding in DX spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/SELF</td>\n\
<td>Turn on self spots by other users</td>\n\
</tr>\n\
<tr>\n\
<td>SET/SKIMMER</td>\n\
<td>Turn on Skimmer spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/TALK</td>\n\
<td>Turn on the display of talk messages</td>\n\
</tr>\n\
<tr>\n\
<td>SET/USSTATE</td>\n\
<td>Turns on US State, toggles CQ Zone, DX Grid, &amp; ITU Zone to off</td>\n\
</tr>\n\
<tr>\n\
<td>SET/WCY</td>\n\
<td>Turn on the display of WCY spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/WIDTH</td>\n\
<td>Sets the line width for DX spots, normally this has been 80 characters.&nbsp; Depending\n\
on your logging program you can use anything between 45 to 130\n\
characters,&nbsp; SET/WIDTH XX where XX is the number of characters.</td>\n\
</tr>\n\
<tr>\n\
<td>SET/WWV</td>\n\
<td>Turn on the display of WWV spots</td>\n\
</tr>\n\
<tr>\n\
<td>SET/WX</td>\n\
<td>Turn on the display of weather announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/CL</td>\n\
<td>Node Info and CCC Uptime&nbsp; See SH/VERSION&nbsp;</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/CONF</td>\n\
<td>Shows nodes and callsigns of USERs, only nodes called LOCAL by Sysop.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/DX</td>\n\
<td>Shows last 30 spots</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/DX &lt;Call&gt;</td>\n\
<td>Shows last 30 spots for that call</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/DX/&lt;number&gt;</td>\n\
<td>Shows that number of spots.&nbsp; SH/DX/100</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/DX &lt;Band&gt;</td>\n\
<td>Shows spots on that band.&nbsp; SH/DX 20&nbsp; for 20 meters</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/DX/ &lt;Freq&gt;</td>\n\
<td>Shows spots by frequency range.&nbsp; Syntax = SH/DX 7020-7130</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/DX &lt;prefix*&gt;</td>\n\
<td>Shows all spots for a country, standard prefix not necessary, asterisk needed</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/DX 'rtty'</td>\n\
<td>Shows spots where the comment field contains (rtty)</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/DXBY &lt;call&gt;</td>\n\
<td>Shows spots where spotter = Call</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/FDX</td>\n\
<td>Shows real time formatted dx spots.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/FILTER</td>\n\
<td>Shows how you have your filters set.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/HEAD &lt;Call&gt;</td>\n\
<td>Shows heading - distance and bearing for the\n\
call.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/MYANN</td>\n\
<td>Shows last 5 announcements allowed by your filter settings.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/MYDX</td>\n\
<td>Shows last 30 spots allowed by your filter settings</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/MYDX &lt;Call&gt;</td>\n\
<td>Shows last 30 spots for the call allowed by your filter settings.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/MYDX/&lt;number&gt;</td>\n\
<td>Shows that number of spots allowed by your filter.&nbsp; SH/MYDX/100</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/MYDX &lt;Band&gt;</td>\n\
<td>Shows spots on that band allowed by your filter settings.&nbsp; SH/MYDX 20&nbsp; for 20 meters</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/MYFDX</td>\n\
<td>Shows last 30 spots allowed by your filter settings.&nbsp;&nbsp;</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/MYWX</td>\n\
<td>Shows last 5 weather announcements allowed by your filter settings.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/RES</td>\n\
<td>Shows the number of digits after the decimal point for frequencies</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/SETTINGS</td>\n\
<td>Shows information on the node for your call and how you are setup.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/STATION</td>\n\
<td>Shows information on the node for a station.&nbsp; (SH/STA &lt;Callsign&gt;)</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/SUN</td>\n\
<td>Shows local sunrise and sunset times.&nbsp; (SH/SUN &lt;Prefix.) for that country</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/TIME</td>\n\
<td>Shows GMT time.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/TIME &lt;Call&gt;</td>\n\
<td>Shows local time for the call.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/USDB</td>\n\
<td>Shows State/Province for US/VE calls.&nbsp; (SH/USDB &lt;Callsign&gt;)</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/USERS</td>\n\
<td>Shows callsigns of everyone connected to the local node.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/VERSION</td>\n\
<td>Shows the CCC Uptime for connections.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/WIDTH</td>\n\
<td>Shows the length of a DX Spot. Normally 80 characters.</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/WWV</td>\n\
<td>Shows WWV info, (SH/WWV) gives last 5 (SH/WWV/99) gives last 99</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/WCY</td>\n\
<td>Shows last 5 DK0WCY, similar to WWV</td>\n\
</tr>\n\
<tr>\n\
<td>Talk</td>\n\
<td>Send a talk message to someone on the node.&nbsp; (T&lt;Callsign&gt; &lt;Message&gt;)</td>\n\
</tr>\n\
<tr>\n\
<td>UNSET/</td>\n\
<td>This command can be used instead of SET/NO, Compatibility for DX-Spider USERs</td>\n\
</tr>\n\
<tr>\n\
<td>WHO</td>\n\
<td>This command will return a list of connections in alphabetical order.&nbsp; Items are: Call User/Node Name IP/AGW</td>\n\
</tr>\n\
<tr>\n\
<td>WX</td>\n\
<td>The command \"WX\" will send a local weather announcement.&nbsp; (WX Sunny and Warm)</td>\n\
</tr>\n\
</tbody>\n\
</table>\n\
</div>\n\
\n\
&nbsp; \n\
<div align=\"center\">\n\
<center>\n\
<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\"\n\
width=\"600\">\n\
<tbody>\n\
<tr>\n\
<td width=\"100%\" align=\"center\"><font color=\"#800000\">Most Common Used Filter Commands</font></td>\n\
</tr>\n\
</tbody>\n\
</table>\n\
</center>\n\
</div>\n\
<div align=\"center\">\n\
<center>\n\
<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\"\n\
width=\"100%\">\n\
<tbody>\n\
<tr>\n\
<td width=\"250\">Filter Settings:</td>\n\
<td width=\"500\">Filters are mostly default to off, but one simple setting for say someone in the US or Canada\n\
 that is happy seeing spots from just the US and Canada can do a quick setting for this: SET/FILTER K,VE/PASS</td>\n\
</tr>\n\
<tr>\n\
<td width=\"250\">SH/FILTER</td>\n\
<td width=\"500\">Shows all of your USER\n\
 filter settings</td>\n\
</tr>\n\
<tr>\n\
<td width=\"250\">SH/FILTER &lt;aaa&gt;</td>\n\
<td width=\"500\">Show setting for specific filter, &lt;aaa&gt; = filter name.<br>\n\
 SH/FILTER DOC = DX Origination Country<br>\n\
 SH/FILTER DOS = DX Origination State<br>\n\
 SH/FILTER AOC = Announce Origination Country<br>\n\
 SH/FILTER AOS = Announce Origination State<br>\n\
 SH/FILTER WOC = Weather Origination Country<br>\n\
 SH/FILTER WOS = Weather Origination State<br>\n\
 SH/FILTER DXCTY = DX spot CounTrY<br>\n\
 SH/FILTER DXSTATE = DX spot STATE&nbsp;</td>\n\
</tr>\n\
<tr>\n\
<td width=\"250\">SET/NOFILTER</td>\n\
<td width=\"500\">Resets all filters to default.&nbsp; If you suspect you have\n\
 entered invalid filter command or commands, reset and start over.</td>\n\
</tr>\n\
<tr>\n\
<td width=\"250\">SET/FILTER &lt;aaa&gt;/OFF</td>\n\
<td width=\"500\">Turn off specific filter.&nbsp; &lt;aaa&gt; = filter name (see SH/FILTER &lt;aaa&gt;)</td>\n\
</tr>\n\
<tr>\n\
<td width=\"250\">SET/FILTER K,VE/PASS</td>\n\
<td width=\"500\">This would be the most common filter setting for say someone\n\
 in the United States or Canada to set so as to only see spots that\n\
 originated in the US or Canada.&nbsp;</td>\n\
</tr>\n\
<tr>\n\
<td width=\"250\">SET/FILTER &lt;aaa&gt;/&lt;p/r&gt; &lt;bbb&gt;</td>\n\
<td width=\"500\">Set specific filter.<br>\n\
 &lt;aaa&gt; = filter name (see SH/FILTER &lt;aaa&gt;<br>\n\
 &lt;p/r&gt; = PASS or REJECT<br>\n\
 &lt;bbb&gt; = Country or State<br>\n\
 Example #1:&nbsp; SET/FILTER DOC/PASS EA,OH,G&nbsp; This would set your\n\
 filter to pass originated spots from Spain, Finland and England only.<br>\n\
 Example #2:&nbsp; SET/FILTER DXCTY/PASS F,OH&nbsp; This would set\n\
 your filter to pass spots for France and Finland only.</td>\n\
</tr>\n\
<tr>\n\
<td width=\"250\">DX Band Mode Filtering</td>\n\
<td width=\"500\">The DXBM filter has many variations for your settings, it\n\
 is defaulted to receive all DX spots for all modes from 160 to 10 meters,\n\
 (see Band &amp; Mode Filtering below).</td>\n\
</tr>\n\
</tbody>\n\
</table>\n\
</center>\n\
</div>\n\
<p>&nbsp;</p>\n\
<div align=\"center\">\n\
<center>\n\
<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\"\n\
width=\"600\">\n\
<tbody>\n\
<tr>\n\
<td>\n\
<p align=\"center\"><font color=\"#800000\">Band &amp; Mode Filtering</font></p>\n\
</td>\n\
</tr>\n\
</tbody>\n\
</table>\n\
</center>\n\
</div>\n\
<div align=\"center\">\n\
<center>\n\
<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\"\n\
width=\"80%\">\n\
<tbody>\n\
<tr>\n\
<td>You can tailor the DX spots from CC Cluster to only the bands and modes that interest you.</td>\n\
</tr>\n\
<tr>\n\
<td>The default setting for new users is to receive all DX\n\
 spots from 160 to 10 meters, all modes.</td>\n\
</tr>\n\
<tr>\n\
<td>To reset the band/mode filter to pass everything, enter \"SET/FILTER DXBM/OFF\".</td>\n\
</tr>\n\
<tr>\n\
<td>To display your current settings, enter \"SH/FILTER DXBM\".</td>\n\
</tr>\n\
<tr>\n\
<td>You can change any band or band/mode</td>\n\
</tr>\n\
<tr>\n\
<td>You can set the band or band/mode to either pass or reject.</td>\n\
</tr>\n\
<tr>\n\
<td>You can add items one at a time, or all at once.</td>\n\
</tr>\n\
<tr>\n\
<td>&nbsp;</td>\n\
</tr>\n\
<tr>\n\
<td>For example:</td>\n\
</tr>\n\
<tr>\n\
<td>To add 6 meters, you enter \"SET/FILTER DXBM/PASS 6\".<br>\n\
 To delete 80 meter and 40 meter CW, enter \"SET/FILTER DXBM/REJECT 80-CW,40-CW\"</td>\n\
</tr>\n\
<tr>\n\
<td>&nbsp;</td>\n\
</tr>\n\
<tr>\n\
<td>Although the band/mode has a \"mode\" name, it does not mean\n\
 that when you select 40-RTTY that you are selecting only RTTY spots. What it\n\
 really means is that you are selecting the frequency range in the following\n\
 table that corresponds to this name.  In this case 7040-7100. The actual\n\
 mode may be anything. The only thing you have selected is a frequency range.</td>\n\
</tr>\n\
<tr>\n\
<td>&nbsp;</td>\n\
</tr>\n\
</tbody>\n\
</table>\n\
</center>\n\
</div>\n\
<div align=\"center\">\n\
<br>\n\
<center>\n\
<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\"\n\
width=\"100%\">\n\
<caption><font color=\"#800000\">DXBM Frequencies</font></caption>\n\
<tbody>\n\
<tr align=\"center\">\n\
<th bgcolor=\"#cccc99\">Band Mode</th>\n\
<th bgcolor=\"#cccc99\">Low</th>\n\
<th bgcolor=\"#cccc99\">High</th>\n\
<th bgcolor=\"#cccc99\">Band Mode</th>\n\
<th bgcolor=\"#cccc99\">Low</th>\n\
<th bgcolor=\"#cccc99\">High</th>\n\
<th bgcolor=\"#cccc99\">Band Mode</th>\n\
<th bgcolor=\"#cccc99\">Low</th>\n\
<th bgcolor=\"#cccc99\">High</th>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">160-CW</td>\n\
<td bgcolor=\"#ffff99\">1800</td>\n\
<td bgcolor=\"#ffff99\">1850</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff66\">160-SSB</td>\n\
<td bgcolor=\"#ffff99\">1850</td>\n\
<td bgcolor=\"#ffff99\">2000</td>\n\
</tr><tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">80-CW</td>\n\
<td bgcolor=\"#ffff99\">3500</td>\n\
<td bgcolor=\"#ffff99\">3580</td>\n\
<td bgcolor=\"#ffff66\">80-RTTY</td>\n\
<td bgcolor=\"#ffff99\">3580</td>\n\
<td bgcolor=\"#ffff99\">3700</td>\n\
<td bgcolor=\"#ffff66\">80-SSB</td>\n\
<td bgcolor=\"#ffff99\">3700</td>\n\
<td bgcolor=\"#ffff99\">4000</td>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff66\">60-SSB</td>\n\
<td bgcolor=\"#ffff99\">5260</td>\n\
<td bgcolor=\"#ffff99\">5405</td>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">40-CW</td>\n\
<td bgcolor=\"#ffff99\">7000</td>\n\
<td bgcolor=\"#ffff99\">7040</td>\n\
<td bgcolor=\"#ffff66\">40-RTTY</td>\n\
<td bgcolor=\"#ffff99\">7040</td>\n\
<td bgcolor=\"#ffff99\">7100</td>\n\
<td bgcolor=\"#ffff66\">40-SSB</td>\n\
<td bgcolor=\"#ffff99\">7100</td>\n\
<td bgcolor=\"#ffff99\">7300</td>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">30-CW</td>\n\
<td bgcolor=\"#ffff99\">10100</td>\n\
<td bgcolor=\"#ffff99\">10130</td>\n\
<td bgcolor=\"#ffff66\">30-RTTY</td>\n\
<td bgcolor=\"#ffff99\">10130</td>\n\
<td bgcolor=\"#ffff99\">10150</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
</tr><tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">20-CW</td>\n\
<td bgcolor=\"#ffff99\">14000</td>\n\
<td bgcolor=\"#ffff99\">14070</td>\n\
<td bgcolor=\"#ffff66\">20-RTTY</td>\n\
<td bgcolor=\"#ffff99\">14070</td>\n\
<td bgcolor=\"#ffff99\">14150</td>\n\
<td bgcolor=\"#ffff66\">20-SSB</td>\n\
<td bgcolor=\"#ffff99\">14150</td>\n\
<td bgcolor=\"#ffff99\">14350</td>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">17-CW</td>\n\
<td bgcolor=\"#ffff99\">18068</td>\n\
<td bgcolor=\"#ffff99\">18100</td>\n\
<td bgcolor=\"#ffff66\">17-RTTY</td>\n\
<td bgcolor=\"#ffff99\">18100</td>\n\
<td bgcolor=\"#ffff99\">18110</td>\n\
<td bgcolor=\"#ffff66\">17-SSB</td>\n\
<td bgcolor=\"#ffff99\">18110</td>\n\
<td bgcolor=\"#ffff99\">18168</td>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">15-CW</td>\n\
<td bgcolor=\"#ffff99\">21000</td>\n\
<td bgcolor=\"#ffff99\">21070</td>\n\
<td bgcolor=\"#ffff66\">15-RTTY</td>\n\
<td bgcolor=\"#ffff99\">21070</td>\n\
<td bgcolor=\"#ffff99\">21200</td>\n\
<td bgcolor=\"#ffff66\">15-SSB</td>\n\
<td bgcolor=\"#ffff99\">21200</td>\n\
<td bgcolor=\"#ffff99\">21450</td>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">12-CW</td>\n\
<td bgcolor=\"#ffff99\">24890</td>\n\
<td bgcolor=\"#ffff99\">24920</td>\n\
<td bgcolor=\"#ffff66\">12-RTTY</td>\n\
<td bgcolor=\"#ffff99\">24920</td>\n\
<td bgcolor=\"#ffff99\">24930</td>\n\
<td bgcolor=\"#ffff66\">12-SSB</td>\n\
<td bgcolor=\"#ffff99\">24930</td>\n\
<td bgcolor=\"#ffff99\">24990</td>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">10-CW</td>\n\
<td bgcolor=\"#ffff99\">28000</td>\n\
<td bgcolor=\"#ffff99\">28070</td>\n\
<td bgcolor=\"#ffff66\">10-RTTY</td>\n\
<td bgcolor=\"#ffff99\">28070</td>\n\
<td bgcolor=\"#ffff99\">28300</td>\n\
<td bgcolor=\"#ffff66\">10-SSB</td>\n\
<td bgcolor=\"#ffff99\">28300</td>\n\
<td bgcolor=\"#ffff99\">29700</td>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">6-CW</td>\n\
<td bgcolor=\"#ffff99\">50000</td>\n\
<td bgcolor=\"#ffff99\">50080</td>\n\
<td bgcolor=\"#ffff66\">6-SSB</td>\n\
<td bgcolor=\"#ffff99\">50080</td>\n\
<td bgcolor=\"#ffff99\">50500</td>\n\
<td bgcolor=\"#ffff66\">6-FM</td>\n\
<td bgcolor=\"#ffff99\">50500</td>\n\
<td bgcolor=\"#ffff99\">54000</td>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff66\">4-MTR</td>\n\
<td bgcolor=\"#ffff99\">70000</td>\n\
<td bgcolor=\"#ffff99\">70650</td>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">2-CW</td>\n\
<td bgcolor=\"#ffff99\">144000</td>\n\
<td bgcolor=\"#ffff99\">144100</td>\n\
<td bgcolor=\"#ffff66\">2-SSB</td>\n\
<td bgcolor=\"#ffff99\">144100</td>\n\
<td bgcolor=\"#ffff99\">144500</td>\n\
<td bgcolor=\"#ffff66\">2-FM</td>\n\
<td bgcolor=\"#ffff99\">144500</td>\n\
<td bgcolor=\"#ffff99\">148000</td>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">1-CW</td>\n\
<td bgcolor=\"#ffff99\">220000</td>\n\
<td bgcolor=\"#ffff99\">221000</td>\n\
<td bgcolor=\"#ffff66\">1-SSB</td>\n\
<td bgcolor=\"#ffff99\">222000</td>\n\
<td bgcolor=\"#ffff99\">224000</td>\n\
<td bgcolor=\"#ffff66\">1-FM</td>\n\
<td bgcolor=\"#ffff99\">221000</td>\n\
<td bgcolor=\"#ffff99\">222000</td>\n\
</tr>\n\
<tr align=\"center\">\n\
<td bgcolor=\"#ffff66\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff66\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff99\">&nbsp;</td>\n\
<td bgcolor=\"#ffff66\">MW-MW</td>\n\
<td bgcolor=\"#ffff99\">500000</td>\n\
<td bgcolor=\"#ffff99\">47000000</td>\n\
</tr>\n\
</tbody>\n\
</table>\n\
</center>\n\
</div>\n\
</td>\n\
</tr>\n\
</tbody>\n\
</table>\n\
</div>\n\
</body>\n\
</html>";

