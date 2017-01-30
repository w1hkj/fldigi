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

static string		sRigWidth = "";
static string		sRigMode = "";
static long long	llFreq = 0;

static void *rigCAT_loop(void *args);

#define RXBUFFSIZE 2000
static unsigned char replybuff[RXBUFFSIZE+1];
static unsigned char retbuf[3];

bool sendCommand (string s, int retnbr, int waitval)
{
	int numwrite = (int)s.length();
	int readafter = 0;
	int numread = 0;
	int retval = 0;

	numread = retnbr;

	if (progdefaults.RigCatECHO)
		numread += numwrite;

	readafter =
		waitval + (int) ceilf (
			numread * (9 + progdefaults.RigCatStopbits) *
			1000.0 / rigio.Baud() );

	if (xmlrig.debug)
		LOG_INFO("%s",
			xmlrig.ascii ? s.c_str() : str2hex(s.data(), s.length()));

	if (xmlrig.noserial) {
		memset(replybuff, 0, RXBUFFSIZE + 1);
		numread = 0;
		return true;
	}

	retval = rigio.WriteBuffer((unsigned char *)s.c_str(), numwrite);
	if (retval <= 0)
		LOG_ERROR("Write error %d", retval);

	if (retnbr == 0) return true;

	memset(replybuff, 0, RXBUFFSIZE + 1);
	numread = 0;
	while (readafter > 50) {
		MilliSleep(50);
		Fl::awake();
		readafter -= 50;
	}
	if (readafter) {
		MilliSleep(readafter);
		Fl::awake();
	}

	while (numread < RXBUFFSIZE) {
		memset(retbuf, 0, 2);
		if (rigio.ReadBuffer(retbuf, 1) == 0) break;
		replybuff[numread] = retbuf[0];
		numread++;
	}
	if (xmlrig.debug)
		LOG_INFO("reply %s",
			xmlrig.ascii ? (const char *)(replybuff) : str2hex(replybuff, numread));
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

long long rigCAT_getfreq(int retries, bool &failed, int waitval)
{
	const char symbol[] = "GETFREQ";
	failed = false;
	if (rigCAT_exit || xmlrig.noserial || !xmlrig.xmlok) {
		failed = true;
		return progStatus.noCATfreq;
	}

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	size_t p = 0, len1 = 0, len2 = 0, pData = 0;
	long long f = 0;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}

	if (itrCmd == commands.end()) {
		failed = true;
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
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
			{
				int timeout = progdefaults.RigCatTimeout;
				while (timeout > 50) {
					MilliSleep(50);
					Fl::awake();
					timeout -= 50;
				}
				if (timeout) {
					MilliSleep(timeout);
					Fl::awake();
				}
			}
// send the command
			if ( !sendCommand(strCmd, rTemp.size, waitval) ) {
				if (xmlrig.debug)
					LOG_INFO("sendCommand failed");
				goto retry_get_freq;
			}
// check the pre data string
			p = 0;
			pData = 0;
			if (len1) {
				for (size_t i = 0; i < len1; i++) {
					if ((char)rTemp.str1[i] != (char)replybuff[i]) {
						LOG_ERROR("failed pre data string test @ %" PRIuSZ, i);
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
						LOG_ERROR("failed post data string test @ %d", static_cast<int>(i));
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
		LOG_ERROR("%s failed", symbol);
	failed = true;
	return 0;
}

void rigCAT_setfreq(long long f)
{
	const char symbol[] = "SETFREQ";
	if (rigCAT_exit || xmlrig.noserial || !xmlrig.xmlok) {
		progStatus.noCATfreq = f;
	}
	if (rigCAT_exit || !xmlrig.xmlok) return;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
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
					if (sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait)) return;
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
			if (sendCommand(strCmd, 0, progdefaults.RigCatWait)) return;
		}
	}
	if (progdefaults.RigCatVSP == false)
		LOG_ERROR("%s failed", symbol);
}

string rigCAT_getmode()
{
	const char symbol[] = "GETMODE";

	if (rigCAT_exit || xmlrig.noserial || !xmlrig.xmlok)
		return progStatus.noCATmode;

//	guard_lock ser_guard( &rigCAT_mutex );

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	list<MODE>::iterator mode;
	list<MODE> *pmode;
	string strCmd, mData;
	size_t len;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return progStatus.noCATmode;
	}

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
			if (progdefaults.RigCatTimeout > 50)
			{
				int timeout = progdefaults.RigCatTimeout;
				while (timeout > 50) {
					MilliSleep(50);
					Fl::awake();
					timeout -= 50;
				}
				if (timeout) {
					MilliSleep(timeout);
					Fl::awake();
				}
			}
			size_t p = 0, pData = 0;
// send the command
			if (!sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait)) goto retry_get_mode;
// check the pre data string
			len = rTemp.str1.size();
			if (len) {
				for (size_t i = 0; i < len; i++)
					if ((char)rTemp.str1[i] != (char)replybuff[i]) {
						LOG_ERROR("failed pre data string test @ %" PRIuSZ, i);
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
		LOG_ERROR("%s failed", symbol);
	return "";
}

void rigCAT_setmode(const string& md)
{
	const char symbol[] = "SETMODE";


	if (rigCAT_exit || xmlrig.noserial || !xmlrig.xmlok)
		progStatus.noCATmode = md;

	if (rigCAT_exit || !xmlrig.xmlok) return;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
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
					if (sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait)) return;
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
			if (sendCommand(strCmd, 0, progdefaults.RigCatWait)) return;
		}
	}
	if (progdefaults.RigCatVSP == false)
		LOG_ERROR("%s failed", symbol);
}

string rigCAT_getwidth()
{
	const char symbol[] = "GETBW";

	if (rigCAT_exit || xmlrig.noserial || !xmlrig.xmlok)
		return progStatus.noCATwidth;

	XMLIOS widthCmd;
	list<XMLIOS>::iterator itrCmd;
	list<BW>::iterator bw;
	list<BW> *pbw;
	string strCmd, mData;
	size_t len = 0, p = 0, pData = 0;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return "";
	}

	widthCmd = *itrCmd;

	if ( widthCmd.str1.empty() == false)
		strCmd.append(widthCmd.str1);

	if (widthCmd.str2.empty() == false)
		strCmd.append(widthCmd.str2);

	if (!widthCmd.info.size()) return "";

	for (list<XMLIOS>::iterator preply = reply.begin(); preply != reply.end(); ++preply) {
		if (preply->SYMBOL != widthCmd.info)
			continue;

		XMLIOS  rTemp = *preply;
		for (int n = 0; n < progdefaults.RigCatRetries; n++) {
			if (progdefaults.RigCatTimeout > 50)
			{
				int timeout = progdefaults.RigCatTimeout;
				while (timeout > 50) {
					MilliSleep(50);
					Fl::awake();
					timeout -= 50;
				}
				if (timeout) {
					MilliSleep(timeout);
					Fl::awake();
				}
			}

			p = 0;
			pData = 0;

// send the command
			if ( !sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait) ) {
				goto retry_get_width;
			}

// check the pre data string
			len = rTemp.str1.size();
			if (len) {
				for (size_t i = 0; i < len; i++)
					if ((char)rTemp.str1[i] != (char)replybuff[i]) {
						LOG_ERROR("failed pre data string test @ %" PRIuSZ, i);
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
					if ((char)rTemp.str2[i] != (char)replybuff[p + i]) {
						LOG_ERROR("failed post data string test @ %" PRIuSZ, i);
						goto retry_get_width;
					}
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
			if (!lbwREPLY.empty())
				pbw = &lbwREPLY;
			else if (lbws.empty() == false)
				pbw = &lbws;
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
		LOG_ERROR("%s failed", symbol);
	return "";
}

void rigCAT_setwidth(const string& w)
{
	const char symbol[] = "SETBW";

	if (rigCAT_exit || xmlrig.noserial || !xmlrig.xmlok)
		progStatus.noCATwidth = w;

	if (rigCAT_exit || !xmlrig.xmlok) return;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
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
					if (sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait)) return;
				}
			}
			preply++;
		}
	} else {
		for (int n = 0; n < progdefaults.RigCatRetries; n++) {
			MilliSleep(50);
			guard_lock ser_guard( &rigCAT_mutex );
			if (rigCAT_exit) return;
			if (sendCommand(strCmd, 0, progdefaults.RigCatWait)) return;
		}
	}
	LOG_ERROR("%s failed", symbol);
}

void rigCAT_pttON()
{
	const char symbol[] = "PTTON";

	if (rigCAT_exit) return;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	rigio.SetPTT(1); // always execute the h/w ptt if enabled

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return;
	}

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
					if (sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait)) return;
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
			if (sendCommand(strCmd, 0, progdefaults.RigCatWait)) return;
		}
	}
	LOG_VERBOSE("%s failed", symbol);
}

void rigCAT_pttOFF()
{
	const char symbol[] = "PTTOFF";

	if (rigCAT_exit) return;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	rigio.SetPTT(0); // always execute the h/w ptt if enabled

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return;
	}

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
					if (sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait)) return;
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
			if (sendCommand(strCmd, 0, progdefaults.RigCatWait)) return;
		}
	}
	LOG_ERROR("%s failed", symbol);
}

void rigCAT_sendINIT(const string& icmd, int multiplier)
{
	if (rigCAT_exit) return;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

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
					if (sendCommand(strCmd, rTemp.size, progdefaults.RigCatInitDelay)) return;
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
			if (sendCommand(strCmd, 0, progdefaults.RigCatInitDelay)) return;
		}
	}
	LOG_ERROR("INIT failed");
}

void rigCAT_defaults()
{
	listbox_xml_rig_baudrate->index(xmlrig.baud);
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
	cntRigCatInitDelay->value(xmlrig.init_delay);
//	cntRigCatWaitForDevice->value(xmlrig.wait_for_device);
	btnRigCatEcho->value(xmlrig.echo);
	btnRigCatCMDptt->value(xmlrig.cmdptt);
	chkRigCatVSP->value(xmlrig.vsp);
}

void rigCAT_init_defaults()
{
	progdefaults.XmlRigDevice = inpXmlRigDevice->value();
	progdefaults.XmlRigBaudrate = listbox_xml_rig_baudrate->index();
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
	progdefaults.RigCatInitDelay = static_cast<int>(cntRigCatInitDelay->value());
	progdefaults.RigCatECHO = btnRigCatEcho->value();
	progdefaults.RigCatCMDptt = btnRigCatCMDptt->value();
	progdefaults.RigCatVSP = chkRigCatVSP->value();
}

bool rigCAT_init()
{

	if (rigCAT_open == true) {
		LOG_ERROR("RigCAT already open");
		return false;
	}

	sRigMode = "";
	sRigWidth = "";

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

	LOG_INFO("\n\
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

	if (xmlrig.noserial == false && xmlrig.xmlok) {
		if (rigio.OpenPort() == false) {
			LOG_ERROR("Cannot open serial port %s", rigio.Device().c_str());
			xmlrig.xmlok = true;
		}
	}

	sRigMode = "";
	sRigWidth = "";

	if (xmlrig.wait_for_device) {
		int delay = xmlrig.wait_for_device / 10;
		while (delay) {
			MilliSleep(10);
			if (delay % 10) Fl::awake();
			delay--;
		}
	}

	rigCAT_sendINIT("INIT", progdefaults.RigCatInitDelay);
	bool failed = false;

	if (xmlrig.noserial)
		rigCAT_getfreq(1, failed, 0);
	else if (xmlrig.xmlok) {
		rigCAT_getfreq(3, failed, progdefaults.RigCatInitDelay);

		if (failed) {
			LOG_ERROR("*****************Failed to read xcvr frequency");
			if (xmlrig.noserial == false)
				rigio.ClosePort();
			xmlrig.xmlok = false;
		} else
			LOG_INFO("Passed serial port test");
	}

	llFreq = 0;

	if (pthread_create(&rigCAT_thread, NULL, rigCAT_loop, NULL) < 0) {
		LOG_ERROR("%s", "pthread_create failed");
		if (xmlrig.xmlok && !xmlrig.noserial)
			rigio.ClosePort();
		xmlrig.xmlok = false;
		return false;
	}

	LOG_INFO("Created rigCAT thread");

	rigCAT_open = true;

	init_Xml_RigDialog();

	return true;
}

void rigCAT_close(void)
{
	if ( rigCAT_open == false || rigCAT_exit == true)
		return;

	rigCAT_sendINIT("CLOSE");

	{
		guard_lock ser_guard( &rigCAT_mutex );
		rigCAT_exit = true;
	}

	LOG_INFO("%s", "Waiting for rigCAT_thread");

	pthread_join(rigCAT_thread, NULL);

	rigio.ClosePort();

	rigCAT_exit = false;
	rigCAT_open = false;
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
	} else{
		rigCAT_pttOFF();
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

bool ModeIsLSB(string s)
{
	if (connected_to_flrig) return !xmlrpc_USB();
#if USE_HAMLIB
	if (hamlib_active()) return !hamlib_USB();
#endif
	list<string>::iterator pM = LSBmodes.begin();
	while (pM != LSBmodes.end() ) {
		if (*pM == s)
			return true;
		pM++;
	}
	return false;
}

int smeter_data(DATA d, size_t p)
{
	int val = 0;

	if (d.dtype == "BCD") {
		if (d.reverse == true)
			val = (int)(fm_bcd_be(p, d.size));
		else
			val = (int)(fm_bcd(p, d.size));
	} else if (d.dtype == "BINARY") {
		if (d.reverse == true)
			val = (int)(fm_binary_be(p, d.size));
		else
			val = (int)(fm_binary(p, d.size));
	} else if (d.dtype == "DECIMAL") {
		if (d.reverse == true)
			val = (int)(fm_decimal_be(p, d.size));
		else
			val = (int)(fm_decimal(p, d.size));
	}

	size_t n;
	int sm1, sm2, val1, val2;
	for (n = 0; n < xmlrig.smeter.size() - 1; n++) {
		if ((val >= xmlrig.smeter[n].val) && (val < xmlrig.smeter[n+1].val)) break;
	}
	if (n == xmlrig.smeter.size() - 1) return 0;
	sm1 = xmlrig.smeter[n].mtr;
	sm2 = xmlrig.smeter[n+1].mtr;
	val1 = xmlrig.smeter[n].val;
	val2 = xmlrig.smeter[n+1].val;

	if (val1 == val2) return 0;
	int mtr = sm1 + (val - val1) * (sm2 - sm1) / (val2 - val1);

	return mtr;
}

static void rigcat_set_smeter(void *data)
{
	if (pwrlevel_grp->visible()) return;
	if (!smeter && !pwrmeter) return;

	if (smeter && progStatus.meters) {
		if (!smeter->visible()) {
			pwrmeter->hide();
			smeter->show();
		}
		int val = reinterpret_cast<long>(data);
		smeter->value(val);
	}
}

void rigCAT_get_smeter()
{
	const char symbol[] = "GET_SMETER";

	if (rigCAT_exit || xmlrig.noserial) return;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	size_t p = 0, len1 = 0, len2 = 0, pData = 0;
	long mtr = 0;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}

	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return;
	}

	modeCmd = *itrCmd;
	if ( !modeCmd.str1.empty() ) strCmd.append(modeCmd.str1);
	if ( !modeCmd.str2.empty() ) strCmd.append(modeCmd.str2);
	if ( !modeCmd.info.size() ) return;

	list<XMLIOS>::iterator preply;

	for (preply = reply.begin(); preply != reply.end(); ++preply) {
		if (preply->SYMBOL == modeCmd.info)
			break;
	}
	if (preply == reply.end()) return;

	XMLIOS  rTemp = *preply;
	len1 = rTemp.str1.size();
	len2 = rTemp.str2.size();

	int timeout = progdefaults.RigCatTimeout;
	while (timeout > 50) {
		MilliSleep(50);
		Fl::awake();
		timeout -= 50;
	}
	if (timeout) {
		MilliSleep(timeout);
		Fl::awake();
	}

// send the command
	if ( !sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait) ) {
		LOG_ERROR("sendCommand failed");
		return;
	}

// check the pre data string
	p = 0;
	pData = 0;
	if (len1) {
		for (size_t i = 0; i < len1; i++) {
			if ((char)rTemp.str1[i] != (char)replybuff[i]) {
					LOG_ERROR("failed pre data string test @ %" PRIuSZ, i);
					return;
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
				LOG_ERROR("failed post data string test @ %d", static_cast<int>(i));
				return;
			}
	}
// convert the data field
	mtr = smeter_data(rTemp.data, pData);
	if (xmlrig.debug) LOG_INFO("Converted %s value to %d", symbol, (int)mtr);

	REQ(rigcat_set_smeter, reinterpret_cast<void *>(mtr));
}

int pmeter_data(DATA d, size_t p)
{
	int val = 0;

	if (d.dtype == "BCD") {
		if (d.reverse == true)
			val = (int)(fm_bcd_be(p, d.size));
		else
			val = (int)(fm_bcd(p, d.size));
	} else if (d.dtype == "BINARY") {
		if (d.reverse == true)
			val = (int)(fm_binary_be(p, d.size));
		else
			val = (int)(fm_binary(p, d.size));
	} else if (d.dtype == "DECIMAL") {
		if (d.reverse == true)
			val = (int)(fm_decimal_be(p, d.size));
		else
			val = (int)(fm_decimal(p, d.size));
	}

	size_t n;
	int pm1, pm2, val1, val2;
	for (n = 0; n < xmlrig.pmeter.size() - 1; n++) {
		if ((val > xmlrig.pmeter[n].val) && (val <= xmlrig.pmeter[n+1].val)) break;
	}
	if (n == xmlrig.pmeter.size() - 1) return 0;
	pm1 = xmlrig.pmeter[n].mtr;
	pm2 = xmlrig.pmeter[n+1].mtr;
	val1 = xmlrig.pmeter[n].val;
	val2 = xmlrig.pmeter[n+1].val;

	if (val1 == val2) return 0;
	int mtr = pm1 + (val - val1) * (pm2 - pm1) / (val2 - val1);

	return mtr;
}

static void rigcat_set_pmeter(void *data)
{
	if (pwrlevel_grp->visible()) return;
	if (!smeter && !pwrmeter) return;

	if (pwrmeter && progStatus.meters) {
		if (!pwrmeter->visible()) {
			smeter->hide();
			pwrmeter->show();
		}
		int val = reinterpret_cast<long>(data);
		pwrmeter->value(val);
	}
}

void rigCAT_get_pwrmeter()
{
	const char symbol[] = "GET_PWRMETER";

	if (rigCAT_exit || xmlrig.noserial) return;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	size_t p = 0, len1 = 0, len2 = 0, pData = 0;
	long mtr = 0;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}

	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return;
	}

	modeCmd = *itrCmd;
	if ( !modeCmd.str1.empty() ) strCmd.append(modeCmd.str1);
	if ( !modeCmd.str2.empty() ) strCmd.append(modeCmd.str2);
	if ( !modeCmd.info.size() ) return;

	list<XMLIOS>::iterator preply;

	for (preply = reply.begin(); preply != reply.end(); ++preply) {
		if (preply->SYMBOL == modeCmd.info)
			break;
	}
	if (preply == reply.end()) return;

	XMLIOS  rTemp = *preply;
	len1 = rTemp.str1.size();
	len2 = rTemp.str2.size();

	int timeout = progdefaults.RigCatTimeout;
	while (timeout > 50) {
		MilliSleep(50);
		Fl::awake();
		timeout -= 50;
	}
	if (timeout) {
		MilliSleep(timeout);
		Fl::awake();
	}

// send the command
	if ( !sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait) ) {
		LOG_ERROR("sendCommand failed");
		return;
	}

// check the pre data string
	p = 0;
	pData = 0;
	if (len1) {
		for (size_t i = 0; i < len1; i++) {
			if ((char)rTemp.str1[i] != (char)replybuff[i]) {
					LOG_ERROR("failed pre data string test @ %" PRIuSZ, i);
					return;
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
				LOG_ERROR("failed post data string test @ %d", static_cast<int>(i));
				return;
			}
	}
// convert the data field
	mtr = pmeter_data(rTemp.data, pData);
	if (xmlrig.debug) LOG_INFO("Converted %s to %d", symbol, (int)mtr);

	REQ(rigcat_set_pmeter, reinterpret_cast<void *>(mtr));

}

static void rigcat_notch(void *data)
{
	int val = reinterpret_cast<long>(data);
	notch_frequency = val;
}

int notch_data(DATA d, size_t p)
{
	int val = 0;

	if (d.dtype == "BCD") {
		if (d.reverse == true)
			val = (int)(fm_bcd_be(p, d.size));
		else
			val = (int)(fm_bcd(p, d.size));
	} else if (d.dtype == "BINARY") {
		if (d.reverse == true)
			val = (int)(fm_binary_be(p, d.size));
		else
			val = (int)(fm_binary(p, d.size));
	} else if (d.dtype == "DECIMAL") {
		if (d.reverse == true)
			val = (int)(fm_decimal_be(p, d.size));
		else
			val = (int)(fm_decimal(p, d.size));
	}

	size_t n;
	int ntch1, ntch2, val1, val2;
	for (n = 0; n < xmlrig.notch.size() - 1; n++) {
		if ((val > xmlrig.notch[n].val) && (val <= xmlrig.notch[n+1].val))
			break;
	}
	if (n == xmlrig.notch.size() - 1)
		return 0;

	val1 = xmlrig.notch[n].val;
	val2 = xmlrig.notch[n+1].val;
	if (val1 == val2)
		return 0;

	ntch1 = xmlrig.notch[n].mtr;
	ntch2 = xmlrig.notch[n+1].mtr;

	return ntch1 + (val - val1) * (ntch2 - ntch1) / (val2 - val1);
}

int notch_val(int freq)
{
	size_t n;
	int val1, val2, freq1, freq2;
	for (n = 0; n < xmlrig.notch.size() - 1; n++) {
		if ((freq > xmlrig.notch[n].mtr) && (freq <= xmlrig.notch[n+1].mtr))
			break;
	}
	if (n == xmlrig.notch.size() - 1)
		return 0;

	freq1 = xmlrig.notch[n].mtr;
	freq2 = xmlrig.notch[n+1].mtr;

	if (freq1 == freq2)
		return 0;

	val1 = xmlrig.notch[n].val;
	val2 = xmlrig.notch[n+1].val;

	return val1 + (freq - freq1) * (val2 - val1) / (freq2 - freq1);

}

bool rigCAT_notchON()
{
	const char symbol[] = "GET_NOTCH_ON";

	if (rigCAT_exit) return 0;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	size_t len1 = 0;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}

	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return 0;
	}

	modeCmd = *itrCmd;
	if ( !modeCmd.str1.empty() ) strCmd.append(modeCmd.str1);
	if ( !modeCmd.str2.empty() ) strCmd.append(modeCmd.str2);
	if ( !modeCmd.info.size() ) return 0;

	list<XMLIOS>::iterator preply;

	for (preply = reply.begin(); preply != reply.end(); ++preply) {
		if (preply->SYMBOL == modeCmd.info)
			break;
	}
	if (preply == reply.end()) return 0;

	XMLIOS  rTemp = *preply;
	len1 = rTemp.str1.size();

	int timeout = progdefaults.RigCatTimeout;
	while (timeout > 50) {
		MilliSleep(50);
		Fl::awake();
		timeout -= 50;
	}
	if (timeout) {
		MilliSleep(timeout);
		Fl::awake();
	}

// send the command
	if ( !sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait) ) {
		LOG_ERROR("sendCommand failed");
		return 0;
	}

	bool is_on = true;
	if (len1) {
		for (size_t i = 0; i < len1; i++) {
			if ((char)rTemp.str1[i] != (char)replybuff[i]) {
				is_on = false;
				break;
			}
		}
	} else
		is_on = false;

	if (xmlrig.debug) LOG_INFO("%s is %s", symbol, is_on ? "ON" : "OFF");
	return is_on;
}

void rigCAT_get_notch()
{
	const char symbol[] = "GET_NOTCH";

	if (rigCAT_exit) return;

	if (!rigCAT_notchON()) {
		REQ(rigcat_notch, reinterpret_cast<void *>(0));
		return;
	}
	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	size_t p = 0, len1 = 0, len2 = 0, pData = 0;
	long ntch = 0;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}

	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return;
	}

	modeCmd = *itrCmd;
	if ( !modeCmd.str1.empty() ) strCmd.append(modeCmd.str1);
	if ( !modeCmd.str2.empty() ) strCmd.append(modeCmd.str2);
	if ( !modeCmd.info.size() ) return;

	list<XMLIOS>::iterator preply;

	for (preply = reply.begin(); preply != reply.end(); ++preply) {
		if (preply->SYMBOL == modeCmd.info)
			break;
	}
	if (preply == reply.end()) return;

	XMLIOS  rTemp = *preply;
	len1 = rTemp.str1.size();
	len2 = rTemp.str2.size();

	int timeout = progdefaults.RigCatTimeout;
	while (timeout > 50) {
		MilliSleep(50);
		Fl::awake();
		timeout -= 50;
	}
	if (timeout) {
		MilliSleep(timeout);
		Fl::awake();
	}

// send the command
	if ( !sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait) ) {
		LOG_ERROR("sendCommand failed");
		return;
	}

// check the pre data string
	p = 0;
	pData = 0;
	if (len1) {
		for (size_t i = 0; i < len1; i++) {
			if ((char)rTemp.str1[i] != (char)replybuff[i]) {
					LOG_ERROR("failed pre data string test @ %" PRIuSZ, i);
					return;
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
				LOG_ERROR("failed post data string test @ %d", static_cast<int>(i));
				return;
			}
	}
// convert the data field
	ntch = notch_data(rTemp.data, pData);
	if (xmlrig.debug) LOG_INFO("%s converted to %d", symbol, (int)ntch);

	REQ(rigcat_notch, reinterpret_cast<void *>(ntch));

}

void rigCAT_notch_ON()
{
	const char symbol[] = "SET_NOTCH_ON";

	if (rigCAT_exit) return;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return;
	}

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
					if (sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait)) return;
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
			if (sendCommand(strCmd, 0, progdefaults.RigCatWait)) return;
		}
	}
	LOG_ERROR("%s failed", symbol);
}

void rigCAT_notch_OFF()
{
	const char symbol[] = "SET_NOTCH_OFF";

	if (rigCAT_exit) return;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return;
	}

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
					if (sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait)) return;
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
			if (sendCommand(strCmd, 0, progdefaults.RigCatWait)) return;
		}
	}
	LOG_ERROR("%s failed", symbol);
}

void rigCAT_set_notch(int freq)
{
	const char symbol[] = "SET_NOTCH_VAL";

	if (rigCAT_exit) return;

	if (!freq) {
		rigCAT_notch_OFF();
		return;
	}
	rigCAT_notch_ON();

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return;
	}

	modeCmd = *itrCmd;

	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);

	string strval = "";
	long long int val = notch_val(freq);

	if (modeCmd.data.size != 0) {
		if (modeCmd.data.dtype == "BCD") {
			if (modeCmd.data.reverse == true)
				strval = to_bcd_be(val, modeCmd.data.size);
		else
			strval = to_bcd(val, modeCmd.data.size);
		} else if (modeCmd.data.dtype == "BINARY") {
			if (modeCmd.data.reverse == true)
				strval = to_binary_be(val, modeCmd.data.size);
			else
				strval = to_binary(val, modeCmd.data.size);
		} else if (modeCmd.data.dtype == "DECIMAL") {
			if (modeCmd.data.reverse == true)
				strval = to_decimal_be(val, modeCmd.data.size);
			else
				strval = to_decimal(val, modeCmd.data.size);
		}
	}
	strCmd.append(strval);

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
					if (sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait)) return;
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
			if (sendCommand(strCmd, 0, progdefaults.RigCatWait)) return;
		}
	}
	if (progdefaults.RigCatVSP == false)
		LOG_ERROR("%s failed", symbol);
}

int pwrlevel_data(DATA d, size_t p)
{
	int val = 0;

	if (d.dtype == "BCD") {
		if (d.reverse == true)
			val = (int)(fm_bcd_be(p, d.size));
		else
			val = (int)(fm_bcd(p, d.size));
	} else if (d.dtype == "BINARY") {
		if (d.reverse == true)
			val = (int)(fm_binary_be(p, d.size));
		else
			val = (int)(fm_binary(p, d.size));
	} else if (d.dtype == "DECIMAL") {
		if (d.reverse == true)
			val = (int)(fm_decimal_be(p, d.size));
		else
			val = (int)(fm_decimal(p, d.size));
	}

	size_t n;
	int pwr1, pwr2, val1, val2;
	for (n = 0; n < xmlrig.pwrlevel.size() - 1; n++) {
		if ((val > xmlrig.pwrlevel[n].val) && (val <= xmlrig.pwrlevel[n+1].val))
			break;
	}
	if (n == xmlrig.pwrlevel.size() - 1)
		return 0;

	val1 = xmlrig.pwrlevel[n].val;
	val2 = xmlrig.pwrlevel[n+1].val;
	if (val1 == val2)
		return 0;

	pwr1 = xmlrig.pwrlevel[n].mtr;
	pwr2 = xmlrig.pwrlevel[n+1].mtr;

	return pwr1 + (val - val1) * (pwr2 - pwr1) / (val2 - val1);
}

int pwrlevel_val(int pwr)
{
	size_t n;
	int val1, val2, pwr1, pwr2;
	for (n = 0; n < xmlrig.pwrlevel.size() - 1; n++) {
		if ((pwr >= xmlrig.pwrlevel[n].mtr) && (pwr <= xmlrig.pwrlevel[n+1].mtr))
			break;
	}
	if (n == xmlrig.pwrlevel.size() - 1)
		return 0;

	pwr1 = xmlrig.pwrlevel[n].mtr;
	pwr2 = xmlrig.pwrlevel[n+1].mtr;

	if (pwr1 == pwr2)
		return 0;

	val1 = xmlrig.pwrlevel[n].val;
	val2 = xmlrig.pwrlevel[n+1].val;

	return val1 + (pwr - pwr1) * (val2 - val1) / (pwr2 - pwr1);

}

// called by rigio thread
// must use REQ(...) to set the power level control

static void rigCAT_update_pwrlevel(void *v)
{
	long pwr = reinterpret_cast<long>(v);
	char szpwr[10];
	snprintf(szpwr, sizeof(szpwr), "%ld", pwr);
	progdefaults.mytxpower = szpwr;

	inpMyPower->value(szpwr);
	pwr_level->value(pwr);

	if (xmlrig.debug) LOG_INFO("Read power level %s", szpwr);
}

void rigCAT_get_pwrlevel()
{
	const char symbol[] = "GET_PWRLEVEL";

	if (rigCAT_exit || xmlrig.noserial || !xmlrig.xmlok) return;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;
	size_t p = 0, len1 = 0, len2 = 0, pData = 0;
	long pwr = 0;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}

	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return;
	}

	modeCmd = *itrCmd;
	if ( !modeCmd.str1.empty() ) strCmd.append(modeCmd.str1);
	if ( !modeCmd.str2.empty() ) strCmd.append(modeCmd.str2);
	if ( !modeCmd.info.size() ) return;

	list<XMLIOS>::iterator preply;

	for (preply = reply.begin(); preply != reply.end(); ++preply) {
		if (preply->SYMBOL == modeCmd.info)
			break;
	}
	if (preply == reply.end()) return;

	XMLIOS  rTemp = *preply;
	len1 = rTemp.str1.size();
	len2 = rTemp.str2.size();

	int timeout = progdefaults.RigCatTimeout;
	while (timeout > 50) {
		MilliSleep(50);
		Fl::awake();
		timeout -= 50;
	}
	if (timeout) {
		MilliSleep(timeout);
		Fl::awake();
	}

// send the command
	{
		guard_lock ser_guard( &rigCAT_mutex );
		if ( !sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait) ) {
			LOG_ERROR("sendCommand failed");
			return;
		}
	}
// check the pre data string
	p = 0;
	pData = 0;
	if (len1) {
		for (size_t i = 0; i < len1; i++) {
			if ((char)rTemp.str1[i] != (char)replybuff[i]) {
					LOG_ERROR("failed pre data string test @ %" PRIuSZ, i);
					return;
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
				LOG_ERROR("failed post data string test @ %d", static_cast<int>(i));
				return;
			}
	}
// convert the data field
	pwr = pwrlevel_data(rTemp.data, pData);

	REQ(rigCAT_update_pwrlevel, (void *)pwr);
}

void rigCAT_set_pwrlevel(int pwr)
{
	const char symbol[] = "SET_PWRLEVEL";

	if (rigCAT_exit || !xmlrig.use_pwrlevel) return;

	XMLIOS modeCmd;
	list<XMLIOS>::iterator itrCmd;
	string strCmd;

	itrCmd = commands.begin();
	while (itrCmd != commands.end()) {
		if ((*itrCmd).SYMBOL == symbol)
			break;
		++itrCmd;
	}
	if (itrCmd == commands.end()) {
		if (xmlrig.debug) LOG_INFO("%s not defined", symbol);
		return;
	}

	modeCmd = *itrCmd;

	if ( modeCmd.str1.empty() == false)
		strCmd.append(modeCmd.str1);

	string strval = "";
	int val = pwrlevel_val(pwr);

	if (modeCmd.data.size != 0) {
		if (modeCmd.data.dtype == "BCD") {
			if (modeCmd.data.reverse == true)
				strval = to_bcd_be(val, modeCmd.data.size);
		else
			strval = to_bcd(val, modeCmd.data.size);
		} else if (modeCmd.data.dtype == "BINARY") {
			if (modeCmd.data.reverse == true)
				strval = to_binary_be(val, modeCmd.data.size);
			else
				strval = to_binary(val, modeCmd.data.size);
		} else if (modeCmd.data.dtype == "DECIMAL") {
			if (modeCmd.data.reverse == true)
				strval = to_decimal_be(val, modeCmd.data.size);
			else
				strval = to_decimal(val, modeCmd.data.size);
		}
	}
	strCmd.append(strval);

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
					if (sendCommand(strCmd, rTemp.size, progdefaults.RigCatWait)) {
						if (xmlrig.debug) LOG_INFO("Power set to %s", strval.c_str());
						return;
					}
				}
				if (xmlrig.debug) LOG_ERROR("%s failed", symbol);
				return;
			}
			preply++;
		}
	} else {
		for (int n = 0; n < progdefaults.RigCatRetries; n++) {
			MilliSleep(50);
			guard_lock ser_guard( &rigCAT_mutex );
			if (rigCAT_exit) return;
			if (sendCommand(strCmd, 0, progdefaults.RigCatWait)) {
				if (xmlrig.debug) LOG_INFO("Power set to %s", strval.c_str());
				return;
			}
		}
	}
	if (progdefaults.RigCatVSP == false)
		if (xmlrig.debug) LOG_ERROR("%s failed", symbol);
}

static void *rigCAT_loop(void *args)
{
	SET_THREAD_ID(RIGCTL_TID);

	long long freq = 0L;
	string sWidth, sMode;
	bool failed;

	for (;;) {
		for (int i = 0; i < xmlrig.pollinterval / 10; i++) {
			MilliSleep(10);
			guard_lock ser_guard( &rigCAT_mutex );
			if (rigCAT_exit == true) {
				LOG_INFO("%s", "Exited rigCAT loop");
				return NULL;
			}
		}

		if (trx_state == STATE_RX) {
			freq = rigCAT_getfreq(progdefaults.RigCatRetries, failed);
			if (rigCAT_exit) continue;

			if ((freq > 0) && (freq != llFreq)) {
				llFreq = freq;
				show_frequency(freq);
				wf->rfcarrier(freq);
			}

			sWidth = rigCAT_getwidth();
			if (rigCAT_exit) continue;

			if (sWidth.size() && sWidth != sRigWidth) {
				sRigWidth = sWidth;
				show_bw(sWidth);
			}

			sMode = rigCAT_getmode();
			if (rigCAT_exit) continue;

			if (sMode.size() && sMode != sRigMode) {
				sRigMode = sMode;
				if (ModeIsLSB(sMode))
					wf->USB(false);
				else
					wf->USB(true);
				show_mode(sMode);
			}

			if (xmlrig.use_pwrlevel) rigCAT_get_pwrlevel();

			if (xmlrig.use_smeter) rigCAT_get_smeter();

			if (xmlrig.use_notch) rigCAT_get_notch();

		} else {
			if ( (trx_state == STATE_TUNE) || (trx_state == STATE_TX) )
				if (xmlrig.use_pwrmeter) rigCAT_get_pwrmeter();
		}

	}

	return NULL;
}

