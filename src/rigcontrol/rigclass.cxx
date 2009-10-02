//
// Hamlib C++ interface is a frontend implementing wrapper functions
// to the hamlib library
//
// derived from rigclass.cc distributed with hamlib
#include <config.h>

#include <hamlib/rig.h>
#include "rigclass.h"
#include "debug.h"

LOG_FILE_SOURCE(debug::LOG_RIGCONTROL);

#define NUMTRIES 5

using namespace std;


Rig::Rig() : rig(0) { }

Rig::Rig(rig_model_t rig_model)
{
	rig = rig_init(rig_model);
}

Rig::~Rig()
{
	close();
}

bool Rig::init(rig_model_t rig_model)
{
	close();
	rig = rig_init(rig_model);
	if (rig) {
		LOG_INFO("Initialised rig model %d: %s", rig_model, getName());
		return true;
	}
	LOG_ERROR("Could not initialize rig");
	return false;
}

const char *Rig::getName()
{
	return rig ? rig->caps->model_name : "";
}

const struct rig_caps* Rig::getCaps(void)
{
	return rig ? rig->caps : 0;
}

bool Rig::open(void)
{
	return (rig_open(rig) == RIG_OK);
}

void Rig::close(void)
{
	if (rig) {
		rig_close(rig);
		rig_cleanup(rig);
		rig = NULL;
	}
}

bool Rig::setFreq(freq_t freq, vfo_t vfo)
{
	err = 0;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((err = rig_set_freq(rig, vfo, freq)) == RIG_OK);
			return true;
	}
	return false;
 }

freq_t Rig::getFreq(bool & b, vfo_t vfo)
{
	err = 0;
	freq_t freq = 0;
	for (int i = 0; i < NUMTRIES; i++)
		if ((err = rig_get_freq(rig, vfo, &freq)) == RIG_OK)
			break;
	b = (err == RIG_OK);
	return freq;
}

bool Rig::setMode(rmode_t mode, pbwidth_t width, vfo_t vfo)
{
	err = 0;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((err = rig_set_mode(rig, vfo, mode, width)) == RIG_OK)
			return true;
	}
	return false;
}

rmode_t Rig::getMode( bool & b, pbwidth_t& width, vfo_t vfo)
{
	int err;
	rmode_t mode;
	for (int i = 0; i < NUMTRIES; i++)
		if ((err = rig_get_mode(rig, vfo, &mode, &width)) == RIG_OK)
			break;
	b = (err == RIG_OK);
	return mode;
}

bool Rig::setPTT(ptt_t ptt, vfo_t vfo)
{
	err = 0;
	for (int i = 0; i < NUMTRIES; i++)
		if ((err = rig_set_ptt(rig, vfo, ptt)) == RIG_OK)
			return true;
	return false;
}

ptt_t Rig::getPTT(bool & b, vfo_t vfo)
{
	err = 0;
	ptt_t ptt;
	for (int i = 0; i < NUMTRIES; i++)
		if ((err = rig_get_ptt(rig, vfo, &ptt)) == RIG_OK)
			break;
	b = (err == RIG_OK);
	return ptt;
}

bool Rig::setConf(token_t token, const char *val)
{
	err = rig_set_conf(rig, token, val);
	return (err == RIG_OK);
}

bool Rig::setConf(const char *name, const char *val)
{
	err = rig_set_conf(rig, tokenLookup(name), val);
	return (err == RIG_OK);
}

bool Rig::getConf(token_t token, char *val)
{
	err = rig_get_conf(rig, token, val);
	return  (err == RIG_OK);
}

bool Rig::getConf(const char *name, char *val)
{
	err = rig_get_conf(rig, tokenLookup(name), val);
	return (err == RIG_OK);
}

token_t Rig::tokenLookup(const char *name)
{
	return rig_token_lookup(rig, name);
}

pbwidth_t Rig::passbandNormal (rmode_t mode)
{
	return rig_passband_normal(rig, mode);
}

pbwidth_t Rig::passbandNarrow (rmode_t mode)
{
	return rig_passband_narrow(rig, mode);
}

pbwidth_t Rig::passbandWide (rmode_t mode)
{
	return rig_passband_wide(rig, mode);
}
