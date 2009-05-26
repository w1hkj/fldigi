//=====================================================================
//
// base64 encoding / decoding class
//
// To create a standalone base64 encode/coder:
// g++ -DTEST b64.cxx -o base64
//
// To use in a calling program:
//
// base64 b64;        // default no CRLF's in output file
// base 64 b64(true); // insert CRLF's in output file
// pass c++ string into encoder / decoder
// return value is encoded / decoded string
// original string is left unchanged
// 
// string instr, outstr;
// outstr = b64.encoder(instr);
// outstr = b64.decoder(instr);
//=====================================================================

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "b64.h"

void base64::init()
{
	iolen = 0;
	iocp = 0;
	ateof = false;
	linelength = 0;

// create the etable for encoding
	for (int i = 0; i < 9; i++) {
		etable[i] = 'A' + i;
		etable[i + 9] = 'J' + i;
		etable[26 + i] = 'a' + i;
		etable[26 + i + 9] = 'j' + i;
	}
	for (int i = 0; i < 8; i++) {
		etable[i + 18] = 'S' + i;
		etable[26 + i + 18] = 's' + i;
	}
	for (int i = 0; i < 10; i++)
		etable[52 + i] = '0' + i;
	etable[62] = '+';
	etable[63] = '/';
	
// create the dtable for decoding
	for (int i= 0; i < 255; i++)
		dtable[i] = 0x80;
	for (int i = 'A'; i <= 'I'; i++)
		dtable[i] = 0 + (i - 'A');
	for (int i = 'J'; i <= 'R'; i++)
		dtable[i] = 9 + (i - 'J');
	for (int i = 'S'; i <= 'Z'; i++)
		dtable[i] = 18 + (i - 'S');
	for (int i = 'a'; i <= 'i'; i++)
		dtable[i] = 26 + (i - 'a');
	for (int i = 'j'; i <= 'r'; i++)
		dtable[i] = 35 + (i - 'j');
	for (int i = 's'; i <= 'z'; i++)
		dtable[i] = 44 + (i - 's');
	for (int i = '0'; i <= '9'; i++)
		dtable[i] = 52 + (i - '0');
	dtable[(int)'+'] = 62;
	dtable[(int)'/'] = 63;
	dtable[(int)'='] = 0;
}

string base64::encode(string in)
{
	int n;
	byte igroup[3], ogroup[4];
	
	output = "";
	iocp = 0;
	ateof = false;
	if (crlf)
		linelength = 0;
	iolen = in.length();
	
	while (!ateof) {
		igroup[0] = igroup[1] = igroup[2] = 0;
		for (n = 0; n < 3; n++) {
			if (iocp == iolen) {
				ateof = true;
				break;
			} 
			igroup[n] = (byte)in[iocp];
			iocp++;
		}
 		if (n > 0) {
			ogroup[0] = etable[igroup[0] >> 2];
			ogroup[1] = etable[((igroup[0] & 3) << 4) | (igroup[1] >> 4)];
			ogroup[2] = etable[((igroup[1] & 0xF) << 2) | (igroup[2] >> 6)];
			ogroup[3] = etable[igroup[2] & 0x3F];
			if (n < 2) {
				ogroup[2] = '=';
				if (n < 1) {
					ogroup[2] = '=';
				}
			}
			for (int i = 0; i < 4; i++) {
				if (crlf)
					if (linelength >= LINELEN) {
//						output += '\r';
						output += '\n';
						linelength = 0;
					}
				output += (byte)ogroup[i];
				if (crlf)
					linelength++;
			}
		}
	}
	if (crlf) {
//		output += '\r';
		output += '\n';
	}

	return output;
}

string base64::decode(string in)
{
	int i;
	output = "";
	iocp = 0;
	iolen = in.length();
	byte c;
	
	while (iocp < iolen) {
		byte a[4], b[4], o[3];

		for (i = 0; i < 4; i++) {
			if (iocp == iolen) {
				output = "b64 file length error.\n";
				return output;
			}
			c = in[iocp++];
			while (c <= ' ') {
				if (iocp == iolen) {
					return output;
				}
				c = in[iocp++];
			}
			if (dtable[c] & 0x80) {
				output = "Illegal character in b64 file.\n";
				return output;
			}
			a[i] = c;
			b[i] = (byte)dtable[c];
		}
		o[0] = (b[0] << 2) | (b[1] >> 4);
		o[1] = (b[1] << 4) | (b[2] >> 2);
		o[2] = (b[2] << 6) | b[3];
		output += o[0];
		if (a[2] != '=') {
			output += o[1];
			if (a[3] != '=')
				output += o[2];
		}
	}
	return output;
}

#ifdef TEST
#include <iostream>
#include <fstream>


void usage(void)
{
	printf("b64  --  Encode/decode file as base64.  Call:\n");
	printf("         b64 e/d < infile > outfile\n");
}

int main(int argc,char*argv[])
{
	char opt;
	bool decoding = false;
	char * cp;
	byte c;
	
	string inputstring;
	string infilename;
	string outputstring;
	string outfilename;
	
	base64 b64;
	
	if (argc < 2) {
		usage();
		return(0);
	}
	opt = *(argv[1]);

	if (opt == 'd' || opt == 'D') {
		while (!std::cin.eof()) {
			c = std::cin.get();
			if (!std::cin.eof())
				inputstring += c;
		}
		outputstring = b64.decode( inputstring );
		size_t len = outputstring.length();
		for (size_t n = 0; n < len; n++)
			std::cout << (unsigned char)outputstring[n];
	} else if (opt == 'e' || opt == 'E') {
		while (!std::cin.eof()) {
			c = std::cin.get();
			if (!std::cin.eof())
				inputstring += c;
		}
		outputstring = b64.encode( inputstring );
		size_t len = outputstring.length();
		for (size_t n = 0; n < len; n++)
			std::cout << (unsigned char)outputstring[n];
	} else
		usage();
		
	return 0;
}
#endif
