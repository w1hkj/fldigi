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

#ifndef _XMLRPCSERVERCONNECTION_H_
#define _XMLRPCSERVERCONNECTION_H_

#if defined(_MSC_VER)
# pragma warning(disable:4786)    // identifier was truncated in debug info
#endif

#include <string>

#include "XmlRpcValue.h"
#include "XmlRpcSource.h"

namespace XmlRpc {


  // The server waits for client connections and provides support for executing methods
  class XmlRpcServer;
  class XmlRpcServerMethod;

  //! A class to handle XML RPC requests from a particular client
  class XmlRpcServerConnection : public XmlRpcSource {
  public:

    //! Constructor
    XmlRpcServerConnection(XmlRpcSocket::Socket fd, XmlRpcServer* server, bool deleteOnClose = false);
    //! Destructor
    virtual ~XmlRpcServerConnection();

    // XmlRpcSource interface implementation
    //! Handle IO on the client connection socket.
    //!   @param eventType Type of IO event that occurred. @see XmlRpcDispatch::EventType.
    virtual unsigned handleEvent(unsigned eventType);

  protected:

    //! Reads the http header
    bool readHeader();

    //! Reads the request (based on the content-length header value)
    bool readRequest();

    //! Executes the request and writes the resulting response
    bool writeResponse();


    //! Helper method to execute the client request
    virtual void executeRequest();


    //! The XmlRpc server that accepted this connection
    XmlRpcServer* _server;

    //! Possible IO states for the connection
    enum ServerConnectionState { READ_HEADER, READ_REQUEST, WRITE_RESPONSE };
    //! Current IO state for the connection
    ServerConnectionState _connectionState;

    //! Request headers
    std::string _header;

    //! Number of bytes expected in the request body (parsed from header)
    int _contentLength;

    //! Request body
    std::string _request;

    //! Response
    std::string _response;

    //! Number of bytes of the response written so far
    int _bytesWritten;

    //! Whether to keep the current client connection open for further requests
    bool _keepAlive;
  };
} // namespace XmlRpc

#endif // _XMLRPCSERVERCONNECTION_H_
