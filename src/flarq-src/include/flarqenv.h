#ifndef flarqenv_h_
#define flarqenv_h_

#include <string>

extern std::string option_help, version_text, build_text;

void generate_option_help(void);
void generate_version_text(void);
int parse_args(int argc, char** argv, int& idx);
void set_platform_ui(void);
void setup_signal_handlers(void);

#endif // flarqenv_h_
