// ----------------------------------------------------------------------------
//
// XmlRpc++ Copyright (c) 2002-2008 by Chris Morley
//
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#if defined(XMLRPC_THREADS)

#include "XmlRpcThreadedServer.h"
//#include "XmlRpcServerConnection.h"

#include <config.h>

using namespace XmlRpc;

// executeRequestThreaded:
//  remove the serverConnection from the dispatcher (but don't close the socket)
//  push the request onto the request queue 
//   (acquire the mutex, push_back request, release mutex, incr semaphore)
//  

// worker::run
//  while ! stopped
//    pop a request off the request queue (block on semaphore/decr, acquire mutex, get request, rel)
//    executeRequest (parse, run, generate response)
//    notify the serverConnection that the response is available
//    (the serverConnection needs to add itself back to the dispatcher safely - mutex)

// How do I interrupt the dispatcher if it is waiting in a select call? 
//  i) Replace select with WaitForMultipleObjects, using WSAEventSelect to associate
//     each socket with an event object, and adding an additional "signal" event.
//

#endif // XMLRPC_THREADS
