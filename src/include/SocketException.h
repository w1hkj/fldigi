// SocketException class


#ifndef SocketException_class
#define SocketException_class

#include <string>

using namespace std;

class SocketException
{
 public:
  SocketException ( string s ) : m_s ( s ) {};
  ~SocketException (){};

  string description() { return m_s; }

 private:

  string m_s;

};

#endif
