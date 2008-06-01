// Implementation of the Socket class.

#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <iostream>

#include "Socket.h"
#include "fl_digi.h"

using namespace std;

static int numsocks = 0;

Socket::Socket() : error(false), m_sock ( INVALID_SOCKET )
{
#ifdef WIN32
// Start Winsock up
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) 
		error = true;
	else
#endif
		numsocks++;
	ssize = 0;
	sptr = 0;
}


Socket::~Socket()
{
	close();
}

void Socket::close()
{
	if (m_sock != INVALID_SOCKET) {
		::close ( m_sock );
		numsocks--;
#ifdef WIN32
// Shut Winsock down to release DLL resources
		if (numsocks == 0)
			WSACleanup();
#endif
	}
}

bool Socket::create()
{
	m_sock = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP	);
	if (m_sock == INVALID_SOCKET) {
		char szError[40];
#ifdef WIN32
		snprintf(szError, sizeof(szError), "Socket error, create(): %ld", WSAGetLastError() );
		put_status(szError, 5);
		WSACleanup();
#else
		snprintf(szError, sizeof(szError), "Socket error, create()");
		put_status(szError, 5);
#endif
		return 1;
	}
	ssize = 0;
	sptr = 0;
	FD_ZERO(&inputs);
	FD_SET(m_sock, &inputs);
	return ( is_valid() );
}

bool Socket::bind ( const int port )
{
	if ( ! is_valid() )
		return false;

	m_addr.sin_family      = AF_INET;
	m_addr.sin_addr.s_addr = INADDR_ANY;
	m_addr.sin_port        = htons ( port );

	int bind_return = ::bind ( m_sock, ( struct sockaddr * ) &m_addr, sizeof ( m_addr ) );

	if ( bind_return == -1 )
		return false;

	return true;
}

bool Socket::listen() const
{
  if ( ! is_valid() )
      return false;

  int listen_return = ::listen ( m_sock, MAXCONNECTIONS );

  if ( listen_return == -1 )
      return false;

  return true;
}

bool Socket::accept ( Socket& new_socket ) const
{
  int addr_length = sizeof ( m_addr );
#ifdef WIN32
  new_socket.m_sock = ::accept ( m_sock, ( sockaddr * ) &m_addr, &addr_length );
#else
  new_socket.m_sock = ::accept ( m_sock, ( sockaddr * ) &m_addr, ( socklen_t * ) &addr_length );
#endif
  if ( new_socket.m_sock <= 0 )
    return false;
  else
    return true;
}

bool Socket::putch(char c)
{
#ifdef WIN32
  int status = ::send (m_sock, &c, 1, 0);
#else
  int status = ::send (m_sock, &c, 1, MSG_NOSIGNAL );
#endif
  if (status == -1)
    return false;
  return true;
}

unsigned char Socket::getbyte()
{
	if (ssize) {
		if (sptr < ssize)
			return sbuff[sptr++];
	}

	ssize = sptr = 0;

	memset(sbuff, 0, 1024);
	error = false;
	timeout.tv_sec = 0;
	timeout.tv_usec = 10000;
	testfds = inputs;

	int result = select(FD_SETSIZE, &testfds, (fd_set *)0, (fd_set *)0, &timeout);
//std::cout << result << " "; std::cout.flush();
	if (result == 0) { // timeout
		return 0;
	}
#ifdef WIN32
	if (result == SOCKET_ERROR) { // select error
//std::cout << WSAGetLastError() << std::endl; std::cout.flush();
		return 0;
	}
#else
	if (result == -1)  // select error
		return 0;	
#endif
	if (FD_ISSET(m_sock, &testfds)) {
		int status = ::recv ( m_sock, sbuff, 1024, 0 );
		if (status > 0) {
//std::cout << "Received " << status << std::endl; std::cout.flush();
			ssize = status;
			sptr = 0;
			return sbuff[sptr++];
		}
		if ( status == -1 )
			error = true;
		return 0;
	}
	return 0;
}

char Socket::getch()
{
	return (char)getbyte();
}

bool Socket::send ( const string s) const
{
//printf("%s",s.c_str());
//std::cout.flush();
#ifdef WIN32
  int status = ::send (m_sock, s.c_str(), s.size(), 0);
#else
  int status = ::send ( m_sock, s.c_str(), s.size(), MSG_NOSIGNAL );
#endif
  if ( status == -1 )
      return false;
  return true;
}

size_t Socket::recv ( string & s)
{
	int c;
	s = "";
	while ( (c = getbyte()) > 0)
		s += (char)c;
	return s.length();
}


bool Socket::connect ( string host, const int port )
{
  if ( ! is_valid() ) 
	  return false;
  unsigned long addr = inet_addr(host.c_str());
  if (addr == INADDR_NONE) {
	hostent* pHostInfo = gethostbyname(host.c_str());
	if (!pHostInfo)
		return false;
	addr = *((unsigned long *)pHostInfo->h_addr_list[0]);
  }
  m_addr.sin_addr.s_addr = addr;
  m_addr.sin_family = AF_INET;
  m_addr.sin_port = htons ( port );
  if (::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) ) == 0)
    return true;
  else
    return false;
}

void Socket::set_non_blocking ( const bool b )
{
#ifndef WIN32
  int opts;
  opts = fcntl ( m_sock, F_GETFL );
  if ( opts < 0 )
      return;
  if ( b )
    opts = ( opts | O_NONBLOCK );
  else
    opts = ( opts & ~O_NONBLOCK );
  fcntl ( m_sock,  F_SETFL,opts );
#endif
  return;
}
