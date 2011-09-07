#ifndef _MACROS_H
#define _MACROS_H

#include <string>

#define NUMMACKEYS 12
#define NUMKEYROWS 2
#define MAXKEYROWS 4
#define MAXMACROS (MAXKEYROWS * NUMMACKEYS)

struct CONTESTCNTR {
	int count;
	char   szCount[20];
	char   szDisp[40];
	std::string fmt;
	CONTESTCNTR() {
		count = 0;
		fmt = "%04d";
	}
	void Format(int ndigits, bool lz) {
		char c;
		fmt = '%';
		if (lz) fmt = fmt + '0';
		c = '0' + ndigits;
		fmt = fmt + c;
		fmt = fmt + 'd';
	}
};

struct MACROTEXT {
	bool	changed;
	std::string name[MAXMACROS];
	std::string text[MAXMACROS];
	int  loadMacros(const std::string& filename);
	void loadDefault();
	void openMacroFile();
	void saveMacroFile();
	void saveMacros(const std::string& fname);
	std::string expandMacro(int n);
	void execute(int n);
	void repeat(int n);
	MACROTEXT();
private:
	std::string expanded;
	void loadnewMACROS(std::string& s, size_t &i, size_t endbracket);
};

extern MACROTEXT macros;
extern CONTESTCNTR contest_count;

extern std::string info1msg;
extern std::string info2msg;
extern std::string qso_time;
extern std::string qso_exchange;

void set_macro_env(void);

void queue_execute();
bool queue_must_rx();
void idleTimer(void *);

#endif
