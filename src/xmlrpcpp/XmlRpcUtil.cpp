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

#include "XmlRpcUtil.h"

#include <ctype.h>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "XmlRpc.h"

using namespace XmlRpc;


//#define USE_WINDOWS_DEBUG // To make the error and log messages go to VC++ debug output
#ifdef USE_WINDOWS_DEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Version id
const char XmlRpc::XMLRPC_VERSION[] = "XMLRPC++ 0.8";

// Default log verbosity: 0 for no messages through 5 (writes everything)
int XmlRpcLogHandler::_verbosity = 0;

// Default log handler
static class DefaultLogHandler : public XmlRpcLogHandler {
public:

  void log(int level, const char* msg) { 
#ifdef USE_WINDOWS_DEBUG
    if (level <= _verbosity) { OutputDebugString(msg); OutputDebugString("\n"); }
#else
    if (level <= _verbosity) std::cout << msg << std::endl; 
#endif  
  }

} defaultLogHandler;

// Message log singleton
XmlRpcLogHandler* XmlRpcLogHandler::_logHandler = &defaultLogHandler;


// Default error handler
static class DefaultErrorHandler : public XmlRpcErrorHandler {
public:

  void error(const char* msg) {
#ifdef USE_WINDOWS_DEBUG
//    OutputDebugString(msg); OutputDebugString("\n");
#else
//    std::cerr << msg << std::endl; 
#endif  
  }
} defaultErrorHandler;


// Error handler singleton
XmlRpcErrorHandler* XmlRpcErrorHandler::_errorHandler = &defaultErrorHandler;


// Easy API for log verbosity
int XmlRpc::getVerbosity() { return XmlRpcLogHandler::getVerbosity(); }
void XmlRpc::setVerbosity(int level) { XmlRpcLogHandler::setVerbosity(level); }

 

void XmlRpcUtil::log(int level, const char* fmt, ...)
{
  if (level <= XmlRpcLogHandler::getVerbosity())
  {
    va_list va;
    char buf[1024];
    va_start( va, fmt);
    vsnprintf(buf,sizeof(buf)-1,fmt,va);
    buf[sizeof(buf)-1] = 0;
    XmlRpcLogHandler::getLogHandler()->log(level, buf);
  }
}


void XmlRpcUtil::error(const char* fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  char buf[1024];
  vsnprintf(buf,sizeof(buf)-1,fmt,va);
  buf[sizeof(buf)-1] = 0;
  XmlRpcErrorHandler::getErrorHandler()->error(buf);
}

// Returns true if the tag is parsed. No attributes are parsed.
// Sets val to the contents between <tag> and </tag>, or an empty string if <tag/> is found.
// Updates offset to char after </tag>
bool 
XmlRpcUtil::parseTag(const char* tag, std::string const& xml, int* offset, std::string &val)
{
  size_t nxml = xml.length();
  if (*offset >= int(nxml)) return false;

  // Find <tag (skips over anything preceeding...)
  std::string stag = "<";
  stag += tag;
  size_t istart = xml.find(stag, *offset);
  if (istart == std::string::npos) return false;

  istart += stag.length();

  // Advance istart past > or />
  bool lastSlash = false;
  while (istart < nxml && xml[istart] != '>')
  {
    lastSlash = (xml[istart] == '/');
    ++ istart;
  }

  if (istart == nxml) return false;

  val.clear();
  if (lastSlash)  // <tag/>
  {
    *offset = int(istart+1);  // 1 after >
  }
  else            // Find </tag>
  {
    std::string etag = "</";
    etag += tag;
    etag += ">";
    size_t iend = xml.find(etag, istart);

    if (iend == std::string::npos) return false;

    *offset = int(iend + etag.length());
    ++ istart;
    val = xml.substr(istart, iend-istart);
  }

  return true;
}


// Returns true if the tag is found and updates offset to the char after the tag
bool 
XmlRpcUtil::findTag(const char* tag, std::string const& xml, int* offset, bool* emptyTag)
{
  size_t nxml = xml.length();
  if (*offset >= int(nxml)) return false;
  std::string stag = "<";
  stag += tag;
  size_t istart = xml.find(stag, *offset);
  if (istart == std::string::npos)
    return false;

  istart += stag.length();

  // Advance istart past > or />, skips attribs
  bool lastSlash = false;
  while (istart < nxml && xml[istart] != '>')
  {
    lastSlash = (xml[istart] == '/');
    ++ istart;
  }

  if (istart == nxml)
    return false;

  *emptyTag = lastSlash;
  *offset = int(istart+1);  // char after >
  return true;
}


// Returns true if the <tag> or <tag/> is found at the specified offset (modulo any whitespace)
// and updates offset to the char after the tag. *emptyTag is set to true if <tag/> is found, false otherwise
bool 
XmlRpcUtil::nextTagIs(const char* tag, std::string const& xml, int* offset, bool *emptyTag)
{
  if (*offset >= int(xml.length()))
    return false;

  const char* cp = xml.c_str() + *offset;
  int nc = 0;
  while (*cp && isspace(*cp))
  {
    ++cp;
    ++nc;
  }

  int len = int(strlen(tag));
  if  (*cp == '<' && (strncmp(cp+1, tag, len) == 0))
  {
    cp += len + 1;
    if (*cp == '>') // <tag>
    {
      *offset += nc + len + 2;
      *emptyTag = false;
      return true;
    }

    while (*cp && isspace(*cp)) { ++cp; ++nc; }

    if (*cp == '/' && *(cp + 1) == '>') // <tag />
    {
      *offset += nc + len + 3;
      *emptyTag = true;
      return true;
    }
  }
  return false;
}


// Returns true if the next tag found at the specified offset is </tag>
// and updates offset to the char after the tag.
bool 
XmlRpcUtil::nextTagIsEnd(const char* tag, std::string const& xml, int* offset)
{
  if (*offset >= int(xml.length()))
    return false;

  const char* cp = xml.c_str() + *offset;
  int nc = 0;
  while (*cp && *cp != '<')
  {
    ++cp;
    ++nc;
  }

  int len = int(strlen(tag));
  if  (*cp == '<' && *(cp+1) == '/' && (strncmp(cp+2, tag, len) == 0) && *(cp + len + 2) == '>')
  {
    *offset += nc + len + 3;
    return true;
  }

  return false;
}


// xml encodings (xml-encoded entities are preceded with '&')
static const char  AMP = '&';
static const char  rawEntity[] = { '<',   '>',   '&',    '\'',    '\"',    0 };
static const char* xmlEntity[] = { "lt;", "gt;", "amp;", "apos;", "quot;", 0 };
static const int   xmlEntLen[] = { 3,     3,     4,      5,       5 };


// Replace xml-encoded entities with the raw text equivalents.

std::string 
XmlRpcUtil::xmlDecode(const std::string& encoded)
{
  std::string::size_type iAmp = encoded.find(AMP);
  if (iAmp == std::string::npos)
    return encoded;

  std::string decoded(encoded, 0, iAmp);
  std::string::size_type iSize = encoded.size();
  decoded.reserve(iSize);

  const char* ens = encoded.c_str();
  while (iAmp != iSize) {
    if (encoded[iAmp] == AMP && iAmp+1 < iSize) {
      int iEntity;
      for (iEntity=0; xmlEntity[iEntity] != 0; ++iEntity)
	//if (encoded.compare(iAmp+1, xmlEntLen[iEntity], xmlEntity[iEntity]) == 0)
	if (strncmp(ens+iAmp+1, xmlEntity[iEntity], xmlEntLen[iEntity]) == 0)
        {
          decoded += rawEntity[iEntity];
          iAmp += xmlEntLen[iEntity]+1;
          break;
        }
      if (xmlEntity[iEntity] == 0)    // unrecognized sequence
        decoded += encoded[iAmp++];

    } else {
      decoded += encoded[iAmp++];
    }
  }
    
  return decoded;
}


// Replace raw text with xml-encoded entities.

std::string 
XmlRpcUtil::xmlEncode(const std::string& raw)
{
  std::string::size_type iRep = raw.find_first_of(rawEntity);
  if (iRep == std::string::npos)
    return raw;

  std::string encoded(raw, 0, iRep);
  std::string::size_type iSize = raw.size();

  while (iRep != iSize) {
    int iEntity;
    for (iEntity=0; rawEntity[iEntity] != 0; ++iEntity)
      if (raw[iRep] == rawEntity[iEntity])
      {
        encoded += AMP;
        encoded += xmlEntity[iEntity];
        break;
      }
    if (rawEntity[iEntity] == 0)
      encoded += raw[iRep];
    ++iRep;
  }
  return encoded;
}



