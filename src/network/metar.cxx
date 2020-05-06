// ----------------------------------------------------------------------------
// metar.cxx
//
// Copyright (C) 2019
//		David Freese, W1HKJ
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

#include "metar.h"

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

void Metar::parse()
{
	size_t p, p1, p2, p3;

	p = _metar_text.find(_metar_station);
	if (p == std::string::npos) {
		_wx_text_full.assign(_metar_station).append(" not found in url response!");
		_wx_text_full.append("\n").append(_metar_text).append("\n");
		return;
	}

	const char *cl = "Content-Length:";
	int content_length;
	p1 = _metar_text.find(cl);
	if (p1 != std::string::npos) {
		content_length = atol(&_metar_text[p1 + strlen(cl)]);
		_metar_text.erase(0, _metar_text.length() - content_length);
	} else {
		p2 = _metar_text.find("(");
		if (p2 != std::string::npos) {
			p3 = _metar_text.rfind("\n", p2);
			if (p3 != std::string::npos)
				_metar_text.erase(0,p3 + 1);
		}
		else while ( (p2 = _metar_text.find("\r\n")) != std::string::npos)
			_metar_text.erase(0, p2 + 2);
	}

	p1 = _metar_text.find("\n");
	_station_name = _metar_text.substr(0, p1);
	if ((p2 = _station_name.find("(")) != std::string::npos)
		_station_name.erase(p2 - 1);

	p3 = _metar_text.find("ob:");
	if (p3 == std::string::npos) {
		_wx_text_full.assign(_metar_station).append(" observations not available");
		_wx_text_parsed.assign(_wx_text_full);
		return;
	}

	_wx_text_full.assign(_metar_text.substr(0, p3));

	p = _metar_text.find(_metar_station, p3 + 1);
	_metar_text.erase(0, p + 1 + _metar_station.length());
	p = _metar_text.find("\n");
	if (p != std::string::npos) _metar_text.erase(p);

// parse field contents
	bool parsed = false;
	_conditions.clear();
	while(_metar_text.length()) { // each ob: field is separated by a space or end of file
		parsed = false;
		p = _metar_text.find(" ");
		if (p != std::string::npos) {
			_field = _metar_text.substr(0, p);
			_metar_text.erase(0, p+1);
		} else {
			_field = _metar_text;
			_metar_text.clear();
		}
		if (_field == "RMK") break;
// parse for general weather
// iiddppooxx
		if (_field == "AUTO") ;
		else if ((p = _field.rfind("KT")) != std::string::npos) {
			if (p == _field.length() - 2) { // wind dir / speed
				int knots;
				_winds.clear();
				if (sscanf(_field.substr(3,2).c_str(), "%d", &knots) == 1) {
					_winds.append(_field.substr(0,3)).append(" at ");
					char ctemp[10];
					if (_mph) {
						snprintf(ctemp, sizeof(ctemp), "%d mph  ", (int)ceil(knots * 600.0  / 528.0 ));
						_winds.append(ctemp);
					}
					if (_kph) {
						snprintf(ctemp, sizeof(ctemp), "%d km/h ", (int)ceil(knots * 600.0 * 1.6094 / 528.0));
						_winds.append(ctemp);
					}
				}
			}
		}
		else if ((p = _field.rfind("MPS")) != std::string::npos) {
			if (p == _field.length() - 3) { // wind dir / speed in meters / second
				int mps;
				_winds.clear();
				if (sscanf(_field.substr(3,2).c_str(), "%d", &mps) == 1) {
					_winds.append(_field.substr(0,3)).append(" at ");
					char ctemp[10];
					if (_mph) {
						snprintf(ctemp, sizeof(ctemp), "%d mph  ", (int)ceil(mps * 2.2369));
						_winds.append(ctemp);
					}
					if (_kph) {
						snprintf(ctemp, sizeof(ctemp), "%d km/h ", (int)ceil(mps * 3.6));
						_winds.append(ctemp);
					}
				}
			}
		}
		else if ((p = _field.find("/") ) != std::string::npos) { // temperature / dewpoint
			std::string cent = _field.substr(0, p);
			if (cent[0] == 'M') cent[0] = '-';
			int tempC, tempF;
			_temp.clear();
			if (sscanf(cent.c_str(), "%d", &tempC) == 1) {
				tempF = (int)(tempC * 1.8 + 32);
				char ctemp[10];
				if (_fahrenheit) {
					snprintf(ctemp, sizeof(ctemp), "%d F  ", tempF);
					_temp.append(ctemp);
				}
				if (_celsius) {
					snprintf(ctemp, sizeof(ctemp), "%d C", tempC);
					_temp.append(ctemp);
				}
			}
		}
		else if ((_field[0] == 'A' && _field.length() == 5) || _field[0] == 'Q') {
			float inches;
			_baro.clear();
			if (sscanf(_field.substr(1).c_str(), "%f", &inches) == 1) {
				if (_field[0] == 'A')
					inches /= 100.0;
				else
					inches /= 33.87;
				char ctemp[20];
				if (_inches) {
					snprintf(ctemp, sizeof(ctemp), "%.2f in. Hg  ", inches);
					_baro.append(ctemp);
				}
				if (_mbars) {
					snprintf(ctemp, sizeof(ctemp), "%.0f mbar", floor(inches * 33.87));
					_baro.append(ctemp);
				}
			}
		} if (!parsed) {
			for (wxpairs *pp = precip; pp->grp != NULL; pp++) {
				if (_field.find(pp->grp) != std::string::npos) { // found a precip group
					wxpairs *ii, *dd, *oo, *xx;
					for (ii = intensity; ii->grp != NULL; ii++)
						if (_field.find(ii->grp) != std::string::npos) break;
					for (dd = descriptor; dd->grp != NULL; dd++)
						if (_field.find(dd->grp) != std::string::npos) break;
					for (oo = obscure; oo->grp != NULL; oo++)
						if (_field.find(oo->grp) != std::string::npos) break;
					for (xx = misc; xx->grp != NULL; xx++)
						if (_field.find(xx->grp) != std::string::npos) break;
					if (ii->grp != NULL) _conditions.append(ii->name).append(" ");
					if (dd->grp != NULL) _conditions.append(dd->name).append(" ");
					_conditions.append(pp->name);
					if (oo->grp != NULL) _conditions.append(", ").append(oo->name);
					if (xx->grp != NULL) _conditions.append(", ").append(xx->name);
					parsed = true;
				}
			}
		} if (!parsed) {
			wxpairs *oo;
			for (oo = obscure; oo->grp != NULL; oo++)
				if (_field.find(oo->grp) != std::string::npos) break;
			if (oo->grp != NULL) {
				_conditions.append(" ").append(oo->name);
				parsed = true;
			}
		} if (!parsed) {
// parse for cloud cover
// use only the first occurance of sky cover report; it is lowest altitude
// cloud cover is reported multiple times for sounding stations
			for (wxpairs *cc = clouds; cc->grp != NULL; cc++) {
				if (_field.find(cc->grp) != std::string::npos) {
					if (_conditions.find(cc->name) != std::string::npos) break;
					if (_conditions.empty())
						_conditions.append(cc->name);
					else
						_conditions.append(", ").append(cc->name);
					wxpairs *ct;
					for (ct = cloud_type; ct->grp != NULL; ct++) {
						if (_field.find(ct->grp) != std::string::npos) {
							if (ct->grp != NULL)
								_conditions.append(" ").append(ct->name);
							break;
						}
					}
					parsed = true;
					break;
				}
			}
		}
	}

	_wx_text_parsed.clear();
	if (_name && !_station_name.empty()) {
		_wx_text_parsed.append("Loc:  ").append(_station_name).append("\n");
	}
	if (_condx && !_conditions.empty()) {
		_wx_text_parsed.append("Cond: ").append(_conditions).append("\n");
	}
	if ((_mph || _kph) && !_winds.empty()){
		_wx_text_parsed.append("Wind: ").append(_winds).append("\n");
	}
	if ((_fahrenheit || _celsius) && !_temp.empty() ) {
		_wx_text_parsed.append("Temp: ").append(_temp).append("\n");
	}
	if ((_inches || _mbars) && !_baro.empty()) {
		_wx_text_parsed.append("Baro: ").append(_baro).append("\n");
	}

	return;

}

int Metar::get()
{
	std::string metar_url = "https://tgftp.nws.noaa.gov/data/observations/metar/decoded/";
	metar_url.append(_metar_station).append(".TXT");
	int ret = url.get(metar_url, _metar_text);
	if (ret == 0) parse();
	return ret;
}
