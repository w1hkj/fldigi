// ----------------------------------------------------------------------------
// weather.cxx  -- a part of fldigi
//
// Copyright (C) 2012
//		Dave Freese, W1HKJ
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

#ifdef __MINGW32__
#  include "compat.h"
#endif

#include <sys/time.h>
#include "signal.h"
#include <string>
#include <iostream>
#include <cstring>
#include <cmath>
#include <cctype>

#include "threads.h"

#include "misc.h"
#include "configuration.h"

#include "main.h"
#include "confdialog.h"
#include "fl_digi.h"
#include "trx.h"

#include "xmlreader.h"

#include "qrunner.h"
#include "debug.h"
#include "network.h"

#include "weather.h"

using namespace std;

void getwx(string& wx, const char *metar)
{
	string url;
	string text;
	string field;
	string wxsta;

	if (!metar)
		wxsta = progdefaults.wx_sta;
	else
		wxsta = metar;

	wx.clear();

	for (size_t n = 0; n < wxsta.length(); n++) wxsta[n] = toupper(wxsta[n]);

	url.assign("http://weather.noaa.gov/pub/data/observations/metar/decoded/")
	   .append(wxsta).append(".TXT");

	int ret = fetch_http(url, text, 5.0);

	if (ret == -1) return;

	size_t p, p1, p2, p3;
	p2 = text.find(wxsta);
	if (p2 == string::npos) return;
	p1 = text.rfind("\n", p2) + 1;
	p3 = text.find("ob:", p2);
	if (p3 == string::npos) return;

	string wx_full = text.substr(p1, p3 - p1);
	if (progdefaults.wx_full) {
		wx.assign("Weather:\n").append(wx_full);
		return;
	}

	wx.assign("Weather:\n");

	if (progdefaults.wx_station_name) { // parse noun name
		if (text[p2-1] == '(') { // have valid line
			wx.append(text.substr(p1, p2 - p1 -2)).append("\n");
		}
	}

	if (progdefaults.wx_condx) {
		size_t p4, p5, p6;
		wx.append("Condx: ");
		if ((p4 = wx_full.find("Weather: ")) != string::npos) {
			p5 = p4 + 9;
			p6 = wx_full.find("\n", p5);
			if (p6 != string::npos)
				wx.append(wx_full.substr(p5, p6 - p5));
		}
		if ((p4 = wx_full.find("Sky conditions: ")) != string::npos) {
			p5 = p4 + 16;
			p6 = wx_full.find("\n", p5);
			if (p6 != string::npos)
				wx.append(wx_full.substr(p5, p6 - p5));
		}
		wx.append("\n");
	}

	p = text.find(wxsta, p3 + 1);
	text.erase(0, p + 1 + wxsta.length());

	while(text.length()) {
		p = text.find(" ");
		if (p != string::npos) {
			field = text.substr(0, p);
			text.erase(0, p+1);
		} else {
			field = text;
			text.clear();
		}
// parse field contents
		if (field == "AUTO") ;

		else if ((progdefaults.wx_mph || progdefaults.wx_kph) &&
				 field.rfind("KT") == field.length() - 2) { // wind dir / speed
			int knots;
			sscanf(field.substr(3,2).c_str(), "%d", &knots);
			wx.append("Wind:  ").append(field.substr(0,3)).append(" at ");
			char ctemp[10];
			if (progdefaults.wx_mph) {
				snprintf(ctemp, sizeof(ctemp), "%d mph  ", (int)(knots * 528.0 / 600.0));
				wx.append(ctemp);
			}
			if (progdefaults.wx_kph) {
				snprintf(ctemp, sizeof(ctemp), "%d kph ", (int)(knots * 528.0 * 1.8288 / 600.0));
				wx.append(ctemp);
			}
			wx.append("\n");
		}

		else if ((p = field.find("/") ) != string::npos &&
				 (progdefaults.wx_fahrenheit || progdefaults.wx_celsius) ) { // temperature / dewpoint
			string cent = field.substr(0, p);
			if (cent[0] == 'M') cent[0] = '-';
			int tempC, tempF;
			sscanf(cent.c_str(), "%d", &tempC);
			tempF = (int)(tempC * 2.12 + 32);
			wx.append("Temp:  ");
			char ctemp[10];
			if (progdefaults.wx_fahrenheit) {
				snprintf(ctemp, sizeof(ctemp), "%d F  ", tempF);
				wx.append(ctemp);
			}
			if (progdefaults.wx_celsius) {
				snprintf(ctemp, sizeof(ctemp), "%d C", tempC);
				wx.append(ctemp);
			}
			wx.append("\n");
		}

		if (progdefaults.wx_inches || progdefaults.wx_mbars) {
			if ((field[0] == 'A' && field.length() == 5) || field[0] == 'Q') {
				float inches;
				sscanf(field.substr(1).c_str(), "%f", &inches);
				if (field[0] == 'A')
					inches /= 100.0;
				else
					inches /= 33.87;
				wx.append("Baro:  ");
				char ctemp[20];
				if (progdefaults.wx_inches) {
					snprintf(ctemp, sizeof(ctemp), "%.2f in Hg  ", inches);
					wx.append(ctemp);
				}
				if (progdefaults.wx_mbars) {
					snprintf(ctemp, sizeof(ctemp), "%.0f mbar", inches * 33.87);
					wx.append(ctemp);
				}
				wx.append("\n");
			}
		}
	}

	return;

}
//======================================================================

