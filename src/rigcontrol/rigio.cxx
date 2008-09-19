// ====================================================================
//
// rigio.cxx  -- thread for reading/writing to serial port
//               to which the transceiver is connected
//
// ====================================================================

#include <config.h>

#include <ctime>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <iostream>
#include <list>
#include <vector>
#include <string>

#ifdef RIGCATTEST
	#include "rigCAT.h"
#else
	#include "fl_digi.h"
	#include "misc.h"
	#include "configuration.h"
#endif

#include "rigsupport.h"
#include "rigdialog.h"
#include "rigxml.h"
#include "serial.h"
#include "rigio.h"
#include "debug.h"
#include "threads.h"
#include "qrunner.h"

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
static bool nonCATrig = false;
long long  noCATfreq = 0L;
string noCATmode = "USB";
string noCATwidth = "";

static void *rigCAT_loop(void *args);

static const char hexsym[] = "0123456789ABCDEF";

const string& printhex(const unsigned char* s, size_t len)
{
	static string hex;
	if (unlikely(len == 0))
		return hex.erase();

	hex.resize(len * 3 - 1);
	string::iterator i = hex.begin();
	size_t j;
	for (j = 0; j < len-1; j++) {
		*i++ = hexsym[s[j] >> 4];
		*i++ = hexsym[s[j] & 0xF];
	        *i++ = ' ';
	}
        *i++ = hexsym[s[j] >> 4];
	*i = hexsym[s[j] & 0xF];

	return hex;
}

const string& printhex(const string& s)
{
	return printhex((const unsigned char*)s.data(), s.length());
}

bool readpending = false;
int  readtimeout;

bool hexout(const string& s, int retnbr)
{
// thread might call this function while a read from the rig is in process
// wait here until that processing is finished or a timeout occurs
// reset the readpending & return false if a timeout occurs

	LOG_DEBUG("cmd = %s", printhex(s).c_str());

	readtimeout = (rig.wait +rig.timeout) * rig.retries + 2000; // 2 second min timeout
	while (readpending && readtimeout--)
		MilliSleep(1);
	if (readtimeout == 0) {
		readpending = false;
		LOG_ERROR("rigio timeout!");
		return false;
	}

	readpending = true;
	
	for (int n = 0; n < rig.retries; n++) {	
		int num = 0;
		memset(sendbuff,0, 200);
		for (unsigned int i = 0; i < s.length(); i++)
			sendbuff[i] = s[i];

		rigio.FlushBuffer();
		rigio.WriteBuffer(sendbuff, s.size());
		if (rig.echo == true) {
//#ifdef __CYGWIN__
			MilliSleep(10);
//#endif
			num = rigio.ReadBuffer (replybuff, s.size());
			LOG_DEBUG("echoed = %s", printhex(replybuff, num).c_str());
		}

		memset (replybuff, 0, 200);
	
// wait interval before trying to read response
		if ((readtimeout = rig.wait) > 0)
			while (readtimeout--)
				MilliSleep(1);

		LOG_DEBUG("waiting for %d bytes", retnbr);

		if (retnbr > 0) {
			num = rigio.ReadBuffer (replybuff, retnbr > 200 ? 200 : retnbr);

// debug code
			if (num)
				LOG_DEBUG("resp (%d) = %s", n, printhex(replybuff, num).c_str());
			else
				LOG_ERROR("resp (%d) no reply", n);
		}

		if (retnbr == 0 || num == retnbr) {
			readpending = false;
			return true;
//
		if ((readtimeout = rig.timeout) > 0)
			while (readtimeout--)
				MilliSleep(1);

		}
	}

	readpending = false;
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

	if (nonCATrig == true) {
		return noCATfreq;
	}
	
	LOG_DEBUG("get frequency");

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "GETFREQ")
			break;
		++itrCmd;
	}
	
	if (itrCmd == commands.end()) {
		LOG_DEBUG("Cmd not defined");
		nonCATrig = true;
		return -2; // get_freq command is not defined!
	}

	modeCmd = *itrCmd;
	
	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);

	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);

	if (modeCmd.info.size()) {
		list<XMLIOS>::iterator preply = reply.begin();
		while (preply != reply.end()) {
			if (preply->SYMBOL == modeCmd.info) {
				XMLIOS  rTemp = *preply;
				size_t p = 0, pData = 0;
				size_t len = rTemp.str1.size();
				
// send the command
				if (hexout(strCmd, rTemp.size ) == false) {
					return -1;
				}
				
// check the pre data string				
				if (len) {
					for (size_t i = 0; i < len; i++)
						if ((char)rTemp.str1[i] != (char)replybuff[i]) {
							return 0;
						}
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
						if ((char)rTemp.str2[i] != (char)replybuff[p + i]) {
							return 0;
						}
				}
// convert the data field
				long long f = fm_freqdata(rTemp.data, pData);
				if ( f >= rTemp.data.min && f <= rTemp.data.max) {
					return f;
				}
				else {
					return 0;
				}
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

	LOG_DEBUG("set frequency %ld", f);

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "SETFREQ")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		noCATfreq = f;
		return;
	}
	
	modeCmd = *itrCmd;
	
	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);

	strCmd.append( to_freqdata(modeCmd.data, f) );

	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);

	if (modeCmd.ok.size()) {
		list<XMLIOS>::iterator preply = reply.begin();
		while (preply != reply.end()) {
			if (preply->SYMBOL == modeCmd.ok) {
				XMLIOS  rTemp = *preply;
// send the command
				fl_lock(&rigCAT_mutex);
					hexout(strCmd, rTemp.size);
				fl_unlock(&rigCAT_mutex);
				return;
			}
			preply++;
		}
	} else {
		fl_lock(&rigCAT_mutex);
			hexout(strCmd, 0);
		fl_unlock(&rigCAT_mutex);
	}
}

string rigCAT_getmode()
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	LOG_DEBUG("get mode");

	if (nonCATrig == true) 
		return noCATmode;

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

void rigCAT_setmode(const string& md)
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	LOG_DEBUG("set mode %s", md.c_str());

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "SETMODE")
			break;
		++itrCmd;
	}
	if (nonCATrig == true) {
		noCATmode = md;
		return;
	}
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

	if (modeCmd.ok.size()) {
		list<XMLIOS>::iterator preply = reply.begin();
		while (preply != reply.end()) {
			if (preply->SYMBOL == modeCmd.ok) {
				XMLIOS  rTemp = *preply;
// send the command
				fl_lock(&rigCAT_mutex);
					hexout(strCmd, rTemp.size);
				fl_unlock(&rigCAT_mutex);
				return;
			}
			preply++;
		}
	} else {
		fl_lock(&rigCAT_mutex);
			hexout(strCmd, 0);
		fl_unlock(&rigCAT_mutex);
	}
}

string rigCAT_getwidth()
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	if (nonCATrig == true) 
		return noCATwidth;
	
	LOG_DEBUG("get width");

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

void rigCAT_setwidth(const string& w)
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	LOG_DEBUG("set width");

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "SETBW")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		noCATwidth = w;
		return;
	}
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
		
	if (modeCmd.ok.size()) {
		list<XMLIOS>::iterator preply = reply.begin();
		while (preply != reply.end()) {
			if (preply->SYMBOL == modeCmd.ok) {
				XMLIOS  rTemp = *preply;
// send the command
				fl_lock(&rigCAT_mutex);
					hexout(strCmd, rTemp.size);
				fl_unlock(&rigCAT_mutex);
				return;
			}
			preply++;
		}
	} else {
		fl_lock(&rigCAT_mutex);
			hexout(strCmd, 0);
		fl_unlock(&rigCAT_mutex);
	}
}

void rigCAT_pttON()
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	LOG_DEBUG("ptt ON");

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

	if (modeCmd.ok.size()) {
		list<XMLIOS>::iterator preply = reply.begin();
		while (preply != reply.end()) {
			if (preply->SYMBOL == modeCmd.ok) {
				XMLIOS  rTemp = *preply;
// send the command
				hexout(strCmd, rTemp.size);
				return;
			}
			preply++;
		}
	} else {
		hexout(strCmd, 0);
	}
	
}

void rigCAT_pttOFF()
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	LOG_DEBUG("ptt OFF");

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

	if (modeCmd.ok.size()) {
		list<XMLIOS>::iterator preply = reply.begin();
		while (preply != reply.end()) {
			if (preply->SYMBOL == modeCmd.ok) {
				XMLIOS  rTemp = *preply;
// send the command
				hexout(strCmd, rTemp.size);
				return;
			}
			preply++;
		}
	} else {
		hexout(strCmd, 0);
	}
}

void rigCAT_sendINIT()
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	
	LOG_DEBUG("INIT rig");

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
	
	if (modeCmd.ok.size()) {
		list<XMLIOS>::iterator preply = reply.begin();
		while (preply != reply.end()) {
			if (preply->SYMBOL == modeCmd.ok) {
				XMLIOS  rTemp = *preply;
// send the command
				fl_lock(&rigCAT_mutex);
					hexout(strCmd, rTemp.size);
				fl_unlock(&rigCAT_mutex);
				return;
			}
			preply++;
		}
	} else {
		fl_lock(&rigCAT_mutex);
			hexout(strCmd, 0);
		fl_unlock(&rigCAT_mutex);
	}
//	fl_lock(&rigCAT_mutex);
//		hexout(strCmd, 0);
//	fl_unlock(&rigCAT_mutex);
}

bool rigCAT_init()
{
	
	if (rigCAT_open == true) {
		LOG_ERROR("RigCAT already open file present");
		return false;
	}

	if (readRigXML() == false) {
		LOG_ERROR("No rig.xml file present");
		return false;
	}

	if (rigio.OpenPort() == false) {
		LOG_ERROR("Cannot open serial port %s", rigio.Device().c_str());
		return false;
	}
	llFreq = 0;
	sRigMode = "";
	sRigWidth = "";
	
	nonCATrig = false;
	
	if (rigCAT_getfreq() == -1) {
		LOG_ERROR("Xcvr Freq request not answered");
		rigio.ClosePort();
		return false;
	}
	
	rigCAT_sendINIT();
	
	rigCAT_bypass = false;
	rigCAT_exit = false;

	if (fl_create_thread(rigCAT_thread, rigCAT_loop, &dummy) < 0) {
		LOG_ERROR("pthread_create failed");
		rigio.ClosePort();
		return false;
	} 

	init_Xml_RigDialog();

	rigCAT_open = true;
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
			LOG_ERROR("RigCAT stuck");
			fl_lock(&rigCAT_mutex);
			rigio.ClosePort();
			fl_unlock(&rigCAT_mutex);
			exit(0);
		}
	}
	rigio.ClosePort();
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
		if (ptt) {
			rigCAT_pttON();
			rigCAT_bypass = true;
		} else{
			rigCAT_pttOFF();
			rigCAT_bypass = false;
		}
	fl_unlock(&rigCAT_mutex);
}

#ifndef RIGCATTEST
void rigCAT_set_qsy(long long f, long long fmid)
{
	if (rigCAT_open == false)
		return;

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
#endif

bool ModeIsLSB(const string& s)
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
	SET_THREAD_ID(RIGCTL_TID);

	long long freq = 0L;
	string sWidth, sMode;

	for (;;) {
		MilliSleep(100);

		if (rigCAT_bypass == true)
			continue;
		if (rigCAT_exit == true)
			break;

		fl_lock(&rigCAT_mutex);
			freq = rigCAT_getfreq();
		fl_unlock(&rigCAT_mutex);

		if ((freq > 0) && (freq != llFreq)) {
			llFreq = freq;
			FreqDisp->value(freq); // REQ is built in to the widget
			wf->rfcarrier(freq);
		}

		if (rigCAT_bypass == true)
			continue;
		if (rigCAT_exit == true)
			break;

		fl_lock(&rigCAT_mutex);
			sWidth = rigCAT_getwidth();
		fl_unlock(&rigCAT_mutex);
		
		if (sWidth.size() && sWidth != sRigWidth) {
			sRigWidth = sWidth;
			REQ(&Fl_ComboBox::put_value, opBW, sWidth.c_str());
		}

		if (rigCAT_bypass == true)
			continue;
		if (rigCAT_exit == true)
			break;

		fl_lock(&rigCAT_mutex);
			sMode = rigCAT_getmode();
		fl_unlock(&rigCAT_mutex);

		if (sMode.size() && sMode != sRigMode) {
			sRigMode = sMode;
			if (ModeIsLSB(sMode))
				wf->USB(false);
			else
				wf->USB(true);
			REQ(&Fl_ComboBox::put_value, opMODE, sMode.c_str());
		}
	}

	rigCAT_open = false;
	rigCAT_exit = false;
	rigCAT_bypass = false;
	if (rigcontrol)
		rigcontrol->hide();
	wf->USB(true);
	wf->rfcarrier(atoi(cboBand->value())*1000L);
	wf->setQSY(0);
	FL_LOCK();
	cboBand->show();
	btnSideband->show();
	activate_rig_menu_item(false);
	FL_UNLOCK();

	fl_lock(&rigCAT_mutex);
		rigio.ClosePort();
	fl_unlock(&rigCAT_mutex);

	return NULL;
}

