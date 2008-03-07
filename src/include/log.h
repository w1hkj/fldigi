#ifndef _LOG_H
#define _LOG_H

#include <fstream>
#include <string>

class cLogfile {
public:
	enum log_t {
		LOG_RX = 0,
		LOG_TX = 1
	};
private:
	std::ofstream	_logfile;
	bool	retflag;
	log_t	logtype;
	std::string	logfilename;

public:
	cLogfile(const std::string& fname = "fldigi.log");
	void	log_to_file(log_t type, const std::string& s);
	void	log_to_file_start();
	void	log_to_file_stop();
};
	
#endif
