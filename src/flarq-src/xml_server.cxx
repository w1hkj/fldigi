// ---------------------------------------------------------------------
//
//			xml_server.cxx, a part of flarq
//
// Copyflarqht (C) 2016
//							 Dave Freese, W1HKJ
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the program; if not, write to the
//
//  Free Software Foundation, Inc.
//  51 Franklin Street, Fifth Floor
//  Boston, MA  02110-1301 USA.
//
// ---------------------------------------------------------------------

#include <config.h>

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Enumerations.H>

//#include "support.h"
//#include "debug.h"

#include "arq.h"
#include "flarq.h"

#include "xml_server.h"

#include "xmlrpcpp/XmlRpc.h"

using namespace XmlRpc;

// The server
XmlRpcServer flarq_server;

//----------------------------------------------------------------------
// get interface
//----------------------------------------------------------------------
bool xml_rx_text_ready = false;

//----------------------------------------------------------------------
// Request for flarq version
//----------------------------------------------------------------------
class flarq_get_version : public XmlRpcServerMethod {
public:
	flarq_get_version(XmlRpcServer* s) : XmlRpcServerMethod("flarq.get_version", s) {}

	void execute(XmlRpcValue& params, XmlRpcValue& result) {
		result = VERSION;
	}

	std::string help() { return std::string("returns version number of flarq"); }

} flarq_get_version(&flarq_server);

//----------------------------------------------------------------------
// Request for ARQ state
//----------------------------------------------------------------------
class flarq_get_state : public XmlRpcServerMethod {
public:
	flarq_get_state(XmlRpcServer* s) : XmlRpcServerMethod("flarq.get_state", s) {}

// x00 - unconnected
// x81 - connected
// x82 - sending
// x83 - receiving
// x84 - send completed
// x85 - recv completed

	void execute(XmlRpcValue& params, XmlRpcValue& result) {
		int state = 0;
		if (arqstate != ARQ_CONNECTED)
			state = 0;
		else if (sendingfile)
			state = 0x82;
		else if (rxARQfile)
			state = 0x83;
		else if (xml_rx_text_ready)
			state = 0x85;
		else
			state = 0x81;
		result = state;
	}

	std::string help() { return std::string("returns state of connection"); }

} flarq_get_state(&flarq_server);

//----------------------------------------------------------------------
// Request for received text
//----------------------------------------------------------------------

class flarq_rcvd_text : public XmlRpcServerMethod {
public:
	flarq_rcvd_text(XmlRpcServer* s) : XmlRpcServerMethod("flarq.rcvd_text", s) {}

	void execute(XmlRpcValue& params, XmlRpcValue& result) {
		std::string result_string = "none";
		if (xml_rx_text_ready) result_string = txtarqload;
		xml_rx_text_ready = false;
		result = result_string;
		txtarqload = "";
	}

	std::string help() { return std::string("returns received text"); }

} flarq_rcvd_text(&flarq_server);

//----------------------------------------------------------------------
// set interface
//----------------------------------------------------------------------

//------------------------------------------------------------------------------
// Send text
//------------------------------------------------------------------------------

class flarq_send_text : public XmlRpcServerMethod {
public:
	flarq_send_text(XmlRpcServer* s) : XmlRpcServerMethod("flarq.send_text", s) {}

	void execute(XmlRpcValue& params, XmlRpcValue &result) {
		std::string txt_to_send = string(params[0]);
		send_xml_text("FLMSG_XFR", txt_to_send);
	}
	std::string help() { return std::string("send_text"); }

} flarq_send_text(&flarq_server);

struct MLIST {
	string name; string signature; string help;
} mlist[] = {
	{ "flarq.rcvd_text",     "s:n", "return MODE of current VFO" },
	{ "flarq.get_state",    "s:n", "return PTT state" },
	{ "flarq.send_text",     "i:i", "set MODE iaw MODE table" }
};

class flarq_list_methods : public XmlRpcServerMethod {
public:
	flarq_list_methods(XmlRpcServer *s) : XmlRpcServerMethod("flarq.list_methods", s) {}

	void execute(XmlRpcValue& params, XmlRpcValue& result) {

		vector<XmlRpcValue> methods;
		for (size_t n = 0; n < sizeof(mlist) / sizeof(*mlist); ++n) {
			XmlRpcValue::ValueStruct item;
			item["name"]      = mlist[n].name;
			item["signature"] = mlist[n].signature;
			item["help"]      = mlist[n].help;
			methods.push_back(item);
		}

		result = methods;
	}
	std::string help() { return std::string("get flarq methods"); }
} flarq_list_methods(&flarq_server);

//------------------------------------------------------------------------------
// support thread xmlrpc clients
//------------------------------------------------------------------------------

pthread_t *xml_thread = 0;

void * xml_thread_loop(void *d)
{
	for(;;) {
		flarq_server.work(-1.0);
	}
	return NULL;
}

void start_xml_server(int port)
{
	XmlRpc::setVerbosity(0);

// Create the server socket on the specified port
	flarq_server.bindAndListen(port);

// Enable introspection
	flarq_server.enableIntrospection(true);

	xml_thread = new pthread_t;
	if (pthread_create(xml_thread, NULL, xml_thread_loop, NULL)) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}
}

void exit_server()
{
	flarq_server.exit();
}


