// ----------------------------------------------------------------------------
//      socket.h
//
// Copyright (C) 2008-2009
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

#ifndef SOCKET_H_
#define SOCKET_H_

#include <config.h>

#include <sys/time.h>
#ifndef __MINGW32__
#  include <sys/socket.h>
#  include <netdb.h>
#  if defined(__OpenBSD__) && defined(nitems)
#    undef nitems
#  endif
#  include <netinet/in.h>
#else
#undef _WINSOCKAPI_
#  include <winsock2.h>
#  include <windows.h>
#  ifndef ENOTCONN
#  define ENOTCONN WSAENOTCONN
#  endif
#endif

#include <string>
#include <exception>
#include <cstring>

class SocketException : public std::exception
{
public:
        SocketException(int err_ = 0)     : err(err_), msg(err_to_str(err_)) { }
        SocketException(const char* msg_) : err(1),    msg(msg_)             { }
	SocketException(int err_, const std::string& prefix)
		: err(err_), msg(std::string(prefix).append(": ").append(err_to_str(err_))) { }
        virtual ~SocketException() throw() { }

        const char*     what(void) const throw() { return msg.c_str(); }
        int             error(void) const { return err; }

protected:
        const char* err_to_str(int e)
        {
#if HAVE_GETADDRINFO
		if (e < 0)
			return gai_strerror(e);
		else
#endif
			return strerror(e);
	}

        int             err;
        std::string     msg;
};

#if HAVE_GETADDRINFO
typedef struct addrinfo addr_info_t;
#else
struct addr_info_t {
	int ai_family;
	int ai_socktype;
	int ai_protocol;
	int ai_addrlen;
	struct sockaddr * ai_addr;
};
#endif

class Address
{

public:
	Address(const char* host = "", int port = 0, const char* proto_name = "tcp");
	Address(const char* host, const char* port_name, const char* proto_name = "tcp");
	Address(const Address& addr);
	~Address();
	Address& operator=(const Address& rhs);

	size_t size(void) const;
	const addr_info_t* get(size_t n = 0) const;
	const addr_info_t* udp_get_recv_addr(void);
	static std::string get_str(const addr_info_t* addr);
	unsigned int get_udp_io_port() {return port_io;};
	unsigned int get_udp_o_port() {return port_out;};
	std::string get_service() {return service;};
	std::string get_node() {return node;};

private:
	void lookup(const char* proto_name);

	std::string node;
	std::string service;
	unsigned int port_io;
	unsigned int port_out;

#if HAVE_GETADDRINFO
	struct addrinfo* info;
#else
	struct hostent host_entry;
	struct servent service_entry;
	mutable addr_info_t addr;
	mutable struct sockaddr_in saddr;
#endif
	bool copied;
};

class Socket
{
public:
	Socket(const Address& addr);
	Socket(int fd = -1);
	Socket(const Socket& s);
	~Socket();
	Socket& operator=(const Socket& rhs);

	void open(const Address& addr);
	void close(void);
	bool wait(int dir);

	// Server
        void bind(void);
	void bindUDP(void);
	void listen(int backlog = SOMAXCONN);
	Socket accept(void);
	Socket accept1(void);
	Socket * accept2(void);

	// Client
	void connect(const Address& addr);
	void connect(void);
	bool connect1(void);

	// Data Transimission
	size_t send(const void* buf, size_t len);
	size_t send(const std::string& buf);

	size_t recv(void* buf, size_t len);
	size_t recv(std::string& buf);

	// Unconnected Data Transmission
	size_t sendTo(const void* buf, size_t len);
	size_t sendTo(const std::string& buf);

	size_t recvFrom(void* buf, size_t len);
	size_t recvFrom(std::string& buf);

	// Options
	int get_bufsize(int dir);
	void set_bufsize(int dir, int len);
	void set_nonblocking(bool v = true);
	void set_nodelay(bool v = true);
	void set_timeout(const struct timeval& t);
	void set_timeout(double t);
	void set_autoclose(bool v) const;
	void set_close_on_exec(bool v, int fd = -1);
	void broadcast(bool flag);

	// Other
	unsigned int get_port(struct sockaddr *sa);
	void set_port(struct sockaddr *sa, unsigned int port);
	unsigned long get_address4(struct sockaddr *sa);
	void dual_port(int * dual_port);
	void set_dual_port_number(unsigned int dual_port_number);
	void set_dual_port_number(std::string port);
	unsigned int get_dual_port_number(void) { return dual_port_number; };
	unsigned int get_local_port() { return (unsigned int) atoi(address.get_service().c_str()); };
	unsigned long get_to_address(void);
	int use_dual_port(void) { if(use_kiss_dual_port) return *use_kiss_dual_port; else return 0; };
   bool is_connected(void);

	int fd(void);
	void shut_down(void);

private:
	int sockfd;
	Address address;
	size_t anum;
	const addr_info_t* ainfo;
	char* buffer;
	struct timeval timeout;
	bool nonblocking;
	mutable bool autoclose;
	struct sockaddr_storage saddr;
	struct sockaddr_storage saddr_dp;
	unsigned int saddr_size;
	int *use_kiss_dual_port;
	unsigned int dual_port_number;
   bool connected_flag;
};

#endif // SOCKET_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
