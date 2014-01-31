#ifndef XMLRPC_H
#define XMLRPC_H

#include <iosfwd>
#include <string>

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
extern int number_of_samples(std::string s);
#endif // XMLRPC_H
