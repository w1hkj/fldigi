// ----------------------------------------------------------------------------
// Copyright (C) 2014
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

#ifndef XMLRPC_H
#define XMLRPC_H

#include <iosfwd>
#include <string>

#include <xmlrpcpp/XmlRpcServer.h>
#include <xmlrpcpp/XmlRpcServerMethod.h>
#include <xmlrpcpp/XmlRpcValue.h>

#include "threads.h"

class XmlRpcImpl;

class XML_RPC_Server
{
public:
	static void start(const char* node, const char* service);
	static void stop(void);
	static std::ostream& list_methods(std::ostream& out);
private:
	XML_RPC_Server();
	~XML_RPC_Server();
	XML_RPC_Server(const XML_RPC_Server&);
	XML_RPC_Server operator=(const XML_RPC_Server&);
	static void add_methods(void);
	static void* thread_func(void*);

private:
	static XML_RPC_Server* inst;
	bool run;
	XmlRpcImpl* server_impl;
};

extern void xmlrpc_set_qsy(long long rfc);
extern int  xmltest_char();
extern bool xmltest_char_available;
extern void reset_xmlchars();
extern int number_of_samples(std::string s);

extern bool flmsg_online;
extern std::string flmsg_data;
extern pthread_mutex_t* server_mutex;

#endif // XMLRPC_H
