// Definition of the ServerSocket class

#ifndef ServerSocket_class
#define ServerSocket_class

#include "Socket.h"

using namespace std;

class ServerSocket : Socket
{
 public:

	ServerSocket ( int port );
	ServerSocket (){};
	virtual ~ServerSocket();

//	const ServerSocket& operator << ( const string& ) const;
//	const ServerSocket& operator >> ( string& ) const;

	void accept ( ServerSocket& );

	bool receive(string buff);
	bool send(string buff);

	bool putChar(char c);
	char getChar();
	unsigned char getByte();
	
	int error() { return errnum;}
	char* errorStr();
	int errnum;

};


#endif
