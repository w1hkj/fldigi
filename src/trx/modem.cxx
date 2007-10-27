// modem class - base for all modems
//

#include "Config.h"
#include "modem.h"
#include "id.h"
#include "configuration.h"

#include "qrunner.h"

#include "status.h"

modem *cw_modem = 0;
modem *mfsk8_modem = 0;
modem *mfsk16_modem = 0;
modem *feld_modem = 0;
modem *feld_FMmodem = 0;
modem *feld_FM105modem = 0;
modem *feld_CMTmodem = 0;
modem *psk31_modem = 0;
modem *psk63_modem = 0;
modem *psk125_modem = 0;
modem *psk250_modem = 0;
modem *qpsk31_modem = 0;
modem *qpsk63_modem = 0;
modem *qpsk125_modem = 0;
modem *qpsk250_modem = 0;
modem *olivia_modem = 0;
modem *rtty_modem = 0;
modem *dominoex4_modem = 0;
modem *dominoex5_modem = 0;
modem *dominoex8_modem = 0;
modem *dominoex11_modem = 0;
modem *dominoex16_modem = 0;
modem *dominoex22_modem = 0;
modem *throb1_modem = 0;
modem *throb2_modem = 0;
modem *throb4_modem = 0;
modem *throbx1_modem = 0;
modem *throbx2_modem = 0;
modem *throbx4_modem = 0;
modem *wwv_modem = 0;
modem *anal_modem = 0;

trx_mode modem::get_mode()
{
	return mode;
}

modem::modem()
{
	twopi = 2.0 * M_PI;
	scptr = 0;
	freqlock = false;
	sigsearch = 0;
	bool wfrev = wf->Reverse();
	bool wfsb = wf->USB();
	reverse = wfrev ^ !wfsb;
	afcon = true;
	squelchon = true;
}

void modem::init()
{
	afcon = progdefaults.afconoff;
	squelchon = progdefaults.sqlonoff;
	squelch = progdefaults.sldrSquelchValue;
	bool wfrev = wf->Reverse();
	bool wfsb = wf->USB();
	reverse = wfrev ^ !wfsb;
	
	if (progdefaults.StartAtSweetSpot) {
		if (active_modem == cw_modem)
			set_freq(progdefaults.CWsweetspot);
		else if (active_modem == rtty_modem)
			set_freq(progdefaults.RTTYsweetspot);
		else
			set_freq(progdefaults.PSKsweetspot);
	} else if (progStatus.carrier != 0) {
			set_freq(progStatus.carrier);
			progStatus.carrier = 0;
	} else
		set_freq(wf->Carrier());
}

void modem::set_freq(double freq)
{
	frequency = freq;
	freqerr = 0.0;
	if (freqlock == false)
		tx_frequency = frequency;
	QUEUE(CMP_CB(put_freq, frequency)); //put_freq(frequency);
}

void modem::set_freqlock(bool on)
{
	if (on == false)
		tx_frequency = frequency;
	freqlock = on;
}


bool modem::freqlocked()
{
	return freqlock;
}

double modem::get_txfreq(void)
{
	return tx_frequency;
}

double modem::get_txfreq_woffset(void)
{
	return (tx_frequency - progdefaults.TxOffset);
}

int modem::get_freq()
{
	return (int)(frequency + 0.5);
}

double modem::get_bandwidth(void)
{
	return bandwidth;
}

void modem::set_bandwidth(double bw)
{
	bandwidth = bw;
	put_Bandwidth((int)bandwidth);
}

void modem::set_reverse(bool on)
{
	reverse = on ^ (!wf->USB());
}

void modem::set_metric(double m)
{
	metric = m;
}

double modem::get_metric(void)
{
	return metric;
}


bool modem::get_cwTrack()
{
	return cwTrack;
}

void modem::set_cwTrack(bool b)
{
	cwTrack = b;
}

bool modem::get_cwLock()
{
	return cwLock;
}

void modem::set_cwLock(bool b)
{
	cwLock = b;
}

double modem::get_cwRcvWPM()
{
	return cwRcvWPM;
}

double modem::get_cwXmtWPM()
{
	return cwXmtWPM;
}

void modem::set_cwXmtWPM(double wpm)
{
	cwXmtWPM = wpm;
}
	
int modem::get_samplerate(void)
{
	return samplerate;
}

void modem::set_samplerate(int smprate)
{
	samplerate = smprate;
}

mbuffer<double, 512 * 2, 2> _mdm_scdbl;

void modem::ModulateXmtr(double *buffer, int len) 
{
	scard->write_samples(buffer, len);

	if (!progdefaults.viewXmtSignal)
		return;
	for (int i = 0; i < len; i++) {
		_mdm_scdbl[scptr] = buffer[i] * 0.1;
		scptr++;
		if (scptr == 512) {
			QUEUE(CMP_CB(&waterfall::sig_data, wf, _mdm_scdbl.c_array(), 512)); //wf->sig_data(scdata, 512);
			scptr = 0;
			_mdm_scdbl.next(); // change buffers
		}
	}
}

void modem::ModulateStereo(double *left, double *right, int len)
{
	scard->write_stereo(left, right, len);

	if (!progdefaults.viewXmtSignal)
		return;
	for (int i = 0; i < len; i++) {
		_mdm_scdbl[scptr] = left[i] * 0.1;
		scptr++;
		if (scptr == 512) {
			QUEUE(CMP_CB(&waterfall::sig_data, wf, _mdm_scdbl.c_array(), 512)); //wf->sig_data(scdata, 512);
			scptr = 0;
			_mdm_scdbl.next(); // change buffers
		}
	}
}


