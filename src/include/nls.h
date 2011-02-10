#ifndef NLS_H_
#define NLS_H_

#include <config.h>

#if ENABLE_NLS && defined(__WOE32__)
struct lang_def_t {
	const char* lang;
	const char* lang_region;
	const char* native_name;
	int percent_done;
};

extern struct lang_def_t ui_langs[];

int get_ui_lang(const char* homedir = NULL);
void set_ui_lang(int lang, const char* homedir = NULL);
#endif

#endif // NLS_H_
