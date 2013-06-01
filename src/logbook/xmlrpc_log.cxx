// =====================================================================
//
// xmlrpc_io.cxx
//
// connect to logbook xmlrpc server
//
// =====================================================================

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
		inpName->value(get_field(adifline, NAME).c_str());
		inpQth->value(get_field(adifline, QTH).c_str());
		inpState->value(get_field(adifline, STATE).c_str());
		inpVEprov->value(get_field(adifline, VE_PROV).c_str());
		inpCountry->value(get_field(adifline, COUNTRY).c_str());
		inpLoc->value(get_field(adifline, GRIDSQUARE).c_str());
		inpNotes->value(get_field(adifline, NOTES).c_str());
	} else {
		inpName->value("");
		inpQth->value("");
		inpState->value("");
		inpVEprov->value("");
		inpCountry->value("");
		inpLoc->value("");
		inpNotes->value("");
	}
	if (inpLoc->value()[0]) {
		double lon1, lat1, lon2, lat2;
		double azimuth, distance;
		char szAZ[4];
		if ( locator2longlat(&lon1, &lat1, progdefaults.myLocator.c_str()) == RIG_OK &&
			 locator2longlat(&lon2, &lat2, inpLoc->value()) == RIG_OK &&
			 qrb(lon1, lat1, lon2, lat2, &distance, &azimuth) == RIG_OK ) {
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
	adif.erase();

	adif_str(QSO_DATE, sDate_on.c_str()); 
	adif_str(QSO_DATE_OFF, sDate_off.c_str());
	adif_str(TIME_ON, sTime_on.c_str());
	adif_str(TIME_OFF, sTime_off.c_str());
	adif_str(CALL, inpCall->value());
	{
		char Mhz[30];
		snprintf(Mhz, sizeof(Mhz), "%-f", atof(inpFreq->value()) / 1000.0);
		inpFreq_log->value(Mhz);
		adif_str(FREQ, Mhz);
	}
	adif_str(MODE, mode_info[active_modem->get_mode()].adif_name);
	adif_str(RST_SENT, inpRstOut->value());
	adif_str(RST_RCVD, inpRstIn->value());
	adif_str(TX_PWR, progdefaults.mytxpower.c_str());
	adif_str(NAME, inpName->value());
	
	adif_str(QTH, inpQth->value());
	adif_str(STATE, inpState->value());
	adif_str(VE_PROV, inpVEprov->value());
	adif_str(COUNTRY, inpCountry->value());
	adif_str(GRIDSQUARE, inpLoc->value());
	adif_str(STX, outSerNo->value());
	adif_str(SRX, inpSerNo->value());
	adif_str(XCHG1, inpXchgIn->value());
	adif_str(MYXCHG, progdefaults.myXchg.c_str());
	adif_str(NOTES, inpNotes->value());
// these fields will always be blank unless they are added to the main
// QSO log area.
// need to add the remaining fields
	adif_str(IOTA, "");
	adif_str(DXCC, "");
	adif_str(QSL_VIA, "");
	adif_str(QSLRDATE, "");
	adif_str(QSLSDATE, "");
	adif.append("<eor>");

// send it to the server
	XmlRpcValue oneArg, result;
	oneArg[0] = adif.c_str();
	std::cout << "result: " << log_client->execute("log.add_record", oneArg, result) << std::endl;
}

bool xml_check_dup()
{
	if (!test_connection()) {
		LOG_INFO("%s","Logbook server down!");
		progdefaults.xml_logbook = false;
		activate_log_menus(true);
		start_logbook();
		return false;
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
			return true;
	}
	return false;
}


void connect_to_log_server(void *)
{
	if (!log_client) {
		int xmllog_port = atoi(progdefaults.xmllog_port.c_str());
		log_client = new XmlRpcClient(progdefaults.xmllog_address.c_str(), xmllog_port);
	} else {
		delete log_client;
		int xmllog_port = atoi(progdefaults.xmllog_port.c_str());
		log_client = new XmlRpcClient(progdefaults.xmllog_address.c_str(), xmllog_port);
	}

	if (progdefaults.xml_logbook) {
		if (test_connection(true)) {
			close_logbook();
			if (dlgLogbook) dlgLogbook->hide();
			activate_log_menus(false);
		} else {
			progdefaults.xml_logbook = false;
			activate_log_menus(true);
			start_logbook();
		}
	} else {
		close_logbook();
		activate_log_menus(true);
		start_logbook();
	}
}

