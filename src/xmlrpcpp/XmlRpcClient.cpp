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

#include <config.h>

#include <sys/types.h>
#include <unistd.h>

#include "XmlRpcClient.h"

#include "XmlRpcSocket.h"
#include "XmlRpc.h"

#include "XmlRpcBase64.h"   // For HTTP authentication encoding

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <string>

using namespace XmlRpc;
using namespace std;

// Static data
const char REQUEST_BEGIN[] = 
  "<?xml version=\"1.0\"?>\r\n";
const char REQUEST_BEGIN_METHODNAME[] =
  "<methodCall><methodName>";
const char REQUEST_END_METHODNAME[] = "</methodName>\r\n";

const char PARAMS_TAG[] = "<params>";
const char PARAMS_ETAG[] = "</params>";
const char PARAM_TAG[] = "<param>";
const char PARAM_ETAG[] =  "</param>";
const char REQUEST_END[] = "</methodCall>\r\n";

XmlRpcClient::XmlRpcClient(const char* host, int port, const char* uri/*=0*/)
{
  XmlRpcUtil::log(1, "XmlRpcClient new client: host %s, port %d.", host, port);

  _host = host;
  _port = port;
  if (uri && *uri)
    _uri = uri;
  else
    _uri = "/RPC2";
  _connectionState = NO_CONNECTION;
  _executing = false;
  _eof = false;

  // Default to keeping the connection open until an explicit close is done
  setKeepOpen();
}


XmlRpcClient::XmlRpcClient(const char* host, int port, 
                           const char* login, const char* password, const char* uri/*=0*/)
{
  XmlRpcUtil::log(1, "XmlRpcClient new client: host %s, port %d, login %s.", host, port, login);

  _host = host;
  _port = port;

  _login = login ? login : "";
  _password = password ? password : "";

  _uri = uri ? uri : "/RPC2";

  _connectionState = NO_CONNECTION;
  _executing = false;
  _eof = false;

  // Default to keeping the connection open until an explicit close is done
  setKeepOpen();
}



XmlRpcClient::~XmlRpcClient()
{
  XmlRpcUtil::log(1, "XmlRpcClient dtor client: host %s, port %d.", _host.c_str(), _port);
  if (_connectionState != NO_CONNECTION) close();
}


// Close the owned fd
void 
XmlRpcClient::close()
{
  XmlRpcUtil::log(4, "XmlRpcClient::close: fd %d.", getfd());
  _connectionState = NO_CONNECTION;
  _disp.exit();
  _disp.removeSource(this);

  XmlRpcSource::close();
}


// Clear the referenced flag even if exceptions or errors occur.
struct ClearFlagOnExit {
  ClearFlagOnExit(bool& flag) : _flag(flag) {}
  ~ClearFlagOnExit() { _flag = false; }
  bool& _flag;
};

// Execute the named procedure on the remote server.
// Params should be an array of the arguments for the method.
// Returns true if the request was sent and a result received (although the result
// might be a fault).
bool 
XmlRpcClient::execute(const char* method, XmlRpcValue const& params, XmlRpcValue& result, double timeoutSeconds)
{
  XmlRpcUtil::log(1, "XmlRpcClient::execute: method %s (_connectionState %d).", method, _connectionState);

  // This is not a thread-safe operation, if you want to do multithreading, use separate
  // clients for each thread. If you want to protect yourself from multiple threads
  // accessing the same client, replace this code with a real mutex.
  if (_executing)
    return false;

  _executing = true;
  ClearFlagOnExit cf(_executing);

  _sendAttempts = 0;
  _isFault = false;

  if ( ! setupConnection())
    return false;

  if ( ! generateRequest(method, params))
    return false;

  result.clear();

  // Process until either a response is received or the timeout period passes
  _disp.work(timeoutSeconds);

  if (_connectionState != IDLE || ! parseResponse(result))
    return false;

  XmlRpcUtil::log(1, "XmlRpcClient::execute: method %s completed.", method);
  _response.clear();
  return true;
}

// XmlRpcSource interface implementation
// Handle server responses. Called by the event dispatcher during execute.
unsigned
XmlRpcClient::handleEvent(unsigned eventType)
{
  if (eventType == XmlRpcDispatch::Exception)
  {
    //if (XmlRpcSocket::nonFatalError())
    //  return (_connectionState == WRITE_REQUEST) 
    //        ? XmlRpcDispatch::WritableEvent : XmlRpcDispatch::ReadableEvent;

    if (_connectionState == WRITE_REQUEST && _bytesWritten == 0)
      XmlRpcUtil::error("Error in XmlRpcClient::handleEvent: could not connect to server (%s).", 
                       XmlRpcSocket::getErrorMsg().c_str());
    else
      XmlRpcUtil::error("Error in XmlRpcClient::handleEvent (state %d): %s.", 
                        _connectionState, XmlRpcSocket::getErrorMsg().c_str());
    return 0;
  }

  if (_connectionState == WRITE_REQUEST)
    if ( ! writeRequest()) return 0;

  if (_connectionState == READ_HEADER)
    if ( ! readHeader()) return 0;

  if (_connectionState == READ_RESPONSE)
    if ( ! readResponse()) return 0;

  // This should probably always ask for Exception events too
  return (_connectionState == WRITE_REQUEST) 
        ? XmlRpcDispatch::WritableEvent : XmlRpcDispatch::ReadableEvent;
}


// Create the socket connection to the server if necessary
bool 
XmlRpcClient::setupConnection()
{
  // If an error occurred last time through, or if the server closed the connection, close our end
  if ((_connectionState != NO_CONNECTION && _connectionState != IDLE) || _eof)
    close();

  _eof = false;
  if (_connectionState == NO_CONNECTION)
    if (! doConnect()) 
      return false;

  // Prepare to write the request
  _connectionState = WRITE_REQUEST;
  _bytesWritten = 0;

  // Notify the dispatcher to listen on this source (calls handleEvent when the socket is writable)
  _disp.removeSource(this);       // Make sure nothing is left over
  _disp.addSource(this, XmlRpcDispatch::WritableEvent | XmlRpcDispatch::Exception);

  return true;
}


// Connect to the xmlrpc server
bool 
XmlRpcClient::doConnect()
{
  XmlRpcSocket::Socket fd = XmlRpcSocket::socket();
  if (fd == XmlRpcSocket::Invalid)
  {
    XmlRpcUtil::error("Error in XmlRpcClient::doConnect: Could not create socket (%s).", XmlRpcSocket::getErrorMsg().c_str());
    return false;
  }

  XmlRpcUtil::log(3, "XmlRpcClient::doConnect: fd %d.", fd);
  this->setfd(fd);

  // Don't block on connect/reads/writes
  if ( ! XmlRpcSocket::setNonBlocking(fd))
  {
    this->close();
    XmlRpcUtil::error("Error in XmlRpcClient::doConnect: Could not set socket to non-blocking IO mode (%s).", XmlRpcSocket::getErrorMsg().c_str());
    return false;
  }

  if ( ! XmlRpcSocket::connect(fd, _host, _port))
  {
    this->close();
    XmlRpcUtil::error("Error in XmlRpcClient::doConnect: Could not connect to server (%s).", XmlRpcSocket::getErrorMsg().c_str());
    return false;
  }

  return XmlRpcSource::doConnect();
}

// Encode the request to call the specified method with the specified parameters into xml

std::string XmlRpc::pname = "N/A";

void 
XmlRpc::set_pname(std::string pn)
{
  XmlRpc::pname = pn;
};

bool 
XmlRpcClient::generateRequest(const char* methodName, XmlRpcValue const& params)
{
  std::string body = REQUEST_BEGIN;
  if (XmlRpc::pname != "N/A") {
    char pid[100];
    snprintf(pid, sizeof(pid), "%s %d", XmlRpc::pname.c_str(), getpid());
    body.append("<?clientid=\"").append(pid).append("\"?>\r\n");
  }
  body.append(REQUEST_BEGIN_METHODNAME);
  body.append(methodName);
  body.append(REQUEST_END_METHODNAME);

  // If params is an array, each element is a separate parameter
  if (params.valid()) {
    body += PARAMS_TAG;
    if (params.getType() == XmlRpcValue::TypeArray)
    {
      for (int i=0; i<params.size(); ++i) {
        body += PARAM_TAG;
        body += params[i].toXml();
        body += PARAM_ETAG;
      }
    }
    else
    {
      body += PARAM_TAG;
      body += params.toXml();
      body += PARAM_ETAG;
    }
      
    body += PARAMS_ETAG;
  }
  body += REQUEST_END;

  std::string header = generateHeader(body);
  XmlRpcUtil::log(4, "XmlRpcClient::generateRequest: header is %d bytes, content-length is %d.", 
                  header.length(), body.length());

  _request = header + body;
  return true;
}

// Prepend http headers
std::string
XmlRpcClient::generateHeader(std::string const& body)
{
  std::string header = 
    "POST " + _uri + " HTTP/1.1\r\n"
    "User-Agent: ";
  header += XMLRPC_VERSION;
  header += "\r\nHost: ";
  header += _host;

  char buff[40];
  sprintf(buff,":%d\r\n", _port);

  header += buff;

  if (_login.length() != 0)
  {
    // convert to base64
    std::vector<char> base64data;
    int iostatus = 0;
    xmlrpc_base64<char> encoder;
    std::back_insert_iterator<std::vector<char> > ins =
      std::back_inserter(base64data);

    std::string authBuf = _login + ":" + _password;

    encoder.put(authBuf.begin(), authBuf.end(), ins, iostatus,
                xmlrpc_base64<>::crlf());

    header += "Authorization: Basic ";
    std::string authEnc(base64data.begin(), base64data.end());
    // handle pesky linefeed characters
    string::size_type lf;
    while ( (lf = authEnc.find("\r")) != string::npos ) {
      authEnc.erase(lf, 1);
    }
    while ( (lf = authEnc.find("\n")) != string::npos ) {
      authEnc.erase(lf, 1);
    }
    header += authEnc;
    header += "\r\n";
  }

  header += "Content-Type: text/xml\r\nContent-length: ";

  sprintf(buff,"%d\r\n\r\n", static_cast<int>(body.size()));

  return header + buff;
}

bool 
XmlRpcClient::writeRequest()
{
  if (_bytesWritten == 0)
    XmlRpcUtil::log(5, "XmlRpcClient::writeRequest (attempt %d):\n%s\n", _sendAttempts+1, _request.c_str());

  // Try to write the request
  if ( ! nbWrite(_request, &_bytesWritten))
  {
    XmlRpcUtil::error("Error in XmlRpcClient::writeRequest: write error (%s).",XmlRpcSocket::getErrorMsg().c_str());
    return false;
  }
    
  XmlRpcUtil::log(3, "XmlRpcClient::writeRequest: wrote %d of %d bytes.", _bytesWritten, _request.length());

  // Wait for the result
  if (_bytesWritten == int(_request.length()))
  {
    _header = "";
    _response = "";
    _connectionState = READ_HEADER;
  }
  return true;
}


// Read the header from the response
bool 
XmlRpcClient::readHeader()
{
  // Read available data
  if ( ! nbRead(_header, &_eof) || (_eof && _header.length() == 0))
  {
    // If we haven't read any data yet and this is a keep-alive connection, the server may
    // have timed out, so we try one more time.
    if (getKeepOpen() && _header.length() == 0 && _sendAttempts++ == 0)
    {
      XmlRpcUtil::log(4, "XmlRpcClient::readHeader: re-trying connection");
      XmlRpcSource::close();

      _connectionState = NO_CONNECTION;
      _eof = false;
      return setupConnection();
    }

    XmlRpcUtil::error("Error in XmlRpcClient::readHeader: error while reading header (%s) on fd %d.",
                      XmlRpcSocket::getErrorMsg().c_str(), getfd());
    return false;
  }

  XmlRpcUtil::log(4, "XmlRpcClient::readHeader: client has read %d bytes", _header.length());

  return parseHeader();
}

bool 
XmlRpcClient::parseHeader()
{
  char const *hp = _header.c_str();         // Start of header
  char const *ep = hp + _header.length();   // End of string
  char const *bp = 0;                       // Start of body
  char const *lp = 0;                       // Start of content-length value

  std::string const CONTINUE100("100 Continue");
  int nc100 = int(CONTINUE100.length());
  for (char const *cp = hp; (bp == 0) && (cp < ep); ++cp)
  {
    if ((ep - cp > 16) && (strncasecmp(cp, "Content-length: ", 16) == 0))
    {
      lp = cp + 16;
    }
    else if ((ep - cp > 4) && (strncmp(cp, "\r\n\r\n", 4) == 0))
    {
      if (cp - hp > nc100 && strncmp(cp-CONTINUE100.length(), CONTINUE100.c_str(), CONTINUE100.length()) == 0)
        cp += 3;
      else
        bp = cp + 4;
    }
    else if ((ep - cp > 2) && (strncmp(cp, "\n\n", 2) == 0))
    {
      if (cp - hp > nc100 && strncmp(cp-CONTINUE100.length(), CONTINUE100.c_str(), CONTINUE100.length()) == 0)
        ++ cp;
      else
        bp = cp + 2;
    }
  }

  // If we haven't gotten the entire header yet, return (keep reading)
  if (bp == 0)
  {
    if (_eof)          // EOF in the middle of a response is an error
    {
      XmlRpcUtil::error("Error in XmlRpcClient::readHeader: EOF while reading header");
      return false;   // Close the connection
    }
    
    return true;  // Keep reading
  }

  // Decode content length
  if (lp == 0)
  {
    XmlRpcUtil::error("Error XmlRpcClient::readHeader: No Content-length specified");
    return false;   // We could try to figure it out by parsing as we read, but for now...
  }

  _contentLength = atoi(lp);
  if (_contentLength <= 0)
  {
    XmlRpcUtil::error("Error in XmlRpcClient::readHeader: Invalid Content-length specified (%d).", _contentLength);
    return false;
  }
  	
  XmlRpcUtil::log(4, "client read content length: %d", _contentLength);

  // Otherwise copy non-header data to response buffer and set state to read response.
  _response = bp;
  _header = "";   // should parse out any interesting bits from the header (connection, etc)...
  _connectionState = READ_RESPONSE;
  return true;    // Continue monitoring this source
}

    
bool 
XmlRpcClient::readResponse()
{
  // If we dont have the entire response yet, read available data
  if (int(_response.length()) < _contentLength)
  {
    if ( ! nbRead(_response, &_eof))
    {
      XmlRpcUtil::error("Error in XmlRpcClient::readResponse: read error (%s).",XmlRpcSocket::getErrorMsg().c_str());
      return false;
    }

    // If we haven't gotten the entire _response yet, return (keep reading)
    if (int(_response.length()) < _contentLength)
    {
      if (_eof)
      {
        XmlRpcUtil::error("Error in XmlRpcClient::readResponse: EOF while reading response");
        return false;
      }
      return true;
    }
  }

  // Otherwise, parse and return the result
  XmlRpcUtil::log(3, "XmlRpcClient::readResponse (read %d bytes)", _response.length());
  XmlRpcUtil::log(5, "response:\n%s", _response.c_str());

  _connectionState = IDLE;

  return false;    // Stop monitoring this source (causes return from work)
}


// Convert the response xml into a result value
bool 
XmlRpcClient::parseResponse(XmlRpcValue& result)
{
  std::string r;
  _response.swap(r);

  // Parse response xml into result
  bool emptyParam;
  int offset = 0;
  if ( ! XmlRpcUtil::findTag("methodResponse",r,&offset,&emptyParam) || emptyParam)
  {
    XmlRpcUtil::error("Error in XmlRpcClient::parseResponse: Invalid response - no methodResponse. Response:\n%s", r.c_str());
    return false;
  }

  // Expect either <params><param>... or <fault>...
  if (XmlRpcUtil::nextTagIs("params",r,&offset,&emptyParam) &&
      XmlRpcUtil::nextTagIs("param",r,&offset,&emptyParam))
  {
    if (emptyParam)
    {
      result = 0; // No result?
    }
    else if (  ! result.fromXml(r, &offset))
    {
      XmlRpcUtil::error("Error in XmlRpcClient::parseResponse: Invalid response value. Response:\n%s", r.c_str());
      return false;
    }
  }
  else if (XmlRpcUtil::nextTagIs("fault",r,&offset,&emptyParam))
  {
    _isFault = true;

    if (emptyParam || ! result.fromXml(r, &offset))
    {
      result = 0; // No result?
      return false;
    }
  }
  else
  {
    XmlRpcUtil::error("Error in XmlRpcClient::parseResponse: Invalid response - no param or fault tag. Response:\n%s", r.c_str());
    return false;
  }
      
  return result.valid();
}

