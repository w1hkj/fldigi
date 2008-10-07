// ----------------------------------------------------------------------------
//      xmlrpc.cxx
//
// Copyright (C) 2008
//              Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
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

#include <config.h>

#include "xmlrpc.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <exception>
#include <cstdlib>

#include <signal.h>

#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

#include <FL/Fl_Value_Slider.H>

#include "globals.h"
#include "socket.h"
#include "threads.h"
#include "modem.h"
#include "trx.h"
#include "fl_digi.h"
#include "configuration.h"
#include "main.h"
#include "waterfall.h"
#include "macros.h"
#include "qrunner.h"

#if USE_HAMLIB
        #include "hamlib.h"
#endif
#include "rigMEM.h"
#include "rigio.h"
#include "debug.h"
#include "re.h"

using namespace std;

struct rpc_method
{
	rpc_method(const xmlrpc_c::methodPtr& m, const char* n)
		: method(m), name(n) { }
	xmlrpc_c::methodPtr method;
	const char* name;
};
typedef list<rpc_method> methods_t;
methods_t* methods = 0;

static pthread_t* server_thread;

XML_RPC_Server* XML_RPC_Server::inst = 0;

XML_RPC_Server::XML_RPC_Server()
{
	server_socket = new Socket;
	add_methods();
	server_thread = new pthread_t;
	run = true;
}
XML_RPC_Server::~XML_RPC_Server()
{
	run = false;
	if (server_thread) {
		pthread_kill(*server_thread, SIGUSR2);
		pthread_join(*server_thread, NULL);
		delete server_thread;
		server_thread = 0;
	}
	delete methods;
	methods = 0;
}


void XML_RPC_Server::start(const char* node, const char* service)
{
	if (inst)
		return;

	inst = new XML_RPC_Server;

	try {
		inst->server_socket->open(Address(node, service));
		inst->server_socket->bind();
	}
	catch (const SocketException& e) {
		LOG_ERROR("Could not start XML-RPC server (%s)", e.what());
		return;
	}

	pthread_create(server_thread, NULL, thread_func, NULL);
}

void XML_RPC_Server::stop(void)
{
	if (!inst)
		return;
	delete inst;
	inst = 0;
}

void* XML_RPC_Server::thread_func(void*)
{
	SET_THREAD_ID(XMLRPC_TID);

	xmlrpc_c::registry reg;
	for (methods_t::iterator i = methods->begin(); i != methods->end(); ++i)
		reg.addMethod(i->name, i->method);

	xmlrpc_c::serverAbyss server(xmlrpc_c::serverAbyss::constrOpt()
				     .registryP(&reg)
#ifndef NDEBUG
				     .logFileName(HomeDir + "xmlrpc.log")
#endif
	    		      );

	setup_signal_handlers();

	while (inst->run) {
		try {
			server.runConn(inst->server_socket->accept().fd());
		}
		catch (const SocketException& e) {
			if (e.error() != EINTR)
				LOG_ERROR("%s", e.what());
			break;
		}
		catch (...) {
			break;
		}
	}

	inst->server_socket->close();
	return NULL;
}

ostream& XML_RPC_Server::list_methods(ostream& out)
{
	add_methods();

	ios_base::fmtflags f = out.flags(ios::left);
	for (methods_t::const_iterator i = methods->begin(); i != methods->end(); ++i)
		out << setw(32) << i->name << setw(8) << i->method->signature()
		    << i->method->help() << '\n';

	return out << setiosflags(f);
}

// =============================================================================
// helper functions

static void set_button(Fl_Button* button, bool value)
{
	button->value(value);
	button->do_callback();
}
static void set_valuator(Fl_Valuator* valuator, double value)
{
	valuator->value(value);
	valuator->do_callback();
}
static void set_text(Fl_Input* textw, string& value)
{
	textw->value(value.c_str());
	textw->do_callback();
}


// =============================================================================

// XML-RPC interface definition

// =============================================================================

class Fldigi_list : public xmlrpc_c::method
{
public:
	Fldigi_list()
	{
		_signature = "A:n";
		_help = "Returns the list of methods.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		vector<xmlrpc_c::value> help;
		for (methods_t::const_iterator i = methods->begin(); i != methods->end(); ++i) {
			map<string, xmlrpc_c::value> item;
			item["name"] = xmlrpc_c::value_string(i->name);
			item["signature"] = xmlrpc_c::value_string(i->method->signature());
			item["help"] = xmlrpc_c::value_string(i->method->help());
			help.push_back(xmlrpc_c::value_struct(item));
		}

		*retval = xmlrpc_c::value_array(help);
	}
};

class Fldigi_name : public xmlrpc_c::method
{
public:
	Fldigi_name()
	{
		_signature = "s:n";
		_help = "Returns the program name.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(PACKAGE_TARNAME);
	}
};

class Fldigi_version_struct : public xmlrpc_c::method
{
public:
	Fldigi_version_struct()
	{
		_signature = "S:n";
		_help = "Returns the program version as a struct.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		map<string, xmlrpc_c::value> vstruct;
		vstruct["major"] = xmlrpc_c::value_int(FLDIGI_VERSION_MAJOR);
		vstruct["minor"] = xmlrpc_c::value_int(FLDIGI_VERSION_MINOR);
		vstruct["patch"] = xmlrpc_c::value_string(FLDIGI_VERSION_PATCH);
		*retval = xmlrpc_c::value_struct(vstruct);
	}
};

class Fldigi_version_string : public xmlrpc_c::method
{
public:
	Fldigi_version_string()
	{
		_signature = "s:n";
		_help = "Returns the program version as a string.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(PACKAGE_VERSION);
	}
};

class Fldigi_name_version : public xmlrpc_c::method
{
public:
	Fldigi_name_version()
	{
		_signature = "s:n";
		_help = "Returns the program name and version.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(PACKAGE_STRING);
	}
};

// =============================================================================

class Modem_get_name : public xmlrpc_c::method
{
public:
	Modem_get_name()
	{
		_signature = "s:n";
		_help = "Returns the name of the current modem.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(mode_info[active_modem->get_mode()].sname);
	}
};

class Modem_get_names : public xmlrpc_c::method
{
public:
	Modem_get_names()
	{
		_signature = "A:n";
		_help = "Returns all modem names.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		vector<xmlrpc_c::value> names;
		names.reserve(NUM_MODES);
		for (size_t i = 0; i < NUM_MODES; i++)
			names.push_back(xmlrpc_c::value_string(mode_info[i].sname));
		*retval = xmlrpc_c::value_array(names);
	}
};

class Modem_get_id : public xmlrpc_c::method
{
public:
	Modem_get_id()
	{
		_signature = "i:n";
		_help = "Returns the ID of the current modem.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_int(active_modem->get_mode());
	}
};

class Modem_get_max_id : public xmlrpc_c::method
{
public:
	Modem_get_max_id()
	{
		_signature = "i:n";
		_help = "Returns the maximum modem ID number.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_int(NUM_MODES - 1);
	}
};

class Modem_set_by_name : public xmlrpc_c::method
{
public:
	Modem_set_by_name()
	{
		_signature = "s:s";
		_help = "Sets the current modem. Returns old name.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		const char* cur = mode_info[active_modem->get_mode()].sname;

		string s = params.getString(0);
		for (size_t i = 0; i < NUM_MODES; i++) {
			if (s == mode_info[i].sname) {
				REQ_SYNC(init_modem_sync, i);
				*retval = xmlrpc_c::value_string(cur);
				return;
			}
		}
		throw xmlrpc_c::fault("No such modem");
	}
};

class Modem_set_by_id : public xmlrpc_c::method
{
public:
	Modem_set_by_id()
	{
		_signature = "i:i";
		_help = "Sets the current modem. Returns old ID.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		int cur = active_modem->get_mode();

		int i = params.getInt(0, 0, NUM_MODES-1);
		REQ_SYNC(init_modem_sync, i);

		*retval = xmlrpc_c::value_int(cur);
	}
};

// =============================================================================

class Modem_set_carrier : public xmlrpc_c::method
{
public:
	Modem_set_carrier()
	{
		_signature = "i:i";
		_help = "Sets modem carrier. Returns old carrier.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		int cur = active_modem->get_freq();
		active_modem->set_freq(params.getInt(0, 1));
		*retval = xmlrpc_c::value_int(cur);
	}
};

class Modem_inc_carrier : public xmlrpc_c::method
{
public:
	Modem_inc_carrier()
	{
		_signature = "i:i";
		_help = "Increments the modem carrier frequency. Returns the new carrier.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		int cur = active_modem->get_freq();
		active_modem->set_freq(cur + params.getInt(0));
		*retval = xmlrpc_c::value_int(active_modem->get_freq());
	}
};

class Modem_get_carrier : public xmlrpc_c::method
{
public:
	Modem_get_carrier()
	{
		_signature = "i:n";
		_help = "Returns the modem carrier frequency.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_int(active_modem->get_freq());
	}
};

// =============================================================================

class Fl_Counter;
extern Fl_Counter* cntSearchRange; // FIXME: export in header

class Modem_get_afc_sr : public xmlrpc_c::method
{
public:
	Modem_get_afc_sr()
	{
		_signature = "i:n";
		_help = "Returns the modem AFC search range.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		if (!(active_modem->get_cap() & modem::CAP_AFC_SR))
			throw xmlrpc_c::fault("Operation not supported by modem");

		*retval = xmlrpc_c::value_int((int)cntSearchRange->value());
	}
};

class Modem_set_afc_sr : public xmlrpc_c::method
{
public:
	Modem_set_afc_sr()
	{
		_signature = "n:i";
		_help = "Sets the modem AFC search range. Returns the old value.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		if (!(active_modem->get_cap() & modem::CAP_AFC_SR))
			throw xmlrpc_c::fault("Operation not supported by modem");

		int v = (int)(cntSearchRange->value());
		REQ(set_valuator, cntSearchRange, params.getInt(0, (int)cntSearchRange->minimum(), (int)cntSearchRange->maximum()));
		*retval = xmlrpc_c::value_int(v);
	}
};

class Modem_inc_afc_sr : public xmlrpc_c::method
{
public:
	Modem_inc_afc_sr()
	{
		_signature = "n:i";
		_help = "Increments the modem AFC search range. Returns the new value.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		if (!(active_modem->get_cap() & modem::CAP_AFC_SR))
			throw xmlrpc_c::fault("Operation not supported by modem");

		int v = (int)(cntSearchRange->value() + params.getInt(0));
		REQ(set_valuator, cntSearchRange, v);
		*retval = xmlrpc_c::value_int(v);
	}
};

// =============================================================================

extern Fl_Value_Slider* sldrHellBW; // FIXME: export in header
extern Fl_Value_Slider* sldrCWbandwidth; // FIXME: export in header
static Fl_Valuator* get_bw_val(void)
{
	if (!(active_modem->get_cap() & modem::CAP_BW))
		throw xmlrpc_c::fault("Operation not supported by modem");

	switch (active_modem->get_mode()) {
	case MODE_FELDHELL: case MODE_SLOWHELL: case MODE_HELLX5:
	case MODE_HELLX9: case MODE_FSKHELL: case MODE_FSKH105: case MODE_HELL80:
		return sldrHellBW;
	case MODE_CW:
		return sldrCWbandwidth;
	default:
		throw xmlrpc_c::fault("Unknown CAP_BW modem");
	}
}

class Modem_get_bw : public xmlrpc_c::method
{
public:
	Modem_get_bw()
	{
		_signature = "i:n";
		_help = "Returns the modem bandwidth.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_int((int)get_bw_val()->value());
	}
};

class Modem_set_bw : public xmlrpc_c::method
{
public:
	Modem_set_bw()
	{
		_signature = "n:i";
		_help = "Sets the modem bandwidth. Returns the old value.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		Fl_Valuator* val = get_bw_val();
		int v = (int)(val->value());
		REQ(set_valuator, val, params.getInt(0, (int)val->minimum(), (int)val->maximum()));
		*retval = xmlrpc_c::value_int(v);
	}
};

class Modem_inc_bw : public xmlrpc_c::method
{
public:
	Modem_inc_bw()
	{
		_signature = "n:i";
		_help = "Increments the modem bandwidth. Returns the new value.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		Fl_Valuator* val = get_bw_val();
		int v = (int)(val->value() + params.getInt(0));
		REQ(set_valuator, val, v);
		*retval = xmlrpc_c::value_int(v);
	}
};

// =============================================================================

class Modem_get_quality : public xmlrpc_c::method
{
public:
	Modem_get_quality()
	{
		_signature = "d:n";
		_help = "Returns the modem signal quality in the range [0:100].";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_double(pgrsSquelch->value());
	}
};

class Modem_search_up : public xmlrpc_c::method
{
public:
	Modem_search_up()
	{
		_signature = "n:n";
		_help = "Searches upward in frequency.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		REQ(&modem::searchUp, active_modem);
		*retval = xmlrpc_c::value_nil();
	}
};

class Modem_search_down : public xmlrpc_c::method
{
public:
	Modem_search_down()
	{
		_signature = "n:n";
		_help = "Searches downward in frequency.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		REQ(&modem::searchDown, active_modem);
		*retval = xmlrpc_c::value_nil();
	}
};

// =============================================================================

class Main_get_status1 : public xmlrpc_c::method
{
public:
	Main_get_status1()
	{
		_signature = "s:n";
		_help = "Returns the contents of the first status field (typically s/n).";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(Status1->label());
	}
};

class Main_get_status2 : public xmlrpc_c::method
{
public:
	Main_get_status2()
	{
		_signature = "s:n";
		_help = "Returns the contents of the second status field.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(Status2->label());
	}
};

class Main_get_sb : public xmlrpc_c::method
{
public:
	Main_get_sb()
	{
		_signature = "s:n";
		_help = "Returns the current sideband.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(wf->USB() ? "USB" : "LSB");
	}
};

class Main_set_sb : public xmlrpc_c::method
{
public:
	Main_set_sb()
	{
		_signature = "n:s";
		_help = "Sets the sideband to USB or LSB.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		string s = params.getString(0);
		if (s != "LSB" && s != "USB")
			throw xmlrpc_c::fault("Invalid argument");

		if (progdefaults.chkUSERIGCATis)
			rigCAT_setmode(s);
#if USE_HAMLIB
		else if (progdefaults.chkUSEHAMLIBis)
			hamlib_setmode(s == "LSB" ? RIG_MODE_LSB : RIG_MODE_USB);
#endif
		else if (progdefaults.chkUSEXMLRPCis)
			REQ(static_cast<void (waterfall::*)(bool)>(&waterfall::USB), wf, s == "USB");

		*retval = xmlrpc_c::value_nil();
	}
};


class Main_get_freq : public xmlrpc_c::method
{
public:
	Main_get_freq()
	{
		_signature = "d:n";
		_help = "Returns the RF carrier frequency.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_double(wf->rfcarrier());
	}
};

void xmlrpc_set_qsy(long long rfc, long long fmid)
{
	if (active_modem->freqlocked()) {
		active_modem->set_freqlock(false);
		active_modem->set_freq((int)fmid);
		active_modem->set_freqlock(true);
	}
	else
		active_modem->set_freq((int)fmid);
	wf->rfcarrier(rfc);
	wf->movetocenter();
}

class Main_set_freq : public xmlrpc_c::method
{
public:
	Main_set_freq()
	{
		_signature = "d:d";
		_help = "Sets the RF carrier frequency. Returns the old value.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		double rfc = wf->rfcarrier();
		qsy((long long int)params.getDouble(0, 0.0), active_modem->get_freq());
		*retval = xmlrpc_c::value_double(rfc);
	}
};

class Main_inc_freq : public xmlrpc_c::method
{
public:
	Main_inc_freq()
	{
		_signature = "d:d";
		_help = "Increments the RF carrier frequency. Returns the new value.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		double rfc = wf->rfcarrier() + params.getDouble(0);
		qsy((long long int)rfc, active_modem->get_freq());
		*retval = xmlrpc_c::value_double(rfc);
	}
};

// =============================================================================

class Main_get_afc : public xmlrpc_c::method
{
public:
	Main_get_afc()
	{
		_signature = "b:n";
		_help = "Returns the AFC state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_boolean(btn_afconoff->value());
	}
};

class Main_set_afc : public xmlrpc_c::method
{
public:
	Main_set_afc()
	{
		_signature = "b:b";
		_help = "Sets the AFC state. Returns the old state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		bool v = btn_afconoff->value();
		REQ(set_button, btn_afconoff, params.getBoolean(0));
		*retval = xmlrpc_c::value_boolean(v);
	}
};

class Main_toggle_afc : public xmlrpc_c::method
{
public:
	Main_toggle_afc()
	{
		_signature = "b:n";
		_help = "Toggles the AFC state. Returns the new state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		bool v = btn_afconoff->value();
		REQ(set_button, btn_afconoff, !v);
		*retval = xmlrpc_c::value_boolean(!v);
	}
};

// =============================================================================

class Main_get_sql : public xmlrpc_c::method
{
public:
	Main_get_sql()
	{
		_signature = "b:n";
		_help = "Returns the squelch state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_boolean(btn_sqlonoff->value());
	}
};

class Main_set_sql : public xmlrpc_c::method
{
public:
	Main_set_sql()
	{
		_signature = "b:b";
		_help = "Sets the squelch state. Returns the old state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		bool v = btn_sqlonoff->value();
		REQ(set_button, btn_sqlonoff, params.getBoolean(0));
		*retval = xmlrpc_c::value_boolean(v);
	}
};

class Main_toggle_sql : public xmlrpc_c::method
{
public:
	Main_toggle_sql()
	{
		_signature = "b:n";
		_help = "Toggles the squelch state. Returns the new state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		bool v = btn_sqlonoff->value();
		REQ(set_button, btn_sqlonoff, !v);
		*retval = xmlrpc_c::value_boolean(!v);
	}
};

// =============================================================================

class Main_get_sql_level : public xmlrpc_c::method
{
public:
	Main_get_sql_level()
	{
		_signature = "d:n";
		_help = "Returns the squelch level.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_double(sldrSquelch->value());
	}
};

class Main_set_sql_level : public xmlrpc_c::method
{
public:
	Main_set_sql_level()
	{
		_signature = "d:d";
		_help = "Sets the squelch level. Returns the old level.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		double v = sldrSquelch->value();
		// Squelch slider min/max are reversed when !twoscopes. Argh.
		if (twoscopes)
			REQ(set_valuator, sldrSquelch, params.getDouble(0, sldrSquelch->minimum(), sldrSquelch->maximum()));
		else
			REQ(set_valuator, sldrSquelch, params.getDouble(0, sldrSquelch->maximum(), sldrSquelch->minimum()));
		*retval = xmlrpc_c::value_double(v);
	}
};

class Main_inc_sql_level : public xmlrpc_c::method
{
public:
	Main_inc_sql_level()
	{
		_signature = "d:d";
		_help = "Increments the squelch level. Returns the new level.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		double v = sldrSquelch->value();
		REQ(set_valuator, sldrSquelch, v + params.getDouble(0)); // FIXME: check range
		*retval = xmlrpc_c::value_double(sldrSquelch->value());
	}
};

// =============================================================================

class Main_get_rev : public xmlrpc_c::method
{
public:
	Main_get_rev()
	{
		_signature = "b:n";
		_help = "Returns the Reverse Sideband state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_boolean(wf->btnRev->value());
	}
};

class Main_set_rev : public xmlrpc_c::method
{
public:
	Main_set_rev()
	{
		_signature = "b:b";
		_help = "Sets the Reverse Sideband state. Returns the old state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		bool v = wf->btnRev->value();
		REQ(set_button, wf->btnRev, params.getBoolean(0));
		*retval = xmlrpc_c::value_boolean(v);
	}
};

class Main_toggle_rev : public xmlrpc_c::method
{
public:
	Main_toggle_rev()
	{
		_signature = "b:n";
		_help = "Toggles the Reverse Sideband state. Returns the new state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		bool v = wf->btnRev->value();
		REQ(set_button, wf->btnRev, !v);
		*retval = xmlrpc_c::value_boolean(!v);
	}
};

// =============================================================================

class Main_get_lock : public xmlrpc_c::method
{
public:
	Main_get_lock()
	{
		_signature = "b:n";
		_help = "Returns the Transmit Lock state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_boolean(wf->xmtlock->value());
	}
};

class Main_set_lock : public xmlrpc_c::method
{
public:
	Main_set_lock()
	{
		_signature = "b:b";
		_help = "Sets the Transmit Lock state. Returns the old state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		bool v = wf->xmtlock->value();
		REQ(set_button, wf->xmtlock, params.getBoolean(0));
		*retval = xmlrpc_c::value_boolean(v);
	}
};

class Main_toggle_lock : public xmlrpc_c::method
{
public:
	Main_toggle_lock()
	{
		_signature = "b:n";
		_help = "Toggles the Reverse Sideband state. Returns the new state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		bool v = wf->xmtlock->value();
		REQ(set_button, wf->xmtlock, !v);
		*retval = xmlrpc_c::value_boolean(!v);
	}
};

// =============================================================================

extern Fl_Button* btnTune; // FIXME: export in fl_digi.h
extern Fl_Button* btnRSID;

class Main_get_trx_status : public xmlrpc_c::method
{
public:
	Main_get_trx_status()
	{
		_signature = "s:n";
		_help = "Returns transmit/tune/receive status.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		if (btnTune->value())
			*retval = xmlrpc_c::value_string("tune");
		else if (wf->xmtrcv->value())
			*retval = xmlrpc_c::value_string("tx");
		else if (btnRSID->value())
			*retval = xmlrpc_c::value_string("rsid");
		else
			*retval = xmlrpc_c::value_string("rx");
	}
};

class Main_tx : public xmlrpc_c::method
{
public:
	Main_tx()
	{
		_signature = "n:n";
		_help = "Transmits.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		if (!wf->xmtrcv->value()) {
			if (btnRSID->value())
				REQ(set_button, btnRSID, false);
			REQ(set_button, wf->xmtrcv, true);
		}
		*retval = xmlrpc_c::value_nil();
	}
};

class Main_tune : public xmlrpc_c::method
{
public:
	Main_tune()
	{
		_signature = "n:n";
		_help = "Tunes.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		if (!btnTune->value()) {
			if (btnRSID->value())
				REQ(set_button, btnRSID, false);
			REQ(set_button, btnTune, !btnTune->value());
		}
		*retval = xmlrpc_c::value_nil();
	}
};

class Main_rx : public xmlrpc_c::method
{
public:
	Main_rx()
	{
		_signature = "n:n";
		_help = "Receives.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		if (wf->xmtrcv->value())
			REQ(set_button, wf->xmtrcv, false);
		else if (btnRSID->value())
			REQ(set_button, btnRSID, false);
		*retval = xmlrpc_c::value_nil();
	}
};

class Main_abort : public xmlrpc_c::method
{
public:
	Main_abort()
	{
		_signature = "n:n";
		_help = "Aborts a transmit or tune.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		if (trx_state == STATE_TX || trx_state == STATE_TUNE)
			REQ(abort_tx);
		*retval = xmlrpc_c::value_nil();
	}
};

class Main_run_macro : public xmlrpc_c::method
{
public:
	Main_run_macro()
	{
		_signature = "n:i";
		_help = "Runs a macro.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		REQ(&Main_run_macro::run_macro, params.getInt(0, 0, MAXMACROS-1));
		*retval = xmlrpc_c::value_nil();
	}
	static void run_macro(int i) { macros.execute(i); }
};

class Main_get_max_macro_id : public xmlrpc_c::method
{
public:
	Main_get_max_macro_id()
	{
		_signature = "i:n";
		_help = "Returns the maximum macro ID number.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_int(MAXMACROS - 1);
	}
};

class Main_rsid : public xmlrpc_c::method
{
public:
	Main_rsid()
	{
		_signature = "n:n";
		_help = "Waits for RSID.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		if (!(wf->xmtrcv->value() || btnTune->value() || btnRSID->value()))
			REQ(set_button, btnRSID, true);
		*retval = xmlrpc_c::value_nil();
	}
};

// =============================================================================

class Log_get_freq : public xmlrpc_c::method
{
public:
	Log_get_freq()
	{
		_signature = "s:n";
		_help = "Returns the Frequency field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpFreq->value());
	}
};

class Log_get_time : public xmlrpc_c::method
{
public:
	Log_get_time()
	{
		_signature = "s:n";
		_help = "Returns the Time field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpTime->value());
	}
};

class Log_get_call : public xmlrpc_c::method
{
public:
	Log_get_call()
	{
		_signature = "s:n";
		_help = "Returns the Call field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpCall->value());
	}
};

class Log_set_call : public xmlrpc_c::method
{
public:
	Log_set_call()
	{
		_signature = "n:s";
		_help = "Sets the Call field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		REQ(set_text, inpCall, params.getString(0));

		*retval = xmlrpc_c::value_nil();
	}
};

class Log_get_name : public xmlrpc_c::method
{
public:
	Log_get_name()
	{
		_signature = "s:n";
		_help = "Returns the Name field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpName->value());
	}
};

class Log_get_rst_in : public xmlrpc_c::method
{
public:
	Log_get_rst_in()
	{
		_signature = "s:n";
		_help = "Returns the RST(r) field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpRstIn->value());
	}
};

class Log_get_rst_out : public xmlrpc_c::method
{
public:
	Log_get_rst_out()
	{
		_signature = "s:n";
		_help = "Returns the RST(s) field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpRstOut->value());
	}
};

class Log_get_qth : public xmlrpc_c::method
{
public:
	Log_get_qth()
	{
		_signature = "s:n";
		_help = "Returns the QTH field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpQth->value());
	}
};

class Log_get_band : public xmlrpc_c::method
{
public:
	Log_get_band()
	{
		_signature = "s:n";
		_help = "Returns the Band selection.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(cboBand->value());
	}
};

class Log_get_sb : public xmlrpc_c::method
{
public:
	Log_get_sb()
	{
		_signature = "s:n";
		_help = "Returns the sideband button label.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(btnSideband->label());
	}
};

class Log_get_notes : public xmlrpc_c::method
{
public:
	Log_get_notes()
	{
		_signature = "s:n";
		_help = "Returns the Notes field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpNotes->value());
	}
};

class Log_get_locator : public xmlrpc_c::method
{
public:
	Log_get_locator()
	{
		_signature = "s:n";
		_help = "Returns the Locator field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpLoc->value());
	}
};

class Log_get_az : public xmlrpc_c::method
{
public:
	Log_get_az()
	{
		_signature = "s:n";
		_help = "Returns the AZ field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpAZ->value());
	}
};

class Log_clear : public xmlrpc_c::method
{
public:
	Log_clear()
	{
		_signature = "n:n";
		_help = "Clears the contents of the log fields.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		REQ(clearQSO);
		*retval = xmlrpc_c::value_nil();
	}
};

// =============================================================================

class Text_get_rx_length : public xmlrpc_c::method
{
public:
	Text_get_rx_length()
	{
		_signature = "i:n";
		_help = "Returns the number of characters in the RX widget.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_int(ReceiveText->buffer()->length());
	}
};

class Text_get_rx : public xmlrpc_c::method
{
public:
	Text_get_rx()
	{
		_signature = "6:ii";
		_help = "Returns a range of characters (start, length) from the RX text widget.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		params.verifyEnd(2);

		Fl_Text_Buffer* tbuf = ReceiveText->buffer();
		int len = tbuf->length();
		int start = params.getInt(0, 0, len - 1);
		int n = params.getInt(1, 0, len - start);

		REQ_SYNC(&Fl_Text_Buffer::select, tbuf, start, start + n);
		char *text = tbuf->selection_text();
		REQ(&Fl_Text_Buffer::unselect, tbuf);

		vector<unsigned char> bytes(text, text + n);
		*retval = xmlrpc_c::value_bytestring(bytes);
		free(text);
	}
};

class Text_clear_rx : public xmlrpc_c::method
{
public:
	Text_clear_rx()
	{
		_signature = "n:n";
		_help = "Clears the RX text widget.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		REQ(&FTextBase::clear, ReceiveText);
		*retval = xmlrpc_c::value_nil();
	}
};

class Text_add_tx : public xmlrpc_c::method
{
public:
	Text_add_tx()
	{
		_signature = "n:s";
		_help = "Adds a string to the TX text widget.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		REQ_SYNC(&FTextBase::addstr, TransmitText, params.getString(0).c_str(), FTextBase::RECV);
		*retval = xmlrpc_c::value_nil();
	}
};

class Text_add_tx_bytes : public xmlrpc_c::method
{
public:
	Text_add_tx_bytes()
	{
		_signature = "n:6";
		_help = "Adds a byte string to the TX text widget.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		vector<unsigned char> bytes = params.getBytestring(0);
		bytes.push_back(0);
		REQ_SYNC(&FTextBase::addstr, TransmitText, (const char*)&bytes[0], FTextBase::RECV);

		*retval = xmlrpc_c::value_nil();
	}
};

class Text_clear_tx : public xmlrpc_c::method
{
public:
	Text_clear_tx()
	{
		_signature = "n:n";
		_help = "Clears the TX text widget.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		REQ(&FTextBase::clear, TransmitText);
		*retval = xmlrpc_c::value_nil();
	}
};

// =============================================================================

// End XML-RPC interface

struct rm_pred
{
	re_t filter;
	bool allow;
	rm_pred(const char* re, bool allow_)
		: filter(re, REG_EXTENDED | REG_NOSUB), allow(allow_) { }
	bool operator()(const methods_t::value_type& v)
	{
		return filter.match(v.name) ^ allow && !strstr(v.name, "fldigi.");
	}
};

void XML_RPC_Server::add_methods(void)
{
	if (methods)
		return;
	methods = new methods_t;

	methods->push_back(rpc_method(new Fldigi_list, "fldigi.list"));
	methods->push_back(rpc_method(new Fldigi_name, "fldigi.name"));
	methods->push_back(rpc_method(new Fldigi_version_struct, "fldigi.version_struct"));
	methods->push_back(rpc_method(new Fldigi_version_string, "fldigi.version"));
	methods->push_back(rpc_method(new Fldigi_name_version, "fldigi.name_version"));

	methods->push_back(rpc_method(new Modem_get_name, "modem.get_name"));
	methods->push_back(rpc_method(new Modem_get_names, "modem.get_names"));
	methods->push_back(rpc_method(new Modem_get_id, "modem.get_id"));
	methods->push_back(rpc_method(new Modem_get_max_id, "modem.get_max_id"));
	methods->push_back(rpc_method(new Modem_set_by_name, "modem.set_by_name"));
	methods->push_back(rpc_method(new Modem_set_by_id, "modem.set_by_id"));

	methods->push_back(rpc_method(new Modem_set_carrier, "modem.set_carrier"));
	methods->push_back(rpc_method(new Modem_inc_carrier, "modem.inc_carrier"));
	methods->push_back(rpc_method(new Modem_get_carrier, "modem.get_carrier"));

	methods->push_back(rpc_method(new Modem_get_afc_sr, "modem.get_afc_search_range"));
	methods->push_back(rpc_method(new Modem_set_afc_sr, "modem.set_afc_search_range"));
	methods->push_back(rpc_method(new Modem_inc_afc_sr, "modem.inc_afc_search_range"));

	methods->push_back(rpc_method(new Modem_get_bw, "modem.get_bandwidth"));
	methods->push_back(rpc_method(new Modem_set_bw, "modem.set_bandwidth"));
	methods->push_back(rpc_method(new Modem_inc_bw, "modem.inc_bandwidth"));

	methods->push_back(rpc_method(new Modem_get_quality, "modem.get_quality"));
	methods->push_back(rpc_method(new Modem_search_up, "modem.search_up"));
	methods->push_back(rpc_method(new Modem_search_down, "modem.search_down"));

	methods->push_back(rpc_method(new Main_get_status1, "main.get_status1"));
	methods->push_back(rpc_method(new Main_get_status2, "main.get_status2"));

	methods->push_back(rpc_method(new Main_get_sb, "main.get_sideband"));
	methods->push_back(rpc_method(new Main_set_sb, "main.set_sideband"));
	methods->push_back(rpc_method(new Main_get_freq, "main.get_frequency"));
	methods->push_back(rpc_method(new Main_set_freq, "main.set_frequency"));
	methods->push_back(rpc_method(new Main_inc_freq, "main.inc_frequency"));

	methods->push_back(rpc_method(new Main_get_afc, "main.get_afc"));
	methods->push_back(rpc_method(new Main_set_afc, "main.set_afc"));
	methods->push_back(rpc_method(new Main_toggle_afc, "main.toggle_afc"));

	methods->push_back(rpc_method(new Main_get_sql, "main.get_squelch"));
	methods->push_back(rpc_method(new Main_set_sql, "main.set_squelch"));
	methods->push_back(rpc_method(new Main_toggle_sql, "main.toggle_squelch"));

	methods->push_back(rpc_method(new Main_get_sql_level, "main.get_squelch_level"));
	methods->push_back(rpc_method(new Main_set_sql_level, "main.set_squelch_level"));
	methods->push_back(rpc_method(new Main_inc_sql_level, "main.inc_squelch_level"));

	methods->push_back(rpc_method(new Main_get_rev, "main.get_reverse"));
	methods->push_back(rpc_method(new Main_set_rev, "main.set_reverse"));
	methods->push_back(rpc_method(new Main_toggle_rev, "main.toggle_reverse"));

	methods->push_back(rpc_method(new Main_get_lock, "main.get_lock"));
	methods->push_back(rpc_method(new Main_set_lock, "main.set_lock"));
	methods->push_back(rpc_method(new Main_toggle_lock, "main.toggle_lock"));

	methods->push_back(rpc_method(new Main_get_trx_status, "main.get_trx_status"));
	methods->push_back(rpc_method(new Main_tx, "main.tx"));
	methods->push_back(rpc_method(new Main_tune, "main.tune"));
	methods->push_back(rpc_method(new Main_rsid, "main.rsid"));
	methods->push_back(rpc_method(new Main_rx, "main.rx"));
	methods->push_back(rpc_method(new Main_abort, "main.abort"));

	methods->push_back(rpc_method(new Main_run_macro, "main.run_macro"));
	methods->push_back(rpc_method(new Main_get_max_macro_id, "main.get_max_macro_id"));

	methods->push_back(rpc_method(new Log_get_freq, "log.get_frequency"));
	methods->push_back(rpc_method(new Log_get_time, "log.get_time"));
	methods->push_back(rpc_method(new Log_get_call, "log.get_call"));
	methods->push_back(rpc_method(new Log_get_name, "log.get_name"));
	methods->push_back(rpc_method(new Log_get_rst_in, "log.get_rst_in"));
	methods->push_back(rpc_method(new Log_get_rst_out, "log.get_rst_out"));
	methods->push_back(rpc_method(new Log_get_qth, "log.get_qth"));
	methods->push_back(rpc_method(new Log_get_band, "log.get_band"));
	methods->push_back(rpc_method(new Log_get_sb, "log.get_sideband"));
	methods->push_back(rpc_method(new Log_get_notes, "log.get_notes"));
	methods->push_back(rpc_method(new Log_get_locator, "log.get_locator"));
	methods->push_back(rpc_method(new Log_get_az, "log.get_az"));
	methods->push_back(rpc_method(new Log_clear, "log.clear"));
	methods->push_back(rpc_method(new Log_set_call, "log.set_call"));

	methods->push_back(rpc_method(new Text_get_rx_length, "text.get_rx_length"));
	methods->push_back(rpc_method(new Text_get_rx, "text.get_rx"));
	methods->push_back(rpc_method(new Text_clear_rx, "text.clear_rx"));
	methods->push_back(rpc_method(new Text_add_tx, "text.add_tx"));
	methods->push_back(rpc_method(new Text_add_tx_bytes, "text.add_tx_bytes"));
	methods->push_back(rpc_method(new Text_clear_tx, "text.clear_tx"));

	if (!progdefaults.xmlrpc_deny.empty())
		methods->remove_if(rm_pred(progdefaults.xmlrpc_deny.c_str(), false));
	else if (!progdefaults.xmlrpc_allow.empty())
		methods->remove_if(rm_pred(progdefaults.xmlrpc_allow.c_str(), true));
}
