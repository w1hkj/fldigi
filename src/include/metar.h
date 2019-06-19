// ----------------------------------------------------------------------------
// Copyright (C) 2019
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef METAR_H
#define METAR_H

#include "network.h"

class Metar {
	Url url;
	std::string _wx_text_full;
	std::string _wx_text_parsed;
	std::string _metar_station;
	std::string _metar_text;
	std::string _station_name;

	std::string _field;
	std::string _conditions;
	std::string _temp;
	std::string _winds;
	std::string _baro;

	bool _inches;
	bool _mbars;
	bool _fahrenheit;
	bool _celsius;
	bool _mph;
	bool _kph;
	bool _condx;
	bool _name;

	bool _debug;
public:
	Metar() { init(); }
	Metar(std::string _station) {
		init();
		_metar_station = _station;
	}
	~Metar() {}

	void init() {
		_metar_station.clear();
		_metar_text.clear();
		_wx_text_full.clear();
		_wx_text_parsed.clear();
		_station_name.clear();

		_field.clear();
		_conditions.clear();
		_temp.clear();
		_winds.clear();
		_baro.clear();

		_inches = _mbars =
		_fahrenheit = _celsius =
		_mph = _kph =
		_condx = _name = true;

		_debug = false;
	}

	void parse();
	int get();
	int get(std::string station) {
		_metar_station = station;
		return get();
	}

	void station(std::string s) { _metar_station = s; }
	std::string station() { return _metar_station; }

	std::string station_name() { return _station_name; }

	std::string full() { return _wx_text_full; }
	std::string parsed() { return _wx_text_parsed; }

	void params( bool inches, bool mbars, 
				 bool fahrenheit, bool celsius,
				 bool mph, bool kph,
				 bool condx, bool name)
	{
		_inches = inches;
		_mbars = mbars;
		_fahrenheit= fahrenheit;
		_celsius = celsius;
		_mph = mph;
		_kph = kph;
		_condx = condx;
		_name = name;
	}
	void inches(bool b) { _inches = b; }
	bool inches() { return _inches; }

	void mbars(bool b) { _mbars = b; }
	bool mbars() { return _mbars; }

	void fahrenheit(bool b) { _fahrenheit = b; }
	bool fahrenheit() { return _fahrenheit; }

	void celsius(bool b) { _celsius = b; }
	bool celsius() { return _celsius; }

	void mph(bool b) { _mph = b; }
	bool mph() { return _mph; }

	void kph(bool b) { _kph = b; }
	bool kph() { return _kph; }

	void condx(bool b) { _condx = b; }
	bool condx() { return _condx; }

	void name(bool b) { _name = b; }
	bool name() { return _name; }

	void debug(bool on) { };//_debug = on; url.debug(on); }
};

#endif //METAR_H

