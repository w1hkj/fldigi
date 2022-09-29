//=====================================================================
//
// base64 encoding / decoding class
//
//=====================================================================

#include <stdio.h> 
#include <stdlib.h> 
#include <ctype.h> 

#include <string>

typedef unsigned char byte;

class base64 {
#define LINELEN 72
private:
	std::string output;
	size_t iolen;
	size_t iocp;
	bool ateof;
	byte dtable[256];
	byte etable[256];
	int linelength;
	bool crlf;
	void init();
public:
	base64(bool t = false) {crlf = t; init(); };
	~base64(){};
	std::string encode(std::string in);
	std::string decode(std::string in);
};
