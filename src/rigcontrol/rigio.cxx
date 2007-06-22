// ====================================================================
//
// rigio.cxx  -- thread for reading/writing to serial port
//               to which the transceiver is connected
//
// ====================================================================

#include <string>

#include "fl_digi.h"
#include "misc.h"

#include "rigsupport.h"
#include "rigdialog.h"
#include "rigxml.h"
#include "serial.h"
#include "rigio.h"
#include "configuration.h"

#include "threads.h"

using namespace std;

Cserial rigio;
static Fl_Mutex		rigCAT_mutex = PTHREAD_MUTEX_INITIALIZER;
static Fl_Thread	rigCAT_thread;

static bool				rigCAT_exit = false;
static bool				rigCAT_open = false;
static bool				rigCAT_bypass = false;
static unsigned char	replybuff[200];
static unsigned char	sendbuff[200];

static string 			sRigWidth = "";
static string 			sRigMode = "";
static long long		llFreq = 0;

static int dummy = 0;

static void *rigCAT_loop(void *args);

void printhex(string s)
{
	for (unsigned int i = 0; i < s.length(); i++) {
		std::cout << hex << (unsigned int)(s[i] & 0xFF) << " ";
	}
	std::cout << std::endl;
}

void printhex(unsigned char *s, int len)
{
	for (int i = 0; i < len; i++)
		std::cout << hex << (unsigned int)(s[i] & 0xFF) << " ";
	std::cout << std::endl;
}

bool readpending = false;
int  readtimeout;

bool hexout( string s, int retnbr)
{
// thread might call this function while a read from the rig is in process
// wait here until that processing is finished or a timeout occurs
// reset the readpending & return false if a timeout occurs
//printhex(s);
	readtimeout = 500; // 500 msec timeout
	while (readpending && readtimeout--)
		MilliSleep(1);
	if (readtimeout == 0) {
		readpending = false;
		std::cout << "rigio timeout!" << std::endl; fflush(stdout);
		return false;
	}

	int num = 0;
	memset(sendbuff,0, 200);
	for (unsigned int i = 0; i < s.length(); i++)
		sendbuff[i] = s[i];

	readpending = true;
	rigio.FlushBuffer();
	rigio.WriteBuffer(sendbuff, s.size());
	if (rig.echo == true)
		rigio.ReadBuffer (replybuff, s.size());

	memset (replybuff, 0, 200);

	if (retnbr > 0) {
		num = rigio.ReadBuffer (replybuff, retnbr > 200 ? 200 : retnbr);
//		if (num)
//			printhex(replybuff, num);
//		else
//			std::cout << "no reply" << std::endl; fflush(stdout);
	}
	
	readpending = false;

	if (retnbr == 0 || num == retnbr)
		return true;

	return false;
}

string to_bcd_be(long long freq, int len)
{
	string bcd = "";
	unsigned char a;
	int numchars = len / 2;
	if (len & 1) numchars ++;
	for (int i = 0; i < numchars; i++) {
		a = 0;
		a |= freq%10;
		freq /= 10;
		a |= (freq%10)<<4;
		freq /= 10;
		bcd += a;
	}
	return bcd;
}

string to_bcd(long long freq, int len)
{
	string bcd = "";
	string bcd_be = to_bcd_be(freq, len);
	int bcdlen = bcd_be.size();
	for (int i = bcdlen - 1; i >= 0; i--)
		bcd += bcd_be[i];
	return bcd;
}

long long fm_bcd (size_t p, int len)
{
	int i;
	long long f = 0;
	int numchars = len/2;
	if (len & 1) numchars ++;
	for (i = 0; i < numchars; i++) {
		f *=10;
		f += replybuff[p + i] >> 4;
		f *= 10;
		f += replybuff[p + i] & 0x0F;
	}
	return f;
}


long long fm_bcd_be(size_t p, int len)
{
	unsigned char temp;
	int numchars = len/2;
	if (len & 1) numchars++;
	for (int i = 0; i < numchars / 2; i++) {
		temp = replybuff[p + i];
		replybuff[p + i] = replybuff[p + numchars -1 - i];
		replybuff[p + numchars -1 - i] = temp;
	}
	return fm_bcd(p, len);
}

string to_binary_be(long long freq, int len)
{
	string bin = "";
	for (int i = 0; i < len; i++) {
		bin += freq & 0xFF;
		freq >>= 8;
	}
	return bin;
}

string to_binary(long long freq, int len)
{
	string bin = "";
	string bin_be = to_binary_be(freq, len);
	int binlen = bin_be.size();
	for (int i = binlen - 1; i >= 0; i--)
		bin += bin_be[i];
	return bin;
}

long long fm_binary(size_t p, int len)
{
	int i;
	long long f = 0;
	for (i = 0; i < len; i++) {
		f *= 256;
		f += replybuff[p + i];
	}
	return f;
}

long long fm_binary_be(size_t p, int len)
{
	unsigned char temp;
	int numchars = len/2;
	if (len & 1) numchars++;
	for (int i = 0; i < numchars / 2; i++) {
		temp = replybuff[p + i];
		replybuff[p + i] = replybuff[p + numchars -1 - i];
		replybuff[p + numchars -1 - i] = temp;
	}
	return fm_binary(p, len);
}

string to_decimal_be(long long d, int len)
{
	string sdec_be = "";
	for (int i = 0; i < len; i++) {
		sdec_be += (char)((d % 10) + '0');
		d /= 10;
	}
	return sdec_be;
}

string to_decimal(long long d, int len)
{
	string sdec = "";
	string sdec_be = to_decimal_be(d, len);
	int bcdlen = sdec_be.size();
	for (int i = bcdlen - 1; i >= 0; i--)
		sdec += sdec_be[i];
	return sdec;
}

long long fm_decimal(size_t p, int len)
{
	long long d = 0;
	for (int i = 0; i < len; i++) {
		d *= 10;
		d += replybuff[p + i] - '0';
	}
	return d;
}

long long fm_decimal_be(size_t p, int len)
{
	unsigned char temp;
	int numchars = len/2;
	if (len & 1) numchars++;
	for (int i = 0; i < numchars / 2; i++) {
		temp = replybuff[p + i];
		replybuff[p + i] = replybuff[p + numchars -1 - i];
		replybuff[p + numchars -1 - i] = temp;
	}
	return fm_decimal(p, len);
}

string to_freqdata(DATA d, long long f)
{
	int num, den;
	num = 100;
	den = (int)(d.resolution * 100);
	if (d.size == 0) return "";
	if (d.dtype == "BCD") {
		if (d.reverse == true)
			return to_bcd_be((long long int)(f * num / den), d.size);
		else
			return to_bcd((long long int)(f * num / den), d.size);
	} else if (d.dtype == "BINARY") {
		if (d.reverse == true)
			return to_binary_be((long long int)(f * num / den), d.size);
		else
			return to_binary((long long int)(f * num / den), d.size);
	} else if (d.dtype == "DECIMAL") {
		if (d.reverse == true)
			return to_decimal_be((long long int)(f * num / den), d.size);
		else
			return to_decimal((long long int)(f * num / den), d.size);
	}
	return "";
}

long long fm_freqdata(DATA d, size_t p)
{
	int num, den;
	num = (int)(d.resolution * 100);
	den = 100;
	if (d.dtype == "BCD") {
		if (d.reverse == true)
			return (long long int)(fm_bcd_be(p, d.size) * num / den);
		else
			return (long long int)(fm_bcd(p, d.size)  * num / den);
	} else if (d.dtype == "BINARY") {
		if (d.reverse == true)
			return (long long int)(fm_binary_be(p, d.size)  * num / den);
		else
			return (long long int)(fm_binary(p, d.size)  * num / den);
	} else if (d.dtype == "DECIMAL") {
		if (d.reverse == true)
			return (long long int)(fm_decimal_be(p, d.size)  * num / den);
		else
			return (long long int)(fm_decimal(p, d.size)  * num / den);
	}
	return 0;
}

long long rigCAT_getfreq()
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "GETFREQ")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end())
		return -1;
	modeCmd = *itrCmd;
	
	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);

	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);

//	if (hexout(strCmd) == false) {
//		return -1;
//	}

	if (modeCmd.info.size()) {
		list<XMLIOS>::iterator preply = reply.begin();
		while (preply != reply.end()) {
			if (preply->SYMBOL == modeCmd.info) {
				XMLIOS  rTemp = *preply;
				size_t p = 0, pData = 0;
				size_t len = rTemp.str1.size();
				
// send the command
				if (hexout(strCmd, rTemp.size ) == false)
					return -1;
				
// check the pre data string				
				if (len) {
					for (size_t i = 0; i < len; i++)
						if ((char)rTemp.str1[i] != (char)replybuff[i])
							return 0;
					p = len;
				}
				if (rTemp.fill1) p += rTemp.fill1;
				pData = p;
				if (rTemp.data.dtype == "BCD") {
					p += rTemp.data.size / 2;
					if (rTemp.data.size & 1) p++;
				} else
					p += rTemp.data.size;
// check the post data string
				len = rTemp.str2.size();
				if (rTemp.fill2) p += rTemp.fill2;
				if (len) {
					for (size_t i = 0; i < len; i++)
						if ((char)rTemp.str2[i] != (char)replybuff[p + i])
							return 0;
				}
// convert the data field
				long long f = fm_freqdata(rTemp.data, pData);
				if ( f >= rTemp.data.min && f <= rTemp.data.max)
					return f;
				else
					return 0;
			}
			preply++;
		}
	}
	return 0;
}

void rigCAT_setfreq(long long f)
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "SETFREQ")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end())
		return;
	modeCmd = *itrCmd;
	
	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);

	strCmd.append( to_freqdata(modeCmd.data, f) );

	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);

	fl_lock(&rigCAT_mutex);
		hexout(strCmd, 0);
	fl_unlock(&rigCAT_mutex);
}

string rigCAT_getmode()
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "GETMODE")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end())
		return "";
	modeCmd = *itrCmd;
	
	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);

	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);

//	if (hexout(strCmd) == false) {
//		return "";
//	}

	if (modeCmd.info.size()) {
		list<XMLIOS>::iterator preply = reply.begin();
		while (preply != reply.end()) {
			if (preply->SYMBOL == modeCmd.info) {
				XMLIOS  rTemp = *preply;
				size_t p = 0, pData = 0;
// send the command
				if (hexout(strCmd, rTemp.size) == false) {
					return "";
				}

// check the pre data string				
				size_t len = rTemp.str1.size();
				if (len) {
					for (size_t i = 0; i < len; i++)
						if ((char)rTemp.str1[i] != (char)replybuff[i])
							return "";
					p = len;
				}
				if (rTemp.fill1) p += rTemp.fill1;
				pData = p;
// check the post data string
				p += rTemp.data.size;
				len = rTemp.str2.size();
				if (rTemp.fill2) p += rTemp.fill2;
				if (len) {
					for (size_t i = 0; i < len; i++)
						if ((char)rTemp.str2[i] != (char)replybuff[p + i])
							return "";
				}
// convert the data field
				string mData = "";
				for (int i = 0; i < rTemp.data.size; i++)
					mData += (char)replybuff[pData + i];
// new for FT100 and the ilk that use bit fields
				if (rTemp.data.size == 1) {
					unsigned char d = mData[0];
					if (rTemp.data.shiftbits)
						d >>= rTemp.data.shiftbits;
					d &= rTemp.data.andmask;
					mData[0] = d;
				}
				list<MODE>::iterator mode;
				list<MODE> *pmode;
				if (lmodes.empty() == false)
					pmode = &lmodes;
				else if (lmodeREPLY.empty() == false)
					pmode = &lmodeREPLY;
				else
					return "";
				mode = pmode->begin();
				while (mode != pmode->end()) {
					if ((*mode).BYTES == mData)
						break;
					mode++;
				}
				if (mode != pmode->end())
					return ((*mode).SYMBOL);
				else
					return "";
			}
			preply++;
		}
	}

	return "";
}

void rigCAT_setmode(string md)
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "SETMODE")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end())
		return;
	modeCmd = *itrCmd;
	
	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);
	
	if ( modeCmd.data.size > 0 ) {
		list<MODE>::iterator mode;
		list<MODE> *pmode;
		if (lmodes.empty() == false)
			pmode = &lmodes;
		else if (lmodeCMD.empty() == false)
			pmode = &lmodeCMD;
		else
			return;
		mode = pmode->begin();
		while (mode != pmode->end()) {
			if ((*mode).SYMBOL == md)
				break;
			mode++;
		}
		if (mode != pmode->end())
			strCmd.append( (*mode).BYTES );
	}
	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);

	fl_lock(&rigCAT_mutex);
		hexout(strCmd, 0);
	fl_unlock(&rigCAT_mutex);
}

string rigCAT_getwidth()
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "GETBW")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end())
		return "";
	modeCmd = *itrCmd;
	
	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);

	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);

//	if (hexout(strCmd) == false) {
//		return "";
//	}

	if (modeCmd.info.size()) {
		list<XMLIOS>::iterator preply = reply.begin();
		while (preply != reply.end()) {
			if (preply->SYMBOL == modeCmd.info) {
				XMLIOS  rTemp = *preply;
				size_t p = 0, pData = 0;
// send the command
				if (hexout(strCmd, rTemp.size) == false) {
					return "";
				}
				
				
// check the pre data string				
				size_t len = rTemp.str1.size();
				if (len) {
					for (size_t i = 0; i < len; i++)
						if ((char)rTemp.str1[i] != (char)replybuff[i])
							return "";
					p = pData = len;
				}
				if (rTemp.fill1) p += rTemp.fill1;
				pData = p;
				p += rTemp.data.size;
// check the post data string
				if (rTemp.fill2) p += rTemp.fill2;
				len = rTemp.str2.size();
				if (len) {
					for (size_t i = 0; i < len; i++)
						if ((char)rTemp.str2[i] != (char)replybuff[p + i])
							return "";
				}
// convert the data field
				string mData = "";
				for (int i = 0; i < rTemp.data.size; i++)
					mData += (char)replybuff[pData + i];
// new for FT100 and the ilk that use bit fields
				if (rTemp.data.size == 1) {
					unsigned char d = mData[0];
					if (rTemp.data.shiftbits)
						d >>= rTemp.data.shiftbits;
					d &= rTemp.data.andmask;
					mData[0] = d;
				}
				list<BW>::iterator bw;
				list<BW> *pbw;
				if (lbws.empty() == false)
					pbw = &lbws;
				else if (lbwREPLY.empty() == false)
					pbw = &lbwREPLY;
				else
					return "";
				bw = pbw->begin();
				while (bw != pbw->end()) {
					if ((*bw).BYTES == mData)
						break;
					bw++;
				}
				if (bw != pbw->end())
					return ((*bw).SYMBOL);
				else
					return "";
			}
			preply++;
		}
	}
	return "";
}

void rigCAT_setwidth(string w)
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "SETBW")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end())
		return;
	modeCmd = *itrCmd;
	
	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);
	
	if ( modeCmd.data.size > 0 ) {

		list<BW>::iterator bw;
		list<BW> *pbw;
		if (lbws.empty() == false)
			pbw = &lbws;
		else if (lbwCMD.empty() == false)
			pbw = &lbwCMD;
		else
			return;
		bw = pbw->begin();
		while (bw != pbw->end()) {
			if ((*bw).SYMBOL == w)
				break;
			bw++;
		}
		if (bw != pbw->end())
			strCmd.append( (*bw).BYTES );
	}
	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);
	fl_lock(&rigCAT_mutex);
		hexout(strCmd, 0);
	fl_unlock(&rigCAT_mutex);
}

void rigCAT_pttON()
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	rigio.SetPTT(1); // always execute the h/w ptt if enabled

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "PTTON")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end())
		return;
	modeCmd = *itrCmd;
	
	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);
	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);
	hexout(strCmd, 0);
}

void rigCAT_pttOFF()
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	rigio.SetPTT(0); // always execute the h/w ptt if enabled

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "PTTOFF")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end())
		return;
	modeCmd = *itrCmd;
	
	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);
	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);
	hexout(strCmd, 0);
}

void rigCAT_sendINIT()
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "INIT")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end())
		return;
	modeCmd = *itrCmd;
	
	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);
	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);

	fl_lock(&rigCAT_mutex);
		hexout(strCmd, 0);
	fl_unlock(&rigCAT_mutex);
}

static void show_error(char * a, char * b)
{
	string msg = a;
	msg.append(": ");
	msg.append(b);
	std::cout << msg << std::endl;
}

bool rigCAT_init()
{
	if (rigCAT_open == true) {
		std::cout << "RigCAT already open file present\n"; fflush(stdout);
		return false;
	}

	if (readRigXML() == false) {
		show_error("No rig.xml file present","");
		return false;
	}

	if (rigio.OpenPort() == false) {
		show_error("Cannot open serial port", (char *)rigio.Device().c_str());
		return false;
	}
	llFreq = 0;
	sRigMode = "";
	sRigWidth = "";
	
	if (fl_create_thread(rigCAT_thread, rigCAT_loop, &dummy) < 0) {
		show_error("rig init:", "pthread_create failed");
		rigio.ClosePort();
		return false;
	} 
//	std::cout << "RigCAT on" << std::endl; fflush(stdout);

	init_Xml_RigDialog();
	rigCAT_sendINIT();
	rigCAT_open = true;
	rigCAT_exit = false;
	return true;
}

void rigCAT_close(void)
{
	int count = 200;
	if (rigCAT_open == false)
		return;
	rigCAT_exit = true;
	
//	std::cout << "Waiting for RigCAT to exit "; fflush(stdout);
	while (rigCAT_open == true) {
//		std::cout << "."; fflush(stdout);
		MilliSleep(50);
		count--;
		if (!count) {
			std::cout << "\nRigCAT stuck\n"; fflush(stdout);
			exit(0);
		}
	}
	
	fl_lock(&rigCAT_mutex);
		rigio.ClosePort();
	fl_unlock(&rigCAT_mutex);
//	std::cout << "\nExit OK" << std::endl; fflush(stdout);
}

bool rigCAT_active(void)
{
	return (rigCAT_open);
}

void rigCAT_set_ptt(int ptt)
{
	if (rigCAT_open == false)
		return;
	fl_lock(&rigCAT_mutex);
		if (ptt)
			rigCAT_pttON();
		else
			rigCAT_pttOFF();
		rigCAT_bypass = ptt;
	fl_unlock(&rigCAT_mutex);
}

void rigCAT_set_qsy(void)
{
	long long fmid;
	long long f;
	if (rigCAT_open == false)
		return;
// this is the sweet spot frequency that is the center of the PBF in the rig
		if (active_modem->get_mode() == MODE_CW)
			fmid = (long long)progdefaults.CWsweetspot;
		else if (active_modem->get_mode() == MODE_RTTY)
			fmid = (long long)progdefaults.RTTYsweetspot;
		else
			fmid = (long long)progdefaults.PSKsweetspot;
		f = wf->rfcarrier();
		if (wf->USB())
			f += (wf->carrier() - fmid);
		else
			f -= (wf->carrier() - fmid);
// send new freq to rig
		rigCAT_setfreq(f);
		
		if (active_modem->freqlocked() == true) {
			active_modem->set_freqlock(false);
			active_modem->set_freq((int)fmid);
			active_modem->set_freqlock(true);
		} else
			active_modem->set_freq((int)fmid);
		wf->rfcarrier(f);
		wf->movetocenter();
}


bool ModeIsLSB(string s)
{
	list<string>::iterator pM = LSBmodes.begin();
	while (pM != LSBmodes.end() ) {
		if (*pM == s)
			return true;
		pM++;
	}
	return false;
}

static void *rigCAT_loop(void *args)
{
	long long freq = 0L;
	string sWidth, sMode;

	for (;;) {
		MilliSleep(70);

		if (rigCAT_bypass == true)
			goto loop3;

		if (rigCAT_exit == true)
			goto exitloop;

		fl_lock(&rigCAT_mutex);
			freq = rigCAT_getfreq();
		fl_unlock(&rigCAT_mutex);
		MilliSleep(10);
		if (rigCAT_exit == true)
			goto exitloop;
		if (rigCAT_bypass == true)
			goto loop2;

		fl_lock(&rigCAT_mutex);
			sWidth = rigCAT_getwidth();
		fl_unlock(&rigCAT_mutex);
		MilliSleep(10);
		if (rigCAT_exit == true)
			goto exitloop;
		if (rigCAT_bypass == true)
			goto loop1;

		fl_lock(&rigCAT_mutex);
			sMode = rigCAT_getmode();
		fl_unlock(&rigCAT_mutex);
		MilliSleep(10);
		if (rigCAT_exit == true)
			goto exitloop;
		if (rigCAT_bypass == true)
			goto loop0;
		
		if (freq && freq != llFreq) {
			llFreq = freq;
			Fl::lock();
				FreqDisp->value(freq);
			Fl::unlock();
			wf->rfcarrier(freq);
		}
		if (sWidth.size() && sWidth != sRigWidth) {
			sRigWidth = sWidth;
			Fl::lock();
				opBW->value(sWidth.c_str());
			Fl::unlock();
		}
		if (sMode.size() && sMode != sRigMode) {
			sRigMode = sMode;
			if (ModeIsLSB(sMode))
				wf->USB(false);
			else
				wf->USB(true);
			Fl::lock();
				opMODE->value(sMode.c_str());
			Fl::unlock();
		}
//		Fl::awake();
		goto loop0;
loop3:
		MilliSleep(10);
loop2:
		MilliSleep(10);
loop1:
		MilliSleep(10);
loop0:
		continue;
	}
exitloop:
	rigCAT_open = false;
	rigCAT_exit = false;
	return NULL;
}

