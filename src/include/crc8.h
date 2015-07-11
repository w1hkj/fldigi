#ifndef __CRC8_H
#define __CRC8_H

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <string>

class CRC8 {
private:
	unsigned char val;
	unsigned char table[256];
	char ss[3];
public:
	CRC8() { 
		val = 0x00;
		unsigned char crc;
		for ( int i = 0; i < 256; i++) {
			crc = i;
			for (int j = 0; j < 8; j++)
				crc = (crc << 1) ^ ((crc & 0x80) ? 0x07 : 0);
			table[i] = crc & 0xFF;
		}
	}
	~CRC8() {};
	std::string sval(std::string s) {
		val = 0x00;
		for ( size_t i = 0; i < s.length(); i++ ) {
			val = table[(val) ^ s[i]];
			val &= 0xFF;
		}
		snprintf(ss, sizeof(ss), "%02x", val);
		return ss;
	}
};

#endif
