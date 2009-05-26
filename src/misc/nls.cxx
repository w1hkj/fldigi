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
