// ----------------------------------------------------------------------------
//      xmlrpc.cxx
//
// Copyright (C) 2008-2010
//              Stelios Bounanos, M0GLD
// Copyright (C) 2008-2010
//              Dave Freese, W1HKJ
// Copyright (C) 2013
//              Remi Chateauneu, F4ECW
//
// See EOF for a list of method names. Run "fldigi --xmlrpc-list"
// to see a list of method names, signatures and descriptions.
//
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

#include <xmlrpcpp/XmlRpcServer.h>
#include <xmlrpcpp/XmlRpcServerMethod.h>
#include <xmlrpcpp/XmlRpcValue.h>

#include "globals.h"
#include "configuration.h"
#ifdef HAVE_VALUES_H
#	include <values.h>
#endif
#include "threads.h"
#include "modem.h"
#include "trx.h"
#include "fl_digi.h"
#include "configuration.h"
#include "main.h"
#include "waterfall.h"
#include "macros.h"
#include "qrunner.h"
#include "wefax.h"
#include "wefax-pic.h"
#include "navtex.h"

#if USE_HAMLIB
        #include "hamlib.h"
#endif
#include "rigio.h"
#include "debug.h"
#include "re.h"
#include "pskrep.h"

// required for flrig support
#include "fl_digi.h"
#include "rigsupport.h"

#include "confdialog.h"
#include "arq_io.h"

LOG_FILE_SOURCE(debug::LOG_RPC);

using namespace std;
using namespace XmlRpc;

/// Not defined the usual way on Mingw
#ifndef DBL_MAX
#define DBL_MAX 1.7976931348623157e+308
#endif

namespace xmlrpc_c
{
	struct method
	{
		const char * _signature ;
		const char * _help ;
		virtual std::string help(void) const { return _help;}
		const char * signature() const { return _signature; }
		virtual ~method() {}
	};

	typedef method * methodPtr ;
	typedef XmlRpcValue value ;
	typedef XmlRpcValue value_string ;
	typedef XmlRpcValue value_bytestring ;
	typedef XmlRpcValue value_struct ;
	typedef XmlRpcValue value_nil ;
	typedef XmlRpcValue value_array ;
	typedef XmlRpcValue value_double ;
	typedef XmlRpcValue value_int ;
	typedef XmlRpcValue value_boolean ;

	struct fault : public std::runtime_error
	{
		typedef enum { CODE_INTERNAL } Codes;

		fault( const char * msg, Codes cd = CODE_INTERNAL ) : std::runtime_error(msg) {}
	};

	struct paramList
	{
		const XmlRpcValue & _params ;
		paramList( const XmlRpcValue & prm ) : _params(prm) {}

		int getInt(int i, int mini = INT_MIN, int maxi = INT_MAX ) const
		{
			int tmp = _params[i];
			if( tmp < mini ) tmp = mini ;
			else if(tmp > maxi) tmp = maxi ;
			return tmp ;
		}
		string getString(int i) const { return _params[i]; }
		std::vector<unsigned char> getBytestring(int i) const
		{
			return _params[i];
		}
		double getDouble(int i, double mini = -DBL_MAX, double maxi = DBL_MAX) const
		{
			double tmp = _params[i];
			if( tmp < mini ) tmp = mini ;
			else if(tmp > maxi) tmp = maxi ;
			return tmp ;
		}
		bool getBoolean(int i) const { return _params[i]; }
		const std::vector<value> & getArray(int i) const { return _params[i]; }
		void verifyEnd(size_t sz) const
		{
			const std::vector<value> & tmpRef = _params ;
			if( sz != tmpRef.size() ) throw std::runtime_error("Bad size");
		}
	};

}

template< class RPC_METHOD >
struct Method : public RPC_METHOD, public XmlRpcServerMethod
{
	Method( const char * n )
	: XmlRpcServerMethod( n ) {}

	void execute (XmlRpcValue &params, XmlRpcValue &result)
	{
		xmlrpc_c::paramList params2(params) ;
		RPC_METHOD::execute( params2, &result );
	}
};

typedef XmlRpcServerMethod * (*RpcFactory)( const char * );

template<class RPC_METHOD>
struct RpcBuilder
{
	static XmlRpcServerMethod * factory( const char * name )
	{
		return new Method< RPC_METHOD >( name );
	}
};

struct XmlRpcImpl : public XmlRpcServer
{
	void open(const char * port)
	{
  		bindAndListen( atoi( port ) );

		enableIntrospection(true);
	}
	void run()
	{
		double milli_secs = -1.0 ;
		// Tell our server to wait indefinately for messages
		work(milli_secs);
	}
	/// BEWARE IT IS CALLED FROM ANOTHER THREAD.
	void close()
	{
		LOG_INFO("Stopping XML-RPC server");
		exit();
		shutdown();
	}
};

struct rpc_method
{
	RpcFactory m_fact ;
	~rpc_method() { delete method ; }
	xmlrpc_c::method * method ;
	const char* name;
};
typedef list<rpc_method> methods_t;
static methods_t* methods = 0;

static pthread_t* server_thread;
static pthread_mutex_t* server_mutex;

XML_RPC_Server* XML_RPC_Server::inst = 0;

XML_RPC_Server::XML_RPC_Server()
{
	server_impl = new XmlRpcImpl;
	add_methods();

	for( methods_t::iterator it = methods->begin(), en = methods->end(); it != en; ++it )
	{
		XmlRpcServerMethod * mth = dynamic_cast< XmlRpcServerMethod * >( it->method );
		server_impl->addMethod( mth );
	}

	server_thread = new pthread_t;
	server_mutex = new pthread_mutex_t;
	pthread_mutex_init(server_mutex, NULL);
//	run = true;
}

XML_RPC_Server::~XML_RPC_Server()
{
//	run = false;
// the xmlrpc server is closed and deleted  when 
// 	XML_RPC_Server::stop();
// is called from main
//	delete methods;
}

void XML_RPC_Server::start(const char* node, const char* service)
{
	if (inst)
		return;

	inst = new XML_RPC_Server;

	try {
		inst->server_impl->open(service);
		if (pthread_create(server_thread, NULL, thread_func, NULL) != 0)
			throw runtime_error(strerror(errno));
	}
	catch (const exception& e) {
		LOG_ERROR("Could not start XML-RPC server (%s)", e.what());
		delete server_thread;
		server_thread = 0;
		delete inst;
		inst = 0;
		return;
	}
}

/// BEWARE IT IS CALLED FROM ANOTHER THREAD.
void XML_RPC_Server::stop(void)
{
// FIXME: uncomment when we have an xmlrpc server that can be interrupted
	// if (!inst)
	//	return;
	inst->server_impl->close();
	delete inst;
	inst = 0;
}

void* XML_RPC_Server::thread_func(void*)
{
	SET_THREAD_ID(XMLRPC_TID);

	save_signals();
	inst->server_impl->run();
	restore_signals();

	SET_THREAD_CANCEL();
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
// Methods that change the server state must call XMLRPC_LOCK
// guard_lock (include/threads.h) ensures that mutex are always unlocked.
#define XMLRPC_LOCK SET_THREAD_ID(XMLRPC_TID); guard_lock autolock_(server_mutex)

// =============================================================================

// generic helper functions

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

static void set_combo_contents(Fl_ComboBox* box, const vector<string>* items)
{
	box->clear();

	if (items->empty()) {
		box->add("");
		box->index(0);
		box->deactivate();
		return;
	}

	for (vector<string>::const_iterator i = items->begin(); i != items->end(); ++i) {
		box->add(i->c_str());
	}

	box->index(0);
	box->activate();
}

static void set_combo_value(Fl_ComboBox* box, const string& s)
{
	box->value(s.c_str());
	box->do_callback();
}

static void get_combo_contents(Fl_ComboBox* box, vector<xmlrpc_c::value>* items)
{
	int n = box->lsize(), p = box->index();
	items->reserve(n);
	for (int i = 0; i < n; i++) {
		box->index(i);
		items->push_back(xmlrpc_c::value_string(box->value()));
	}
	box->index(p);
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

class Fldigi_config_dir : public xmlrpc_c::method
{
public:
	Fldigi_config_dir()
	{
		_signature = "s:n";
		_help = "Returns the name of the configuration directory.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(HomeDir);
	}
};

class Fldigi_terminate : public xmlrpc_c::method
{
public:
	Fldigi_terminate()
	{
		_signature = "n:i";
		_help = "Terminates fldigi. ``i'' is bitmask specifying data to save: 0=options; 1=log; 2=macros.";
	}
	enum {
		TERM_SAVE_OPTIONS = 1 << 0,
		TERM_SAVE_LOG = 1 << 1,
		TERM_SAVE_MACROS = 1 << 2
	};
	static void terminate(int how)
	{
		if (how & TERM_SAVE_OPTIONS)
			progdefaults.saveDefaults();
		progdefaults.changed = false;

		extern bool oktoclear;
		if (how & TERM_SAVE_LOG && !oktoclear)
			qsoSave->do_callback();
		oktoclear = true;
		progdefaults.NagMe = false;

		if (how & TERM_SAVE_MACROS && macros.changed)
			macros.saveMacroFile();
		macros.changed = false;

		fl_digi_main->do_callback();
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		REQ(terminate, params.getInt(0));
		*retval = xmlrpc_c::value_nil();
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
		const char* cur = mode_info[active_modem->get_mode()].sname;
		*retval = xmlrpc_c::value_string(cur);
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
        int md = active_modem->get_mode();
		*retval = xmlrpc_c::value_int(md);
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
		XMLRPC_LOCK;
		const char* cur = mode_info[active_modem->get_mode()].sname;

		string s = params.getString(0);
		for (size_t i = 0; i < NUM_MODES; i++) {
			if (s == mode_info[i].sname) {
				REQ_SYNC(init_modem_sync, i, 0);
				*retval = xmlrpc_c::value_string(cur);
				return;
			}
		}
		*retval = "No such modem";
		return;
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
		XMLRPC_LOCK;
		int cur = active_modem->get_mode();

		int i = params.getInt(0, 0, NUM_MODES-1);
		REQ_SYNC(init_modem_sync, i, 0);

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
		XMLRPC_LOCK;
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
		XMLRPC_LOCK;
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
			*retval = "Operation not supported by modem";
		else
			*retval = xmlrpc_c::value_int((int)cntSearchRange->value());
	}
};

class Modem_set_afc_sr : public xmlrpc_c::method
{
public:
	Modem_set_afc_sr()
	{
		_signature = "i:i";
		_help = "Sets the modem AFC search range. Returns the old value.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		if (!(active_modem->get_cap() & modem::CAP_AFC_SR)) {
			*retval = "Operation not supported by modem";
			return;
		}

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
		_signature = "i:i";
		_help = "Increments the modem AFC search range. Returns the new value.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		if (!(active_modem->get_cap() & modem::CAP_AFC_SR)) {
			*retval = "Operation not supported by modem";
			return;
		}

		int v = (int)(cntSearchRange->value() + params.getInt(0));
		REQ(set_valuator, cntSearchRange, v);
		*retval = xmlrpc_c::value_int(v);
	}
};

// =============================================================================

static Fl_Valuator* get_bw_val(void)
{
	if (!(active_modem->get_cap() & modem::CAP_BW))
		return 0;

	trx_mode m = active_modem->get_mode();
	if (m >= MODE_HELL_FIRST && m <= MODE_HELL_LAST)
		return sldrHellBW;
	else if (m == MODE_CW)
		return sldrCWbandwidth;
	return 0;
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
			Fl_Valuator* val = get_bw_val();
			if (val)
				*retval = xmlrpc_c::value_int((int)get_bw_val()->value());
			else
				*retval = xmlrpc_c::value_int(0);
	}
};

class Modem_set_bw : public xmlrpc_c::method
{
public:
	Modem_set_bw()
	{
		_signature = "i:i";
		_help = "Sets the modem bandwidth. Returns the old value.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		Fl_Valuator* val = get_bw_val();
		if (val) {
			int v = (int)(val->value());
			REQ(set_valuator, val, params.getInt(0, (int)val->minimum(), (int)val->maximum()));
			*retval = xmlrpc_c::value_int(v);
		} else
			*retval = xmlrpc_c::value_int(0);
	}
};

class Modem_inc_bw : public xmlrpc_c::method
{
public:
	Modem_inc_bw()
	{
		_signature = "i:i";
		_help = "Increments the modem bandwidth. Returns the new value.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		Fl_Valuator* val = get_bw_val();
		if (val) {
			int v = (int)(val->value() + params.getInt(0));
			REQ(set_valuator, val, v);
			*retval = xmlrpc_c::value_int(v);
		} else
			*retval = xmlrpc_c::value_int(0);
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
		XMLRPC_LOCK;
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
		XMLRPC_LOCK;
		REQ(&modem::searchDown, active_modem);
		*retval = xmlrpc_c::value_nil();
	}
};

class Modem_olivia_set_bandwidth : public xmlrpc_c::method
{
public:
	Modem_olivia_set_bandwidth()
	{
		_signature = "n:i";
		_help = "Sets the Olivia bandwidth.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		int bw;
		switch (bw = params.getInt(0)) {
		case 125: case 250: case 500: case 1000: case 2000:
		{
			XMLRPC_LOCK;
			REQ_SYNC(set_olivia_bw, bw);
			*retval = xmlrpc_c::value_nil();
		}
			break;
		default:
			*retval = "Invalid Olivia bandwidth";
		}
	}
};

class Modem_olivia_get_bandwidth : public xmlrpc_c::method
{
public:
	Modem_olivia_get_bandwidth()
	{
		_signature = "i:n";
		_help = "Returns the Olivia bandwidth.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		int bw, v = mnuOlivia_Bandwidth->value();

		if (v == 0)
			bw = 125;
		else if (v == 1)
			bw = 250;
		else if (v == 2)
			bw = 500;
		else if (v == 3)
			bw = 1000;
		else
			bw = 2000;

		*retval = xmlrpc_c::value_int(bw);
	}
};

class Modem_olivia_set_tones : public xmlrpc_c::method
{
public:
	Modem_olivia_set_tones()
	{
		_signature = "n:i";
		_help = "Sets the Olivia tones.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		int tones = params.getInt(0, 2, 256);
		if (powerof2(tones)) {
			XMLRPC_LOCK;
			REQ_SYNC(set_olivia_tones, tones);
			*retval = xmlrpc_c::value_nil();
		}
		else
			*retval = "Invalid Olivia tones";
	}
};

class Modem_olivia_get_tones : public xmlrpc_c::method
{
public:
	Modem_olivia_get_tones()
	{
		_signature = "i:n";
		_help = "Returns the Olivia tones.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		*retval = xmlrpc_c::value_int(1 << (mnuOlivia_Tones->value() + 1));
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
		_help = "[DEPRECATED; use main.get_wf_sideband and/or rig.get_mode]";
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
		_help = "[DEPRECATED; use main.set_wf_sideband and/or rig.set_mode]";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		string s = params.getString(0);
		if (s != "LSB" && s != "USB") {
			*retval = "Invalid argument";
			return;
		}

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

class Main_get_wf_sideband : public xmlrpc_c::method
{
public:
	Main_get_wf_sideband()
	{
		_signature = "s:n";
		_help = "Returns the current waterfall sideband.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(wf->USB() ? "USB" : "LSB");
	}
};

class Main_set_wf_sideband : public xmlrpc_c::method
{
public:
	Main_set_wf_sideband()
	{
		_signature = "n:s";
		_help = "Sets the waterfall sideband to USB or LSB.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		string s = params.getString(0);
		if (s != "USB" && s != "LSB")
			*retval = "Invalid argument";
		else
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
        double rfc = wf->rfcarrier();
		*retval = xmlrpc_c::value_double(rfc);
	}
};

void xmlrpc_set_qsy(long long rfc)
{
	wf->rfcarrier(rfc);
	wf->movetocenter();
	show_frequency(rfc);
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
		XMLRPC_LOCK;
		double rfc = wf->rfcarrier();
		qsy((long long int)params.getDouble(0, 0.0));
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
		XMLRPC_LOCK;
		double rfc = wf->rfcarrier() + params.getDouble(0);
		qsy((long long int)rfc);
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
		*retval = xmlrpc_c::value_boolean(btnAFC->value());
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
		XMLRPC_LOCK;
		bool v = btnAFC->value();
		REQ(set_button, btnAFC, params.getBoolean(0));
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
		XMLRPC_LOCK;
		bool v = !btnAFC->value();
		REQ(set_button, btnAFC, v);
		*retval = xmlrpc_c::value_boolean(v);
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
		*retval = xmlrpc_c::value_boolean(btnSQL->value());
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
		XMLRPC_LOCK;
		bool v = btnSQL->value();
		REQ(set_button, btnSQL, params.getBoolean(0));
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
		XMLRPC_LOCK;
		bool v = !btnSQL->value();
		REQ(set_button, btnSQL, v);
		*retval = xmlrpc_c::value_boolean(v);
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
		XMLRPC_LOCK;
		double v = sldrSquelch->value();
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
		XMLRPC_LOCK;
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
		XMLRPC_LOCK;
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
		XMLRPC_LOCK;
		bool v = !wf->btnRev->value();
		REQ(set_button, wf->btnRev, v);
		*retval = xmlrpc_c::value_boolean(v);
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
		XMLRPC_LOCK;
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
		_help = "Toggles the Transmit Lock state. Returns the new state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		bool v = !wf->xmtlock->value();
		REQ(set_button, wf->xmtlock, v);
		*retval = xmlrpc_c::value_boolean(v);
	}
};

// =============================================================================

class Main_get_rsid : public xmlrpc_c::method
{
public:
	Main_get_rsid()
	{
		_signature = "b:n";
		_help = "Returns the RSID state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_boolean(btnRSID->value());
	}
};

class Main_set_rsid : public xmlrpc_c::method
{
public:
	Main_set_rsid()
	{
		_signature = "b:b";
		_help = "Sets the RSID state. Returns the old state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		bool v = btnRSID->value();
		REQ(set_button, btnRSID, params.getBoolean(0));
		*retval = xmlrpc_c::value_boolean(v);
	}
};

class Main_toggle_rsid : public xmlrpc_c::method
{
public:
	Main_toggle_rsid()
	{
		_signature = "b:n";
		_help = "Toggles the RSID state. Returns the new state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		bool v = !btnRSID->value();
		REQ(set_button, btnRSID, v);
		*retval = xmlrpc_c::value_boolean(v);
	}
};

// =============================================================================

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
		XMLRPC_LOCK;
		if (!wf->xmtrcv->value())
			REQ(set_button, wf->xmtrcv, true);
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
		XMLRPC_LOCK;
		if (!btnTune->value())
			REQ(set_button, btnTune, !btnTune->value());
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
		XMLRPC_LOCK;
		if (wf->xmtrcv->value())
			REQ(set_button, wf->xmtrcv, false);
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
		XMLRPC_LOCK;
		if (trx_state == STATE_TX || trx_state == STATE_TUNE) {
			REQ(abort_tx);
			REQ(AbortARQ);
		}
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
		XMLRPC_LOCK;
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
		_help = "[DEPRECATED; use main.{get,set,toggle}_rsid]";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		if (!(wf->xmtrcv->value() || btnTune->value() || btnRSID->value()))
			REQ(set_button, btnRSID, true);
		*retval = xmlrpc_c::value_nil();
	}
};

// =============================================================================
// classes added to support flrig
//
// dhf 6/23/09

class Main_get_trx_state : public xmlrpc_c::method
{
public:
	Main_get_trx_state()
	{
		_signature = "s:n";
		_help = "Returns T/R state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		if (trx_state == STATE_TX || trx_state == STATE_TUNE)
			*retval = xmlrpc_c::value_string("TX");
		else if (trx_state == STATE_RX)
			*retval = xmlrpc_c::value_string("RX");
		else
			*retval = xmlrpc_c::value_string("OTHER");
	}
};

class Rig_set_name : public xmlrpc_c::method
{
public:
	Rig_set_name()
	{
		_signature = "n:s";
		_help = "Sets the rig name for xmlrpc rig";
	}
	static void set_rig_name(const string& name)
	{
		windowTitle = name;
		setTitle();
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		REQ(set_rig_name, params.getString(0));
		*retval = xmlrpc_c::value_nil();
	}
};

class Rig_get_name : public xmlrpc_c::method
{
public:
	Rig_get_name()
	{
		_signature = "s:n";
		_help = "Returns the rig name previously set via rig.set_name";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		*retval = xmlrpc_c::value_string(windowTitle);
	}
};

class Rig_set_frequency : public xmlrpc_c::method
{
public:
	Rig_set_frequency()
	{
		_signature = "d:d";
		_help = "Sets the RF carrier frequency. Returns the old value.";
	}
	static void set_frequency(long long rfc)
	{
		wf->rfcarrier(rfc);
		show_frequency(rfc);
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		double rfc = wf->rfcarrier();
		REQ(set_frequency, (long long int)params.getDouble(0, 0.0));
		*retval = xmlrpc_c::value_double(rfc);
	}
};

class Rig_set_modes : public xmlrpc_c::method
{
public:
	Rig_set_modes()
	{
		_signature = "n:A";
		_help = "Sets the list of available rig modes";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		vector<xmlrpc_c::value> v = params.getArray(0);

		vector<string> modes;
		modes.reserve(v.size());
		// copy
		for (vector<xmlrpc_c::value>::const_iterator i = v.begin(); i != v.end(); ++i)
			modes.push_back(static_cast<string>(xmlrpc_c::value_string(*i)));

		REQ_SYNC(set_combo_contents, qso_opMODE, &modes);

		*retval = xmlrpc_c::value_nil();
	}
};

class Rig_set_mode : public xmlrpc_c::method
{
public:
	Rig_set_mode()
	{
		_signature = "n:s";
		_help = "Selects a mode previously added by rig.set_modes";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		REQ(set_combo_value, qso_opMODE, params.getString(0));
		*retval = xmlrpc_c::value_nil();
	}
};

class Rig_get_modes : public xmlrpc_c::method
{
public:
	Rig_get_modes()
	{
		_signature = "A:n";
		_help = "Returns the list of available rig modes";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		vector<xmlrpc_c::value> modes;
		REQ_SYNC(get_combo_contents, qso_opMODE, &modes);

		*retval = xmlrpc_c::value_array(modes);
	}
};

class Rig_get_mode : public xmlrpc_c::method
{
public:
	Rig_get_mode()
	{
		_signature = "s:n";
		_help = "Returns the name of the current transceiver mode";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		*retval = xmlrpc_c::value_string(qso_opMODE->value());
	}
};


class Rig_set_bandwidths : public xmlrpc_c::method
{
public:
	Rig_set_bandwidths()
	{
		_signature = "n:A";
		_help = "Sets the list of available rig bandwidths";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		vector<xmlrpc_c::value> v = params.getArray(0);

		vector<string> bws;
		bws.reserve(v.size());
		for (vector<xmlrpc_c::value>::const_iterator i = v.begin(); i != v.end(); ++i)
			bws.push_back(static_cast<string>(xmlrpc_c::value_string(*i)));

		REQ_SYNC(set_combo_contents, qso_opBW, &bws);

		*retval = xmlrpc_c::value_nil();
	}
};

class Rig_set_bandwidth : public xmlrpc_c::method
{
public:
	Rig_set_bandwidth()
	{
		_signature = "n:s";
		_help = "Selects a bandwidth previously added by rig.set_bandwidths";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		REQ(set_combo_value, qso_opBW, params.getString(0));
		*retval = xmlrpc_c::value_nil();
	}
};

class Rig_get_bandwidths : public xmlrpc_c::method
{
public:
	Rig_get_bandwidths()
	{
		_signature = "A:n";
		_help = "Returns the list of available rig bandwidths";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		vector<xmlrpc_c::value> bws;
		REQ_SYNC(get_combo_contents, qso_opBW, &bws);

		*retval = xmlrpc_c::value_array(bws);
	}
};

class Rig_get_bandwidth : public xmlrpc_c::method
{
public:
	Rig_get_bandwidth()
	{
		_signature = "s:n";
		_help = "Returns the name of the current transceiver bandwidth";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		*retval = xmlrpc_c::value_string(qso_opBW->value());
	}
};

class Rig_get_notch : public xmlrpc_c::method
{
public:
	Rig_get_notch()
	{
		_signature = "s:n";
		_help = "Reports a notch filter frequency based on WF action";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		*retval = xmlrpc_c::value_int(notch_frequency);
	}
};

class Rig_set_notch : public xmlrpc_c::method
{
public:
	Rig_set_notch()
	{
		_signature = "n:i";
		_help = "Sets the notch filter position on WF";
	}
	static void set_notch(int freq)
	{
		notch_frequency = freq;
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		int notch = notch_frequency;
		REQ(set_notch, params.getInt(0));
		*retval = xmlrpc_c::value_int(notch);
	}
};

static int rig_control_counter;

static void set_rig_control(bool xmlrpc)
{
	struct rcb_t {
		Fl_Button *toggle, *init;
	};
	static struct rcb_t prev_rigcontrol;

	if (xmlrpc) {
		rcb_t b[] = {
			{ chkUSERIGCAT, btnInitRIGCAT },
			{ chkUSEHAMLIB, btnInitHAMLIB },
			{ chkUSEXMLRPC, btnInitXMLRPC }
		};
		for (size_t i = 0; i < sizeof(b)/sizeof(*b); i++) {
			if (b[i].toggle->value()) {
				prev_rigcontrol = b[i];
				break;
			}
		}
		if (!prev_rigcontrol.toggle) {
			prev_rigcontrol.toggle = chkUSEXMLRPC;
			prev_rigcontrol.init = btnInitXMLRPC;
		}
		bool changed = progdefaults.changed;
		chkUSEXMLRPC->value(1);
		chkUSEXMLRPC->do_callback();
		btnInitXMLRPC->do_callback();
		wf->setQSY(true);
		progdefaults.changed = changed;
	}
	else {
		bool changed = progdefaults.changed;
		prev_rigcontrol.toggle->value(1);
		prev_rigcontrol.toggle->do_callback();
		prev_rigcontrol.init->do_callback();
		progdefaults.changed = changed;
	}
}

class Rig_take_control : public xmlrpc_c::method
{
public:
	Rig_take_control()
	{
		_signature = "n:n";
		_help = "Switches rig control to XML-RPC";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		if (++rig_control_counter == 1)
			REQ_SYNC(set_rig_control, true);
		*retval = xmlrpc_c::value_nil();
	}
};

class Rig_release_control : public xmlrpc_c::method
{
public:
	Rig_release_control()
	{
		_signature = "n:n";
		_help = "Switches rig control to previous setting";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		if (rig_control_counter && !--rig_control_counter)
			REQ_SYNC(set_rig_control, false);
		*retval = xmlrpc_c::value_nil();
	}
};

// =============================================================================

class Main_set_rig_name : public Rig_set_name
{
public:
	Main_set_rig_name() { _help = "[DEPRECATED; use rig.set_name]"; }
};
class Main_set_rig_frequency : public Rig_set_frequency
{
public:
	Main_set_rig_frequency() { _help = "[DEPRECATED; use rig.set_frequency]"; }
};
class Main_set_rig_modes : public Rig_set_modes
{
public:
	Main_set_rig_modes() { _help = "[DEPRECATED; use rig.set_modes"; }
};
class Main_set_rig_mode : public Rig_set_mode
{
public:
	Main_set_rig_mode() { _help = "[DEPRECATED; use rig.set_mode"; }
};
class Main_get_rig_modes : public Rig_get_modes
{
public:
	Main_get_rig_modes() { _help = "[DEPRECATED; use rig.get_modes]"; }
};
class Main_get_rig_mode : public Rig_get_mode
{
public:
	Main_get_rig_mode() { _help = "[DEPRECATED; use rig.get_mode]"; }
};
class Main_set_rig_bandwidths : public Rig_set_bandwidths
{
public:
	Main_set_rig_bandwidths() { _help = "[DEPRECATED; use rig.set_bandwidths]"; }
};
class Main_set_rig_bandwidth : public Rig_set_bandwidth
{
public:
	Main_set_rig_bandwidth() { _help = "[DEPRECATED; use rig.set_bandwidth]"; }
};
class Main_get_rig_bandwidths : public Rig_set_bandwidths
{
public:
	Main_get_rig_bandwidths() { _help = "[DEPRECATED; use rig.get_bandwidths]"; }
};
class Main_get_rig_bandwidth : public Rig_get_bandwidth
{
public:
	Main_get_rig_bandwidth() { _help = "[DEPRECATED; use rig.get_bandwidth]"; }
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

class Log_get_time_on : public xmlrpc_c::method
{
public:
	Log_get_time_on()
	{
		_signature = "s:n";
		_help = "Returns the Time-On field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpTimeOn->value());
	}
};

class Log_get_time_off : public xmlrpc_c::method
{
public:
	Log_get_time_off()
	{
		_signature = "s:n";
		_help = "Returns the Time-Off field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpTimeOff->value());
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
		XMLRPC_LOCK;
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

class Log_set_name : public xmlrpc_c::method
{
public:
	Log_set_name()
	{
		_signature = "n:s";
		_help = "Sets the Name field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		REQ(set_text, inpName, params.getString(0));

		*retval = xmlrpc_c::value_nil();
	}
};

class Log_set_qth : public xmlrpc_c::method
{
public:
	Log_set_qth()
	{
		_signature = "n:s";
		_help = "Sets the QTH field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		REQ(set_text, inpQth, params.getString(0));

		*retval = xmlrpc_c::value_nil();
	}
};

class Log_set_locator : public xmlrpc_c::method
{
public:
	Log_set_locator()
	{
		_signature = "n:s";
		_help = "Sets the Locator field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		REQ(set_text, inpLoc, params.getString(0));

		*retval = xmlrpc_c::value_nil();
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

class Log_get_serial_number : public xmlrpc_c::method
{
public:
	Log_get_serial_number()
	{
		_signature = "s:n";
		_help = "Returns the serial number field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpSerNo->value());
	}
};

class Log_set_serial_number : public xmlrpc_c::method
{
public:
	Log_set_serial_number()
	{
		_signature = "n:s";
		_help = "Sets the serial number field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		REQ(set_text, inpSerNo, params.getString(0));
		*retval = xmlrpc_c::value_nil();
	}
};

class Log_get_serial_number_sent : public xmlrpc_c::method
{
public:
	Log_get_serial_number_sent()
	{
		_signature = "s:n";
		_help = "Returns the serial number (sent) field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(outSerNo->value());
	}
};

class Log_get_exchange : public xmlrpc_c::method
{
public:
	Log_get_exchange()
	{
		_signature = "s:n";
		_help = "Returns the contest exchange field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpXchgIn->value());
	}
};

class Log_set_exchange : public xmlrpc_c::method
{
public:
	Log_set_exchange()
	{
		_signature = "n:s";
		_help = "Sets the contest exchange field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		REQ(set_text, inpXchgIn, params.getString(0));
		*retval = xmlrpc_c::value_nil();
	}
};

class Log_get_state : public xmlrpc_c::method
{
public:
	Log_get_state()
	{
		_signature = "s:n";
		_help = "Returns the State field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpState->value());
	}
};

class Log_get_province : public xmlrpc_c::method
{
public:
	Log_get_province()
	{
		_signature = "s:n";
		_help = "Returns the Province field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpVEprov->value());
	}
};

class Log_get_country : public xmlrpc_c::method
{
public:
	Log_get_country()
	{
		_signature = "s:n";
		_help = "Returns the Country field contents.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(inpCountry->value());
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
		_help = "Returns the current band name.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		*retval = xmlrpc_c::value_string(band_name(band(wf->rfcarrier())));
	}
};

class Log_get_sb : public Main_get_wf_sideband
{
public:
	Log_get_sb() { _help = "[DEPRECATED; use main.get_wf_sideband]"; }
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
		XMLRPC_LOCK;
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
	static void get_rx_text_range(const xmlrpc_c::paramList* params, xmlrpc_c::fault** err,
				      char** text, int* size)
	{
		// the get* methods may throw but this function is not allowed to do so
		try {
			params->verifyEnd(2);

			Fl_Text_Buffer_mod* tbuf = ReceiveText->buffer();
			int len = tbuf->length();
			int start = params->getInt(0, 0, len - 1);
			int n = params->getInt(1, -1, len - start);
			if (n == -1)
				n = len; // we can request more text than is available

			*text = tbuf->text_range(start, start + n);
			*size = n;
		}
		catch (const xmlrpc_c::fault& f) {
			*err = new xmlrpc_c::fault(f);
		}
		catch (const exception& e) {
			*err = new xmlrpc_c::fault(e.what(), xmlrpc_c::fault::CODE_INTERNAL);
		}
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		xmlrpc_c::fault* err = NULL;
		char* text;
		int size;

		REQ_SYNC(get_rx_text_range, &params, &err, &text, &size);
		if (unlikely(err)) {
			xmlrpc_c::fault f(*err);
			delete err;
			throw f;
		}

		if (!size) {
			*retval = xmlrpc_c::value_bytestring("empty rx buffer!");
		} else {
			vector<unsigned char> bytes(size);
			memcpy(&bytes[0], text, size);
			*retval = xmlrpc_c::value_bytestring(bytes);
		}
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
		XMLRPC_LOCK;
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
		XMLRPC_LOCK;
		REQ_SYNC(&FTextTX::add_text, TransmitText, params.getString(0));
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
		XMLRPC_LOCK;
		vector<unsigned char> bytes = params.getBytestring(0);
		bytes.push_back(0);
		REQ_SYNC(&FTextTX::add_text, TransmitText, string((const char*)&bytes[0]));

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
		XMLRPC_LOCK;
		REQ(&FTextBase::clear, TransmitText);
		*retval = xmlrpc_c::value_nil();
	}
};

// =============================================================================

class RXTX_get_data : public xmlrpc_c::method
{
public:
	RXTX_get_data()
	{
		_signature = "6:n";
		_help = "Returns all RXTX combined data since last query.";
	}
	static void get_rxtx(char **text, int *size)
	{
		// the get* methods may throw but this function is not allowed to do so
		*text = get_rxtx_data();
		*size = strlen(*text);
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		char *text;
		int size;
		REQ_SYNC(get_rxtx, &text, &size);

		vector<unsigned char> bytes(size);
		memcpy(&bytes[0], text, size);
		*retval = xmlrpc_c::value_bytestring(bytes);
	}
};

// =============================================================================

class RX_get_data : public xmlrpc_c::method
{
public:
	RX_get_data()
	{
		_signature = "6:n";
		_help = "Returns all RX data received since last query.";
	}
	static void get_rx(char **text, int *size)
	{
		// the get* methods may throw but this function is not allowed to do so
		*text = get_rx_data();
		*size = strlen(*text);
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		char *text;
		int size;
		REQ_SYNC(get_rx, &text, &size);

		vector<unsigned char> bytes(size);
		memcpy(&bytes[0], text, size);
		*retval = xmlrpc_c::value_bytestring(bytes);
	}
};

// =============================================================================

class TX_get_data : public xmlrpc_c::method
{
public:
	TX_get_data()
	{
		_signature = "6:n";
		_help = "Returns all TX data transmitted since last query.";
	}
	static void get_tx(char **text, int *size)
	{
		// the get* methods may throw but this function is not allowed to do so
		*text = get_tx_data();
		*size = strlen(*text);
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
        {
		XMLRPC_LOCK;
		char *text;
		int size;
		REQ_SYNC(get_tx, &text, &size);

		vector<unsigned char> bytes(size);
		memcpy(&bytes[0], text, size);
		*retval = xmlrpc_c::value_bytestring(bytes);
	}
};

// =============================================================================

extern Fl_Button* btnAutoSpot; // FIXME: export in fl_digi.h

class Spot_get_auto : public xmlrpc_c::method
{
public:
	Spot_get_auto()
	{
		_signature = "b:n";
		_help = "Returns the autospotter state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		*retval = xmlrpc_c::value_boolean(btnAutoSpot->value());
	}
};

class Spot_set_auto : public xmlrpc_c::method
{
public:
	Spot_set_auto()
	{
		_signature = "b:b";
		_help = "Sets the autospotter state. Returns the old state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		bool v = btnAutoSpot->value();
		REQ(set_button, btnAutoSpot, params.getBoolean(0));
		*retval = xmlrpc_c::value_boolean(v);
	}
};

class Spot_toggle_auto : public xmlrpc_c::method
{
public:
	Spot_toggle_auto()
	{
		_signature = "b:n";
		_help = "Toggles the autospotter state. Returns the new state.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		XMLRPC_LOCK;
		bool v = !btnAutoSpot->value();
		REQ(set_button, btnAutoSpot, v);
		*retval = xmlrpc_c::value_boolean(v);
	}
};

class Spot_pskrep_get_count : public xmlrpc_c::method
{
public:
	Spot_pskrep_get_count()
	{
		_signature = "i:n";
		_help = "Returns the number of callsigns spotted in the current session.";
	}
	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	{
		*retval = xmlrpc_c::value_int(static_cast<unsigned>(pskrep_count()));
	}
};

// =============================================================================

// Returns the current wefax modem pointer.
static wefax * get_wefax(void)
{
	if( ( active_modem->get_mode() >= MODE_WEFAX_FIRST )
 	 && ( active_modem->get_mode() <= MODE_WEFAX_LAST ) )
	{
		wefax * ptr = dynamic_cast<wefax *>( active_modem );
		if( ptr == NULL ) throw runtime_error("Inconsistent wefax object");
		return ptr ;
	}
	throw runtime_error("Not in wefax mode");
}

struct Wefax_state_string : public xmlrpc_c::method
{
	Wefax_state_string() {
		_signature = "s:n";
		_help = "Returns Wefax engine state (tx and rx) for information."; }

	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	try
	{
		*retval = xmlrpc_c::value_string( get_wefax()->state_string() );
	}
	catch( const exception & e )
	{
		*retval = xmlrpc_c::value_string( e.what());
	}
};

struct Wefax_skip_apt : public xmlrpc_c::method
{
	Wefax_skip_apt() {
		_signature = "s:n";
		_help = "Skip APT during Wefax reception"; }

	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	try
	{
		get_wefax()->skip_apt();
		*retval = xmlrpc_c::value_string( "" );
	}
	catch( const exception & e )
	{
		*retval = xmlrpc_c::value_string( e.what() );
	}
};

/// TODO: Refresh the screen with the new value.
struct Wefax_skip_phasing : public xmlrpc_c::method
{
	Wefax_skip_phasing() {
		_signature = "s:n";
		_help = "Skip phasing during Wefax reception"; }

	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	try
	{
		get_wefax()->skip_phasing(true);
		*retval = xmlrpc_c::value_string( "" );
	}
	catch( const exception & e )
	{
		*retval = xmlrpc_c::value_string( e.what() );
	}
};

// TODO: The image should be reloaded just like cancelling from the GUI.
struct Wefax_set_tx_abort_flag : public xmlrpc_c::method
{
	Wefax_set_tx_abort_flag() {
		_signature = "s:n";
		_help = "Cancels Wefax image transmission"; }

	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	try
	{
		get_wefax()->set_tx_abort_flag();
		*retval = xmlrpc_c::value_string( "" );
	}
	catch( const exception & e )
	{
		*retval = xmlrpc_c::value_string( e.what() );
	}
};

struct Wefax_end_reception : public xmlrpc_c::method
{
	Wefax_end_reception() {
		_signature = "s:n";
		_help = "End Wefax image reception"; }

	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	try
	{
		get_wefax()->end_reception();
		*retval = xmlrpc_c::value_string( "" );
	}
	catch( const exception & e )
	{
		*retval = xmlrpc_c::value_string( e.what() );
	}
};

struct Wefax_start_manual_reception : public xmlrpc_c::method
{
	Wefax_start_manual_reception() {
		_signature = "s:n";
		_help = "Starts fax image reception in manual mode"; }

	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	try
	{
		get_wefax()->set_rx_manual_mode(true);
		get_wefax()->skip_apt();
		get_wefax()->skip_phasing(true);
		*retval = xmlrpc_c::value_string( "" );
	}
	catch( const exception & e )
	{
		*retval = xmlrpc_c::value_string( e.what() );
	}
};

struct Wefax_set_adif_log : public xmlrpc_c::method
{
	Wefax_set_adif_log() {
		_signature = "s:b";
		_help = "Set/reset logging to received/transmit images to ADIF log file"; }

	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	try
	{
		progdefaults.WEFAX_AdifLog = params.getBoolean(0);
		*retval = xmlrpc_c::value_string( "" );
	}
	catch( const exception & e )
	{
		*retval = xmlrpc_c::value_string( e.what() );
	}
};

struct Wefax_set_max_lines : public xmlrpc_c::method
{
	Wefax_set_max_lines() {
		_signature = "s:i";
		_help = "Set maximum lines for fax image reception"; }

	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	try
	{
		progdefaults.WEFAX_MaxRows = params.getInt(0);
		/// This updates the GUI.
		*retval = xmlrpc_c::value_string( "" );
	}
	catch( const exception & e )
	{
		*retval = xmlrpc_c::value_string( e.what() );
	}
};

struct Wefax_get_received_file : public xmlrpc_c::method
{
	Wefax_get_received_file() {
		_signature = "s:i";
		_help = "Waits for next received fax file, returns its name with a delay. Empty string if timeout."; }

	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	try
	{
		std::string filename = get_wefax()->get_received_file( params.getInt(0));
		*retval = xmlrpc_c::value_string( filename );
	}
	catch( const exception & e )
	{
		*retval = xmlrpc_c::value_string( e.what() );
	}
};

struct Wefax_send_file : public xmlrpc_c::method
{
	Wefax_send_file() {
		_signature = "s:si";
		_help = "Send file. returns an empty string if OK otherwise an error message."; }

	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	try
	{
		std::string status = get_wefax()->send_file( params.getString(0), params.getInt(1) );
		*retval = xmlrpc_c::value_string( status );
	}
	catch( const exception & e )
	{
		*retval = xmlrpc_c::value_string( e.what() );
	}
};

// =============================================================================

// Returns the current navtex modem pointer.
static navtex * get_navtex(void)
{
	if( ( active_modem->get_mode() != MODE_NAVTEX )
	 && ( active_modem->get_mode() != MODE_SITORB ) )
	{
		navtex * ptr = dynamic_cast<navtex *>( active_modem );
		if( ptr == NULL ) throw runtime_error("Inconsistent navtex object");
		return ptr ;
	}
	throw runtime_error("Not in navtex or sitorB mode");
}

struct Navtex_get_message : public xmlrpc_c::method
{
	Navtex_get_message() {
		_signature = "s:i";
		_help = "Returns next Navtex/SitorB message with a max delay in seconds.. Empty string if timeout."; }

	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	try
	{
		*retval = xmlrpc_c::value_string( get_navtex()->get_message( params.getInt(0)) );
	}
	catch( const exception & e )
	{
		*retval = xmlrpc_c::value_string( e.what());
	}
};

struct Navtex_send_message : public xmlrpc_c::method
{
	Navtex_send_message() {
		_signature = "s:s";
		_help = "Send a Navtex/SitorB message. Returns an empty string if OK otherwise an error message."; }

	void execute(const xmlrpc_c::paramList& params, xmlrpc_c::value* retval)
	try
	{
		std::string status = get_navtex()->send_message( params.getString(0) );
		*retval = xmlrpc_c::value_string( status );
	}
	catch( const exception & e )
	{
		*retval = xmlrpc_c::value_string( e.what() );
	}
};

// =============================================================================

// End XML-RPC interface

// method list: ELEM_(class_name, "method_name")
#undef ELEM_
#define METHOD_LIST													\
	ELEM_(Fldigi_list, "fldigi.list")								\
	ELEM_(Fldigi_name, "fldigi.name")								\
	ELEM_(Fldigi_version_struct, "fldigi.version_struct")			\
	ELEM_(Fldigi_version_string, "fldigi.version")					\
	ELEM_(Fldigi_name_version, "fldigi.name_version")				\
	ELEM_(Fldigi_config_dir, "fldigi.config_dir")					\
	ELEM_(Fldigi_terminate, "fldigi.terminate")						\
																	\
	ELEM_(Modem_get_name, "modem.get_name")							\
	ELEM_(Modem_get_names, "modem.get_names")						\
	ELEM_(Modem_get_id, "modem.get_id")								\
	ELEM_(Modem_get_max_id, "modem.get_max_id")						\
	ELEM_(Modem_set_by_name, "modem.set_by_name")					\
	ELEM_(Modem_set_by_id, "modem.set_by_id")						\
																	\
	ELEM_(Modem_set_carrier, "modem.set_carrier")					\
	ELEM_(Modem_inc_carrier, "modem.inc_carrier")					\
	ELEM_(Modem_get_carrier, "modem.get_carrier")					\
																	\
	ELEM_(Modem_get_afc_sr, "modem.get_afc_search_range")			\
	ELEM_(Modem_set_afc_sr, "modem.set_afc_search_range")			\
	ELEM_(Modem_inc_afc_sr, "modem.inc_afc_search_range")			\
																	\
	ELEM_(Modem_get_bw, "modem.get_bandwidth")						\
	ELEM_(Modem_set_bw, "modem.set_bandwidth")						\
	ELEM_(Modem_inc_bw, "modem.inc_bandwidth")						\
																	\
	ELEM_(Modem_get_quality, "modem.get_quality")					\
	ELEM_(Modem_search_up, "modem.search_up")						\
	ELEM_(Modem_search_down, "modem.search_down")					\
																	\
	ELEM_(Modem_olivia_set_bandwidth, "modem.olivia.set_bandwidth")	\
	ELEM_(Modem_olivia_get_bandwidth, "modem.olivia.get_bandwidth")	\
	ELEM_(Modem_olivia_set_tones, "modem.olivia.set_tones")			\
	ELEM_(Modem_olivia_get_tones, "modem.olivia.get_tones")			\
																	\
	ELEM_(Main_get_status1, "main.get_status1")						\
	ELEM_(Main_get_status2, "main.get_status2")						\
																	\
	ELEM_(Main_get_sb, "main.get_sideband")							\
	ELEM_(Main_set_sb, "main.set_sideband")							\
	ELEM_(Main_get_wf_sideband, "main.get_wf_sideband")				\
	ELEM_(Main_set_wf_sideband, "main.set_wf_sideband")				\
	ELEM_(Main_get_freq, "main.get_frequency")						\
	ELEM_(Main_set_freq, "main.set_frequency")						\
	ELEM_(Main_inc_freq, "main.inc_frequency")						\
																	\
	ELEM_(Main_get_afc, "main.get_afc")								\
	ELEM_(Main_set_afc, "main.set_afc")								\
	ELEM_(Main_toggle_afc, "main.toggle_afc")						\
																	\
	ELEM_(Main_get_sql, "main.get_squelch")							\
	ELEM_(Main_set_sql, "main.set_squelch")							\
	ELEM_(Main_toggle_sql, "main.toggle_squelch")					\
																	\
	ELEM_(Main_get_sql_level, "main.get_squelch_level")				\
	ELEM_(Main_set_sql_level, "main.set_squelch_level")				\
	ELEM_(Main_inc_sql_level, "main.inc_squelch_level")				\
																	\
	ELEM_(Main_get_rev, "main.get_reverse")							\
	ELEM_(Main_set_rev, "main.set_reverse")							\
	ELEM_(Main_toggle_rev, "main.toggle_reverse")					\
																	\
	ELEM_(Main_get_lock, "main.get_lock")							\
	ELEM_(Main_set_lock, "main.set_lock")							\
	ELEM_(Main_toggle_lock, "main.toggle_lock")						\
																	\
	ELEM_(Main_get_rsid, "main.get_rsid")							\
	ELEM_(Main_set_rsid, "main.set_rsid")							\
	ELEM_(Main_toggle_rsid, "main.toggle_rsid")						\
																	\
	ELEM_(Main_get_trx_status, "main.get_trx_status")				\
	ELEM_(Main_tx, "main.tx")										\
	ELEM_(Main_tune, "main.tune")									\
	ELEM_(Main_rsid, "main.rsid")									\
	ELEM_(Main_rx, "main.rx")										\
	ELEM_(Main_abort, "main.abort")									\
																	\
	ELEM_(Main_get_trx_state, "main.get_trx_state")					\
	ELEM_(Main_set_rig_name, "main.set_rig_name")					\
	ELEM_(Main_set_rig_frequency, "main.set_rig_frequency")			\
	ELEM_(Main_set_rig_modes, "main.set_rig_modes")					\
	ELEM_(Main_set_rig_mode, "main.set_rig_mode")					\
	ELEM_(Main_get_rig_modes, "main.get_rig_modes")					\
	ELEM_(Main_get_rig_mode, "main.get_rig_mode")					\
	ELEM_(Main_set_rig_bandwidths, "main.set_rig_bandwidths")		\
	ELEM_(Main_set_rig_bandwidth, "main.set_rig_bandwidth")			\
	ELEM_(Main_get_rig_bandwidth, "main.get_rig_bandwidth")			\
	ELEM_(Main_get_rig_bandwidths, "main.get_rig_bandwidths")		\
																	\
	ELEM_(Main_run_macro, "main.run_macro")							\
	ELEM_(Main_get_max_macro_id, "main.get_max_macro_id")			\
																	\
	ELEM_(Rig_set_name, "rig.set_name")								\
	ELEM_(Rig_get_name, "rig.get_name")								\
	ELEM_(Rig_set_frequency, "rig.set_frequency")					\
	ELEM_(Rig_set_modes, "rig.set_modes")							\
	ELEM_(Rig_set_mode, "rig.set_mode")								\
	ELEM_(Rig_get_modes, "rig.get_modes")							\
	ELEM_(Rig_get_mode, "rig.get_mode")								\
	ELEM_(Rig_set_bandwidths, "rig.set_bandwidths")					\
	ELEM_(Rig_set_bandwidth, "rig.set_bandwidth")					\
	ELEM_(Rig_get_bandwidth, "rig.get_bandwidth")					\
	ELEM_(Rig_get_bandwidths, "rig.get_bandwidths")					\
	ELEM_(Rig_get_notch, "rig.get_notch")							\
	ELEM_(Rig_set_notch, "rig.set_notch")							\
	ELEM_(Rig_take_control, "rig.take_control")						\
	ELEM_(Rig_release_control, "rig.release_control")				\
																	\
	ELEM_(Log_get_freq, "log.get_frequency")						\
	ELEM_(Log_get_time_on, "log.get_time_on")						\
	ELEM_(Log_get_time_off, "log.get_time_off")						\
	ELEM_(Log_get_call, "log.get_call")								\
	ELEM_(Log_get_name, "log.get_name")								\
	ELEM_(Log_get_rst_in, "log.get_rst_in")							\
	ELEM_(Log_get_rst_out, "log.get_rst_out")						\
	ELEM_(Log_get_serial_number, "log.get_serial_number")			\
	ELEM_(Log_set_serial_number, "log.set_serial_number")			\
	ELEM_(Log_get_serial_number_sent, "log.get_serial_number_sent")	\
	ELEM_(Log_get_exchange, "log.get_exchange")						\
	ELEM_(Log_set_exchange, "log.set_exchange")						\
	ELEM_(Log_get_state, "log.get_state")							\
	ELEM_(Log_get_province, "log.get_province")						\
	ELEM_(Log_get_country, "log.get_country")						\
	ELEM_(Log_get_qth, "log.get_qth")								\
	ELEM_(Log_get_band, "log.get_band")								\
	ELEM_(Log_get_sb, "log.get_sideband")							\
	ELEM_(Log_get_notes, "log.get_notes")							\
	ELEM_(Log_get_locator, "log.get_locator")						\
	ELEM_(Log_get_az, "log.get_az")									\
	ELEM_(Log_clear, "log.clear")									\
	ELEM_(Log_set_call, "log.set_call")								\
	ELEM_(Log_set_name, "log.set_name")								\
	ELEM_(Log_set_qth, "log.set_qth")								\
	ELEM_(Log_set_locator, "log.set_locator")						\
																	\
	ELEM_(Text_get_rx_length, "text.get_rx_length")					\
	ELEM_(Text_get_rx, "text.get_rx")								\
	ELEM_(Text_clear_rx, "text.clear_rx")							\
	ELEM_(Text_add_tx, "text.add_tx")								\
	ELEM_(Text_add_tx_bytes, "text.add_tx_bytes")					\
	ELEM_(Text_clear_tx, "text.clear_tx")							\
																	\
	ELEM_(RXTX_get_data, "rxtx.get_data")							\
	ELEM_(RX_get_data, "rx.get_data")								\
	ELEM_(TX_get_data, "tx.get_data")								\
																	\
	ELEM_(Spot_get_auto, "spot.get_auto")							\
	ELEM_(Spot_set_auto, "spot.set_auto")							\
	ELEM_(Spot_toggle_auto, "spot.toggle_auto")						\
	ELEM_(Spot_pskrep_get_count, "spot.pskrep.get_count")				\
																		\
	ELEM_(Wefax_state_string, "wefax.state_string")						\
	ELEM_(Wefax_skip_apt, "wefax.skip_apt")								\
	ELEM_(Wefax_skip_phasing, "wefax.skip_phasing")						\
	ELEM_(Wefax_set_tx_abort_flag, "wefax.set_tx_abort_flag")			\
	ELEM_(Wefax_end_reception, "wefax.end_reception")					\
	ELEM_(Wefax_start_manual_reception, "wefax.start_manual_reception")	\
	ELEM_(Wefax_set_adif_log, "wefax.set_adif_log")						\
	ELEM_(Wefax_set_max_lines, "wefax.set_max_lines")					\
	ELEM_(Wefax_get_received_file, "wefax.get_received_file")			\
	ELEM_(Wefax_send_file, "wefax.send_file")							\
																		\
	ELEM_(Navtex_get_message, "navtex.get_message")						\
	ELEM_(Navtex_send_message, "navtex.send_message")					\

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
#undef ELEM_
#define ELEM_(class_, name_) { RpcBuilder<class_>::factory, NULL, name_ },
	rpc_method m[] = { METHOD_LIST };
	methods = new methods_t(m, m + sizeof(m)/sizeof(*m));

	if (!progdefaults.xmlrpc_deny.empty())
		methods->remove_if(rm_pred(progdefaults.xmlrpc_deny.c_str(), false));
	else if (!progdefaults.xmlrpc_allow.empty())
		methods->remove_if(rm_pred(progdefaults.xmlrpc_allow.c_str(), true));

	for( methods_t::iterator it = methods->begin(), en = methods->end(); it != en; ++it )
	{
		XmlRpcServerMethod * mth = it->m_fact( it->name );
		it->method = dynamic_cast< xmlrpc_c::method * >( mth );
	}
}
