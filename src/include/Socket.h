// Definition of the Socket class

#ifndef Socket_class
#define Socket_class


#include <sys/types.h>
#ifdef WIN32
	#include <winsock.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET -1
#endif
#include <unistd.h>
#include <string>

using namespace std;

#define szERRSOCKET_CREATE	"Socket Err: create"
#define szERRSOCKET_CONNECT	"Socket Err: connect"
#define szERRSOCKET_BIND	"Socket Err: bind"
#define szERRSOCKET_WRITE	"Socket Err: write"
#define szERRSOCKET_READ	"Socket Err: read"
#define szERRSOCKET_LISTEN	"Socket Err: listen"
#define szERRSOCKET_ACCEPT	"Socket Err: accept"

const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 5;
const int MAXRECV = 500;

class Socket
{
 public:
  Socket();
  virtual ~Socket();
  
  void close();

  // Server initialization
  bool create();
  bool bind ( const int port );
  bool listen() const;
  bool accept ( Socket& ) const;

  // Client initialization
  bool connect ( string host, const int port );

  // Data Transimission
  bool send ( const string) const;
  
  size_t recv ( string & );
  
  bool putch (char c);
  char getch();
  unsigned char getbyte();

  void set_non_blocking ( const bool );

  bool is_valid() const { return m_sock != INVALID_SOCKET; }
  bool error;
  char rcvbuf [ MAXRECV + 1 ];
	
 private:

  SOCKET m_sock;
  sockaddr_in m_addr;
  char sbuff[1024];
  int  ssize;
  int  sptr;
  fd_set inputs, testfds;
  struct timeval timeout;

};


#endif
