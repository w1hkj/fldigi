#include <config.h>

#ifdef __CYGWIN__
	#include "winserial.cxx"
#else
	#include "linserial.cxx"
#endif
