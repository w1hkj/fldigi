// ----------------------------------------------------------------------------
//      debug.h
//
// Copyright (C) 2008
//              Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUG_PSKMAIL 0

#include <config.h>

#include <string>
#include <iostream>

#include "util.h"

extern void rotate_log(std::string);

class debug
{
public:
	enum level_e {
		QUIET_LEVEL, ERROR_LEVEL, WARN_LEVEL, INFO_LEVEL,
		VERBOSE_LEVEL, DEBUG_LEVEL, LOG_NLEVELS
	};
#ifdef FLARQ_DEBUG
	enum source_e {
		LOG_ARQCONTROL = 1 << 0,
		LOG_RPC_SERVER = 1 << 1,
		LOG_OTHER = 1 << 2
	};
#else
	enum source_e {
		LOG_ARQCONTROL = 1 << 0,
		LOG_AUDIO = 1 << 1,
		LOG_MODEM = 1 << 2,
		LOG_RIGCONTROL = 1 << 3,
		LOG_RPC_CLIENT = 1 << 4,
		LOG_RPC_SERVER = 1 << 5,
		LOG_SPOTTER = 1 << 6,
		LOG_DATASOURCES = 1 << 7,
		LOG_SYNOP = 1 << 8,
		LOG_KML = 1 << 9,
		LOG_KISSCONTROL = 1 << 10,
		LOG_MACLOGGER = 1 << 11,
		LOG_FD = 1 << 12,
		LOG_N3FJP = 1 << 13,
		LOG_OTHER = 1 << 14
	};
#endif
	static void start(const char* filename);
	static void stop(void);
	static void hex_dump(const char * func, const char * data, int len);
	static void log(level_e level, const char* func, const char* srcf, int line,
			const char* format, ...) format__(printf, 5, 6);
	static void elog(const char* func, const char* srcf, int line, const char* text);
	static void show(void);
	static level_e level;
	static unsigned int mask;
private:
	static void sync_text(void*);
	debug(const char* filename);
	debug(const debug&);
	debug& operator=(const debug&);
	~debug();
	static debug* inst;
};

#define LOG(level__, source__, ...)							                \
	do {										                            \
		if (level__ <= debug::level && source__ & debug::mask)			    \
			debug::log(level__, __func__, __FILE__, __LINE__, __VA_ARGS__); \
	} while (0)


#define LOG_DEBUG(...) LOG(debug::DEBUG_LEVEL, log_source_, __VA_ARGS__)
#define LOG_VERBOSE(...) LOG(debug::VERBOSE_LEVEL, log_source_, __VA_ARGS__)
#define LOG_INFO(...) LOG(debug::INFO_LEVEL, log_source_, __VA_ARGS__)
#define LOG_WARN(...) LOG(debug::WARN_LEVEL, log_source_, __VA_ARGS__)
#define LOG_ERROR(...) LOG(debug::ERROR_LEVEL, log_source_, __VA_ARGS__)

#define LOG_HD(source__, a, b)               \
	do {									 \
		if (source__ & debug::mask)          \
			debug::hex_dump(__func__, a, b); \
	} while (0)

#define LOG_HEX(a,b) LOG_HD(log_source_, a, b)

#define LOG_PERROR(msg__)								\
	do {										\
		if (debug::ERROR_LEVEL <= debug::level && log_source_ & debug::mask)	\
			debug::elog(__func__, __FILE__, __LINE__, msg__);		\
	} while (0)

unused__ static unsigned int log_source_ = debug::LOG_OTHER;
#if defined(__GNUC__) && (__GNUC__ >= 3)
#  define LOG_FILE_SOURCE(source__)						\
	__attribute__((constructor))						\
	static void log_set_source_(void) { log_source_ = source__; }
#else
#  define LOG_FILE_SOURCE(source__)
#endif

#define LOG_SET_SOURCE(source__) log_source_ = source__

extern void mnu_debug_level_cb();
extern void btn_debug_source_cb(int n);
extern void clear_debug();
extern void set_debug_mask(int mask);

#endif // _DEBUG_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
