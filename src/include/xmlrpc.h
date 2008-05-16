#ifndef XMLRPC_H
#define XMLRPC_H

class XML_RPC_Server
{
public:
	static void start(const char* node, const char* service);
	static void stop(void);
private:
	XML_RPC_Server(int sfd_);
	XML_RPC_Server();
	~XML_RPC_Server();
	XML_RPC_Server(const XML_RPC_Server&);
	XML_RPC_Server operator=(const XML_RPC_Server&);
	void add_methods(void);
	static int get_socket(const char* node, const char* service, int& fd);
	static void* thread_func(void* arg);

private:
	static XML_RPC_Server* inst;
	bool run;
	unsigned short sfd;
};

#endif // XMLRPC_H
