// ----------------------------------------------------------------------------
// dx_cluster.h  --  constants, variables, arrays & functions that need to be
//                  outside of any thread
//
// Copyright (C) 2006-2007
//		Dave Freese, W1HKJ
// Copyright (C) 2007-2010
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.  Adapted in part from code contained in gmfsk
// source code distribution.
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

#ifndef DX_CLUSTER_H
#define DX_CLUSTER_H

#include <string>

extern void DXcluster_init(void);
extern void DXcluster_close(void);
extern void DXcluster_connect(bool);
extern void DXcluster_submit();
extern void DXcluster_select();
extern void dxc_wwv_query();
extern void dxc_wwv_clear();
extern void dxc_help_query();
extern void dxc_help_clear();

extern void DXcluster_logoff();
extern void DXcluster_add_record();

extern void DXcluster_mode_check();
extern void DXcluster_band_check();
extern bool DXcluster_dupcheck();

extern bool DXcluster_connected;

extern void send_DXcluster_spot();

extern void dxcluster_hosts_save();
extern void dxcluster_hosts_load();

extern void dxcluster_hosts_clear_setup(Fl_Button*, void*);
extern void dxcluster_hosts_load_setup(Fl_Button*, void*);
extern void dxcluster_hosts_save_setup(Fl_Button*, void*);
extern void dxcluster_hosts_send_setup(Fl_Button*, void*);
extern void dxcluster_ccc_help(Fl_Button*, void*);
extern void dxcluster_arc_help(Fl_Button*, void*);
extern void dxcluster_dxc_help(Fl_Button*, void*);
extern void dxcluser_servers(Fl_Button*, void*);
extern void dxcluster_hosts_load();

extern void dxc_lines();

extern void dxc_click_m1(Fl_Button*, void*);
extern void dxc_click_m2(Fl_Button*, void*);
extern void dxc_click_m3(Fl_Button*, void*);
extern void dxc_click_m4(Fl_Button*, void*);
extern void dxc_click_m5(Fl_Button*, void*);
extern void dxc_click_m6(Fl_Button*, void*);
extern void dxc_click_m7(Fl_Button*, void*);
extern void dxc_click_m8(Fl_Button*, void*);

struct dxinfo {
	std::string  spotter;
	std::string  freq;
	std::string  dxcall;
	std::string  remark;
	std::string  time;
	std::string  info;
	std::string  state;
	std::string  country;
	std::string  toall;
	bool    dx;
	dxinfo() { clear(); } 
	void clear() {
		spotter.clear();
		freq.clear();
		dxcall.clear();
		remark.clear();
		time.clear();
		info.clear();
		country.clear();
		toall.clear();
		dx = false;
	}
	~dxinfo() {}
};

#endif
