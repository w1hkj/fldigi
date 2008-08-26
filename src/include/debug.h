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

#ifndef DEBUG_H_
#define DEBUG_H_


class debug
{
public:
	enum level_e { QUIET, ERROR, WARN, INFO, DEBUG, NLEVELS };
	static void start(const char* filename);
	static void stop(void);
	static void log(level_e level, const char* func, const char* srcf, int line, const char* format, ...);
	static void elog(const char* func, const char* srcf, int line, const char* text);
	static void show(void);
	static level_e level;
private:
	static void sync_text(void*);
	debug(const char* filename);
	debug(const debug&);
	debug& operator=(const debug&);
	~debug();
	static debug* inst;
};

#define LOG(level__, ...)						\
	do {								\
		if (level__ <= debug::level)				\
			debug::log(level__, __func__, __FILE__, __LINE__, __VA_ARGS__); \
	} while (0)

#define LOG_DEBUG(...) LOG(debug::DEBUG, __VA_ARGS__)
#define LOG_INFO(...) LOG(debug::INFO, __VA_ARGS__)
#define LOG_WARN(...) LOG(debug::WARN, __VA_ARGS__)
#define LOG_ERROR(...) LOG(debug::ERROR, __VA_ARGS__)

#define LOG_PERROR(msg__)						\
	do {								\
		if (debug::ERROR <= debug::level)			\
			debug::elog(__func__, __FILE__, __LINE__, msg__); \
	} while (0)



#endif // DEBUG_H_

// Local Variables:
// mode: c++
// c-file-style: "linux"
// End:
