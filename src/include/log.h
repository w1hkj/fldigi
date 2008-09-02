#ifndef _LOG_H
#define _LOG_H

#include <cstdio>
#include <string>

class cLogfile {
public:
	enum log_t { LOG_RX, LOG_TX, LOG_START, LOG_STOP };
private:
	FILE*	logfile;
	bool	retflag;
	log_t	logtype;

public:
	cLogfile(const std::string& fname);
	~cLogfile();
	void	log_to_file(log_t type, const std::string& s);
	void	log_to_file_start();
	void	log_to_file_stop();
};
	
#endif
