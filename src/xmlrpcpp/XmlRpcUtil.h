#ifndef _XMLRPCUTIL_H_
#define _XMLRPCUTIL_H_
//
// XmlRpc++ Copyright (c) 2002-2008 by Chris Morley
//
#if defined(_MSC_VER)
# pragma warning(disable:4786)    // identifier was truncated in debug info
#endif

#include <string>

#if defined(_MSC_VER)
# define snprintf	    _snprintf
# define vsnprintf    _vsnprintf
# define strcasecmp	  _stricmp
# define strncasecmp	_strnicmp
#elif defined(__BORLANDC__)
# define strcasecmp stricmp
# define strncasecmp strnicmp
#endif

namespace XmlRpc {

  //! Utilities for XML parsing, encoding, and decoding and message handlers.
  class XmlRpcUtil {
  public:

    //! Parses the specified tag. No attributes are parsed, no validation is done.
    //! Sets val to the contents between <tag> and </tag>, or an empty string if <tag/> is found.
    //! Returns true if the tag is parsed. Updates offset to char after </tag>
    static bool parseTag(const char* tag, std::string const& xml, int* offset, std::string &val);

    //! Returns true if the tag is found and updates offset to the char after the tag.
    //! If the tag is of the form <tag/>, emptyTag is set to true.
    static bool findTag(const char* tag, std::string const& xml, int* offset, bool* emptyTag);

    //! Returns true if the tag is found at the specified offset (modulo any whitespace)
    //! and updates offset to the char after the tag. If an empty tag is found (eg,
    //! <tag/>), true is returned, offset is updated to be after the close of the tag, and
    //! emptyTag is set to true.
    static bool nextTagIs(const char* tag, std::string const& xml, int* offset, bool* emptyTag);

    //! Passes over the next tag found at the specified offset is </tag>, 
    //! offset is updated to be after the close of the tag.
    //! Will skip over all characters until < is seen.
    static bool nextTagIsEnd(const char* tag, std::string const& xml, int* offset);


    //! Convert raw text to encoded xml.
    static std::string xmlEncode(const std::string& raw);

    //! Convert encoded xml to raw text
    static std::string xmlDecode(const std::string& encoded);


    //! Dump messages somewhere
    static void log(int level, const char* fmt, ...);

    //! Dump error messages somewhere
    static void error(const char* fmt, ...);

  };
} // namespace XmlRpc

#endif // _XMLRPCUTIL_H_
