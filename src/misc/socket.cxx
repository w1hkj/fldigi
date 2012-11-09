// ----------------------------------------------------------------------------
//      socket.cxx
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

#include <config.h>

#include <sys/types.h>

#ifndef __MINGW32__
#  include <sys/socket.h>
#  include <netdb.h>
#  if defined(__OpenBSD__) && defined(nitems)
#    undef nitems
#  endif
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/time.h>
#  include <sys/select.h>
#else
#  include "compat.h"
#endif

#include <unistd.h>
#include <fcntl.h>

#include <string>
#include <sstream>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cstdio>

#ifndef NDEBUG
  #include "debug.h"
#endif

#include "socket.h"

#if HAVE_GETADDRINFO && !defined(AI_NUMERICSERV)
#  define AI_NUMERICSERV 0
#endif


using namespace std;

//
// utility functions
//

#if HAVE_GETADDRINFO
static void copy_addrinfo(struct addrinfo** info, const struct addrinfo* src)
{
	struct addrinfo* p = *info;

	for (const struct addrinfo* rp = src; rp; rp = rp->ai_next) {
		if (p) {
			p->ai_next = new struct addrinfo;
			p = p->ai_next;
		}
		else {
			p = new struct addrinfo;
			if (!*info)
				*info = p;
		}

		p->ai_flags = rp->ai_flags;
		p->ai_family = rp->ai_family;
		p->ai_socktype = rp->ai_socktype;
		p->ai_protocol = rp->ai_protocol;
		p->ai_addrlen = rp->ai_addrlen;
		if (rp->ai_addr) {
			p->ai_addr = reinterpret_cast<struct sockaddr*>(new struct sockaddr_storage);
			memcpy(p->ai_addr, rp->ai_addr, rp->ai_addrlen);
		}
		else
			p->ai_addr = NULL;
		if (rp->ai_canonname)
			p->ai_canonname = strdup(rp->ai_canonname);
		else
			p->ai_canonname = NULL;

		p->ai_next = NULL;
	}
}

static void free_addrinfo(struct addrinfo* ai)
{
	for (struct addrinfo *next, *p = ai; p; p = next) {
		next = p->ai_next;
		delete reinterpret_cast<struct sockaddr_storage*>(p->ai_addr);
		free(p->ai_canonname);
		delete p;
	}
}

#else

static void copy_charpp(char*** dst, const char* const* src)
{
	if (src == NULL) {
		*dst = NULL;
		return;
	}

	size_t n = 0;
	for (const char* const* s = src; *s; s++)
		n++;
	*dst = new char*[n+1];
	for (size_t i = 0; i < n; i++)
		(*dst)[i] = strdup(src[i]);
	(*dst)[n] = NULL;
}

static void copy_hostent(struct hostent* dst, const struct hostent* src)
{
	if (src->h_name)
		dst->h_name = strdup(src->h_name);
	else
		dst->h_name = NULL;
	copy_charpp(&dst->h_aliases, src->h_aliases);
	dst->h_length = src->h_length;

	if (src->h_addr_list) {
		size_t n = 0;
		for (const char* const* p = src->h_addr_list; *p; p++)
			n++;
		dst->h_addr_list = new char*[n+1];
		for (size_t i = 0; i < n; i++) {
			dst->h_addr_list[i] = new char[src->h_length];
			memcpy(dst->h_addr_list[i], src->h_addr_list[i], src->h_length);
		}
		dst->h_addr_list[n] = NULL;
	}
	else
		dst->h_addr_list = NULL;
}

static void copy_servent(struct servent* dst, const struct servent* src)
{
	if (src->s_name)
		dst->s_name = strdup(src->s_name);
	else
		dst->s_name = NULL;
	copy_charpp(&dst->s_aliases, src->s_aliases);
	dst->s_port = src->s_port;
	if (src->s_proto)
		dst->s_proto = strdup(src->s_proto);
	else
		dst->s_proto = NULL;
}

static void free_charpp(char** pp)
{
	if (!pp)
		return;
	for (char** p = pp; *p; p++)
		free(*p);
	delete [] pp;
}

static void free_hostent(struct hostent* hp)
{
	free(const_cast<char*>(hp->h_name));
	free_charpp(hp->h_aliases);
	if (hp->h_addr_list) {
		for (char** p = hp->h_addr_list; *p; p++)
			delete [] *p;
		delete [] hp->h_addr_list;
	}
}

static void free_servent(struct servent* sp)
{
	free(const_cast<char*>(sp->s_name));
	free_charpp(sp->s_aliases);
	free(sp->s_proto);
}
#endif // HAVE_GETADDRINFO


//
// Address class
//

Address::Address(const char* host, int port, const char* proto_name)
	: node(host), copied(false)
{
#if HAVE_GETADDRINFO
	info = NULL;
#else
	memset(&host_entry, 0, sizeof(host_entry));
	memset(&service_entry, 0, sizeof(service_entry));
#endif

	if (node.empty() && port == 0)
		return;

	ostringstream s;
	s << port;
	service = s.str();

	lookup(proto_name);
}

Address::Address(const char* host, const char* port_name, const char* proto_name)
	: node(host), service(port_name), copied(false)
{
#if HAVE_GETADDRINFO
	info = NULL;
#else
	memset(&host_entry, 0, sizeof(host_entry));
	memset(&service_entry, 0, sizeof(service_entry));
#endif

	lookup(proto_name);
}

Address::Address(const Address& addr)
{
#if HAVE_GETADDRINFO
	info = NULL;
#else
	memset(&host_entry, 0, sizeof(host_entry));
	memset(&service_entry, 0, sizeof(service_entry));
#endif

	*this = addr;
}

Address::~Address()
{
#if HAVE_GETADDRINFO
	if (info) {
		if (!copied)
			freeaddrinfo(info);
		else
			free_addrinfo(info);
	}
#else
	free_hostent(&host_entry);
	free_servent(&service_entry);
#endif
}

Address& Address::operator=(const Address& rhs)
{
	if (this == &rhs)
		return *this;

	node = rhs.node;
	service = rhs.service;

#if HAVE_GETADDRINFO
	if (info) {
		if (!copied)
			freeaddrinfo(info);
		else
			free_addrinfo(info);
	}
	copy_addrinfo(&info, rhs.info);
#else
	free_hostent(&host_entry);
	free_servent(&service_entry);
	copy_hostent(&host_entry, &rhs.host_entry);
	copy_servent(&service_entry, &rhs.service_entry);

	addr.ai_protocol = rhs.addr.ai_protocol;
	addr.ai_socktype = rhs.addr.ai_socktype;
#endif

	copied = true;
	return *this;
}

void Address::lookup(const char* proto_name)
{
	int proto;
	if (!strcasecmp(proto_name, "tcp"))
		proto = IPPROTO_TCP;
	else if (!strcasecmp(proto_name, "udp"))
		proto = IPPROTO_UDP;
	else
		throw SocketException("Bad protocol name");

#if HAVE_GETADDRINFO
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
#  ifdef AI_ADDRCONFIG
	hints.ai_flags = AI_ADDRCONFIG;
#  endif
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = (proto == IPPROTO_TCP ? SOCK_STREAM : SOCK_DGRAM);

	if (service.find_first_not_of("0123456789") == string::npos)
		hints.ai_flags |= AI_NUMERICSERV;

	int r;
	if ((r = getaddrinfo(node.empty() ? NULL : node.c_str(), service.c_str(), &hints, &info)) < 0)
		throw SocketException(r, "getaddrinfo");

#else // use gethostbyname etc.
	memset(&host_entry, 0, sizeof(host_entry));
	memset(&service_entry, 0, sizeof(service_entry));

	if (node.empty())
		node = "0.0.0.0";
	struct hostent* hp;
	if ((hp = gethostbyname(node.c_str())) == NULL)
		throw SocketException(hstrerror(HOST_NOT_FOUND));
	copy_hostent(&host_entry, hp);

	int port;
	struct servent* sp;
	if ((sp = getservbyname(service.c_str(), NULL)) == NULL) {
		// if a service name string could not be looked up by name, it must be numeric
		if (service.find_first_not_of("0123456789") != string::npos)
			throw SocketException("Unknown service name");
		port = htons(atoi(service.c_str()));
		sp = getservbyport(port, NULL);
	}
	if (!sp)
		service_entry.s_port = port;
	else
		copy_servent(&service_entry, sp);

	memset(&addr, 0, sizeof(addr));
	addr.ai_protocol = proto;
	addr.ai_socktype = (proto == IPPROTO_TCP ? SOCK_STREAM : SOCK_DGRAM);
#endif
}

///
/// Returns the number of addresses available for
/// the node and service
///
size_t Address::size(void) const
{
	size_t n = 0;
#if HAVE_GETADDRINFO
	if (!info)
		return 0;
	for (struct addrinfo* p = info; p; p = p->ai_next)
		n++;
#else
	if (!host_entry.h_addr_list)
		return 0;
	for (char** p = host_entry.h_addr_list; *p; p++)
		n++;
#endif
	return n;
}

///
/// Returns an address from the list of those available
/// for the node and service
///
const addr_info_t* Address::get(size_t n) const
{
#if HAVE_GETADDRINFO
	if (!info)
		return NULL;

	struct addrinfo* p = info;
	for (size_t i = 0; i < n; i++)
		p = p->ai_next;
#  ifndef NDEBUG
	LOG_DEBUG("Found address %s", get_str(p).c_str());
#  endif
	return p;
#else
	if (!host_entry.h_addr_list)
		return NULL;

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr = *(struct in_addr*)host_entry.h_addr_list[n];
	saddr.sin_port = service_entry.s_port;

	addr.ai_family = saddr.sin_family;
	addr.ai_addrlen = sizeof(saddr);
	addr.ai_addr = (struct sockaddr*)&saddr;
#  ifndef NDEBUG
	LOG_DEBUG("Found address %s", get_str(&addr).c_str());
#  endif
	return &addr;
#endif
}

///
/// Returns the string representation of an address
///
string Address::get_str(const addr_info_t* addr)
{
	if (!addr)
		return "";

#if HAVE_GETADDRINFO
	char host[NI_MAXHOST], port[NI_MAXSERV];
	memset(host, 0, sizeof(host));
	if (getnameinfo(addr->ai_addr, sizeof(struct sockaddr_storage),
			host, sizeof(host), port, sizeof(port),
			NI_NUMERICHOST | NI_NUMERICSERV) == 0)
		return string("[").append(host).append("]:").append(port);
	else
		return "";
#else
	char* host, port[8];
	host = inet_ntoa(((struct sockaddr_in*)addr->ai_addr)->sin_addr);
	snprintf(port, sizeof(port), "%u", htons(((struct sockaddr_in*)addr->ai_addr)->sin_port));
	return string("[").append(host).append("]:").append(port);
#endif
}

//
// Socket class
//

/// Constructs a Socket object and associates the address addr with it.
/// This address will be used by subsequent calls to the bind() or connect()
/// methods
///
/// @param addr An Address object
///
Socket::Socket(const Address& addr)
{
	buffer = new char[BUFSIZ];
	memset(&timeout, 0, sizeof(timeout));
	anum = 0;
	nonblocking = false;
	autoclose = true;
	open(addr);
}

/// Constructs a Socket object from a file descriptor
///
/// @param fd A file descriptor
///
Socket::Socket(int fd)
	: sockfd(fd)
{
	buffer = new char[BUFSIZ];
	anum = 0;
	memset(&timeout, 0, sizeof(timeout));

	if (sockfd == -1)
		return;

#ifndef __MINGW32__
	int r = fcntl(sockfd, F_GETFL);
	if (r == -1)
		throw SocketException(errno, "fcntl");
	nonblocking = r & O_NONBLOCK;
#else
	// no way to retrieve nonblocking status on woe32(?!)
	set_nonblocking(false);
#endif
	autoclose = true;
}

///
/// Constructs a Socket object by copying another instance
///
Socket::Socket(const Socket& s)
	: sockfd(s.sockfd), address(s.address), anum(s.anum),
	  nonblocking(s.nonblocking), autoclose(true)
{
	buffer = new char[BUFSIZ];
	ainfo = address.get(anum);
	memcpy(&timeout, &s.timeout, sizeof(timeout));
	s.set_autoclose(false);
}

Socket::~Socket()
{
	delete [] buffer;
	if (autoclose)
		close();
}

Socket& Socket::operator=(const Socket& rhs)
{
	if (this == &rhs)
		return *this;

	sockfd = rhs.sockfd;
	address = rhs.address;
	anum = rhs.anum;
	ainfo = address.get(anum);
	memcpy(&timeout, &rhs.timeout, sizeof(timeout));
	nonblocking = rhs.nonblocking;
	autoclose = rhs.autoclose;

	rhs.set_autoclose(false);

	return *this;
}

///
/// Associates the Socket with an address
///
/// This address will be used by subsequent calls to the bind() or connect
/// methods.
///
/// @params addr An address object
///
void Socket::open(const Address& addr)
{
	address = addr;
	size_t n = address.size();

	for (anum = 0; anum < n; anum++) {
		ainfo = address.get(anum);
#ifndef NDEBUG
		LOG_DEBUG("Trying %s", address.get_str(ainfo).c_str());
#endif

		if ((sockfd = socket(ainfo->ai_family, ainfo->ai_socktype, ainfo->ai_protocol)) != -1)
			break;
	}
	if (sockfd == -1)
		throw SocketException(errno, "socket");
	set_close_on_exec(true);
}

///
/// Shuts down the socket
///
void Socket::close(void)
{
	::close(sockfd);
}

///
/// Waits for the socket file descriptor to become ready for I/O
///
/// @params dir Specifies the I/O direction. 0 is input, 1 is output.
///
/// @return True if the file descriptor became ready within the timeout
///         period, false otherwise. @see Socket::set_timeout
bool Socket::wait(int dir)
{
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET((unsigned)sockfd, &fdset);
	struct timeval t = { timeout.tv_sec, timeout.tv_usec };

	int r;
	if (dir == 0)
		r = select(sockfd + 1, &fdset, NULL, NULL, &t);
	else if (dir == 1)
		r = select(sockfd + 1, NULL, &fdset, NULL, &t);
	else
		throw SocketException(EINVAL, "Socket::wait");
	if (r == -1)
		throw SocketException(errno, "select");

	return r;
}

///
/// Binds the socket to the address associated with the object
/// @see Socket::open
///
void Socket::bind(void)
{
	int r = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&r, sizeof(r)) == -1)
#ifndef NDEBUG
		perror("setsockopt SO_REUSEADDR");
#else
		;
#endif
	if (::bind(sockfd, ainfo->ai_addr, ainfo->ai_addrlen) == -1)
		throw SocketException(errno, "bind");
}

///
/// Calls listen(2) on the socket file desriptor
///
/// The socket must already have been bound to an address via a call to the bind
/// method.
///
/// @params backlog The maximum number of pending connections (default SOMAXCONN)
///
void Socket::listen(int backlog)
{
	if (::listen(sockfd, backlog) == -1)
		throw SocketException(errno, "listen");
}

///
/// Accepts a connection
///
/// The socket must already have been bound to an address via a call to the bind
/// method.
///
/// @return A Socket instance for the accepted connection
///
Socket Socket::accept(void)
{
	listen();

	// wait for fd to become readable
	if (nonblocking && (timeout.tv_sec > 0 || timeout.tv_usec > 0))
		if (!wait(0))
			throw SocketException(ETIMEDOUT, "select");

	int r;
	if ((r = ::accept(sockfd, NULL, 0)) == -1)
		throw SocketException(errno, "accept");
	set_close_on_exec(true, r);

	return Socket(r);
}

///
/// Accepts a single connection and then closes the listening socket
/// @see Socket::accept
///
/// @return A Socket instance for the accepted connection
///
Socket Socket::accept1(void)
{
	bind();
	Socket s = accept();
	close();
	s.set_close_on_exec(true);

	return s;
}

///
/// Connects the socket to the address that is associated with the object
///
void Socket::connect(void)
{
#ifndef NDEBUG
	LOG_DEBUG("Connecting to %s", address.get_str(ainfo).c_str());
#endif
	if (::connect(sockfd, ainfo->ai_addr, ainfo->ai_addrlen) == -1)
		throw SocketException(errno, "connect");
}

///
/// Connects the socket to an address
///
/// @param addr The address to connect to
///
void Socket::connect(const Address& addr)
{
	close();
	open(addr);
	connect();
}

///
/// Sends a buffer
///
/// @param buf
/// @param len
///
/// @return The amount of data that was sent. This may be less than len
///         if the socket is non-blocking.
///
size_t Socket::send(const void* buf, size_t len)
{
	// if we have a nonblocking socket and a nonzero timeout,
	// wait for fd to become writeable
	if (nonblocking && (timeout.tv_sec > 0 || timeout.tv_usec > 0))
		if (!wait(1))
			return 0;

	size_t nToWrite = len;
	int r = 0;
	const char *sp = (const char *)buf;

	while ( nToWrite > 0) {
		try {
#if defined(__WIN32__)
		r = ::send(sockfd, sp, nToWrite, 0);
#else
		r = ::write(sockfd, sp, nToWrite);
#endif
		}
		catch (...) {
			throw;
		}
		if (r > 0) {
			sp += r;
			nToWrite -= r;
		} else {
			if (r == 0) {
				shutdown(sockfd, SHUT_WR);
				throw SocketException(errno, "send");
			} else if (r == -1) {
				if (errno != EAGAIN)
					throw SocketException(errno, "send");
				r = 0;
			}
		}
	}
	return r;

}

///
/// Sends a string
///
/// @param buf
///
/// @return The amount of data that was sent. This may be less than len
///         if the socket is non-blocking.
///
size_t Socket::send(const string& buf)
{
	return send(buf.data(), buf.length());
}

///
/// Receives data into a buffer
///
/// @arg buf
/// @arg len The maximum number of bytes to write to buf.
///
/// @return The amount of data that was received. This may be less than len
///         if the socket is non-blocking.
size_t Socket::recv(void* buf, size_t len)
{
	// if we have a nonblocking socket and a nonzero timeout,
	// wait for fd to become writeable
	if (nonblocking && (timeout.tv_sec > 0 || timeout.tv_usec > 0))
		if (!wait(0))
			return 0;

	int r = 0;
	try {
		r = ::recv(sockfd, (char*)buf, len, 0);
		if (r == 0)
			shutdown(sockfd, SHUT_RD);
		else if (r == -1) {
			if (errno != EAGAIN)
				throw SocketException(errno, "recv");
			r = 0;
		}
	}
	catch (...) {
		throw;
	}

	return r;
}

///
/// Receives all available data and appends it to a string.
///
/// @arg buf
///
/// @return The amount of data that was received.
///
size_t Socket::recv(string& buf)
{
	size_t n = 0;
	ssize_t r;
	try {
		while ((r = recv(buffer, BUFSIZ)) > 0) {
			buf.reserve(buf.length() + r);
			buf.append(buffer, r);
			n += r;
		}
	} catch (...) {
		throw;
	}

	return n;
}

///
/// Retrieves the socket's receive or send buffer size
///
/// @param dir Specifies the I/O direction. 0 is input, 1 is output.
///
int Socket::get_bufsize(int dir)
{
	int len;
	if (::get_bufsize(sockfd, dir, &len) == -1)
		throw SocketException(errno, "get_bufsize");
	return len;
}

///
/// Sets the socket's receive or send buffer size
///
/// @param dir Specifies the I/O direction. 0 is input, 1 is output.
/// @param len Specifies the new buffer size
///
void Socket::set_bufsize(int dir, int len)
{
	if (::set_bufsize(sockfd, dir, len) == -1)
		throw SocketException(errno, "set_bufsize");
}

///
/// Sets the socket's blocking mode
///
/// @param v If true, the socket is set to non-blocking
///
void Socket::set_nonblocking(bool v)
{
	if (set_nonblock(sockfd, v) == -1)
		throw SocketException(errno, "set_nonblock");
	nonblocking = v;
}

///
/// Enables the use of Nagle's algorithm for the socket
///
/// @param v If true, Nagle's algorithm is disabled.
///
void Socket::set_nodelay(bool v)
{
	if (::set_nodelay(sockfd, v) == -1)
		throw SocketException(errno, "set_nodelay");
}

///
/// Sets the timeout associated with non-blocking operations
///
/// @param t
///
void Socket::set_timeout(const struct timeval& t)
{
	timeout.tv_sec = t.tv_sec;
	timeout.tv_usec = t.tv_usec;
}
void Socket::set_timeout(double t)
{
	timeout.tv_sec = (time_t)floor(t);
	timeout.tv_usec = (suseconds_t)((t - timeout.tv_sec) * 1e6);
}

///
/// Sets the socket's autoclose mode.
///
/// If autoclose is disabled, the socket file descriptor will not be closed when
/// the Socket object is destructed.
///
/// @param v If true, the socket will be closed by the destructor
///
void Socket::set_autoclose(bool v) const
{
	autoclose = v;
}

///
/// Sets the socket's close-on-exec flag
///
void Socket::set_close_on_exec(bool v, int fd)
{
	if (fd == -1)
		fd = sockfd;
	if (set_cloexec(fd, v) == -1)
		throw SocketException(errno, "set_cloexec");
}

///
/// Returns the Socket's file descriptor.
///
/// The descriptor should only be used for reading and writing.
///
/// @return the socket file descriptor
///
int Socket::fd(void)
{
	return sockfd;
}
