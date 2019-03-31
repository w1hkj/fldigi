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
#include "metar.h"

void getwx(std::string& wx, std::string wxsta)
{
	std::string metar;
	if (wxsta.empty())
		metar = progdefaults.wx_sta;
	else
		metar = wxsta;

	for (size_t n = 0; n < metar.length(); n++)
		metar[n] = toupper(metar[n]);

	Metar local_wx;

	local_wx.params(	progdefaults.wx_inches, progdefaults.wx_mbars, 
				progdefaults.wx_fahrenheit, progdefaults.wx_celsius,
				progdefaults.wx_mph, progdefaults.wx_kph,
				progdefaults.wx_condx, progdefaults.wx_station_name);

	local_wx.debug(true);

	if (local_wx.get(metar) == 0) {
		if (progdefaults.wx_full) {
			wx = local_wx.full();
			return;
		}
		wx = local_wx.parsed();
	} else
		wx.clear();
	return;

}

void get_METAR_station()
{
	cb_mnuVisitURL(0, (void*)std::string("http://www.rap.ucar.edu/weather/surface/stations.txt").c_str());
}
