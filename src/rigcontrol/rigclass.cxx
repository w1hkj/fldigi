//
// Hamlib C++ interface is a frontend implementing wrapper functions
// to the hamlib library
//
// derived from rigclass.cc distributed with hamlib
#include <config.h>

#include <hamlib/rig.h>
#include "rigclass.h"
#include "debug.h"

#define NUMTRIES 5

using namespace std;


Rig::Rig() : rig(0) { }

Rig::Rig(rig_model_t rig_model)
{
	rig = rig_init(rig_model);
   	if (!rig)
		throw RigException ("init");
}

Rig::~Rig()
{
	close();
}

void Rig::init(rig_model_t rig_model)
{
	close();
	if ((rig = rig_init(rig_model)) == NULL)
		throw RigException ("init");
	LOG_INFO("Initialised rig model %d: %s", rig_model, getName());
}

const char *Rig::getName()
{
	return rig ? rig->caps->model_name : "";
}

const struct rig_caps* Rig::getCaps(void)
{
	return rig ? rig->caps : 0;
}

void Rig::open(void)
{
	int err;
	if ((err = rig_open(rig)) != RIG_OK)
		throw RigException(err);
}

void Rig::close(void)
{
	if (rig) {
		rig_close(rig);
		rig_cleanup(rig);
		rig = NULL;
	}
}

void Rig::setFreq(freq_t freq, vfo_t vfo) 
{
	int err;
	for (int i = 0; i < NUMTRIES; i++) {
		err = rig_set_freq(rig, vfo, freq);
		if (err == RIG_OK)
			return;
	}
	throw RigException(err);
}

freq_t Rig::getFreq(vfo_t vfo)
{
	freq_t freq = 0;
	int i;
	for (i = 0; i < NUMTRIES; i++)
		if (rig_get_freq(rig, vfo, &freq) == RIG_OK)
			break;
	return freq;
}

void Rig::setMode(rmode_t mode, pbwidth_t width, vfo_t vfo) 
{
	int err;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((err = rig_set_mode(rig, vfo, mode, width)) == RIG_OK)
			return;
	}
	throw RigException(err);
}

rmode_t Rig::getMode(pbwidth_t& width, vfo_t vfo) 
{
	int err;
	rmode_t mode;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((err = rig_get_mode(rig, vfo, &mode, &width)) == RIG_OK)
			return mode;
	}
	throw RigException(err);
}

void Rig::setPTT(ptt_t ptt, vfo_t vfo)
{
	int err;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((err = rig_set_ptt(rig, vfo, ptt)) == RIG_OK)
			return;
	}
	throw RigException(err);
}

ptt_t Rig::getPTT(vfo_t vfo)
{
	int err;
	ptt_t ptt;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((err = rig_get_ptt(rig, vfo, &ptt)) == RIG_OK)
			return ptt;
	}
	throw RigException(err);
}

void Rig::setConf(token_t token, const char *val)
{
	int err = rig_set_conf(rig, token, val);
	if (err != RIG_OK)
		throw RigException(err);
}
void Rig::setConf(const char *name, const char *val)
{
	LOG_INFO("setting \"%s\" to \"%s\"", name, val);
	int err = rig_set_conf(rig, tokenLookup(name), val);
	if (err != RIG_OK)
		throw RigException(name, err);
}

void Rig::getConf(token_t token, char *val)
{
	int err = rig_get_conf(rig, token, val);
	if (err != RIG_OK)
		throw RigException(err);
}

void Rig::getConf(const char *name, char *val)
{
	int err = rig_get_conf(rig, tokenLookup(name), val);
	if (err != RIG_OK)
		throw RigException(name, err);
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
