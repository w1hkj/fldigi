//
// Hamlib C++ interface is a frontend implementing wrapper functions
// to the hamlib library
//
// derived from rigclass.cc distributed with hamlib
#include <config.h>

#include <list>
#include <cstring>
#include <hamlib/rig.h>
#include "rigclass.h"

#define NUMTRIES 5

using namespace std;

int riglist_addto_list(const struct rig_caps *caps, void *data)
{
	reinterpret_cast<Rig*>(data)->riglist.push_back(caps);
	return 1;
}

bool riglist_compare_func(const void *a, const void *b)
{
	const struct rig_caps *rig1 = (const struct rig_caps *)a;
	const struct rig_caps *rig2 = (const struct rig_caps *)b;
	int ret;

	ret = strcasecmp(rig1->mfg_name, rig2->mfg_name);
	if (ret > 0) return false;
	if (ret < 0) return true;
	ret = strcasecmp(rig1->model_name, rig2->model_name);
	if (ret > 0) return false;
	if (ret <= 0) return true;
	if (rig1->rig_model > rig2->rig_model)
		return false;
	return true;
}

Rig::Rig() {
	theRig = NULL;
	caps = NULL;
}

Rig::Rig(rig_model_t rig_model) {
	theRig = rig_init(rig_model);
   	if (!theRig)
		throw RigException ("init");
	caps = theRig->caps;
}

Rig::~Rig() {
	riglist.clear();
	rignames.clear();
	if (theRig)
		rig_cleanup(theRig);
}

void Rig::init(rig_model_t rig_model) {
	if (theRig) close();
	theRig = rig_init(rig_model);
	if (!theRig)
		throw RigException ("init");
	caps = theRig->caps;
}

void Rig::get_rignames()
{
	string rig_name_model;

	if (riglist.empty())
		get_riglist();

	if (!rignames.empty())
		return;

	prig1 = riglist.begin();
	
	while (prig1 != riglist.end()) {
		rig_name_model.clear();
		switch ((*prig1)->status) {
			case RIG_STATUS_ALPHA :
				rig_name_model.append((*prig1)->model_name);
				rig_name_model.append(" - Alpha");
				rignames.push_back(rig_name_model);
				break;
			case RIG_STATUS_UNTESTED :
				rig_name_model.append((*prig1)->model_name);
				rig_name_model.append(" - Untested");
				rignames.push_back(rig_name_model);
				break;
			case RIG_STATUS_BETA :
				rig_name_model.append((*prig1)->model_name);
				rig_name_model.append(" - Beta");
				rignames.push_back(rig_name_model);
				break;
			case RIG_STATUS_BUGGY : 
				break;
			case RIG_STATUS_NEW :
				rig_name_model.append((*prig1)->model_name);
				rig_name_model.append(" - New");
				rignames.push_back(rig_name_model);
				break;
			case RIG_STATUS_STABLE :
			default :
				rig_name_model.append((*prig1)->model_name);
				rig_name_model.append(" - Stable");
				rignames.push_back(rig_name_model);
		}
		prig1++;
	}		
}

void Rig::get_riglist()
{
	rig_set_debug(RIG_DEBUG_NONE);
	rig_load_all_backends();
	rig_list_foreach(riglist_addto_list, this);
	riglist.sort(riglist_compare_func);
}

void Rig::open(void) 
{
	int err;
	if ((err = rig_open(theRig)) != RIG_OK)
		throw RigException(err);
}

void Rig::close(void) 
{
	if (theRig) {
		rig_close(theRig);
		theRig = NULL;
	}
}

void Rig::setFreq(freq_t freq, vfo_t vfo) 
{
	int err;
	for (int i = 0; i < NUMTRIES; i++) {
		err = rig_set_freq(theRig, vfo, freq);
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
		if (rig_get_freq(theRig, vfo, &freq) == RIG_OK)
			break;
	return freq;
}

void Rig::setMode(rmode_t mode, pbwidth_t width, vfo_t vfo) 
{
	int err;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((err = rig_set_mode(theRig, vfo, mode, width)) == RIG_OK)
			return;
	}
	throw RigException(err);
}

rmode_t Rig::getMode(pbwidth_t& width, vfo_t vfo) 
{
	int err;
	rmode_t mode;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((err = rig_get_mode(theRig, vfo, &mode, &width)) == RIG_OK)
			return mode;
	}
	throw RigException(err);
}

void Rig::setPTT(ptt_t ptt, vfo_t vfo)
{
	int err;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((err = rig_set_ptt(theRig, vfo, ptt)) == RIG_OK)
			return;
	}
	throw RigException(err);
}

ptt_t Rig::getPTT(vfo_t vfo)
{
	int err;
	ptt_t ptt;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((err = rig_get_ptt(theRig, vfo, &ptt)) == RIG_OK)
			return ptt;
	}
	throw RigException(err);
}

void Rig::setConf(token_t token, const char *val)
{
	rig_set_conf(theRig, token, val);
}
void Rig::setConf(const char *name, const char *val)
{
	rig_set_conf(theRig, tokenLookup(name), val);
}

void Rig::getConf(token_t token, char *val)
{
	rig_get_conf(theRig, token, val);
}

void Rig::getConf(const char *name, char *val)
{
	rig_get_conf(theRig, tokenLookup(name), val);
}

token_t Rig::tokenLookup(const char *name)
{
	return rig_token_lookup(theRig, name);
}

pbwidth_t Rig::passbandNormal (rmode_t mode)
{
	return rig_passband_normal(theRig, mode);
}

pbwidth_t Rig::passbandNarrow (rmode_t mode)
{
	return rig_passband_narrow(theRig, mode);
}

pbwidth_t Rig::passbandWide (rmode_t mode)
{
	return rig_passband_wide(theRig, mode);
}

