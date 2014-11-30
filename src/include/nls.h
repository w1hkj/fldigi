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

#ifndef NLS_H_
#define NLS_H_

#include <config.h>

#if ENABLE_NLS && defined(__WOE32__)
struct lang_def_t {
	const char* lang;
	const char* lang_region;
	const char* native_name;
};

extern struct lang_def_t ui_langs[];

int get_ui_lang(const char* homedir = NULL);
void set_ui_lang(int lang, const char* homedir = NULL);
#endif

#endif // NLS_H_
