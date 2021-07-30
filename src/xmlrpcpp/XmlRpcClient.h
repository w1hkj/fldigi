// ----------------------------------------------------------------------------
//
// flxmlrpc Copyright (c) 2015 by W1HKJ, Dave Freese <iam_w1hkj@w1hkj.com>
//    
// XmlRpc++ Copyright (c) 2002-2008 by Chris Morley
//
// This file is part of fldigi
//
// flxmlrpc is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef _XMLRPCCLIENT_H_
#define _XMLRPCCLIENT_H_

#if defined(_MSC_VER)
# pragma warning(disable:4786)    // identifier was truncated in debug info
#endif


#include <string>

#include "XmlRpcDispatch.h"
#include "XmlRpcSource.h"

namespace XmlRpc {

  extern std::string pname;
  
  extern void set_pname(std::string pn);

  // Arguments and results are represented by XmlRpcValues
  class XmlRpcValue;

  //! A class to send XML RPC requests to a server and return the results.
  class XmlRpcClient : public XmlRpcSource {
  public:

    //! Construct a client to connect to the server at the specified host:port address
    //!  @param host The name of the remote machine hosting the server, eg "myserver.mycompany.com"
    //!  @param port The port on the remote machine where the server is listening
    //!  @param uri  An optional string to be sent as the URI in the HTTP GET header
    //! Note that the host is not a URL, do not prepend "http://" or other protocol specifiers.
    XmlRpcClient(const char* host, int port, const char* uri=0);

    //! Construct a client to connect to the server at the specified host:port address including HTTP authentication
    //!  @param host  The name of the remote machine hosting the server
    //!  @param port  The port on the remote machine where the server is listening
    //!  @param login The username passed to the server
    //!  @param pass  The password passed to the server
    //!  @param uri   An optional string to be sent as the URI in the HTTP GET header
    XmlRpcClient(const char* host, int port, const char* login, const char* password, const char* uri=0);

    //! Destructor
    virtual ~XmlRpcClient();

    //! Execute the named procedure on the remote server.
    //!  @param method The name of the remote procedure to execute
    //!  @param params An array of the arguments for the method
    //!  @param result The result value to be returned to the client
    //!  @param timeoutSeconds Seconds to wait for a response (defaults to forever)
    //!  @return true if the request was sent and a result received 
    //!   (although the result might be a fault).
    //!
    //! Currently this is a synchronous (blocking) implementation (execute
    //! does not return until it receives a response or an error). Use isFault()
    //! to determine whether the result is a fault response.
    bool execute(const char* method, XmlRpcValue const& params, XmlRpcValue& result, double timeoutSeconds = -1);

    //! Returns true if the result of the last execute() was a fault response.
    bool isFault() const { return _isFault; }

    //! Return the host name of the server
    const char* const host() const { return _host.c_str(); }

    //! Return the port
    int port() const { return _port; }

    //! Return the URI
    const char* const uri() const { return _uri.c_str(); }

    // XmlRpcSource interface implementation
    //! Close the connection
    virtual void close();

    //! Handle server responses. Called by the event dispatcher during execute.
    //!  @param eventType The type of event that occurred. 
    //!  @see XmlRpcDispatch::EventType
    virtual unsigned handleEvent(unsigned eventType);

  protected:
    // Execution processing helpers
    virtual bool doConnect();
    virtual bool setupConnection();

    virtual bool generateRequest(const char* method, XmlRpcValue const& params);
    virtual std::string generateHeader(std::string const& body);
    virtual bool writeRequest();
    virtual bool readHeader();
    virtual bool parseHeader();
    virtual bool readResponse();
    virtual bool parseResponse(XmlRpcValue& result);

    // Possible IO states for the connection
    enum ClientConnectionState { NO_CONNECTION, CONNECTING, WRITE_REQUEST, READ_HEADER, READ_RESPONSE, IDLE };
    ClientConnectionState _connectionState;

    // Server location
    std::string _host;
    std::string _uri;
    int _port;

    // Login information for HTTP authentication
    std::string _login;
    std::string _password;

    // The xml-encoded request, http header of response, and response xml
    std::string _request;
    std::string _header;
    std::string _response;

    // Number of times the client has attempted to send the request
    int _sendAttempts;

    // Number of bytes of the request that have been written to the socket so far
    int _bytesWritten;

    // True if we are currently executing a request. If you want to multithread,
    // each thread should have its own client.
    bool _executing;

    // True if the server closed the connection
    bool _eof;

    // True if a fault response was returned by the server
    bool _isFault;

    // Number of bytes expected in the response body (parsed from response header)
    int _contentLength;

    // Event dispatcher
    XmlRpcDispatch _disp;

  };	// class XmlRpcClient

}	// namespace XmlRpc

#endif	// _XMLRPCCLIENT_H_
