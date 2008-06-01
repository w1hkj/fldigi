// Definition of the ClientSocket class

#ifndef ClientSocket_class
#define ClientSocket_class

#include "Socket.h"
#include <string>

using namespace std;

class ClientSocket : public Socket
{
 public:
	ClientSocket ( string host, int port);
	virtual ~ClientSocket(){}
  
//	const ClientSocket& operator << ( const string& ) const;
//	const ClientSocket& operator >> ( string& ) const;

	bool receive(string buff);
	bool send (string s);
	bool putChar(char c);
	char getChar();
	unsigned char getByte();
	int error() { return errnum;}
	char* errorStr();
	int errnum;

};


#endif
