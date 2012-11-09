#ifndef ARQIO_H
#define ARQIO_H

#include <vector>

#include "threads.h"
#include "socket.h"

extern void WriteARQsocket(unsigned char* data, size_t len);
extern bool Socket_arqRx();
extern void AbortARQ();
extern void pskmail_notify_rsid(trx_mode mode);
extern void pskmail_notify_s2n(double s2n_count, double s2n_avg, double s2n_stddev);

class ARQ_SOCKET_Server
{
public:
	static bool start(const char* node, const char* service);
	static void stop(void);
private:
	ARQ_SOCKET_Server();
	~ARQ_SOCKET_Server();
	ARQ_SOCKET_Server(const ARQ_SOCKET_Server&);
	ARQ_SOCKET_Server operator=(const ARQ_SOCKET_Server&);
	static void* thread_func(void*);

private:
	static ARQ_SOCKET_Server* inst;
	bool run;
	Socket* server_socket;
};

#endif // ARQIO_H
