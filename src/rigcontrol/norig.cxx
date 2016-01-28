// ----------------------------------------------------------------------------
// norig.cxx
//
// Copyright (C) 2016
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

//----------------------------------------------------------------------
// functions used when no xcvr control is in use
//----------------------------------------------------------------------

long long noCAT_getfreq() {
	return progStatus.noCATfreq;
}

void noCAT_setfreq(long long f)
{
	progStatus.noCATfreq = f;
	wf->rfcarrier(f);
}

std::string noCAT_getmode()
{
	return progStatus.noCATmode;
}

void noCAT_setmode(const std::string &md)
{
	progStatus.noCATmode = md;
	if (ModeIsLSB(md))
		wf->USB(false);
	else
		wf->USB(true);
}

std::string noCAT_getwidth()
{
	return progStatus.noCATwidth;
}

void noCAT_setwidth(const std::string &w)
{
	progStatus.noCATwidth = w;
	show_bw(w);
}

void noCAT_setPTT(bool val)
{
	rigio.SetPTT(val); // always execute the h/w ptt if enabled
}

void noCAT_init()
{
	qso_setFreq(progStatus.noCATfreq);
	wf->rfcarrier(progStatus.noCATfreq);
	init_NoRig_RigDialog();

	qso_opMODE->value(progStatus.noCATmode.c_str());

	chkUSEHAMLIB->value(0);
	chkUSEHAMLIB->redraw();
	btnInitHAMLIB->labelcolor(FL_BLACK);
	btnInitHAMLIB->redraw_label();

	chkUSEXMLRPC->value(0);
	chkUSEXMLRPC->redraw();
	btnInitXMLRPC->labelcolor(FL_BLACK);
	btnInitXMLRPC->redraw_label();

	chkUSERIGCAT->value(0);
	chkUSERIGCAT->redraw();
	btnInitRIGCAT->labelcolor(FL_BLACK);
	btnInitRIGCAT->redraw_label();

	return;
}

