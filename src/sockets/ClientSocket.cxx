// Implementation of the ClientSocket class

#include "ClientSocket.h"
#include "SocketException.h"

using namespace std;

char* ClientSocket::errorStr() {
	switch (errnum) 
	{
		case 1 : return szERRSOCKET_CREATE;
		case 2 : return szERRSOCKET_CONNECT;
		case 3 : return szERRSOCKET_WRITE;
		case 4 : return szERRSOCKET_READ;
		case 5 : return szERRSOCKET_BIND;
	}
	return "";
}	
		
ClientSocket::ClientSocket ( string host, int port )
{
  if ( ! Socket::create() || Socket::error) {
		errnum = 1;
	} else if ( ! Socket::connect ( host, port ) ) {
		errnum = 2;
	}
}

//const ClientSocket& ClientSocket::operator << ( const string & s) const
//{
//	Socket::send(s);
//	return *this;
//}

//const ClientSocket& ClientSocket::operator >> ( string & s) const
//{
//	Socket::recv(s);
//	return *this;
//}

bool ClientSocket::send(string buff)
{
	return(Socket::send(buff));
}


bool ClientSocket::receive(string buff)
{
	return Socket::recv(buff);
}


bool ClientSocket::putChar(char c)
{
	errnum = 0;
	if (!Socket::putch(c)) {
		errnum = 3;
		return false;
	}
	return true;
}

char ClientSocket::getChar()
{
	char c;
	errnum = 0;
	c = Socket::getch();
	if (Socket::error)
		errnum = 4;
	return c;
}

unsigned char ClientSocket::getByte()
{
	unsigned char c;
	errnum = 0;
	c = Socket::getbyte();
	if (Socket::error)
		errnum = 4;
	return c;
}
