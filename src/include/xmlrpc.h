#ifndef XMLRPC_H
#define XMLRPC_H

class Socket;

class XML_RPC_Server
{
public:
	static void start(const char* node, const char* service);
	static void stop(void);
private:
	XML_RPC_Server();
	~XML_RPC_Server();
	XML_RPC_Server(const XML_RPC_Server&);
	XML_RPC_Server operator=(const XML_RPC_Server&);
	void add_methods(void);
	static void* thread_func(void*);

private:
	static XML_RPC_Server* inst;
	bool run;
	Socket* server_socket;
};

#endif // XMLRPC_H
