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
#  include <netinet/in.h>
#else
#  include <winsock2.h>
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
	struct sockaddr* ai_addr;
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
	static std::string get_str(const addr_info_t* addr);

private:
	void lookup(const char* proto_name);

	std::string node;
	std::string service;
	int port;

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
	void listen(int backlog = SOMAXCONN);
	Socket accept(void);
	Socket accept1(void);

	// Client
	void connect(const Address& addr);
	void connect(void);

	// Data Transimission
	size_t send(const void* buf, size_t len);
	size_t send(const std::string& buf);

	size_t recv(void* buf, size_t len);
	size_t recv(std::string& buf);

	// Options
	int get_bufsize(int dir);
	void set_bufsize(int dir, int len);
	void set_nonblocking(bool v = true);
	void set_nodelay(bool v = true);
	void set_timeout(const struct timeval& t);
	void set_timeout(double t);
	void set_autoclose(bool v) const;
	void set_close_on_exec(bool v, int fd = -1);

	int fd(void);

private:
	int sockfd;
	Address address;
	size_t anum;
	const addr_info_t* ainfo;
	char* buffer;
	struct timeval timeout;
	bool nonblocking;
	mutable bool autoclose;
};

#endif // SOCKET_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
