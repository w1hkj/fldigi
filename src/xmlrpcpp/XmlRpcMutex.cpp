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

#if defined(XMLRPC_THREADS)

#include <config.h>

#include "XmlRpcMutex.h"

#if defined(_WINDOWS)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# include <pthread.h>
#endif

using namespace XmlRpc;


//! Destructor.
XmlRpcMutex::~XmlRpcMutex()
{
  if (_pMutex)
  {
#if defined(_WINDOWS)
    ::CloseHandle((HANDLE)_pMutex);
#else
    ::pthread_mutex_destroy((pthread_mutex_t*)_pMutex);
    delete _pMutex;
#endif
    _pMutex = 0;
  }
}

//! Wait for the mutex to be available and then acquire the lock.
void XmlRpcMutex::acquire()
{
#if defined(_WINDOWS)
  if ( ! _pMutex)
    _pMutex = ::CreateMutex(0, TRUE, 0);
  else
    ::WaitForSingleObject(_pMutex, INFINITE);
#else
  if ( ! _pMutex)
  {
    _pMutex = new pthread_mutex_t;
    ::pthread_mutex_init((pthread_mutex_t*)_pMutex, 0);
  }
  ::pthread_mutex_lock((pthread_mutex_t*)_pMutex);
#endif
}

//! Release the mutex.
void XmlRpcMutex::release()
{
  if (_pMutex)
#if defined(_WINDOWS)
    ::ReleaseMutex(_pMutex);
#else
    ::pthread_mutex_unlock((pthread_mutex_t*)_pMutex);
#endif
}

#endif // XMLRPC_THREADS

