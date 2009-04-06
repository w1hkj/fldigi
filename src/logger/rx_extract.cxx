// ====================================================================
//  rx_extract.cxx extract delineated data stream to file
//
// Copyright W1HKJ, Dave Freese 2006
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems to "w1hkj@w1hkj.com".
//
// ====================================================================

#include <config.h>

#include <iostream>
#include <fstream>
#include <string>

#include "rx_extract.h"
#include "main.h"
#include "status.h"
#include "fl_digi.h"
#include "configuration.h"

using namespace std;

const char *beg = "[WRAP:beg]";
const char *end = "[WRAP:end]";
const char *txtWrapInfo = "\
Detect the occurance of [WRAP:beg] and [WRAP:end]\n\
Save tags and all enclosed text to date-time stamped files placed\n\
in the folder 'wrap' located in the fldigi files folder, ie:\n\n\
    fldigi.files\\wrap\\extract-20090127-0925.wrap (Windows)\n\
    ~/.fldigi/wrap/extract-20090127-0925.wrap (Linux, OS X, Free BSD)"; 

#define   bufsize  16
char  rx_extract_buff[bufsize + 1];
string rx_buff;
string rx_extract_msg;
bool extracting = false;
bool bInit = false;

char dttm[64];

void rx_extract_reset()
{
	rx_buff.clear();
	memset(rx_extract_buff, ' ', bufsize);
	rx_extract_buff[bufsize] = 0;
	extracting = false;
}

void rx_extract_add(int c)
{
	if (!c) return;
	
	if (!bInit) {
		rx_extract_reset();
		bInit = true;
	}	
	char ch = (char)c;

	memmove(rx_extract_buff, &rx_extract_buff[1], bufsize - 1);
	rx_extract_buff[bufsize - 1] = ch;

	if ( strstr(rx_extract_buff, beg) != NULL ) {
		rx_buff = rx_extract_buff;
		rx_extract_msg = "Extracting";
		
		put_status(rx_extract_msg.c_str(), 60, STATUS_CLEAR);
		
		memset(rx_extract_buff, ' ', bufsize);
		extracting = true;
	} else if (extracting) {
		rx_buff += ch;
		if (strstr(rx_extract_buff, end) != NULL) {
			struct tm tim;
			time_t t;
			time(&t);
	        gmtime_r(&t, &tim);
			strftime(dttm, sizeof(dttm), "%Y%m%d-%H%M", &tim);
			
			string outfilename = WrapDir;
			outfilename.append("extract-");
			outfilename.append(dttm);
			outfilename.append(".wrap");			
			ofstream extractstream(outfilename.c_str(), ios::binary);
			if (extractstream) {
				extractstream << rx_buff;
				extractstream.close();
			}
			rx_extract_msg = "File saved in temp folder";
			put_status(rx_extract_msg.c_str(), 20, STATUS_CLEAR);
			
			rx_extract_reset();
		} else if (rx_buff.length() > 16384) {
			rx_extract_msg = "Extract length exceeded 16384 bytes";
			put_status(rx_extract_msg.c_str(), 20, STATUS_CLEAR);
			rx_extract_reset();
		}
	}
}
