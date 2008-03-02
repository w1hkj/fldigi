#ifndef _MACROS_H
#define _MACROS_H

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

#include "main.h"

using namespace std;

struct CONTESTCNTR {
	int count;
	char   szCount[20];
	char   szDisp[40];
	string fmt;
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
	string name[24];
	string text[24];
	int  loadMacros(string filename);
	void loadDefault();
	void openMacroFile();
	void saveMacroFile();
	void saveMacros(string);
	string expandMacro(int n);
	void execute(int n);
	MACROTEXT() {
		changed = false;
		for (int i = 0; i < 24; i++) {
			name[i] = "";
			text[i] = "";
		}
	}
private:
	string expanded;
};

extern MACROTEXT macros;
extern CONTESTCNTR contest_count;

#endif
