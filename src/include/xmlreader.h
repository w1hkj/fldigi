#include <string>

#include "irrXML.h"

using namespace irr;
using namespace io;

class IIrrXMLStringReader: public IFileReadCallBack {
  const char *s;
  int len;
  int p;

public:

  IIrrXMLStringReader(const char *szStr) {
    s = szStr;
    len = strlen(s);
    p=0;
  }

  IIrrXMLStringReader(const std::string &str) {
    s=str.c_str();
    len = strlen(s);
    p=0;
  }

  int read(void * buffer, int sizeToRead) {
    char *sss = (char *)buffer;
    if (p >= len) return 0;
    int j = 0;
    for (int i = p; i < len && j < sizeToRead; ) {
      sss[j++] = s[i++];
    }
    return 1;
  }
    
  int getSize() {
    return len-p;
  }

};

