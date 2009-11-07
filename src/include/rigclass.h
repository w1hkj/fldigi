//
//  Hamlib C++ bindings - API header
//  modified from the c++ bindings distributed with hamlib
//

#ifndef _RIGCLASS_H
#define _RIGCLASS_H

#include <exception>
#include <string>

#include <hamlib/rig.h>


class RigException : public std::exception
{
public:
	RigException(int e = 0)
		: err(e), msg(rigerror(e)) { }
	RigException(const char* m)
		: err(0), msg(m) { }
	RigException(const char* prefix, int e)
		: err(e), msg(std::string(prefix).append(": ").append(rigerror(e))) { }
	virtual ~RigException() throw() { }

	const char* what(void) const throw() { return msg.c_str(); }
	int error(void) const { return err; }

protected:
	int		err;
	std::string	msg;
};


class Rig {
protected:
	RIG	*rig;  // Global ref. to the rig
	freq_t fnull;
public:
	Rig();
	Rig(rig_model_t rig_model);
	virtual ~Rig();

	void	init(rig_model_t rig_model);

	bool	isOnLine() { return rig; }

// This method open the communication port to the rig
	void open(void);

// This method close the communication port to the rig
	void close(void);

	void setFreq(freq_t freq, vfo_t vfo = RIG_VFO_CURR);
	freq_t getFreq(vfo_t vfo = RIG_VFO_CURR);
	bool canSetFreq();
	bool canGetFreq();

	void setMode(rmode_t, pbwidth_t width = RIG_PASSBAND_NORMAL, vfo_t vfo = RIG_VFO_CURR);
	rmode_t getMode(pbwidth_t&, vfo_t vfo = RIG_VFO_CURR);
	bool canSetMode();
	bool canGetMode();

	void setPTT (ptt_t ptt, vfo_t vfo = RIG_VFO_CURR);
	ptt_t getPTT (vfo_t vfo = RIG_VFO_CURR);
	bool canSetPTT();
	bool canGetPTT();

	void setVFO(vfo_t);
	vfo_t getVFO();

	void setConf(token_t token, const char *val);
	void setConf(const char *name, const char *val);
	void getConf(token_t token, char *val);
	void getConf(const char *name, char *val);
	const char *getName();
	const struct rig_caps* getCaps(void);

	token_t tokenLookup(const char *name);
	pbwidth_t passbandNormal (rmode_t mode);
	pbwidth_t passbandNarrow (rmode_t mode);
	pbwidth_t passbandWide (rmode_t mode);

};

#endif
