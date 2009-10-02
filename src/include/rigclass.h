//
//  Hamlib C++ bindings - API header
//  modified from the c++ bindings distributed with hamlib
//

#ifndef _RIGCLASS_H
#define _RIGCLASS_H

#include <string>

#include <hamlib/rig.h>

class Rig {
protected:
	RIG		*rig;
	int		err;
public:
	Rig();
	Rig(rig_model_t rig_model);
	virtual ~Rig();

	const char *error() { return rigerror(err); }

	bool init(rig_model_t rig_model);
	bool isOnLine() { return rig; }
	bool open(void);
	void close(void);

	bool setFreq(freq_t freq, vfo_t vfo = RIG_VFO_CURR);
	freq_t getFreq(bool &b, vfo_t vfo = RIG_VFO_CURR );

	bool setMode(rmode_t, pbwidth_t width = RIG_PASSBAND_NORMAL, vfo_t vfo = RIG_VFO_CURR);
	rmode_t getMode(bool &b, pbwidth_t&, vfo_t vfo = RIG_VFO_CURR);

	bool setPTT (ptt_t ptt, vfo_t vfo = RIG_VFO_CURR);
	ptt_t getPTT ( bool &b, vfo_t vfo = RIG_VFO_CURR);

	bool setConf(token_t token, const char *val);
	bool setConf(const char *name, const char *val);
	bool getConf(token_t token, char *val);
	bool getConf(const char *name, char *val);
	const char *getName();
	const struct rig_caps* getCaps(void);

	token_t tokenLookup(const char *name);
	pbwidth_t passbandNormal (rmode_t mode);
	pbwidth_t passbandNarrow (rmode_t mode);
	pbwidth_t passbandWide (rmode_t mode);

};

#endif
