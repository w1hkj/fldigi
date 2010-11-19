
#ifndef _XMLRPCSOURCE_H_
#define _XMLRPCSOURCE_H_
//
// XmlRpc++ Copyright (c) 2002-2008 by Chris Morley
//
#if defined(_MSC_VER)
# pragma warning(disable:4786)    // identifier was truncated in debug info
#endif

#include "XmlRpcSocket.h"


namespace XmlRpc {

  //! Proxy for Ssl data to avoid including headers here.
  struct SslProxy;

  //! An RPC source represents a file descriptor to monitor
  class XmlRpcSource {
  public:
    //! Constructor
    //!  @param fd The socket file descriptor to monitor.
    //!  @param deleteOnClose If true, the object deletes itself when close is called.
    XmlRpcSource(XmlRpcSocket::Socket fd = XmlRpcSocket::Invalid, bool deleteOnClose = false);

    //! Destructor
    virtual ~XmlRpcSource();

    //! Return the file descriptor being monitored.
    XmlRpcSocket::Socket getfd() const { return _fd; }
    //! Specify the file descriptor to monitor.
    void setfd(XmlRpcSocket::Socket fd) { _fd = fd; }

    //! Return whether the file descriptor should be kept open if it is no longer monitored.
    bool getKeepOpen() const { return _keepOpen; }
    //! Specify whether the file descriptor should be kept open if it is no longer monitored.
    void setKeepOpen(bool b=true) { _keepOpen = b; }

    //! Return whether ssl is enabled.
    bool getSslEnabled() const { return _sslEnabled; }
    //! Specify whether to enable ssl. Use getSslEnabled() to verify that Ssl is available.
    void setSslEnabled(bool b=true);

    //! Close the owned fd. If deleteOnClose was specified at construction, the object is deleted.
    virtual void close();

    //! Return true to continue monitoring this source
    virtual unsigned handleEvent(unsigned eventType) = 0;

  protected:

    // Execution processing helpers
    virtual bool doConnect();

    //! Read text from the source. Returns false on error.
    bool nbRead(std::string& s, bool *eof);

    //! Write text to the source. Returns false on error.
    bool nbWrite(std::string const& s, int *bytesSoFar);

  private:

    // Socket. This is an int for linux/unix, and unsigned on win32, and unsigned __int64 on win64.
    // Casting to int/long/unsigned on win64 is a bad idea.
    XmlRpcSocket::Socket _fd;

    // In the server, a new source (XmlRpcServerConnection) is created
    // for each connected client. When each connection is closed, the
    // corresponding source object is deleted.
    bool _deleteOnClose;

    // In the client, keep connections open if you intend to make multiple calls.
    bool _keepOpen;

    // Enable use of SSL
    bool _sslEnabled;

    // SSL data
    SslProxy *_ssl;
  };
} // namespace XmlRpc

#endif //_XMLRPCSOURCE_H_
