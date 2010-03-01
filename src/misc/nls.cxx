// ----------------------------------------------------------------------------
// nls.cxx
//
// Copyright (C) 2008
//		Stéphane Fillod, F8CFE
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

#include <config.h>
#include <locale.h>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

#include "gettext.h"

int setup_nls(void)
{
	static int nls_set_up = 0;
	if (nls_set_up)
		return nls_set_up;

	setlocale (LC_MESSAGES, "");
	setlocale (LC_CTYPE, "C");
	setlocale (LC_TIME, "");
	// setting LC_NUMERIC might break the config read/write routines

	const char* ldir;
	char buf[4096];
	if (!(ldir = getenv("FLDIGI_LOCALE_DIR"))) {
		if (getcwd(buf, sizeof(buf) - strlen("/locale") - 1)) {
			strcpy(buf + strlen(buf), "/locale");
			struct stat s;
			if (stat(buf, &s) != -1 && S_ISDIR(s.st_mode))
				ldir = buf;
			else
				ldir = LOCALEDIR;
		}
	}

	bindtextdomain(PACKAGE, ldir);
	/* fltk-1.1.x only knows about Latin-1 */
	bind_textdomain_codeset(PACKAGE, "ISO-8859-1");
	textdomain(PACKAGE);

	return nls_set_up = 1;
}
