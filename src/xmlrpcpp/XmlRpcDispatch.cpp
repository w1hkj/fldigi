//
// XmlRpc++ Copyright (c) 2002-2008 by Chris Morley
//

#include <config.h>

#include "XmlRpcDispatch.h"
#include "XmlRpcSource.h"
#include "XmlRpcUtil.h"

#include <errno.h>
#include <math.h>

#if defined(__FreeBSD__) 
#	ifdef USE_FTIME
#		include <sys/timeb.h>
#	endif
#else
#	include <sys/timeb.h>
#endif // __FreeBSD__

#if defined(_WINDOWS)
# include <winsock2.h>

# define USE_FTIME
# if defined(_MSC_VER)
#  define timeb _timeb
#  define ftime _ftime
# endif
#else
# include <sys/time.h>
#endif  // _WINDOWS


using namespace XmlRpc;


XmlRpcDispatch::XmlRpcDispatch()
{
  _endTime = -1.0;
  _doClear = false;
  _inWork = false;
}


XmlRpcDispatch::~XmlRpcDispatch()
{
}

// Monitor this source for the specified events and call its event handler
// when the event occurs
void
XmlRpcDispatch::addSource(XmlRpcSource* source, unsigned mask)
{
  _sources.push_back(MonitoredSource(source, mask));
}

// Stop monitoring this source. Does not close the source.
void
XmlRpcDispatch::removeSource(XmlRpcSource* source)
{
  for (SourceList::iterator it=_sources.begin(); it!=_sources.end(); ++it)
    if (it->getSource() == source)
    {
      _sources.erase(it);
      break;
    }
}


// Modify the types of events to watch for on this source
void 
XmlRpcDispatch::setSourceEvents(XmlRpcSource* source, unsigned eventMask)
{
  for (SourceList::iterator it=_sources.begin(); it!=_sources.end(); ++it)
    if (it->getSource() == source)
    {
      it->getMask() = eventMask;
      break;
    }
}



// Watch current set of sources and process events
void
XmlRpcDispatch::work(double timeoutSeconds)
{
  // Compute end time
  double timeNow = getTime();
  _endTime = (timeoutSeconds < 0.0) ? -1.0 : (timeNow + timeoutSeconds);
  _doClear = false;
  _inWork = true;

  // Only work while there is something to monitor
  while (_sources.size() > 0) {

    // Wait for and dispatch events
    if ( ! waitForAndProcessEvents(timeoutSeconds))
    {
      _inWork = false;
      return;
    }


    // Check whether to clear all sources
    if (_doClear)
    {
      SourceList sourcesToClose;
      _sources.swap(sourcesToClose);
      for (SourceList::iterator it=sourcesToClose.begin(); it!=sourcesToClose.end(); ++it)
      {
        XmlRpcSource *src = it->getSource();
        src->close();
      }

      _doClear = false;
    }

    // Check whether end time has passed or exit has been called
    if (_endTime == 0.0)        // Exit
    {
      break;
    }
    else if (_endTime > 0.0)    // Check for timeout
    {
      double t = getTime();
      if (t > _endTime)
        break;

      // Decrement timeout by elapsed time
      timeoutSeconds -= (t - timeNow);
      if (timeoutSeconds < 0.0) 
        timeoutSeconds = 0.0;    // Shouldn't happen but its fp math...
      timeNow = t;
    }
  }

  _inWork = false;
}



// Exit from work routine. Presumably this will be called from
// one of the source event handlers.
void
XmlRpcDispatch::exit()
{
  _endTime = 0.0;   // Return from work asap
}


// Clear all sources from the monitored sources list
void
XmlRpcDispatch::clear()
{
  if (_inWork)
  {
    _doClear = true;  // Finish reporting current events before clearing
  }
  else
  {
    SourceList sourcesToClose;
    _sources.swap(sourcesToClose);
    for (SourceList::iterator it=sourcesToClose.begin(); it!=sourcesToClose.end(); ++it)
      it->getSource()->close();
  }
}


// Time utility- return time in seconds
double
XmlRpcDispatch::getTime()
{
#ifdef USE_FTIME
  struct timeb	tbuff;

  ftime(&tbuff);
  return ((double) tbuff.time + ((double)tbuff.millitm / 1000.0) +
	  ((double) tbuff.timezone * 60));
#else
  struct timeval	tv;
  struct timezone	tz;

  gettimeofday(&tv, &tz);
  return (tv.tv_sec + tv.tv_usec / 1000000.0);
#endif /* USE_FTIME */
}


// Wait for I/O on any source, timeout, or interrupt signal.
bool
XmlRpcDispatch::waitForAndProcessEvents(double timeoutSeconds)
{
  // Construct the sets of descriptors we are interested in
  fd_set inFd, outFd, excFd;
  FD_ZERO(&inFd);
  FD_ZERO(&outFd);
  FD_ZERO(&excFd);

  XmlRpcSocket::Socket maxFd = 0;
  for (SourceList::iterator it=_sources.begin(); it!=_sources.end(); ++it)
  {
    XmlRpcSocket::Socket fd = it->getSource()->getfd();
    if (it->getMask() & ReadableEvent) FD_SET(fd, &inFd);
    if (it->getMask() & WritableEvent) FD_SET(fd, &outFd);
    if (it->getMask() & Exception)     FD_SET(fd, &excFd);
    if (it->getMask() && fd > maxFd)   maxFd = fd;
  }

  // Check for events
  int nEvents;
  if (_endTime < 0.0)
  {
    nEvents = select(int(maxFd+1), &inFd, &outFd, &excFd, NULL);
  }
  else 
  {
    struct timeval tv;
    tv.tv_sec = (int)floor(timeoutSeconds);
    tv.tv_usec = ((int)floor(1000000.0 * (timeoutSeconds-floor(timeoutSeconds)))) % 1000000;
    nEvents = select(int(maxFd+1), &inFd, &outFd, &excFd, &tv);
  }

  if (nEvents < 0 && errno != EINTR)
  {
    XmlRpcUtil::error("Error in XmlRpcDispatch::work: error in select (%d).", nEvents);
    return false;
  }

  // Process events. Copy source list to avoid invalidating iterator by removing sources.
  SourceList s(_sources);
  for (SourceList::iterator it=s.begin(); it != s.end(); ++it)
  {
    XmlRpcSource* src = it->getSource();
    XmlRpcSocket::Socket fd = src->getfd();

    if (fd <= maxFd)
    {
      // handleEvent is called once per event type signalled
      unsigned newMask = 0;
      int nset = 0;
      if (FD_ISSET(fd, &inFd))
      {
        newMask |= src->handleEvent(ReadableEvent);
        ++nset;
      }
      if (FD_ISSET(fd, &outFd))
      {
        newMask |= src->handleEvent(WritableEvent);
        ++nset;
      }
      if (FD_ISSET(fd, &excFd))
      {
        newMask |= src->handleEvent(Exception);
        ++nset;
      }

      // Some event occurred
      if (nset)
      {
        // This bit is not terribly efficient if many connections are active...
        if (newMask)
        {
          setSourceEvents(src, newMask);
        }
        else       // Stop monitoring this one
        {
          removeSource(src);

          if ( ! src->getKeepOpen())
            src->close();
        }
      }
    }
  }

  return true;
}
