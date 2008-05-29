// Implementation of the ServerSocket class

#include "ServerSocket.h"
#include "SocketException.h"

using namespace std;

char* ServerSocket::errorStr() {
	switch (errnum) {
		case 1 : return szERRSOCKET_CREATE;
		case 2 : return szERRSOCKET_CONNECT;
		case 3 : return szERRSOCKET_WRITE;
		case 4 : return szERRSOCKET_READ;
		case 5 : return szERRSOCKET_BIND;
		case 6 : return szERRSOCKET_LISTEN;
		case 7 : return szERRSOCKET_ACCEPT;
	}
	return "";
}	
		
ServerSocket::ServerSocket ( int port )
{
	if ( ! Socket::create() || Socket::error)
		errnum = 1;
    else if (! Socket::bind ( port ) )
		errnum = 5;
	else if ( ! Socket::listen() )
		errnum = 6;
	else
		errnum = 0;
}

ServerSocket::~ServerSocket()
{
}


//const ServerSocket& ServerSocket::operator << ( const string& s ) const
//{
//	Socket::send ( s );
//	return *this;
//}


//const ServerSocket& ServerSocket::operator >> ( string& s ) const
//{
//	Socket::recv ( s );
//	return *this;
//}

void ServerSocket::accept ( ServerSocket& sock )
{
	if ( ! Socket::accept ( sock ) )
		errnum = 7;
}

bool ServerSocket::send(string buff)
{
	return(Socket::send(buff));
}

bool ServerSocket::receive(string buff)
{
	return Socket::recv(buff);
}

bool ServerSocket::putChar(char c)
{
	errnum = 0;
	if (!Socket::putch(c)) {
		errnum = 3;
		return false;
	}
	return true;
}

char ServerSocket::getChar()
{
	char c;
	errnum = 0;
	c = Socket::getch();
	if (Socket::error)
		errnum = 4;
	return c;
}

unsigned char ServerSocket::getByte()
{
	unsigned char c;
	errnum = 0;
	c = Socket::getbyte();
	if (Socket::error)
		errnum = 4;
	return c;
}
