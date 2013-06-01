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

/*======================================================================
 * 
 * WEATHER The weather group
 * iiddppooxx
 * ii is intensity group
 * ii Description
 * -  light
 *    moderate
 * +  heavy
 * VC in the vicinity
 * 
 * dd is the descriptor group
 * dd Description
 * MI shallow
 * PR partial
 * BC patches
 * DR low drifting
 * BL blowing
 * SH shower
 * TS thunderstorm
 * FZ freezing 
 * 
 * pp is the precipitation group
 * pp Description
 * DZ drizzle
 * RA rain
 * SN snow
 * SG snow grains
 * IC ice crystals
 * PE ice pellets
 * GR hail
 * GS small hail/snow pellets
 * UP unknown
 * 
 * oo is the obscuration group
 * oo Description
 * BR mist
 * FG fog
 * FU smoke
 * VA volcanic ash
 * DU dust
 * SA sand
 * HZ haze
 * PY spray
 * 
 * xx is the misc group
 * xx Description
 * PO dust whirls
 * SQ squalls
 * FC funnel cloud/tornado/waterspout
 * SS duststorm
 * 
 * CLOUDS
 * The cloud levels
 * ccchhhtt
 * ccc is the coverage 
 * CLR or SKC = clear 
 * FEW = 1/8 coverage 
 * SCT = 2,3,4/8 coverage 
 * BKN = 5,6,7/8 coverage 
 * OVC = overcast 
 * VV = vertical visibility for obscuration
 * hhh is the height of base in 30m or 100ft increments. ie 30 = 3000 feet
 * tt is an optional type
 * CU = cumulus
 * CB = cumulonumbus
 * TCU = towering cumulus 
 * CI = cirrus
*/

struct wxpairs {const char *grp; const char *name;};

static wxpairs precip[] = {
{"DZ", "drizzle"},
{"RA", "rain"}, 
{"SN", "snow"},
{"SG", "snow grains"},
{"IC", "ice crystals"},
{"PE", "ice pellets"},
{"GR", "hail"},
{"GS", "small hail / show pellets"},
{"UP", "unknown"},
{NULL, NULL} };

static wxpairs intensity[] = {
{"-", "light"},
{"+", "heavy"},
{"VC", "in the vicinity"},
{NULL, NULL} };

static wxpairs descriptor[] = {
{"MI", "shallow "},
{"PR", "partial"},
{"BC", "patches"},
{"DR", "low drifting"},
{"BL", "blowing"},
{"SH", "shower"},
{"TS", "thunderstorm"},
{"FZ", "freezing"},
{NULL, NULL} };

static wxpairs obscure[] = {
{"BR", "mist"},
{"FG", "fog"},
{"FU", "smoke"},
{"VA", "volcanic ash"},
{"DU", "dust"},
{"SA", "sand"},
{"HZ", "haze"},
{"PY", "spray"},
{NULL, NULL} };

static wxpairs misc[] = {
{"PO", "dust whirls"},
{"SQ", "squalls"},
{"FC", "funnel cloud/tornado/waterspout"},
{"SS", "duststorm"},
{NULL, NULL} };

static wxpairs clouds[] = {
{"CLR", "clear skies"},
{"SKC", "clear skies"},
{"FEW", "few clouds"},
{"SCT", "scattered clouds"},
{"BKN", "broken cloud cover"},
{"OVC", "overcast"},
{NULL, NULL} };

static wxpairs cloud_type[] = {
{"CU", "cumulus"},
{"CB", "cumulonumbus"},
{"TCU", "towering cumulus"}, 
{"CI", "cirrus"},
{NULL, NULL} };

void getwx(string& wx, const char *metar)
{
	string url;
	string text;
	string field;
	string wxsta;
	string name = "";
	string condx = "";
	string temperature = "";
	string winds = "";
	string baro = "";

	size_t p, p1, p2, p3;

	if (!metar)
		wxsta = progdefaults.wx_sta;
	else
		wxsta = metar;

	wx.clear();

	for (size_t n = 0; n < wxsta.length(); n++)
		wxsta[n] = toupper(wxsta[n] & 0x7F);

	url.assign("http://weather.noaa.gov/pub/data/observations/metar/decoded/")
	   .append(wxsta).append(".TXT");

	if (!fetch_http_gui(url, text, 5.0)) {
		LOG_WARN("%s", "url not available\n");
		return;
	}

	LOG_DEBUG("\n%s", text.c_str());

	p2 = text.find(wxsta);
	if (p2 == string::npos) {
		LOG_WARN("%s", "station not found\n");
		return;
	}

	string eoh = progdefaults.wx_eoh;
	if (eoh.empty()) eoh = "Connection: close";

	p1 = text.find(eoh);
	if (p1 != string::npos) {
		p1 = text.find("\n",p1);
		text.erase(0, p1);
		while (text[0] == '\r' || text[0] == '\n') text.erase(0,1);
		p1 = text.find("\n");
		name = text.substr(0, p1);
	}

	p3 = text.find("ob:");
	if (p3 == string::npos) {
		LOG_WARN("%s", "observations not available\n");
		return;
	}

	if (progdefaults.wx_full) {
		wx.assign(text.substr(0, p3));
		return;
	}

	p = text.find(wxsta, p3 + 1);
	text.erase(0, p + 1 + wxsta.length());
	p = text.find("\n");
	if (p != string::npos) text.erase(p);

// parse field contents
	bool parsed = false;
	while(text.length()) { // each ob: field is separated by a space or end of file
		parsed = false;
		p = text.find(" ");
		if (p != string::npos) {
			field = text.substr(0, p);
			text.erase(0, p+1);
		} else {
			field = text;
			text.clear();
		}
		if (field == "RMK") break;
// parse for general weather
// iiddppooxx
		if (field == "AUTO") ;
		else if ((p = field.rfind("KT")) != string::npos) {
			if (p == field.length() - 2) { // wind dir / speed
				int knots;
				sscanf(field.substr(3,2).c_str(), "%d", &knots);
				winds.clear();
				winds.append(field.substr(0,3)).append(" at ");
				char ctemp[10];
				if (progdefaults.wx_mph) {
					snprintf(ctemp, sizeof(ctemp), "%d mph  ", (int)ceil(knots * 600.0  / 528.0 ));
					winds.append(ctemp);
				}
				if (progdefaults.wx_kph) {
					snprintf(ctemp, sizeof(ctemp), "%d kph ", (int)ceil(knots * 600.0 * 1.6094 / 528.0));
					winds.append(ctemp);
				}
			}
		}
		else if ((p = field.rfind("MPS")) != string::npos) {
			if (p == field.length() - 3) { // wind dir / speed in meters / second
				int mps;
				sscanf(field.substr(3,2).c_str(), "%d", &mps);
				winds.clear();
				winds.append(field.substr(0,3)).append(" at ");
				char ctemp[10];
				if (progdefaults.wx_mph) {
					snprintf(ctemp, sizeof(ctemp), "%d mph  ", (int)ceil(mps * 2.2369));
					winds.append(ctemp);
				}
				if (progdefaults.wx_kph) {
					snprintf(ctemp, sizeof(ctemp), "%d kph ", (int)ceil(mps * 3.6));
					winds.append(ctemp);
				}
			}
		}
		else if ((p = field.find("/") ) != string::npos) { // temperature / dewpoint
			string cent = field.substr(0, p);
			if (cent[0] == 'M') cent[0] = '-';
			int tempC, tempF;
			sscanf(cent.c_str(), "%d", &tempC);
			tempF = (int)(tempC * 1.8 + 32);
			temperature.clear();
			char ctemp[10];
			if (progdefaults.wx_fahrenheit) {
				snprintf(ctemp, sizeof(ctemp), "%d F  ", tempF);
				temperature.append(ctemp);
			}
			if (progdefaults.wx_celsius) {
				snprintf(ctemp, sizeof(ctemp), "%d C", tempC);
				temperature.append(ctemp);
			}
		}
		else if ((field[0] == 'A' && field.length() == 5) || field[0] == 'Q') {
			float inches;
			sscanf(field.substr(1).c_str(), "%f", &inches);
			if (field[0] == 'A')
				inches /= 100.0;
			else
				inches /= 33.87;
			baro.clear();
			char ctemp[20];
			if (progdefaults.wx_inches) {
				snprintf(ctemp, sizeof(ctemp), "%.2f in Hg  ", inches);
				baro.append(ctemp);
			}
			if (progdefaults.wx_mbars) {
				snprintf(ctemp, sizeof(ctemp), "%.0f mbar", inches * 33.87);
				baro.append(ctemp);
			}
		} if (!parsed) {
			for (wxpairs *pp = precip; pp->grp != NULL; pp++) {
				if (field.find(pp->grp) != string::npos) { // found a precip group
					wxpairs *ii, *dd, *oo, *xx;
					for (ii = intensity; ii->grp != NULL; ii++)
						if (field.find(ii->grp) != string::npos) break;
					for (dd = descriptor; dd->grp != NULL; dd++)
						if (field.find(dd->grp) != string::npos) break;
					for (oo = obscure; oo->grp != NULL; oo++)
						if (field.find(oo->grp) != string::npos) break;
					for (xx = misc; xx->grp != NULL; xx++)
						if (field.find(xx->grp) != string::npos) break;
					if (ii->grp != NULL) condx.append(ii->name).append(" ");
					if (dd->grp != NULL) condx.append(dd->name).append(" ");
					condx.append(pp->name);
					if (oo->grp != NULL) condx.append(", ").append(oo->name);
					if (xx->grp != NULL) condx.append(", ").append(xx->name);
					parsed = true;
				}
			}
		} if (!parsed) {
			wxpairs *oo;
			for (oo = obscure; oo->grp != NULL; oo++)
				if (field.find(oo->grp) != string::npos) break;
			if (oo->grp != NULL) {
				condx.append(" ").append(oo->name);
				parsed = true;
			}
		} if (!parsed) {
// parse for cloud cover
// use only the first occurance of sky cover report; it is lowest altitude
// cloud cover is reported multiple times for sounding stations
			for (wxpairs *cc = clouds; cc->grp != NULL; cc++) {
				if (field.find(cc->grp) != string::npos) {
					if (condx.find(cc->name) != string::npos) break;
					if (condx.empty())
						condx.append(cc->name);
					else
						condx.append(", ").append(cc->name);
					wxpairs *ct;
					for (ct = cloud_type; ct->grp != NULL; ct++) {
						if (field.find(ct->grp) != string::npos) {
							if (ct->grp != NULL)
								condx.append(" ").append(ct->name);
							break;
						}
					}
					parsed = true;
					break;
				}
			}
		}
	}

	if (progdefaults.wx_station_name && !name.empty()) { // parse noun name
		wx.append("Sta:  ").append(name).append("\n");
	}
	if (progdefaults.wx_condx && !condx.empty()) {
		wx.append("Cond: ").append(condx).append("\n");
	}
	if ((progdefaults.wx_mph || progdefaults.wx_kph) && !winds.empty()){
		wx.append("Wind: ").append(winds).append("\n");
	}
	if ((progdefaults.wx_fahrenheit || progdefaults.wx_celsius) && !temperature.empty() ) {
		wx.append("Temp: ").append(temperature).append("\n");
	}
	if ((progdefaults.wx_inches || progdefaults.wx_mbars) && !baro.empty()) {
		wx.append("Baro: ").append(baro).append("\n");
	}

	return;

}

void get_METAR_station()
{
	cb_mnuVisitURL(0, (void*)string("http://www.rap.ucar.edu/weather/surface/stations.txt").c_str());
}
