// ----------------------------------------------------------------------------
// Copyright (C) 2014
//              David Freese, W1HKJ
//
// This file is part of fldigi
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <iostream>
#include <cmath>
#include <cstring>
#include <sstream>
#include <stdlib.h>

#include <FL/Fl.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>

#include "xmlrpcpp/XmlRpc.h"

#include "config.h"
#include "lgbook.h"
#include "icons.h"
#include "gettext.h"
#include "debug.h"
#include "util.h"
#include "date.h"
#include "logbook.h"
#include "logger.h"
#include "locator.h"
#include "counties.h"
#include "confdialog.h"
#include "fl_digi.h"
#include "adif_io.h"
#include "modem.h"
#include "trx.h"
#include "status.h"

#include "configuration.h"

using namespace XmlRpc;

XmlRpcClient *log_client = (XmlRpcClient *)0;

bool test_connection(bool info = false)
{
	if (!log_client) {
		create_logbook_dialogs();
		return false;
	}
	XmlRpcValue query, result;
	if (log_client->execute("system.listMethods", query, result)) {
		if (info) {
			string res;
			int asize = result.size();
			XmlRpcValue oneArg, help;
			res = "Xml-log methods:";
			for (int i = 0; i < asize; i++) {
				oneArg[0] = result[i];
				try {
					if (std::string(result[i]).find("system") == string::npos) {
						log_client->execute("system.methodHelp", oneArg, help);
						res.append("\n\t").append(help);
					}
				} catch ( XmlRpcException err) {
					res.append("\n").append(oneArg[0]).append(": ").append(err.getMessage());
				}
			}
			LOG_INFO("%s", res.c_str());
		}
		return true;
	}
	return false;
}

void activate_log_menus(bool val)
{
	set_server_label(!val);
	activate_menu_item(_("View"), val);
	activate_menu_item(_("New"), val);
	activate_menu_item(_("Open..."), val);
	activate_menu_item(_("Save"), val);
	activate_menu_item(_("ADIF"), val);
	activate_menu_item(_("Reports"), val);
}

string get_field(string &adifline, int field)
{
	string fld;
	fld.append("<").append(fields[field].name).append(":");
	size_t pos1 = adifline.find(fld);
	if (pos1 == std::string::npos)
		return "";

	pos1 = adifline.find(">", pos1) + 1;
	size_t pos2 = adifline.find("<", pos1);
	fld = adifline.substr(pos1, pos2 - pos1);
	return fld;
}

cQsoRec* search_fllog(const char *callsign)
{
	cQsoRec *rec = new cQsoRec;

	XmlRpcValue oneArg, result;
	if (!test_connection()) {
		LOG_INFO("%s","Logbook server down!");
		progdefaults.xml_logbook = false;
		activate_log_menus(true);
		start_logbook();
		return (cQsoRec *)0;
	}
	oneArg[0] = callsign;
	if (log_client->execute("log.get_record", oneArg, result)) {
		string adifline = std::string(result);

		rec->putField(NAME, get_field(adifline, NAME).c_str());
		rec->putField(QTH, get_field(adifline, QTH).c_str());
		rec->putField(QSO_DATE, get_field(adifline, QSO_DATE).c_str());
		rec->putField(BAND, get_field(adifline, BAND).c_str());
		rec->putField(ADIF_MODE, get_field(adifline, ADIF_MODE).c_str());

		return rec;
	}
	return (cQsoRec *)0;
}

bool xml_get_record(const char *callsign)
{
	XmlRpcValue oneArg, result;
	if (!test_connection()) {
		LOG_INFO("%s","Logbook server down!");
		progdefaults.xml_logbook = false;
		activate_log_menus(true);
		start_logbook();
		return false;
	}
	oneArg[0] = callsign;
	if (log_client->execute("log.get_record", oneArg, result)) {
		string adifline = std::string(result);
//std::cout << adifline << std::endl;

		inpName->value(get_field(adifline, NAME).c_str());
		inpQth->value(get_field(adifline, QTH).c_str());
		inpState->value(get_field(adifline, STATE).c_str());
		inpVEprov->value(get_field(adifline, VE_PROV).c_str());
		cboCountry->value(get_field(adifline, COUNTRY).c_str());
		inpCounty->value(get_field(adifline, CNTY).c_str());
		inpLoc->value(get_field(adifline, GRIDSQUARE).c_str());

		inp_SS_SerialNoR->value(get_field(adifline, SS_SERNO).c_str());
		inp_SS_Precedence->value(get_field(adifline, SS_PREC).c_str());
		inp_SS_Check->value(get_field(adifline, SS_CHK).c_str());
		inp_SS_Section->value(get_field(adifline, SS_SEC).c_str());

		inp_KD_age->value(get_field(adifline, AGE).c_str());
		inp_ARR_check->value(get_field(adifline, CHECK).c_str());
		inp_1010_nr->value(get_field(adifline, TEN_TEN).c_str());

		inp_JOTA_troop->value(get_field(adifline, TROOPR).c_str());
		inp_JOTA_scout->value(get_field(adifline, SCOUTR).c_str());

		inpNotes->value(get_field(adifline, NOTES).c_str());
	} else {
		inpName->value("");
		inpQth->value("");
		inpState->value("");
		inpVEprov->value("");
		cboCountry->value("");
		inpCounty->value("");
		inpLoc->value("");
		inp_SS_SerialNoR->value("");
		inp_SS_Precedence->value("");
		inp_SS_Check->value("");
		inp_SS_Section->value("");
		inp_KD_age->value("");
		inp_ARR_check->value("");
		inp_1010_nr->value("");
		inp_JOTA_troop->value("");
		inp_JOTA_scout->value("");
		inpNotes->value("");
	}
	if (inpLoc->value()[0]) {
		double lon1, lat1, lon2, lat2;
		double azimuth, distance;
		char szAZ[4];
		if ( QRB::locator2longlat(&lon1, &lat1, progdefaults.myLocator.c_str()) == QRB::QRB_OK &&
			 QRB::locator2longlat(&lon2, &lat2, inpLoc->value()) == QRB::QRB_OK &&
			 QRB::qrb(lon1, lat1, lon2, lat2, &distance, &azimuth) == QRB::QRB_OK ) {
			snprintf(szAZ,sizeof(szAZ),"%0.f", azimuth);
			inpAZ->value(szAZ);
		} else
			inpAZ->value("");
	} else
		inpAZ->value("");
	return true;
}

static std::string adif;
static std::string notes;

#define adif_str(a, b) { \
std::ostringstream os; \
os << "<" << fields[(a)].name << ":" << strlen((b)) << ">" << (b); \
adif.append(os.str()); }

void xml_add_record()
{
	if (!test_connection()) {
		LOG_INFO("%s","Logbook server down!");
		progdefaults.xml_logbook = false;
		activate_log_menus(true);
		start_logbook();
		AddRecord();
		return;
	}

// create the ADIF record
	char Mhz[30];
	adif.erase();

	adif_str(QSO_DATE, sDate_on.c_str()); 
	adif_str(QSO_DATE_OFF, sDate_off.c_str());
	adif_str(TIME_ON, sTime_on.c_str());
	adif_str(TIME_OFF, sTime_off.c_str());
	adif_str(CALL, inpCall->value());
	{
		snprintf(Mhz, sizeof(Mhz), "%-f", atof(inpFreq->value()) / 1000.0);
		inpFreq_log->value(Mhz);
		adif_str(FREQ, Mhz);
	}
	adif_str(ADIF_MODE, mode_info[active_modem->get_mode()].adif_name);
	adif_str(RST_SENT, inpRstOut->value());
	adif_str(RST_RCVD, inpRstIn->value());
	adif_str(TX_PWR, progdefaults.mytxpower.c_str());
	adif_str(NAME, inpName->value());
	
	adif_str(QTH, inpQth->value());
	adif_str(STATE, inpState->value());
	adif_str(VE_PROV, inpVEprov->value());
	adif_str(COUNTRY, cboCountry->value());
	adif_str(CNTY, inpCounty->value());
	adif_str(GRIDSQUARE, inpLoc->value());
	adif_str(STX, outSerNo->value());
	adif_str(SRX, inpSerNo->value());
	adif_str(XCHG1, inpXchgIn->value());
	adif_str(MYXCHG, progdefaults.myXchg.c_str());
	adif_str(NOTES, inpNotes->value());
	adif_str(CLASS, inpClass->value());
	adif_str(ARRL_SECT, inpSection->value());
	adif_str(CQZ, inp_CQzone->value());
// these fields will always be blank unless they are added to the main
// QSO log area.
// need to add the remaining fields
	adif_str(IOTA, "");
	adif_str(DXCC, "");
	adif_str(QSL_VIA, "");
	adif_str(QSLRDATE, "");
	adif_str(QSLSDATE, "");

// new contest fields

	adif_str(SS_SEC, inp_SS_Section->value());
	adif_str(SS_SERNO, inp_SS_SerialNoR->value());
	adif_str(SS_PREC, inp_SS_Precedence->value());
	adif_str(SS_CHK, inp_SS_Check->value());

	adif_str(AGE, inp_KD_age->value());

	adif_str(TEN_TEN, inp_1010_nr->value());
	adif_str(CHECK, inp_ARR_check->value());

	adif_str(TROOPS, progdefaults.my_JOTA_troop.c_str());
	adif_str(TROOPR, inp_JOTA_troop->value());
	adif_str(SCOUTS,  progdefaults.my_JOTA_scout.c_str());
	adif_str(SCOUTR, inp_JOTA_scout->value());

	adif_str(OP_CALL, progdefaults.operCall.c_str());
	adif_str(STA_CALL, progdefaults.myCall.c_str());
	adif_str(MY_CITY,
		std::string(progdefaults.myQth).
		append(", ").
		append(inp_QP_state_short->value()).c_str());
	adif_str(MY_GRID, progdefaults.myLocator.c_str());

	adif.append("<eor>");

// send it to the server
	XmlRpcValue oneArg, result;
	oneArg[0] = adif.c_str();
	log_client->execute("log.add_record", oneArg, result);

// submit it foreign log programs
	cQsoRec rec;
	rec.putField(CALL, inpCall->value());
	rec.putField(NAME, inpName->value());
	rec.putField(QSO_DATE, sDate_on.c_str());
	rec.putField(QSO_DATE_OFF, sDate_off.c_str());
	rec.putField(TIME_ON, inpTimeOn->value());
	rec.putField(TIME_OFF, ztime());
	rec.putField(FREQ, Mhz);
	rec.putField(ADIF_MODE, mode_info[active_modem->get_mode()].adif_name);
	rec.putField(QTH, inpQth->value());
	rec.putField(STATE, inpState->value());
	rec.putField(VE_PROV, inpVEprov->value());
	rec.putField(COUNTRY, cboCountry->value());
	rec.putField(CNTY, inpCounty->value());
	rec.putField(GRIDSQUARE, inpLoc->value());
	rec.putField(NOTES, inpNotes->value());
	rec.putField(QSLRDATE, "");
	rec.putField(QSLSDATE, "");
	rec.putField(RST_RCVD, inpRstIn->value ());
	rec.putField(RST_SENT, inpRstOut->value ());
	rec.putField(SRX, inpSerNo->value());
	rec.putField(STX, outSerNo->value());
	rec.putField(XCHG1, inpXchgIn->value());
	rec.putField(MYXCHG, progdefaults.myXchg.c_str());
	rec.putField(CLASS, inpClass->value());
	rec.putField(ARRL_SECT, inpSection->value());
	rec.putField(CNTY, "");
	rec.putField(IOTA, "");
	rec.putField(DXCC, "");
	rec.putField(CONT, "");
	rec.putField(CQZ, "");
	rec.putField(ITUZ, "");
	rec.putField(TX_PWR, "");

// new contest fields

	rec.putField(SS_SEC, inp_SS_Section->value());
	rec.putField(SS_SERNO, inp_SS_SerialNoR->value());
	rec.putField(SS_PREC, inp_SS_Precedence->value());
	rec.putField(SS_CHK, inp_SS_Check->value());

	rec.putField(AGE, inp_KD_age->value());

	rec.putField(TEN_TEN, inp_1010_nr->value());
	rec.putField(CHECK, inp_ARR_check->value());

	rec.putField(TROOPS, progdefaults.my_JOTA_troop.c_str());
	rec.putField(TROOPR, inp_JOTA_troop->value());
	rec.putField(SCOUTS,  progdefaults.my_JOTA_scout.c_str());
	rec.putField(SCOUTR, inp_JOTA_scout->value());

	rec.putField(OP_CALL, progdefaults.operCall.c_str());
	rec.putField(STA_CALL, progdefaults.myCall.c_str());

	submit_record(rec);

}

int xml_check_dup()
{
	int dup_test = 0;
	if (!test_connection()) {
		LOG_INFO("%s","Logbook server down!");
		progdefaults.xml_logbook = false;
		progdefaults.changed = true;
		activate_log_menus(true);
		if (!dlgLogbook) create_logbook_dialogs();
		start_logbook();
		return dup_test;
	}
	XmlRpcValue six_args, result;
	six_args[0] = inpCall->value();
	six_args[1] = progdefaults.dupmode ? mode_info[active_modem->get_mode()].adif_name : "0";
	char tspn[10];
	snprintf(tspn, sizeof(tspn), "%d", progdefaults.timespan);
	six_args[2] = progdefaults.duptimespan ? tspn : "0";
	six_args[3] = progdefaults.dupband ? inpFreq->value() : "0";
	six_args[4] = (progdefaults.dupstate && inpState->value()[0]) ? inpState->value() : "0";
	six_args[5] = (progdefaults.dupxchg1 && inpXchgIn->value()[0]) ? inpXchgIn->value() : "0";
	if (log_client->execute("log.check_dup", six_args, result)) {
		string res = std::string(result);
		if (res == "true")
			dup_test = 1;
		else if (res == "possible")
			dup_test = 2;
	}
	return dup_test;
}

void xml_update_eqsl()
{
	adif.erase();
	adif_str(EQSLSDATE, sDate_on.c_str());
	adif.append("<EOR>");

	XmlRpcValue oneArg, result;
	oneArg[0] = adif.c_str();
	LOG_INFO("%s", "xmlrpc log: update eqsl date");
	log_client->execute("log.update_record", oneArg, result);
}

void xml_update_lotw()
{
	adif.erase();
	adif_str(LOTWSDATE, sDate_on.c_str());
	adif.append("<EOR>");

	XmlRpcValue oneArg, result;
	oneArg[0] = adif.c_str();
	LOG_INFO("%s", "xmlrpc log: update LoTW date");
	log_client->execute("log.update_record", oneArg, result);
}

void connect_to_log_server(void *)
{
	if (log_client) {
		delete log_client;
		log_client = 0;
	}
	LOG_INFO("%s","Create XMLRPC client");
	log_client = new XmlRpcClient(
					progdefaults.xmllog_address.c_str(),
					atoi(progdefaults.xmllog_port.c_str()));

	LOG_INFO("%s","Created");
	if (progdefaults.xml_logbook) {
		if (test_connection(true)) {
			LOG_INFO("%s","Close local logbook");
			close_logbook();
			if (dlgLogbook) dlgLogbook->hide();
			activate_log_menus(false);
		} else {
			LOG_INFO("%s","Remote server not responding");
			progdefaults.xml_logbook = false;
			activate_log_menus(true);
			start_logbook();
			LOG_INFO("%s","Use local logbook");
		}
	} else {
		LOG_INFO("%s","Enable local logbook");
		activate_log_menus(true);
		start_logbook();
	}
}

