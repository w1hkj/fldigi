// ----------------------------------------------------------------------------
// dxcc.h
//
// Copyright (C) 2009
//		Stelios Bounanos, M0GLD
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

#ifndef DXCC_H_
#define DXCC_H_

#include <vector>

struct dxcc {
	const char* country;
	int cq_zone;
	int itu_zone;
	char continent[3];
	float latitude;
	float longitude;
	float gmt_offset;
	dxcc(const char* cn = "", int cq = 0, int itu = 0, const char* ct = "",
	     float lat = 0.0f, float lon = 0.0f, float tz = 0.0f);
};

enum qsl_t { QSL_LOTW, QSL_EQSL, QSL_END };
extern const char* qsl_names[];

bool dxcc_open(const char* filename);
bool dxcc_is_open(void);
void dxcc_close(void);
const dxcc* dxcc_lookup(const char* callsign);
const std::vector<dxcc*>* dxcc_entity_list(void);

bool qsl_open(const char* filename, qsl_t qsl_type);
unsigned char qsl_is_open(void);
void qsl_close(void);
unsigned char qsl_lookup(const char* callsign);

extern void reload_cty_dat();
extern void default_cty_dat_pathname();
extern void select_cty_dat_pathname();

#endif // DXCC_H_
