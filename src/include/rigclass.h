//
//  Hamlib C++ bindings - API header
//  modified from the c++ bindings distributed with hamlib
//

#ifndef _RIGCLASS_H
#define _RIGCLASS_H 1

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/time.h>
// for FreeBSD compilation
#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif
#include <iostream>
#include <cstring>
#include <cstdio>
#include <list>

#include <hamlib/rig.h>
#ifndef BSD
 #include <values.h>
#endif
#include <assert.h>
#include <errno.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>

using namespace std;

class RigException {
public:
char message[80];
	RigException() { *message = 0;}
	RigException (const char * msg) {
		snprintf(message, sizeof(message), "Hamlib %s error", msg);
	}
};

class Rig {
#define NUMTRIES 5

friend int riglist_make_list(const struct rig_caps *caps, void *data);
friend class configuration;
public:
	list< const struct rig_caps *> riglist;
	list<string> rignames;
protected:
	RIG	*theRig;  // Global ref. to the rig
	list< const struct rig_caps *>::iterator prig1;
	list<string>::iterator pstr;
public:
	Rig();
	Rig(rig_model_t rig_model);
	virtual ~Rig();

	const struct rig_caps *caps;

	void	init(rig_model_t rig_model);
	
	bool	isOnLine() { return (theRig != NULL);}
	void	get_rignames();
	void	get_riglist();

// This method open the communication port to the rig
	void open(void);

// This method close the communication port to the rig
	void close(void);

	void setFreq(freq_t freq, vfo_t vfo = RIG_VFO_CURR);
	freq_t getFreq(vfo_t vfo = RIG_VFO_CURR);
	
	void setMode(rmode_t, pbwidth_t width = RIG_PASSBAND_NORMAL, vfo_t vfo = RIG_VFO_CURR);
	rmode_t getMode(pbwidth_t&, vfo_t vfo = RIG_VFO_CURR);
	
	void setPTT (ptt_t ptt, vfo_t vfo = RIG_VFO_CURR);
	ptt_t getPTT (vfo_t vfo = RIG_VFO_CURR);
	
	void setVFO(vfo_t);
	vfo_t getVFO();

	void setConf(token_t token, const char *val);
	void setConf(const char *name, const char *val);
	void getConf(token_t token, char *val);
	void getConf(const char *name, char *val);
	token_t tokenLookup(const char *name);
	pbwidth_t passbandNormal (rmode_t mode);
	pbwidth_t passbandNarrow (rmode_t mode);
	pbwidth_t passbandWide (rmode_t mode);

};

#endif
