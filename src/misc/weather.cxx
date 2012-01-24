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

void getwx(std::string& wx, const char *metar)
{
	std::string url;
	std::string retxml;
	std::string wxsta;
	if (!metar)
		wxsta = progdefaults.wx_sta;
	else
		wxsta = metar;

	size_t p1, p2;

	wx.clear();
	if (wxsta.empty()) return;

	for (size_t n = 0; n < wxsta.length(); n++) wxsta[n] = toupper(wxsta[n]);

	url.assign("http://www.weather.gov/xml/current_obs/")
	   .append(wxsta).append(".xml");

	int ret = fetch_http(url, retxml, 5.0);

	if (ret == -1)
		return;

	if (progdefaults.wx_condx &&
		(p1 = retxml.find("<weather>")) != std::string::npos) {
		p1 += 9;
		p2 = retxml.find("</weather>", p1);
		if (p2 != std::string::npos)
			wx.append("WX:   ").append(retxml.substr(p1, p2 - p1)).append("\n");
	}
	if (progdefaults.wx_temp &&
		(p1 = retxml.find("<temp_f>")) != std::string::npos) {
		p1 += 8;
		p2 = retxml.find("</temp_f>", p1);
		if (p2 != std::string::npos) {
			wx.append("Temp: ");
			if (!progdefaults.wx_celsius)
				wx.append(retxml.substr(p1, p2 - p1)).append(" F\n");
			else {
				float temp;
				sscanf(retxml.substr(p1, p2 - p1).c_str(), "%f", &temp);
				char ctemp[10];
				snprintf(ctemp, sizeof(ctemp), "%.1f C\n", temp);
				wx.append(ctemp);
			}
		}
	}
	if (progdefaults.wx_wind &&
		(p1 = retxml.find("<wind_degrees>")) != std::string::npos) {
		p1 += 14;
		p2 = retxml.find("</wind_degrees>", p1);
		if (p2 != std::string::npos) {
			wx.append("Wind: ").append(retxml.substr(p1, p2 - p1));
			if ((p1 = retxml.find("<wind_mph>")) != std::string::npos) {
				p1 += 10;
				p2 = retxml.find("</wind_mph>");
				if (p2 != std::string::npos) {
					wx.append(" at ");
					if (!progdefaults.wx_kph)
						wx.append(retxml.substr(p1, p2 - p1)).append(" mph");
					else {
						float mph;
						sscanf(retxml.substr(p1, p2 - p1).c_str(), "%f", &mph);
						char ckph[10];
						snprintf(ckph, sizeof(ckph), "%.1f kph", 
							mph * 1.8288);
						wx.append(ckph);
					}
				}
			}
			wx.append("\n");
		}
	}
	if (progdefaults.wx_baro &&
		(p1 = retxml.find("<pressure_in>")) != std::string::npos) {
		p1 += 13;
		p2 = retxml.find("</pressure_in>", p1);
		if (p2 != std::string::npos) {
			wx.append("Baro: ");
			if (!progdefaults.wx_mbars)
				wx.append(retxml.substr(p1, p2 - p1)).append(" in.");
			else {
				float inches;
				sscanf(retxml.substr(p1, p2 - p1).c_str(), "%f", &inches);
				char cmbar[10];
				snprintf(cmbar, sizeof(cmbar), "%.0f mbar", 
					inches * 33.87);
				wx.append(cmbar);
			}
		}
	}
	size_t len = wx.length();
	if (wx[len-1] == '\n') wx.erase(len-1);

	return;
}
