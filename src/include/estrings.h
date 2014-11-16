#ifndef _WINERROR_STRINGS_H
#define _WINERROR_STRINGS_H

#ifdef __WIN32__

#include <winerror.h>
#include <string>

struct ESTRINGS {
	long ecode;
	std::string estring;
};

extern std::string &win_error_string(long err);

#endif

#endif
