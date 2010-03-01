// ----------------------------------------------------------------------------
//
// fileselect.h
//
// Copyright (C) 2008-2009
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#ifndef FILESELECT_H
#define FILESELECT_H

#ifdef __WOE32__
#  define FSEL_THREAD 1
#endif

class Fl_Native_File_Chooser;

class FSEL
{
public:
	static void create(void);
	static void destroy(void);
	static const char* select(const char* title, const char* filter, const char* def = 0, int* fsel = 0);
	static const char* saveas(const char* title, const char* filter, const char* def = 0, int* fsel = 0);
	static const char* dir_select(const char* title, const char* filter, const char* def = 0);
	~FSEL();
private:
	FSEL();
	FSEL(const FSEL&);
	FSEL& operator=(const FSEL&);

	const char* get_file(void);
#if FSEL_THREAD
	static void* thread_func(void* arg);
#endif
private:
	static FSEL* inst;
	Fl_Native_File_Chooser* chooser;
	int result;
};

#endif // FILESELECT_H
