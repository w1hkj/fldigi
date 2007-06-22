//
// Hamlib C++ interface is a frontend implementing wrapper functions
// to the hamlib library
//
// derived from rigclass.cc distributed with hamlib

#include <hamlib/rig.h>
#include "rigclass.h"

using namespace std;

bool riglist_compare_func(const void * a, const void * b);

// ----------------------------------------------------------------------

int riglist_addto_list(const struct rig_caps *caps, void *data)
{
	Rig *rig = (Rig *)data;
	(rig->riglist).push_back(caps);
	return 1;
}

bool riglist_compare_func(const void *a, const void *b)
{
	const struct rig_caps *rig1 = (const struct rig_caps *)a;
	const struct rig_caps *rig2 = (const struct rig_caps *)b;
	int ret;

	ret = strcmp(rig1->mfg_name, rig2->mfg_name);
	if (ret > 0) return false;
	if (ret < 0) return true;
	ret = strcmp(rig1->model_name, rig2->model_name);
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
		rig_name_model = (*prig1)->mfg_name;
		rig_name_model.append( (*prig1)->model_name );
		rignames.push_back(rig_name_model);
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
	rig_open(theRig);
}

void Rig::close(void) 
{
	rig_close(theRig);
	theRig = NULL;
}

void Rig::setFreq(freq_t freq, vfo_t vfo) 
{
	for (int i = 0; i < NUMTRIES; i++) {
		if (rig_set_freq(theRig, vfo, freq) == RIG_OK)
			return;
	}
	throw RigException ("setFreq");
}

freq_t Rig::getFreq(vfo_t vfo)
{
	freq_t freq;
	for (int i = 0; i < NUMTRIES; i++) {
		if ( rig_get_freq(theRig, vfo, &freq) == RIG_OK)
			return freq;
	}
	throw RigException ("getFreq");
}

void Rig::setMode(rmode_t mode, pbwidth_t width, vfo_t vfo) 
{
	for (int i = 0; i < NUMTRIES; i++) {
		if (rig_set_mode(theRig, vfo, mode, width) == RIG_OK)
			return;
	}
	throw RigException ("setFreq");
}

rmode_t Rig::getMode(pbwidth_t& width, vfo_t vfo) 
{
	rmode_t mode;
	for (int i = 0; i < NUMTRIES; i++) {
		if (rig_get_mode(theRig, vfo, &mode, &width) == RIG_OK)
			return mode;
	}
	throw RigException ("getMode");
}

void Rig::setPTT(ptt_t ptt, vfo_t vfo)
{
	for (int i = 0; i < NUMTRIES; i++) {
		if (rig_set_ptt(theRig, vfo, ptt) == RIG_OK)
			return;
	}
	throw RigException ("setPTT");
}

ptt_t Rig::getPTT(vfo_t vfo)
{
	ptt_t ptt;
	int erc;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((erc = rig_get_ptt(theRig, vfo, &ptt)) == RIG_OK)
			return ptt;
	}
	throw RigException ("getPTT");
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

