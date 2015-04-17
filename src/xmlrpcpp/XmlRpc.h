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

#ifndef _XMLRPC_H_
#define _XMLRPC_H_

#if defined(_MSC_VER)
# pragma warning(disable:4786)    // identifier was truncated in debug info
#endif


#include <string>

#include "XmlRpcClient.h"
#include "XmlRpcException.h"
#include "XmlRpcServer.h"
#include "XmlRpcServerMethod.h"
#include "XmlRpcValue.h"
#include "XmlRpcUtil.h"

namespace XmlRpc {


  //! An interface allowing custom handling of error message reporting.
  class XmlRpcErrorHandler {
  public:
  
    XmlRpcErrorHandler() {}
    virtual ~XmlRpcErrorHandler() {}
    
    //! Returns a pointer to the currently installed error handling object.
    static XmlRpcErrorHandler* getErrorHandler() 
    { return _errorHandler; }

    //! Specifies the error handler.
    static void setErrorHandler(XmlRpcErrorHandler* eh)
    { _errorHandler = eh; }

    //! Report an error. Custom error handlers should define this method.
    virtual void error(const char* msg) = 0;

  protected:
    static XmlRpcErrorHandler* _errorHandler;
  };

  //! An interface allowing custom handling of informational message reporting.
  class XmlRpcLogHandler {
  public:
    XmlRpcLogHandler() {}
    virtual ~XmlRpcLogHandler() {}

    //! Returns a pointer to the currently installed message reporting object.
    static XmlRpcLogHandler* getLogHandler() 
    { return _logHandler; }

    //! Specifies the message handler.
    static void setLogHandler(XmlRpcLogHandler* lh)
    { _logHandler = lh; }

    //! Returns the level of verbosity of informational messages. 0 is no output, 5 is very verbose.
    static int getVerbosity() 
    { return _verbosity; }

    //! Specify the level of verbosity of informational messages. 0 is no output, 5 is very verbose.
    static void setVerbosity(int v) 
    { _verbosity = v; }

    //! Output a message. Custom error handlers should define this method.
    virtual void log(int level, const char* msg) = 0;

  protected:
    static XmlRpcLogHandler* _logHandler;
    static int _verbosity;
  };

  //! Returns log message verbosity. This is short for XmlRpcLogHandler::getVerbosity()
  int getVerbosity();
  //! Sets log message verbosity. This is short for XmlRpcLogHandler::setVerbosity(level)
  void setVerbosity(int level);


  //! Version identifier
  extern const char XMLRPC_VERSION[];

} // namespace XmlRpc

#endif // _XMLRPC_H_
