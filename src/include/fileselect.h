// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
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

#ifndef FILESELECT_H
#define FILESELECT_H

#include <config.h>

namespace FSEL {

	void create(void);
	void destroy(void);
	const char* select(const char* title, const char* filter, const char* def = 0, int *fsel = NULL);
	const char* saveas(const char* title, const char* filter, const char* def = 0, int *fsel = NULL);
	const char* dir_select(const char* title, const char* filter, const char* def = 0);

}

#endif // FILESELECT_H
