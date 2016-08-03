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

#ifndef ARQIO_H
#define ARQIO_H

class Socket;

extern void WriteARQsocket(unsigned char* data, size_t len);
extern bool Socket_arqRx();
extern void AbortARQ();
extern void pskmail_notify_rsid(trx_mode mode);
extern void pskmail_notify_s2n(double s2n_count, double s2n_avg, double s2n_stddev);
extern void flush_arq_tx_buffer(void);

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
