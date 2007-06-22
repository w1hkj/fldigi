/*
 *    rigMEM.cxx  --  rigMEM (rig control) interface
 *
 */

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/time.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "threads.h"
#include "misc.h"
#include "configuration.h"

#include "rigMEM.h"
#include "fl_digi.h"
#include "main.h"

/* ---------------------------------------------------------------------- */

#define PTT_OFF false
#define PTT_ON  true

static int dummy;

static Fl_Thread rigMEM_thread;
static bool rigMEM_exit = false;
static bool rigMEM_enabled;
static void *rigMEM_loop(void *args);
static bool rigMEM_PTT = PTT_OFF;
static bool rig_qsy = false;
static bool TogglePTT = false;
static bool rigMEMisPTT = false;

struct ST_SHMEM {
	int  flag;
	long freq;
	long midfreq;
}sharedmem;


struct ST_SHMEM *freqflag;
struct ST_SHMEM noshare;

void *shared_memory = (void *)0;
int  shmid = -1;

key_t hash (char *str)
{
	key_t key = 1;
	unsigned int i;
	for (i = 0; i < strlen(str); i++)
		key *= str[i];
	return key;
}

void rigMEM_init(void)
{
//  szFreqFormat[2] = '0' + TEN_TEC_LOG_PREC;
  
	rigMEM_enabled = false;
	noshare.freq = 7070000L;
	noshare.midfreq = 1000L;
	noshare.flag = 2;
	freqflag = &noshare;
//	bool fail = false;

	shmid = shmget ((key_t)1234, sizeof(sharedmem), 0666 | IPC_CREAT);

	if (shmid < 0) {
		  fl_message("shmget failed");
		  return;
		 }
 	shared_memory = shmat (shmid, (void *)0, 0);
 	if (shared_memory == (void *)-1) {
 	  fl_message("shmat failed");
 	  return;
 	}
	freqflag = (struct ST_SHMEM *) shared_memory;

	rig_qsy = false;
	rigMEM_PTT = PTT_OFF;
	TogglePTT = false;
	rigMEMisPTT = false;
	
	if (fl_create_thread(rigMEM_thread, rigMEM_loop, &dummy) < 0) {
		fl_message("rigMEM init: pthread_create failed");
		return;
	} 
//	else {
//		std::cout << "rigMEM thread\n"; fflush(stdout);
//	}
	rigMEM_enabled = true;
}

void rigMEM_close(void)
{
	if (!rigMEM_enabled) return;

// tell the rigMEM thread to kill it self
	rigMEM_exit = true;

// and then wait for it to die
	fl_join(rigMEM_thread);
//std::cout <<"rigMEM down\n"; fflush(stdout);
	rigMEM_enabled = false;
	rigMEM_exit = false;

	wf->rfcarrier(0L);
	wf->USB(true);

}

bool rigMEM_active(void)
{
	return (rigMEM_enabled);
}

bool rigMEM_CanPTT(void)
{
	return rigMEMisPTT;
}
	
void setrigMEM_PTT (bool on)
{
  rigMEM_PTT = on;
  TogglePTT = true;
}

void rigMEM_set_qsy(void)
{
	if (!rigMEM_enabled)
		return;

	rig_qsy = true;
}

static void *rigMEM_loop(void *args)
{
	int sb = true;

	freqflag->freq = 0L;
	freqflag->flag = -1;

	for (;;) {
	/* see if we are being canceled */
		if (rigMEM_exit)
			break;

//		MilliSleep(20);

		if (rig_qsy) {
			long int fmid;
			long int f0 = wf->carrier();

// this is the sweet spot frequency that is the center of the PBF in the rig
			if (active_modem->get_mode() == MODE_CW)
				fmid = (long int)progdefaults.CWsweetspot;
			else if (active_modem->get_mode() == MODE_RTTY)
				fmid = (long int)progdefaults.RTTYsweetspot;
			else
				fmid = (long int)progdefaults.PSKsweetspot;
			
			if (wf->USB())
				freqflag->freq += (f0 - fmid);
			else
				freqflag->freq -= (f0 - fmid);
			
			freqflag->flag = -2; // set frequency
			MilliSleep(20);
			if (active_modem->freqlocked() == true) {
				active_modem->set_freqlock(false);
				active_modem->set_freq((int)fmid);
				active_modem->set_freqlock(true);
			} else
				active_modem->set_freq((int)fmid);
			wf->carrier((int)fmid);
			wf->rfcarrier(freqflag->freq);
			wf->movetocenter();
			rig_qsy = false;
		} else if (TogglePTT) {
			if (rigMEM_PTT == PTT_ON)
				freqflag->flag = -3;
			else
				freqflag->flag = -4;
			TogglePTT = false;
			MilliSleep(20);
		}// else
//			MilliSleep(20);

		if (freqflag->flag >= 0) {
			freqflag->flag = -1; // read frequency
			sb = false;
		}
	/* delay for 20 msec interval */
		MilliSleep(20);

	/* if rigMEM control program running it will update the freqflag structure every 10 msec */
		if (freqflag->flag > -1) {
			if ((freqflag->flag & 0x10) == 0x10)
				rigMEMisPTT = true;
			if ((freqflag->flag & 0x0F) == 2)
				sb = false;
			else
				sb = true;
			
			if (wf->rfcarrier() != freqflag->freq) {
				wf->rfcarrier(freqflag->freq);
			}
			if (wf->USB() != sb) {
				wf->USB(sb);
			}
		}
	}

	if (shmdt(shared_memory) == -1) {
		fl_message("shmdt failed");
	}

	shmid = -1;

	/* this will exit the rigMEM thread */
	return NULL;
}

/* ---------------------------------------------------------------------- */

