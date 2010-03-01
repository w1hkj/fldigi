// ----------------------------------------------------------------------------
// rigclass.cxx
//
// Hamlib C++ interface is a frontend implementing wrapper functions
// to the hamlib library
//
// derived from rigclass.cc distributed with hamlib
//
// Copyright (C) 2007-2009
//		Dave Freese, W1HKJ
//
// This file is part of fldigi.
//
// Fldigi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

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
	fnull = 3580.0;
   	if (!rig)
		throw RigException ("Could not initialize rig");
}

Rig::~Rig()
{
	close();
}

void Rig::init(rig_model_t rig_model)
{
	close();
	if ((rig = rig_init(rig_model)) == NULL)
		throw RigException ("Could not initialize rig");
	LOG_VERBOSE("Initialised rig model %d: %s", rig_model, getName());
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
	int err = rig_open(rig);
	if (err != RIG_OK)
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

bool Rig::canSetFreq()
{
	if (!rig) return false;
	return (rig->caps->set_freq != NULL);
}

bool Rig::canGetFreq()
{
	if (!rig) return false;
	return (rig->caps->get_freq != NULL);
}

bool Rig::canSetMode()
{
	if (!rig) return false;
	return (rig->caps->set_mode != NULL);
}

bool Rig::canGetMode()
{
	if (!rig) return false;
	return (rig->caps->get_mode != NULL);
}

bool Rig::canSetPTT()
{
	if (!rig) return false;
	return (rig->caps->set_ptt != NULL);
}

bool Rig::canGetPTT()
{
	if (!rig) return false;
	return (rig->caps->get_ptt != NULL);
}

void Rig::setFreq(freq_t freq, vfo_t vfo) 
{
	fnull = freq;
	if (!canSetFreq()) { // rig does not support set_freq
		return;
	}
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
	if (!canGetFreq()) { // rig does not support get_freq
		return fnull;
	}
	freq_t freq = fnull;
	int i;
	for (i = 0; i < NUMTRIES; i++)
		if (rig_get_freq(rig, vfo, &freq) == RIG_OK)
			break;
	return freq;
}

void Rig::setMode(rmode_t mode, pbwidth_t width, vfo_t vfo) 
{
	if (!canSetMode())
		throw RigException(RIG_ENAVAIL);
	int err;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((err = rig_set_mode(rig, vfo, mode, width)) == RIG_OK)
			return;
	}
	throw RigException(err);
}

rmode_t Rig::getMode(pbwidth_t& width, vfo_t vfo) 
{
	if (!canGetMode())
		throw RigException(RIG_ENAVAIL);
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
	if (!canSetPTT())
		throw RigException(RIG_ENAVAIL);
	int err;
	for (int i = 0; i < NUMTRIES; i++) {
		if ((err = rig_set_ptt(rig, vfo, ptt)) == RIG_OK)
			return;
	}
	throw RigException(err);
}

ptt_t Rig::getPTT(vfo_t vfo)
{
	if (!canGetPTT())
		throw RigException(RIG_ENAVAIL);
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
	LOG_VERBOSE("setting \"%s\" to \"%s\"", name, val);
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
