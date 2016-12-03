string arc_commands = "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 5.0//EN\">\n\
<html>\n\
<head>\n\
<meta http-equiv=\"Content-Type\" content=\"text/html;\">\n\
<title>AR Commands</title>\n\
</head>\n\
<body style=\"tab-interval:.5in\" lang=\"EN-US\">\n\
<div class=\"Section1\">\n\
<h2><span style=\"font-family:Verdana;color:blue\">Overview of\n\
AR-Cluster User\n\
Commands<span style=\"mso-spacerun: yes\"></span></span><span\n\
style=\"font-family:Verdana;\n\
color:blue\"></h2>\n\
<p><span\n\
style=\"font-family:Verdana\">The\n\
following list is a summary of AR-Cluster commands.<span\n\
style=\"mso-spacerun:\n\
yes\">&nbsp; </span>The upper case part of the commands is\n\
required and the lower\n\
case part of the command is optional.<span\n\
style=\"mso-spacerun: yes\">&nbsp;\n\
</span>Thus the command </span><b><span\n\
style=\"color:#993300\">A/F</span></b><span\n\
style=\"font-family:Verdana\"> is the same as the </span><b><span\n\
style=\"color:#993300\">Announce/Full</span></b><span\n\
style=\"font-family:Verdana\"> command.<span\n\
style=\"mso-spacerun: yes\"></span></p>\n\
<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\">\n\
<tbody>\n\
<tr>\n\
<td valign=\"top\" width=\"28%\">Announce</td>\n\
<td valign=\"top\" width=\"72%\">Announcement to all locally users</td>\n\
</tr>\n\
<tr>\n\
<td>Announce/Full</td>\n\
<td>Announcement to all users</td>\n\
</tr>\n\
<tr>\n\
<td>Bye</td>\n\
<td>Terminate the connection to the cluster</td>\n\
</tr>\n\
<tr>\n\
<td>CLEAR/QSL</td>\n\
<td>Remove a QSL route from the local QSL database</td>\n\
</tr>\n\
<tr>\n\
<td>CONFErence</td>\n\
<td>Enter a local conference</td>\n\
</tr>\n\
<tr>\n\
<td>CONFErence/Full</td>\n\
<td>Enter a network wide conference</td>\n\
</tr>\n\
<tr>\n\
<td>DB</td>\n\
<td>Built in database information</td>\n\
</tr>\n\
<tr>\n\
<td>DEelete</td>\n\
<td>Delete a mail message</td>\n\
</tr>\n\
<tr>\n\
<td>DIrectroy</td>\n\
<td>List mail messages</td>\n\
</tr>\n\
<tr>\n\
<td>Dx</td>\n\
<td>Enter as DX spot</td>\n\
</tr>\n\
<tr>\n\
<td>Help</td>\n\
<td>Command for help</td>\n\
</tr>\n\
<tr>\n\
<td>List</td>\n\
<td>List mail messages</td>\n\
</tr>\n\
<tr>\n\
<td>Quit</td>\n\
<td>Terminate the connection to the cluster</td>\n\
</tr>\n\
<tr>\n\
<td>Read</td>\n\
<td>Read a mail message</td>\n\
</tr>\n\
<tr>\n\
<td>REPly</td>\n\
<td>Reply to a mail message</td>\n\
</tr>\n\
<tr>\n\
<td>Send</td>\n\
<td>Send a mail message</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/ANNouncements</td>\n\
<td>Turn on announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/BEep</td>\n\
<td>Turn on a beep for DX and Announcement spots</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/DX_Announcements</td>\n\
<td>Activate DX spots</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/DXSqth</td>\n\
<td>Turn on the display of the spotter QTH</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/EMAIL</td>\n\
<td>Set your Internet email address</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/FILTER</td>\n\
<td>Set the spot filters</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/HEre</td>\n\
<td>Let others know you are at your station</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/HOMenode</td>\n\
<td>Set your home node</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/LOCAtion</td>\n\
<td>Set the location (lat/lon) of your station</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/LOGIN_announcements</td>\n\
<td>Show user logins</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/Name</td>\n\
<td>Set your name</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/NOAnnouncements</td>\n\
<td>Turn off announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/NOBeep</td>\n\
<td>Turn off the beep for DX and Announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/NODX_Announcements</td>\n\
<td>Turn off DX announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/NODXSqth</td>\n\
<td>Turn off the display of the spotter QTH</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/NOHere</td>\n\
<td>Indicate you are away from your station</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/NOLOGin_announcements</td>\n\
<td>Turn off login announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/NOTalk</td>\n\
<td>Turn off the display of talk messages</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/NOWWV_announcements</td>\n\
<td>Turn off the display of WWV spots</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/NOWX_announcements</td>\n\
<td>Turn off the display of weather announcements</td>\n\
</tr>\n\
<tr>\n\
<td>Set/PHONE</td>\n\
<td>Set your phone number</td>\n\
</tr>\n\
<tr>\n\
<td>Set/QRA</td>\n\
<td>Set the QRA for your station</td>\n\
</tr>\n\
<tr>\n\
<td>Set/QSL</td>\n\
<td>Add a QSL route to the local database</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/QTH</td>\n\
<td>Set your location (city, etc)</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/TAlk</td>\n\
<td>Turn on the display of talk messages</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/WWV_announcements</td>\n\
<td>Turn on the display of WWV spots</td>\n\
</tr>\n\
<tr>\n\
<td>SEt/WX_announcements</td>\n\
<td>Turn on the display of weather announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/ANnounce</td>\n\
<td>Show previous announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/ARCHive</td>\n\
<td>Show the files in the archive folder</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/BUCmaster</td>\n\
<td>Show callbook information for a specified callsign</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/BULLEtins</td>\n\
<td>Show the files in the bulletins folder</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/CBA</td>\n\
<td>Show callbook information for a specified callsign</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/CLuster</td>\n\
<td>Show the configuration of the cluster</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/Configuration</td>\n\
<td>Show the users on the node</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/Dx</td>\n\
<td>Show previous DX spots</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/Dx SQL</td>\n\
<td>Query for past DX using SQL</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/EMAIL</td>\n\
<td>Show a users Internet email address</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/FDx</td>\n\
<td>Display a formatted SH/DX command</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/FILEs</td>\n\
<td>Show the files in the files folder</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/FILTER</td>\n\
<td>Shows the current DX spot filters</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/FITu</td>\n\
<td>Display a formatted SH/ITU</td>\n\
</tr>\n\
<tr>\n\
<td>#Formatted\">SHow/FZOne</td>\n\
<td>Display a formatted SH/ZONE</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/Grid</td>\n\
<td>Show the MaidenHead grid locator for a station</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/HAM</td>\n\
<td>Show callbook information for a specified callsign</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/Heading</td>\n\
<td>Show the heading and distance to a station</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/HOMEnode</td>\n\
<td>Show a users homenode</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/ITu</td>\n\
<td>Show past DX based on a ITU zone</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/LOCation</td>\n\
<td>Show the location (lat/lon) of a station</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/LOG</td>\n\
<td>Show the node logins for a call</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/Muf</td>\n\
<td>Show the MUF for a country</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/NEeds</td>\n\
<td>Show the CTY needs for a station</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/NOdes</td>\n\
<td>Displays a list of nodes in the network</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/Prefix</td>\n\
<td>Show the prefix information for a call</td>\n\
</tr>\n\
<tr>\n\
<td>Show/QRA</td>\n\
<td>Show the station QRA</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/Qsl</td>\n\
<td>Show QSL information for a call</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/STation</td>\n\
<td>Show detail information for a call</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/Sun</td>\n\
<td>Show the sunrise/sunset for a location</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/TAlk</td>\n\
<td>Show past talk messages</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/TIme</td>\n\
<td>Show the time </td>\n\
</tr>\n\
<tr>\n\
<td>SHow/TIP</td>\n\
<td>Show a tip about using the cluster</td>\n\
</tr>\n\
<tr>\n\
<td>Show/UPTime</td>\n\
<td>Shows the uptime for the node</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/Users</td>\n\
<td>Show the users connected to the node</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/Version</td>\n\
<td>Shows the AR-Cluster software version</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/WWv</td>\n\
<td>Shows past WWV information</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/WX</td>\n\
<td>Show past weather announcements</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/WXStation</td>\n\
<td>Show data from an optional weather station</td>\n\
</tr>\n\
<tr>\n\
<td>SHow/Zone</td>\n\
<td>Show past DX based on a CQ zone</td>\n\
</tr>\n\
<tr>\n\
<td>Talk</td>\n\
<td>Talk to a station</td>\n\
</tr>\n\
<tr>\n\
<td>Talk/Timestamp</td>\n\
<td>Talk to a station with a timestamp</td>\n\
</tr>\n\
<tr>\n\
<td>Type/ARChive</td>\n\
<td>Display a file in the archive folder</td>\n\
</tr>\n\
<tr>\n\
<td>Type/BULletins</td>\n\
<td>Display a file in the bulletin folder</td>\n\
</tr>\n\
<tr>\n\
<td>Type/FILes</td>\n\
<td>Display a file in the files folder</td>\n\
</tr>\n\
<tr>\n\
<td>Wwv</td>\n\
<td>Send a WWV spot to the cluster</td>\n\
</tr>\n\
<tr>\n\
<td>WX</td>\n\
<td>Make a local weather announcement</td>\n\
</tr>\n\
<tr>\n\
<td>WX/Full</td>\n\
<td>Make a weather announcement to the network</td>\n\
</tr>\n\
</tbody>\n\
</table>\n\
</div>\n\
</body>\n\
</html>";
