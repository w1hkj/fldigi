#ifndef _LOG_H
#define _LOG_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/time.h>
#include <string>

using namespace std;


class cLogfile {
public:
	enum log_t {
		LOG_RX = 0,
		LOG_TX = 1
	};
private:
	ofstream	_logfile;
	bool	retflag;
	log_t	logtype;
	string	logfilename;

public:
	cLogfile(string fname = "fldigi.log");
	~cLogfile();
	void	log_to_file(log_t type, string s);
	void	log_to_file_start();
	void	log_to_file_stop();
};
	
#endif
