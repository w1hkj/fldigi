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

#ifndef _XMLRPCSERVERMETHOD_H_
#define _XMLRPCSERVERMETHOD_H_

#if defined(_MSC_VER)
# pragma warning(disable:4786)    // identifier was truncated in debug info
#endif

#include <string>

namespace XmlRpc {

  // Representation of a parameter or result value
  class XmlRpcValue;

  // The XmlRpcServer processes client requests to call RPCs
  class XmlRpcServer;

  //! Abstract class representing a single RPC method
  class XmlRpcServerMethod {
  public:
    //! Constructor
    XmlRpcServerMethod(std::string const& name, XmlRpcServer* server = 0);
    //! Destructor
    virtual ~XmlRpcServerMethod();

    //! Returns the name of the method
    std::string& name() { return _name; }

    //! Execute the method. Subclasses must provide a definition for this method.
    virtual void execute(XmlRpcValue& params, XmlRpcValue& result) = 0;

    //! Returns a help string for the method.
    //! Subclasses should define this method if introspection is being used.
    virtual std::string help() { return std::string(); }

  protected:
    std::string _name;
    XmlRpcServer* _server;
  };
} // namespace XmlRpc

#endif // _XMLRPCSERVERMETHOD_H_
