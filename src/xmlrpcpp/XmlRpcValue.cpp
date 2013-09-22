//
// XmlRpc++ Copyright (c) 2002-2008 by Chris Morley
//

#include <config.h>

#include "XmlRpcValue.h"
#include "XmlRpcException.h"
#include "XmlRpcUtil.h"
#include "base64.h"

#include <iostream>
#include <ostream>
#include <stdlib.h>
#include <stdio.h>


namespace XmlRpc {


  static const char VALUE_TAG[]     = "value";
  static const char NIL_TAG[]       = "nil";
  static const char BOOLEAN_TAG[]   = "boolean";
  static const char DOUBLE_TAG[]    = "double";
  static const char INT_TAG[]       = "int";
  static const char I4_TAG[]        = "i4";
  static const char STRING_TAG[]    = "string";
  static const char DATETIME_TAG[]  = "dateTime.iso8601";
  static const char BASE64_TAG[]    = "base64";

  static const char ARRAY_TAG[]     = "array";
  static const char DATA_TAG[]      = "data";

  static const char STRUCT_TAG[]    = "struct";
  static const char MEMBER_TAG[]    = "member";
  static const char NAME_TAG[]      = "name";

      
  // Format strings
  std::string XmlRpcValue::_doubleFormat("%f");



  // Clean up
  void XmlRpcValue::invalidate()
  {
    switch (_type) {
      case TypeString:    delete _value.asString; break;
      case TypeDateTime:  delete _value.asTime;   break;
      case TypeBase64:    delete _value.asBinary; break;
      case TypeArray:     delete _value.asArray;  break;
      case TypeStruct:    delete _value.asStruct; break;
      default: break;
    }
    _type = TypeInvalid;
    _value.asBinary = 0;
  }

  
  // Type checking
  void XmlRpcValue::assertType(Type t) const
  {
    if (_type != t)
    {
      throw XmlRpcException("type error");
    }
  }

  void XmlRpcValue::assertType(Type t)
  {
    if (_type == TypeInvalid)
    {
      _type = t;
      switch (_type) {    // Ensure there is a valid value for the type
        case TypeString:   _value.asString = new std::string(); break;
        case TypeDateTime: _value.asTime = new struct tm();     break;
        case TypeBase64:   _value.asBinary = new BinaryData();  break;
        case TypeArray:    _value.asArray = new ValueArray();   break;
        case TypeStruct:   _value.asStruct = new ValueStruct(); break;
        default:           _value.asBinary = 0; break;
      }
    }
    else if (_type != t)
    {
      throw XmlRpcException("type error");
    }
  }

  void XmlRpcValue::assertArray(int size) const
  {
    if (_type != TypeArray)
      throw XmlRpcException("type error: expected an array");
    else if (int(_value.asArray->size()) < size)
      throw XmlRpcException("range error: array index too large");
  }


  void XmlRpcValue::assertArray(int size)
  {
    if (_type == TypeInvalid) {
      _type = TypeArray;
      _value.asArray = new ValueArray(size);
    } else if (_type == TypeArray) {
      if (int(_value.asArray->size()) < size)
        _value.asArray->resize(size);
    } else
      throw XmlRpcException("type error: expected an array");
  }

  void XmlRpcValue::assertStruct()
  {
    if (_type == TypeInvalid) {
      _type = TypeStruct;
      _value.asStruct = new ValueStruct();
    } else if (_type != TypeStruct)
      throw XmlRpcException("type error: expected a struct");
  }


  // Operators
  XmlRpcValue& XmlRpcValue::operator=(XmlRpcValue const& rhs)
  {
    if (this != &rhs)
    {
      invalidate();
      _type = rhs._type;
      switch (_type) {
        case TypeBoolean:  _value.asBool = rhs._value.asBool; break;
        case TypeInt:      _value.asInt = rhs._value.asInt; break;
        case TypeDouble:   _value.asDouble = rhs._value.asDouble; break;
        case TypeDateTime: _value.asTime = new struct tm(*rhs._value.asTime); break;
        case TypeString:   _value.asString = new std::string(*rhs._value.asString); break;
        case TypeBase64:   _value.asBinary = new BinaryData(*rhs._value.asBinary); break;
        case TypeArray:    _value.asArray = new ValueArray(*rhs._value.asArray); break;
        case TypeStruct:   _value.asStruct = new ValueStruct(*rhs._value.asStruct); break;
        default:           _value.asBinary = 0; break;
      }
    }
    return *this;
  }


  // Predicate for tm equality
  static bool tmEq(struct tm const& t1, struct tm const& t2) {
    return t1.tm_sec == t2.tm_sec && t1.tm_min == t2.tm_min &&
            t1.tm_hour == t2.tm_hour && t1.tm_mday == t1.tm_mday &&
            t1.tm_mon == t2.tm_mon && t1.tm_year == t2.tm_year;
  }

  bool XmlRpcValue::operator==(XmlRpcValue const& other) const
  {
    if (_type != other._type)
      return false;

    switch (_type) {
      case TypeBoolean:  return ( !_value.asBool && !other._value.asBool) ||
                                ( _value.asBool && other._value.asBool);
      case TypeInt:      return _value.asInt == other._value.asInt;
      case TypeDouble:   return _value.asDouble == other._value.asDouble;
      case TypeDateTime: return tmEq(*_value.asTime, *other._value.asTime);
      case TypeString:   return *_value.asString == *other._value.asString;
      case TypeBase64:   return *_value.asBinary == *other._value.asBinary;
      case TypeArray:    return *_value.asArray == *other._value.asArray;

      // The map<>::operator== requires the definition of value< for kcc
      case TypeStruct:   //return *_value.asStruct == *other._value.asStruct;
        {
          if (_value.asStruct->size() != other._value.asStruct->size())
            return false;
          
          ValueStruct::const_iterator it1=_value.asStruct->begin();
          ValueStruct::const_iterator it2=other._value.asStruct->begin();
          while (it1 != _value.asStruct->end()) {
            const XmlRpcValue& v1 = it1->second;
            const XmlRpcValue& v2 = it2->second;
            if ( ! (v1 == v2))
              return false;
            it1++;
            it2++;
          }
          return true;
        }
      default: break;
    }
    return true;    // Both invalid values ...
  }

  bool XmlRpcValue::operator!=(XmlRpcValue const& other) const
  {
    return !(*this == other);
  }


  // Works for strings, binary data, arrays, and structs.
  int XmlRpcValue::size() const
  {
    switch (_type) {
      case TypeString: return int(_value.asString->size());
      case TypeBase64: return int(_value.asBinary->size());
      case TypeArray:  return int(_value.asArray->size());
      case TypeStruct: return int(_value.asStruct->size());
      default: break;
    }

    throw XmlRpcException("type error");
  }

  // Checks for existence of struct member
  bool XmlRpcValue::hasMember(const std::string& name) const
  {
    return _type == TypeStruct && _value.asStruct->find(name) != _value.asStruct->end();
  }


  // Set the value from xml. The chars at *offset into valueXml 
  // should be the start of a <value> tag. Destroys any existing value.
  bool XmlRpcValue::fromXml(std::string const& valueXml, int* offset)
  {
    int savedOffset = *offset;

    invalidate();
    bool emptyTag;
    if ( ! XmlRpcUtil::nextTagIs(VALUE_TAG, valueXml, offset, &emptyTag))
      return false;       // Not a value, offset not updated

    // No value? Pretend its an empty string...
    if (emptyTag)
    {
      *this = "";
      return true;
    }

    // No type tag? Assume string
//    bool result = true;
    bool result = false;

    int valueOffset = *offset;

    if (XmlRpcUtil::nextTagIsEnd(VALUE_TAG, valueXml, offset))
    {
      result = true;
      return stringFromXml(valueXml, &valueOffset);
    }
    else if (XmlRpcUtil::nextTagIs(NIL_TAG, valueXml, offset, &emptyTag))
    {
      _type = TypeNil;
      result = true;
    }
    else if (XmlRpcUtil::nextTagIs(BOOLEAN_TAG, valueXml, offset, &emptyTag))
    {
      if (emptyTag) {
        *this = false;
        result = true;
      } else
        result = boolFromXml(valueXml, offset) && 
                 XmlRpcUtil::nextTagIsEnd(BOOLEAN_TAG, valueXml, offset);
    }
    else if (XmlRpcUtil::nextTagIs(I4_TAG, valueXml, offset, &emptyTag))
    {
      if (emptyTag) {
        *this = 0;
        result = true;
      } else
        result = intFromXml(valueXml, offset) && 
                 XmlRpcUtil::nextTagIsEnd(I4_TAG, valueXml, offset);
    }
    else if (XmlRpcUtil::nextTagIs(INT_TAG, valueXml, offset, &emptyTag))
    {
      if (emptyTag) {
        *this = 0;
        result = true;
      } else
        result = intFromXml(valueXml, offset) && 
                 XmlRpcUtil::nextTagIsEnd(INT_TAG, valueXml, offset);
    }
    else if (XmlRpcUtil::nextTagIs(DOUBLE_TAG, valueXml, offset, &emptyTag))
    {
      if (emptyTag) {
        *this = 0.0;
        result = true;
      } else
        result = doubleFromXml(valueXml, offset) && 
                 XmlRpcUtil::nextTagIsEnd(DOUBLE_TAG, valueXml, offset);
    }
    else if (XmlRpcUtil::nextTagIs(STRING_TAG, valueXml, offset, &emptyTag))
    {
      if (emptyTag) {
        *this = "";
        result = true;
      } else
        result = stringFromXml(valueXml, offset) && 
                 XmlRpcUtil::nextTagIsEnd(STRING_TAG, valueXml, offset);
    }
    else if (XmlRpcUtil::nextTagIs(DATETIME_TAG, valueXml, offset, &emptyTag))
    {
      if (emptyTag)
        result = false;
      else
        result = timeFromXml(valueXml, offset) && 
                 XmlRpcUtil::nextTagIsEnd(DATETIME_TAG, valueXml, offset);
    }
    else if (XmlRpcUtil::nextTagIs(BASE64_TAG, valueXml, offset, &emptyTag))
    {
      if (emptyTag)
        result = binaryFromXml("", 0);
      else
        result = binaryFromXml(valueXml, offset) && 
                 XmlRpcUtil::nextTagIsEnd(BASE64_TAG, valueXml, offset);
    }
    else if (XmlRpcUtil::nextTagIs(ARRAY_TAG, valueXml, offset, &emptyTag))
    {
      if (emptyTag)
        result = false;
      else
        result = arrayFromXml(valueXml, offset) && 
                 XmlRpcUtil::nextTagIsEnd(ARRAY_TAG, valueXml, offset);
    }
    else if (XmlRpcUtil::nextTagIs(STRUCT_TAG, valueXml, offset, &emptyTag))
    {
      if (emptyTag)
        result = false;
      else
        result = structFromXml(valueXml, offset) && 
                 XmlRpcUtil::nextTagIsEnd(STRUCT_TAG, valueXml, offset);
    }

    // Unrecognized tag after <value> or no </value>
    if ( ! result || ! XmlRpcUtil::nextTagIsEnd(VALUE_TAG, valueXml, offset))
    {
      *offset = savedOffset;
      return false;
    }

    return true;
  }

  // Encode the Value in xml
  std::string XmlRpcValue::toXml() const
  {
    switch (_type) {
      case TypeNil:      return nilToXml();
      case TypeBoolean:  return boolToXml();
      case TypeInt:      return intToXml();
      case TypeDouble:   return doubleToXml();
      case TypeString:   return stringToXml();
      case TypeDateTime: return timeToXml();
      case TypeBase64:   return binaryToXml();
      case TypeArray:    return arrayToXml();
      case TypeStruct:   return structToXml();
      default: break;
    }
    return std::string();   // Invalid value
  }


  // Boolean
  bool XmlRpcValue::boolFromXml(std::string const& valueXml, int* offset)
  {
    const char* valueStart = valueXml.c_str() + *offset;
    char* valueEnd;
    long ivalue = strtol(valueStart, &valueEnd, 10);
    if (valueEnd == valueStart || (ivalue != 0 && ivalue != 1))
      return false;

    _type = TypeBoolean;
    _value.asBool = (ivalue == 1);
    *offset += int(valueEnd - valueStart);
    return true;
  }

  std::string XmlRpcValue::nilToXml() const
  {
    return "<value><nil/></value>";
  }

  std::string XmlRpcValue::boolToXml() const
  {
    static std::string booleanTrueXml("<value><boolean>1</boolean></value>");
    static std::string booleanFalseXml("<value><boolean>0</boolean></value>");
    return _value.asBool ? booleanTrueXml : booleanFalseXml;
  }

  // Int
  bool XmlRpcValue::intFromXml(std::string const& valueXml, int* offset)
  {
    const char* valueStart = valueXml.c_str() + *offset;
    char* valueEnd;
    long ivalue = strtol(valueStart, &valueEnd, 10);
    if (valueEnd == valueStart)
      return false;

    _type = TypeInt;
    _value.asInt = int(ivalue);
    *offset += int(valueEnd - valueStart);
    return true;
  }

  std::string XmlRpcValue::intToXml() const
  {
    char buf[256];
    snprintf(buf, sizeof(buf)-1, "<value><i4>%d</i4></value>", _value.asInt);
    buf[sizeof(buf)-1] = 0;

    return std::string(buf);
  }

  // Double
  bool XmlRpcValue::doubleFromXml(std::string const& valueXml, int* offset)
  {
    const char* valueStart = valueXml.c_str() + *offset;
    char* valueEnd;
    double dvalue = strtod(valueStart, &valueEnd);
    if (valueEnd == valueStart)
      return false;

    _type = TypeDouble;
    _value.asDouble = dvalue;
    *offset += int(valueEnd - valueStart);
    return true;
  }

  std::string XmlRpcValue::doubleToXml() const
  {
    char fmtbuf[256], buf[256];
    snprintf(fmtbuf, sizeof(fmtbuf)-1, "<value><double>%s</double></value>", getDoubleFormat().c_str());
    fmtbuf[sizeof(fmtbuf)-1] = 0;
    snprintf(buf, sizeof(buf)-1, fmtbuf, _value.asDouble);
    buf[sizeof(buf)-1] = 0;

    return std::string(buf);
  }

  // String
  bool XmlRpcValue::stringFromXml(std::string const& valueXml, int* offset)
  {
    size_t valueEnd = valueXml.find('<', *offset);
    if (valueEnd == std::string::npos)
      return false;     // No end tag;

    _type = TypeString;
    _value.asString = new std::string(XmlRpcUtil::xmlDecode(valueXml.substr(*offset, valueEnd-*offset)));
    *offset += int(_value.asString->length());
    return true;
  }

  std::string XmlRpcValue::stringToXml() const
  {
    return std::string("<value>") + XmlRpcUtil::xmlEncode(*_value.asString) + std::string("</value>");
  }

  // DateTime (stored as a struct tm)
  bool XmlRpcValue::timeFromXml(std::string const& valueXml, int* offset)
  {
    size_t valueEnd = valueXml.find('<', *offset);
    if (valueEnd == std::string::npos)
      return false;     // No end tag;

    std::string stime = valueXml.substr(*offset, valueEnd-*offset);

    struct tm t;
    if (sscanf(stime.c_str(),"%4d%2d%2dT%2d:%2d:%2d",&t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec) != 6)
      return false;

    t.tm_year -= 1900;    // 	years since 1900
    t.tm_mon -= 1;        // 	months 0..11
    t.tm_isdst = -1;
    _type = TypeDateTime;
    _value.asTime = new struct tm(t);
    *offset += int(stime.length());
    return true;
  }

  std::string XmlRpcValue::timeToXml() const
  {
    struct tm* t = _value.asTime;
    char buf[20];
    snprintf(buf, sizeof(buf)-1, "%04d%02d%02dT%02d:%02d:%02d", 
      1900+t->tm_year,1+t->tm_mon,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
    buf[sizeof(buf)-1] = 0;

    return std::string("<value><dateTime.iso8601>") + buf + std::string("</dateTime.iso8601></value>");
  }


  // Base64
  bool XmlRpcValue::binaryFromXml(std::string const& valueXml, int* offset)
  {
    size_t valueEnd = valueXml.find('<', *offset);
    if (valueEnd == std::string::npos)
      return false;     // No end tag;

    _type = TypeBase64;
    std::string asString = valueXml.substr(*offset, valueEnd-*offset);
    _value.asBinary = new BinaryData();
    // check whether base64 encodings can contain chars xml encodes...

    // convert from base64 to binary
    int iostatus = 0;
	  base64<char> decoder;
    std::back_insert_iterator<BinaryData> ins = std::back_inserter(*(_value.asBinary));
		decoder.get(asString.begin(), asString.end(), ins, iostatus);

    *offset += int(asString.length());
    return true;
  }


  std::string XmlRpcValue::binaryToXml() const
  {
    // convert to base64
    std::vector<char> base64data;
    int iostatus = 0;
    base64<char> encoder;
    std::back_insert_iterator<std::vector<char> > ins = std::back_inserter(base64data);
    encoder.put(_value.asBinary->begin(), _value.asBinary->end(), ins, iostatus, base64<>::crlf());

    // Wrap with xml
    std::string xml = "<value><base64>";
    xml.append(base64data.begin(), base64data.end());
    xml += "</base64></value>";
    return xml;
  }


  // Array
  bool XmlRpcValue::arrayFromXml(std::string const& valueXml, int* offset)
  {
    bool emptyTag;
    if ( ! XmlRpcUtil::nextTagIs(DATA_TAG, valueXml, offset, &emptyTag))
      return false;

    _type = TypeArray;
    _value.asArray = new ValueArray;

    if ( ! emptyTag)
    {
      XmlRpcValue v;
      while (v.fromXml(valueXml, offset))
        _value.asArray->push_back(v);       // copy...

      // Skip the trailing </data>
      (void) XmlRpcUtil::nextTagIsEnd(DATA_TAG, valueXml, offset);
    }
    return true;
  }


  // In general, its preferable to generate the xml of each element of the
  // array as it is needed rather than glomming up one big string.
  std::string XmlRpcValue::arrayToXml() const
  {
    std::string xml = "<value><array><data>";

    int s = int(_value.asArray->size());
    for (int i=0; i<s; ++i)
       xml += _value.asArray->at(i).toXml();

    xml += "</data></array></value>";
    return xml;
  }


  // Struct
  bool XmlRpcValue::structFromXml(std::string const& valueXml, int* offset)
  {
    _type = TypeStruct;
    _value.asStruct = new ValueStruct;

    std::string name;
    bool emptyTag;
    while (XmlRpcUtil::nextTagIs(MEMBER_TAG, valueXml, offset, &emptyTag))
    {
      if ( ! emptyTag)
      {
        if (XmlRpcUtil::parseTag(NAME_TAG, valueXml, offset, name))
        {
          // value
          XmlRpcValue val(valueXml, offset);
          if ( ! val.valid()) {
            invalidate();
            return false;
          }
          const std::pair<const std::string, XmlRpcValue> p(name, val);
          _value.asStruct->insert(p);

          (void) XmlRpcUtil::nextTagIsEnd(MEMBER_TAG, valueXml, offset);
        }
      }
    }

    return true;
  }


  // In general, its preferable to generate the xml of each element
  // as it is needed rather than glomming up one big string.
  std::string XmlRpcValue::structToXml() const
  {
    std::string xml = "<value><struct>";

    ValueStruct::const_iterator it;
    for (it=_value.asStruct->begin(); it!=_value.asStruct->end(); ++it)
    {
      xml += "<member><name>";
      xml += XmlRpcUtil::xmlEncode(it->first);
      xml += "</name>";
      xml += it->second.toXml();
      xml += "</member>";
    }

    xml += "</struct></value>";
    return xml;
  }



  // Write the value without xml encoding it
  std::ostream& XmlRpcValue::write(std::ostream& os) const {
    switch (_type) {
      default:           break;
      case TypeBoolean:  os << _value.asBool; break;
      case TypeInt:      os << _value.asInt; break;
      case TypeDouble:   os << _value.asDouble; break;
      case TypeString:   os << *_value.asString; break;
      case TypeDateTime:
        {
          struct tm* t = _value.asTime;
          char buf[20];
          snprintf(buf, sizeof(buf)-1, "%4d%02d%02dT%02d:%02d:%02d", 
            t->tm_year,t->tm_mon,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
          buf[sizeof(buf)-1] = 0;
          os << buf;
          break;
        }
      case TypeBase64:
        {
          int iostatus = 0;
          std::ostreambuf_iterator<char> out(os);
          base64<char> encoder;
          encoder.put(_value.asBinary->begin(), _value.asBinary->end(), out, iostatus, base64<>::crlf());
          break;
        }
      case TypeArray:
        {
          int s = int(_value.asArray->size());
          os << '{';
          for (int i=0; i<s; ++i)
          {
            if (i > 0) os << ',';
            _value.asArray->at(i).write(os);
          }
          os << '}';
          break;
        }
      case TypeStruct:
        {
          os << '[';
          ValueStruct::const_iterator it;
          for (it=_value.asStruct->begin(); it!=_value.asStruct->end(); ++it)
          {
            if (it!=_value.asStruct->begin()) os << ',';
            os << it->first << ':';
            it->second.write(os);
          }
          os << ']';
          break;
        }
      
    }
    
    return os;
  }

} // namespace XmlRpc


// ostream
std::ostream& operator<<(std::ostream& os, XmlRpc::XmlRpcValue& v) 
{ 
  // If you want to output in xml format:
  //return os << v.toXml(); 
  return v.write(os);
}

