// ----------------------------------------------------------------------------
// rigio.cxx
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

#include <ctime>
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
#include "rigxml.h"
#include "trx.h"
#include "serial.h"
#include "rigio.h"
#include "debug.h"
#include "threads.h"
#include "qrunner.h"
#include "confdialog.h"
#include "status.h"

LOG_FILE_SOURCE(debug::LOG_RIGCONTROL);

using namespace std;

Cserial rigio;
static pthread_mutex_t	rigCAT_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t		rigCAT_thread;

static bool			rigCAT_exit = false;
static bool			rigCAT_open = false;
static bool			rigCAT_bypass = false;

static string		sRigWidth = "";
static string		sRigMode = "";
static long long	llFreq = 0;

static bool nonCATrig = false;

static void *rigCAT_loop(void *args);

#define RXBUFFSIZE 2000
static unsigned char replybuff[RXBUFFSIZE+1];
static unsigned char retbuf[3];

bool sendCommand (string s, int retnbr)
{
	int numwrite = (int)s.length();
	int readafter = progdefaults.RigCatWait;
	int numread;
	int retval;
	numread = retnbr;
	if (progdefaults.RigCatECHO) numread += numwrite;
	readafter =
		progdefaults.RigCatWait + (int) ceilf (
			numread * (9 + progdefaults.RigCatStopbits) *
			1000.0 / rigio.Baud() );

	LOG_DEBUG("%s", str2hex(s.data(), s.length()));

	retval = rigio.WriteBuffer((unsigned char *)s.c_str(), numwrite);
	if (retval <= 0)
		LOG_VERBOSE("Write error %d", retval);

	if (retnbr == 0) return true;

	memset(replybuff, 0, RXBUFFSIZE + 1);
	numread = 0;
	MilliSleep( readafter );
	while (numread < RXBUFFSIZE) {
		memset(retbuf, 0, 2);
		if (rigio.ReadBuffer(retbuf, 1) == 0) break;
		replybuff[numread] = retbuf[0];
		numread++;
	}
	LOG_DEBUG("reply %s", str2hex(replybuff, numread));
	if (numread > retnbr) {
		memmove(replybuff, replybuff + numread - retnbr, retnbr);
		numread = retnbr;
	}

	return (numread == retnbr);
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
		f += (replybuff[p + i] >> 4) & 0x0F;
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
	long long fret = 0;
	if (d.dtype == "BCD") {
		if (d.reverse == true)
			fret = (long long int)(fm_bcd_be(p, d.size) * num / den);
		else
			fret = (long long int)(fm_bcd(p, d.size)  * num / den);
	} else if (d.dtype == "BINARY") {
		if (d.reverse == true)
			fret = (long long int)(fm_binary_be(p, d.size)  * num / den);
		else
			fret = (long long int)(fm_binary(p, d.size)  * num / den);
	} else if (d.dtype == "DECIMAL") {
		if (d.reverse == true)
			fret = (long long int)(fm_decimal_be(p, d.size)  * num / den);
		else
			fret = (long long int)(fm_decimal(p, d.size)  * num / den);
	}
	return fret;
}

long long rigCAT_getfreq(int retries, bool &failed)
{
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	size_t p = 0, len1 = 0, len2 = 0, pData = 0;
	long long f = 0;

	failed = false;
	if (nonCATrig) {
		failed = true;
		return progStatus.noCATfreq;
	}

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "GETFREQ")
			break;
		++itrCmd;
	}

	if (itrCmd == commands.end()) {
		failed = true;
		return progStatus.noCATfreq; // get_freq command is not defined!
	}

	modeCmd = *itrCmd;

	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);

	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);

	if ( !modeCmd.info.size() ) {
		failed = true;
		return 0;
	}

	for (list<XMLIOS>::iterator preply = reply.begin(); preply != reply.end(); ++preply) {
		if (preply->SYMBOL != modeCmd.info)
			continue;

		XMLIOS  rTemp = *preply;
		len1 = rTemp.str1.size();
		len2 = rTemp.str2.size();

//		for (int n = 0; n < progdefaults.RigCatRetries; n++) {
		for (int n = 0; n < retries; n++) {
			if (n && progdefaults.RigCatTimeout > 0)
				MilliSleep(progdefaults.RigCatTimeout);
// send the command
			if ( !sendCommand(strCmd, rTemp.size) ) {
				LOG_VERBOSE("sendCommand failed");
				goto retry_get_freq;
			}
// check the pre data string
			p = 0;
			pData = 0;
			if (len1) {
				for (size_t i = 0; i < len1; i++) {
					if ((char)rTemp.str1[i] != (char)replybuff[i]) {
						LOG_VERBOSE("failed pre data string test @ %" PRIuSZ, i);
						goto retry_get_freq;
					}
				}
				p = len1;
			}
			if (rTemp.fill1) p += rTemp.fill1;
			pData = p;
			if (rTemp.data.dtype == "BCD") {
				p += rTemp.data.size / 2;
				if (rTemp.data.size & 1) p++;
			} else
				p += rTemp.data.size;
// check the post data string
			if (rTemp.fill2) p += rTemp.fill2;

			if (len2) {
				for (size_t i = 0; i < len2; i++)
					if ((char)rTemp.str2[i] != (char)replybuff[p + i]) {
						LOG_VERBOSE("failed post data string test @ %d", static_cast<int>(i));
						goto retry_get_freq;
					}
			}
// convert the data field
			f = fm_freqdata(rTemp.data, pData);
			if ( f >= rTemp.data.min && f <= rTemp.data.max)
				return f;
			LOG_VERBOSE("freq: %d", static_cast<int>(f));
retry_get_freq: ;
		}
	}
	if (progdefaults.RigCatVSP == false)
		LOG_VERBOSE("Retries failed");
	failed = true;
	return 0;
}

void rigCAT_setfreq(long long f)
{
	{
		guard_lock ser_guard( &rigCAT_mutex );
		if (rigCAT_exit) return;
	}
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	progStatus.noCATfreq = f;

	if (nonCATrig) {
		return;
	}

//	LOG_DEBUG("set frequency %lld", f);

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "SETFREQ")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		LOG_VERBOSE("SET_FREQ not defined");
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
				for (int n = 0; n < progdefaults.RigCatRetries; n++) {
					MilliSleep(50);
					guard_lock ser_guard( &rigCAT_mutex );
					if (rigCAT_exit) return;
					if (sendCommand(strCmd, rTemp.size)) return;
				}
				return;
			}
			preply++;
		}
	} else {
		for (int n = 0; n < progdefaults.RigCatRetries; n++) {
			MilliSleep(50);
			guard_lock ser_guard( &rigCAT_mutex );
			if (rigCAT_exit) return;
			if (sendCommand(strCmd, 0)) return;
		}
	}
	if (progdefaults.RigCatVSP == false)
		LOG_VERBOSE("Retries failed");
}

string rigCAT_getmode()
{
	{
		guard_lock ser_guard( &rigCAT_mutex );
		if (rigCAT_exit) return "";
	}
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	list<MODE>::iterator mode;
	list<MODE> *pmode;
	string strCmd, mData;
	size_t len;

	if (nonCATrig)
		return progStatus.noCATmode;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "GETMODE")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end())
		return progStatus.noCATmode;

	modeCmd = *itrCmd;

	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);

	if (modeCmd.str2.empty() == false)
		strCmd.append(modeCmd.str2);

	if (!modeCmd.info.size()) return "";

	for (list<XMLIOS>::iterator preply = reply.begin(); preply != reply.end(); ++preply) {
		if (preply->SYMBOL != modeCmd.info)
			continue;

		XMLIOS  rTemp = *preply;

		for (int n = 0; n < progdefaults.RigCatRetries; n++) {
			if (n && progdefaults.RigCatTimeout > 0)
				MilliSleep(progdefaults.RigCatTimeout);
			size_t p = 0, pData = 0;
// send the command
			if (!sendCommand(strCmd, rTemp.size)) goto retry_get_mode;
// check the pre data string
			len = rTemp.str1.size();
			if (len) {
				for (size_t i = 0; i < len; i++)
					if ((char)rTemp.str1[i] != (char)replybuff[i]) {
						LOG_VERBOSE("failed pre data string test @ %" PRIuSZ, i);
						goto retry_get_mode;
					}
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
						goto retry_get_mode;
			}
// convert the data field
			mData = "";
			for (int i = 0; i < rTemp.data.size; i++)
				mData += (char)replybuff[pData + i];
// for FT100 and the ilk that use bit fields
			if (rTemp.data.size == 1) {
				unsigned char d = mData[0];
				if (rTemp.data.shiftbits)
					d >>= rTemp.data.shiftbits;
				d &= rTemp.data.andmask;
				mData[0] = d;
			}
			if (lmodes.empty() == false)
					pmode = &lmodes;
			else if (lmodeREPLY.empty() == false)
				pmode = &lmodeREPLY;
			else
				goto retry_get_mode;
			mode = pmode->begin();
			while (mode != pmode->end()) {
				if ((*mode).BYTES == mData)
					break;
				mode++;
			}
			if (mode != pmode->end())
				return ((*mode).SYMBOL);
retry_get_mode: ;
		}
	}
	if (progdefaults.RigCatVSP == false)
		LOG_VERBOSE("Retries failed");
	return "";
}

void rigCAT_setmode(const string& md)
{
	{
		guard_lock ser_guard( &rigCAT_mutex );
		if (rigCAT_exit) return;
	}
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	progStatus.noCATmode = md;

	if (nonCATrig) {
		return;
	}

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "SETMODE")
			break;
		++itrCmd;
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
				for (int n = 0; n < progdefaults.RigCatRetries; n++) {
					MilliSleep(50);
					guard_lock ser_guard( &rigCAT_mutex );
					if (rigCAT_exit) return;
					if (sendCommand(strCmd, rTemp.size)) return;
				}
				return;
			}
			preply++;
		}
	} else {
		for (int n = 0; n < progdefaults.RigCatRetries; n++) {
			MilliSleep(50);
			guard_lock ser_guard( &rigCAT_mutex );
			if (rigCAT_exit) return;
			if (sendCommand(strCmd, 0)) return;
		}
	}
	if (progdefaults.RigCatVSP == false)
		LOG_VERBOSE("Retries failed");
}

string rigCAT_getwidth()
{
	{
		guard_lock ser_guard( &rigCAT_mutex );
		if (rigCAT_exit) return "";
	}
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	list<BW>::iterator bw;
	list<BW> *pbw;
	string strCmd, mData;
	size_t len = 0, p = 0, pData = 0;

	if (nonCATrig)
		return progStatus.noCATwidth;

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

	if (!modeCmd.info.size()) return "";

	for (list<XMLIOS>::iterator preply = reply.begin(); preply != reply.end(); ++preply) {
		if (preply->SYMBOL != modeCmd.info)
			continue;

		XMLIOS  rTemp = *preply;
		for (int n = 0; n < progdefaults.RigCatRetries; n++) {
			if (n && progdefaults.RigCatTimeout > 0)
				MilliSleep(progdefaults.RigCatTimeout);

			p = 0;
			pData = 0;
// send the command
			if ( !sendCommand(strCmd, rTemp.size) ) goto retry_get_width;
// check the pre data string
			len = rTemp.str1.size();
			if (len) {
				for (size_t i = 0; i < len; i++)
					if ((char)rTemp.str1[i] != (char)replybuff[i]) {
						LOG_VERBOSE("failed pre data string test @ %" PRIuSZ, i);
						goto retry_get_width;
					}
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
						goto retry_get_width;
			}
// convert the data field
			mData = "";
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
			if (lbws.empty() == false)
				pbw = &lbws;
			else if (lbwREPLY.empty() == false)
				pbw = &lbwREPLY;
			else
				goto retry_get_width;
			bw = pbw->begin();
			while (bw != pbw->end()) {
				if ((*bw).BYTES == mData)
					break;
				bw++;
			}
			if (bw != pbw->end())
				return ((*bw).SYMBOL);
retry_get_width: ;
		}
	}
	if (progdefaults.RigCatVSP == false)
		LOG_VERBOSE("Retries failed");
	return "";
}

void rigCAT_setwidth(const string& w)
{
	{
		guard_lock ser_guard( &rigCAT_mutex );
		if (rigCAT_exit) return;
	}
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	if (nonCATrig) {
		progStatus.noCATwidth = w;
		return;
	}

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == "SETBW")
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		progStatus.noCATwidth = w;
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
				for (int n = 0; n < progdefaults.RigCatRetries; n++) {
					MilliSleep(50);
					guard_lock ser_guard( &rigCAT_mutex );
					if (rigCAT_exit) return;
					if (sendCommand(strCmd, rTemp.size)) return;
				}
			}
			preply++;
		}
	} else {
		for (int n = 0; n < progdefaults.RigCatRetries; n++) {
			MilliSleep(50);
			guard_lock ser_guard( &rigCAT_mutex );
			if (rigCAT_exit) return;
			if (sendCommand(strCmd, 0)) return;
		}
	}
	LOG_VERBOSE("Retries failed");
}

void rigCAT_pttON()
{
	{
		guard_lock ser_guard( &rigCAT_mutex );
		if (rigCAT_exit) return;
	}
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	rigio.SetPTT(1); // always execute the h/w ptt if enabled

	if (nonCATrig) return;

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
				for (int n = 0; n < progdefaults.RigCatRetries; n++) {
					MilliSleep(50);
					guard_lock ser_guard( &rigCAT_mutex );
					if (rigCAT_exit) return;
					if (sendCommand(strCmd, rTemp.size)) return;
				}
				return;
			}
			preply++;
		}
	} else {
		for (int n = 0; n < progdefaults.RigCatRetries; n++) {
			MilliSleep(50);
			guard_lock ser_guard( &rigCAT_mutex );
			if (rigCAT_exit) return;
			if (sendCommand(strCmd, 0)) return;
		}
	}
	LOG_VERBOSE("Retries failed");
}

void rigCAT_pttOFF()
{
	{
		guard_lock ser_guard( &rigCAT_mutex );
		if (rigCAT_exit) return;
	}
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	rigio.SetPTT(0); // always execute the h/w ptt if enabled
	if (nonCATrig) return;

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
				for (int n = 0; n < progdefaults.RigCatRetries; n++) {
					MilliSleep(50);
					guard_lock ser_guard( &rigCAT_mutex );
					if (rigCAT_exit) return;
					if (sendCommand(strCmd, rTemp.size)) return;
				}
				return;
			}
			preply++;
		}
	} else {
		for (int n = 0; n < progdefaults.RigCatRetries; n++) {
			MilliSleep(50);
			guard_lock ser_guard( &rigCAT_mutex );
			if (rigCAT_exit) return;
			if (sendCommand(strCmd, 0)) return;
		}
	}
	LOG_VERBOSE("Retries failed");
}

void rigCAT_sendINIT(const string& icmd)
{
	{
		guard_lock ser_guard( &rigCAT_mutex );
		if (rigCAT_exit) return;
	}
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	if (nonCATrig)
		return;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == icmd)
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
				for (int n = 0; n < progdefaults.RigCatRetries; n++) {
					MilliSleep(50);
					guard_lock ser_guard( &rigCAT_mutex );
					if (rigCAT_exit) return;
					if (sendCommand(strCmd, rTemp.size)) return;
				}
				return;
			}
			preply++;
		}
	} else {
		for (int n = 0; n < progdefaults.RigCatRetries; n++) {
			MilliSleep(50);
			guard_lock ser_guard( &rigCAT_mutex );
			if (rigCAT_exit) return;
			if (sendCommand(strCmd, 0)) return;
		}
	}
	LOG_VERBOSE("Retries failed");
}

void rigCAT_defaults()
{
	mnuXmlRigBaudrate->value(xmlrig.baud);
	valRigCatStopbits->value(xmlrig.stopbits);
	btnRigCatRTSplus->value(xmlrig.rts);
	btnRigCatDTRplus->value(xmlrig.dtr);
	btnRigCatRTSptt->value(xmlrig.rtsptt);
	btnRigCatDTRptt->value(xmlrig.dtrptt);
	chk_restore_tio->value(xmlrig.restore_tio);
	chkRigCatRTSCTSflow->value(xmlrig.rtscts);
	cntRigCatRetries->value(xmlrig.retries);
	cntRigCatTimeout->value(xmlrig.timeout);
	cntRigCatWait->value(xmlrig.write_delay);
	btnRigCatEcho->value(xmlrig.echo);
	btnRigCatCMDptt->value(xmlrig.cmdptt);
	chkRigCatVSP->value(xmlrig.vsp);
}

void rigCAT_restore_defaults()
{
	inpXmlRigDevice->value(progdefaults.XmlRigDevice.c_str());
	mnuXmlRigBaudrate->value(progdefaults.XmlRigBaudrate);
	valRigCatStopbits->value(progdefaults.RigCatStopbits);
	btnRigCatRTSplus->value(progdefaults.RigCatRTSplus);
	btnRigCatDTRplus->value(progdefaults.RigCatDTRplus);
	btnRigCatRTSptt->value(progdefaults.RigCatRTSptt);
	btnRigCatDTRptt->value(progdefaults.RigCatDTRptt);
	chk_restore_tio->value(progdefaults.RigCatRestoreTIO);
	chkRigCatRTSCTSflow->value(progdefaults.RigCatRTSCTSflow);
	cntRigCatRetries->value(progdefaults.RigCatRetries);
	cntRigCatTimeout->value(progdefaults.RigCatTimeout);
	cntRigCatWait->value(progdefaults.RigCatWait);
	btnRigCatEcho->value(progdefaults.RigCatECHO);
	btnRigCatCMDptt->value(progdefaults.RigCatCMDptt);
	chkRigCatVSP->value(progdefaults.RigCatVSP);

	btnInitRIGCAT->labelcolor(FL_FOREGROUND_COLOR);
	btnRevertRIGCAT->deactivate();
	dlgConfig->redraw();
}

void rigCAT_init_defaults()
{
	progdefaults.XmlRigDevice = inpXmlRigDevice->value();
	progdefaults.XmlRigBaudrate = mnuXmlRigBaudrate->value();
	progdefaults.RigCatStopbits = static_cast<int>(valRigCatStopbits->value());
	progdefaults.RigCatRTSplus = btnRigCatRTSplus->value();
	progdefaults.RigCatDTRplus = btnRigCatDTRplus->value();
	progdefaults.RigCatRTSptt = btnRigCatRTSptt->value();
	progdefaults.RigCatDTRptt = btnRigCatDTRptt->value();
	progdefaults.RigCatRestoreTIO = chk_restore_tio->value();
	progdefaults.RigCatRTSCTSflow = chkRigCatRTSCTSflow->value();
	progdefaults.RigCatRetries = static_cast<int>(cntRigCatRetries->value());
	progdefaults.RigCatTimeout = static_cast<int>(cntRigCatTimeout->value());
	progdefaults.RigCatWait = static_cast<int>(cntRigCatWait->value());
	progdefaults.RigCatECHO = btnRigCatEcho->value();
	progdefaults.RigCatCMDptt = btnRigCatCMDptt->value();
	progdefaults.RigCatVSP = chkRigCatVSP->value();
}

bool rigCAT_init(bool useXML)
{

	if (rigCAT_open == true) {
		LOG_ERROR("RigCAT already open");
		return false;
	}

	sRigMode = "";
	sRigWidth = "";

	if (useXML == true) {
		rigCAT_init_defaults();
		rigio.Device(progdefaults.XmlRigDevice);
		rigio.Baud(progdefaults.BaudRate(progdefaults.XmlRigBaudrate));
		rigio.RTS(progdefaults.RigCatRTSplus);
		rigio.DTR(progdefaults.RigCatDTRplus);
		rigio.RTSptt(progdefaults.RigCatRTSptt);
		rigio.DTRptt(progdefaults.RigCatDTRptt);
		rigio.RestoreTIO(progdefaults.RigCatRestoreTIO);
		rigio.RTSCTS(progdefaults.RigCatRTSCTSflow);
		rigio.Stopbits(progdefaults.RigCatStopbits);

		LOG_VERBOSE("\n\
Serial port parameters:\n\
device	 : %s\n\
baudrate   : %d\n\
stopbits   : %d\n\
retries	: %d\n\
timeout	: %d\n\
wait	   : %d\n\
initial rts: %+d\n\
use rts ptt: %c\n\
initial dtr: %+d\n\
use dtr ptt: %c\n\
restore tio: %c\n\
flowcontrol: %c\n\
echo	   : %c\n",
		rigio.Device().c_str(),
		rigio.Baud(),
		rigio.Stopbits(),
		progdefaults.RigCatRetries,
		progdefaults.RigCatTimeout,
		progdefaults.RigCatWait,
			(rigio.RTS() ? +12 : -12), (rigio.RTSptt() ? 'T' : 'F'),
			(rigio.DTR() ? +12 : -12), (rigio.DTRptt() ? 'T' : 'F'),
			(rigio.RestoreTIO() ? 'T' : 'F'),
			(rigio.RTSCTS() ? 'T' : 'F'),
			progdefaults.RigCatECHO ? 'T' : 'F');

		if (rigio.OpenPort() == false) {
			LOG_VERBOSE("Cannot open serial port %s", rigio.Device().c_str());
			nonCATrig = true;
			init_NoRig_RigDialog();
			return false;
		}
		sRigMode = "";
		sRigWidth = "";

		nonCATrig = false;
		rigCAT_sendINIT("INIT");

// must be able to get frequency 3 times in sequence or serial port might
// be shared with another application (flrig)
		bool failed = false;
		for (int i = 1; i <= 5; i++) {
			rigCAT_getfreq(1, failed);
			if (failed) break;
			LOG_INFO("Passed serial port test # %d", i);
//			MilliSleep(50);
		}

		if (failed) {
			LOG_INFO("Failed serial port test");
			rigio.ClosePort();
			nonCATrig = true;
			init_NoRig_RigDialog();
			return false;
		} else {
			nonCATrig = false;
			init_Xml_RigDialog();
		}
	} else { // rigcat thread just being used for the human interface
		nonCATrig = true;
		init_NoRig_RigDialog();
		llFreq = 0;
		rigCAT_bypass = false;

		if (pthread_create(&rigCAT_thread, NULL, rigCAT_loop, NULL) < 0) {
			LOG_ERROR("%s", "pthread_create failed");
			rigio.ClosePort();
			return false;
		}

		rigCAT_open = true;
		return true;
	}

	llFreq = 0;
	rigCAT_bypass = false;

	if (pthread_create(&rigCAT_thread, NULL, rigCAT_loop, NULL) < 0) {
		LOG_ERROR("%s", "pthread_create failed");
		rigio.ClosePort();
		return false;
	}

	rigCAT_open = true;

	return true;
}

void rigCAT_close(void)
{
	if ( rigCAT_open == false || rigCAT_exit == true)
		return;

	{
		guard_lock ser_guard( &rigCAT_mutex );
		rigCAT_exit = true;
	}

	LOG_INFO("%s", "Waiting for rigCAT_thread");

	pthread_join(rigCAT_thread, NULL);

	rigCAT_sendINIT("CLOSE");

	rigio.ClosePort();

	rigCAT_open = false;
	rigCAT_exit = false;
	rigCAT_bypass = false;
	wf->USB(true);

}

bool rigCAT_active(void)
{
	return (rigCAT_open);
}

void rigCAT_set_ptt(int ptt)
{
	if (rigCAT_open == false)
		return;
	if (ptt) {
		rigCAT_pttON();
		rigCAT_bypass = true;
	} else{
		rigCAT_pttOFF();
		rigCAT_bypass = false;
	}
}

#ifndef RIGCATTEST
void rigCAT_set_qsy(long long f)
{
	if (rigCAT_open == false)
		return;

	// send new freq to rig
	rigCAT_setfreq(f);
	wf->rfcarrier(f);
	wf->movetocenter();
}
#endif

bool ModeIsLSB(const string& s)
{
	if (nonCATrig) {
		if (s == "LSB" || s == "PKTLSB" || s == "CW" || s == "RTTY")
			return true;
		return false;
	}
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
	bool failed;

	for (;;) {
		MilliSleep(100);

		{
			guard_lock ser_guard( &rigCAT_mutex );

			if (rigCAT_exit == true) {
				LOG_INFO("%s", "Exit rigCAT loop");
				return NULL;
			}

			if (rigCAT_bypass == true)
				continue;

		}

		freq = rigCAT_getfreq(progdefaults.RigCatRetries, failed);

		if ((freq > 0) && (freq != llFreq)) {
			llFreq = freq;
			show_frequency(freq);
			wf->rfcarrier(freq);
		}

		sWidth = rigCAT_getwidth();
		if (sWidth.size() && sWidth != sRigWidth) {
			sRigWidth = sWidth;
			show_bw(sWidth);
		}

		sMode = rigCAT_getmode();
		if (sMode.size() && sMode != sRigMode) {
			sRigMode = sMode;
			if (ModeIsLSB(sMode))
				wf->USB(false);
			else
				wf->USB(true);
			show_mode(sMode);
		}
	}

	return NULL;
}

