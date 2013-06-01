// ----------------------------------------------------------------------------
// synop.cxx  --  SYNOP decoding
//
// Copyright (C) 2012
//		Remi Chateauneu, F4ECW
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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include "synop.h"
#include "kmlserver.h"
#include "field_def.h"

#define G_N_ELEMENTS(arr) sizeof(arr)/sizeof(*arr)

// ----------------------------------------------------------------------------

static std::ostream * dbg_strm = &std::cout ;

struct tst_callback : public synop_callback {
	// Callback for writing decoded synop messages.
	void print(const char * str, size_t nb, bool bold ) const {
		dbg_strm->write( str, nb );
	}
	bool log_adif(void) const { return true ;}
	bool log_kml(void) const { return true ;}
};
// ----------------------------------------------------------------------------
static void test_coordinates()
{
	CoordinateT::Pair jn45op( "JN45op" );
	if( (int)( jn45op.longitude().angle() * 100 ) != 920 ) {
		std::cout << "Bad longitude\n" ;
		exit(EXIT_FAILURE);
	}
	if( (int)( jn45op.latitude().angle() * 100 ) != 4564 ) {
		std::cout << "Bad latitude\n" ;
		exit(EXIT_FAILURE);
	}

	std::cout << "Coordinates OK\n";
}

// ----------------------------------------------------------------------------
static const struct {
	int m_wmo_indicator ;
	const char * m_name ;
} wmo_tests[] = {
	{ 62722, "Aroma" },
	{ 95613, "Pemberton" },
	{ 41939, "Madaripur" },
	{ 71121, "Edmonton Namao Alta." }
};

static const size_t wmo_tests_nb = G_N_ELEMENTS(wmo_tests);


static void test_wmo(void)
try 
{
	for( size_t i = 0; i < wmo_tests_nb; ++i )
	{
		const std::string & wmo_name = SynopDB::IndicatorToName( wmo_tests[i].m_wmo_indicator );
		std::cout << "wmo_name=" << wmo_name << "\n";
		std::cout << SynopDB::IndicatorToCoordinates( wmo_tests[i].m_wmo_indicator ) << "\n";
		if( wmo_name != wmo_tests[i].m_name ) {
			std::cout << wmo_name << '\n';
			std::cout << wmo_tests[i].m_name << '\n';
			std::cout << "ERROR\n";
			exit(1) ;
		}
	}
	std::cout << "Tested " << wmo_tests_nb << " records\n";
}
catch(...) {
	std::cout << "Error when testing wmo loading\n";
	return ;
}

// ----------------------------------------------------------------------------
static const struct {
	const char * m_buoy_id  ;
	const char * m_name ;
} buoy_tests[] = {
	{ "44022", "Execution Rocks" },
	{ "kcmb",  "East Cameron 47JP (Apache Corp)" }
};

static const size_t buoy_tests_nb = G_N_ELEMENTS(buoy_tests);


static void test_buoy(void)
try 
{
	for( size_t i = 0; i < buoy_tests_nb; ++i )
	{
		const std::string & buoy_name = SynopDB::BuoyToName( buoy_tests[i].m_buoy_id );
		std::cout << "buoy_name=" << buoy_name << "\n";
		if( buoy_name != buoy_tests[i].m_name ) {
			std::cout << buoy_name << '\n';
			std::cout << buoy_tests[i].m_name << '\n';
			std::cout << "ERROR\n";
			exit(1) ;
		}
	}
	std::cout << "Tested " << buoy_tests_nb << " records\n";
}
catch(...) {
	std::cout << "Error when testing buoy loading\n";
	return ;
}

// ----------------------------------------------------------------------------
static const struct {
	const char * m_ship_callsign  ;
	const char * m_name ;
} ship_tests[] = {
	{ "3EPD8", "Trinity Arrow" },
	{ "WYP8657", "James R. Barker" }
};

static const size_t ship_tests_nb = G_N_ELEMENTS(ship_tests);

static void test_ship(void)
try 
{
	for( size_t i = 0; i < ship_tests_nb; ++i )
	{
		const std::string & ship_name = SynopDB::ShipToName( ship_tests[i].m_ship_callsign );
		std::cout << "ship_name=" << ship_name << "\n";
		if( ship_name != ship_tests[i].m_name ) {
			std::cout << ship_tests[i].m_ship_callsign << ".\n";
			std::cout << ship_name << ".\n";
			std::cout << ship_tests[i].m_name << ".\n";
			std::cout << "ERROR\n";
			exit(1) ;
		}
	}
	std::cout << "Tested " << ship_tests_nb << " records\n";
}
catch(...) {
	std::cout << "Error when testing ship loading\n";
	return ;
}

// ----------------------------------------------------------------------------
static const struct {
	const char * m_jcomm_callsign  ;
	const char * m_name ;
} jcomm_tests[] = {
	{ "13002", "TAO21N23W" },
	{ "13590", "ARGOS:71125" }
};

static const size_t jcomm_tests_nb = G_N_ELEMENTS(jcomm_tests);

static void test_jcomm(void)
try 
{
	for( size_t i = 0; i < jcomm_tests_nb; ++i )
	{
		const std::string & jcomm_name = SynopDB::JCommToName( jcomm_tests[i].m_jcomm_callsign );
		std::cout << "jcomm_name=" << jcomm_name << "\n";
		if( jcomm_name != jcomm_tests[i].m_name ) {
			std::cout << jcomm_tests[i].m_jcomm_callsign << ".\n";
			std::cout << jcomm_name << ".\n";
			std::cout << jcomm_tests[i].m_name << ".\n";
			std::cout << "ERROR\n";
			exit(1) ;
		}
	}
	std::cout << "Tested " << jcomm_tests_nb << " records\n";
}
catch(...) {
	std::cout << "Error when testing jcomm loading\n";
	return ;
}

// ----------------------------------------------------------------------------

// Used in a special mode where the synop decoder just prints output the name of the tokens.
struct synop_test {
	int          m_expected_nb_msgs ;
	const char * m_input ;
	const char * m_output ;
};

static const synop_test tests_arr_full[] = {
	   { 1,
		"08495 12575 72512 10171 20128 30242 40250 57005 60002 83502 91750\n"
		" 333 10182 81622 83633 87072=\n",
		"Day of the month: 11 Observation time: 18 hr\n"
		"Weather station: 08495 LXGB GIBRALTAR (CIV/MIL) GI\n"
		"Latitude: 3609N  Longitude: 00521W  Elevation: 5 m\n"
		"Cloud base: 600 - 999 m (2000 - 3333 ft).\n"
		"Horizontal visibility: 25 km.\n"
		"Total cloud cover: 7/8ths or more, but not 8/8ths.\n"
		"Wind direction: 250°.\n"
		"Wind speed: 12 knots, from anemometer.\n"
		"Air temperature: 17.1 °C.\n"
		"Dewpoint temperature: 12.8 °C.\n"
		"Sea level pressure: 1025.0 hPa.\n"
		"Pressure change over last 3 hours: -0.5 hPa, decreasing steadily.\n"
		"Present weather: not significant.\n"
		"Past weather: not significant.\n"
		"Low cloud type: stratocumulus other than stratocumulus cumulogenitus.\n"
		"Middle cloud type: no altocumulus, altostratus or nimbostratus.\n"
		"High cloud type: cirrus spissatus, or cirrus castellanus or cirrus floccus.\n"
		"Maximum temperature: 18.2 °C.\n"
	}, { 1,
		"16597 32562 33113 10139 20097 30053 40140 53022 81130 333 10170 81826\n"
		" 83357 91132 91531 =\n",
		"Day of the month: 11 Observation time: 18 hr\n"
		"Weather station: 16597 LMML LUQA/MALTA ML\n"
		"Latitude: 3551N  Longitude: 01429E  Elevation: 91 m\n"
 
		"Cloud base: 600 - 999 m (2000 - 3333 ft).\n"
		"Horizontal visibility: 12 km.\n"
		"Total cloud cover: 3/8ths.\n"
		"Wind direction: 310°.\n"
		"Wind speed: 13 knots, from anemometer.\n"
		"Air temperature: 13.9 °C.\n"
		"Dewpoint temperature: 9.7 °C.\n"
		"Sea level pressure: 1014.0 hPa.\n"
		"Pressure change over last 3 hours: 2.2 hPa, decreasing or steady, then increasing.\n"
		"Precipitation amount: 0.0 mm.\n"
		"Present weather: not significant.\n"
		"Past weather: not significant.\n"
		"Low cloud type: cumulus humulis or fractus (no vertical development).\n"
		"Middle cloud type: altocumulus translucidous at one level.\n"
		"High cloud type: no cirrus, cirrocumulus or cirrostratus.\n"
		"Maximum temperature: 17.0 °C.\n"
	}
};
static const size_t nb_tests_full = G_N_ELEMENTS(tests_arr_full);

/*
 * TODO: Put apart the tests with errors. Ideally we should generate them starting from good patterns.
 */


static const synop_test tests_arr[] = {
	   { 0,
		"a b   c d\n\ne f\n",
		"a b   c d\n\ne f\n"
	}, { 0,
		"a b 20123   99536    e f\n20123   99536\ng h\n",
		"a b YYGGi+99LLL+    e f\nYYGGi+99LLL+\ng h\n"
	}, { 0,
		"20123\txyz 20123 30101\nx",
		"20123\txyz IIiii+YYGGi+\nx"
	}, { 1,
		"a b 20123 99536 70307 d e f\n",
		"a b YYGGi+99LLL+QLLLL+ d e f\n"
	}, { 0,
		"0393) 32375 71902 10140 20081 30101 40125 57007 878//\n",
		"0393) iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+\n"
	}, { 1,
		"AMOUK36 20184 99556 70051 46/// ///// 10114 20068 40134 54001;\n",
		"IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+\n"
	}, { 0,
		"81/1/ 222// 00156 2//// 3//// 4//// 5//// 6//// 80220 ICE /////;\n",
		"81/1/ 222Dv+0sTTT+2PPHH+3dddd+4PPHH+5PPHH+6IEER+8aTTT+ICE+cSbDz+\n"
	}, { 1,
		"03075 15981 /1212 10086 20041 30090 40134 56007 60002 91750\n"
  		"333 10099 82/68;\n\n",
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+9GGgg+\n"
  		"333+1sTTT_max+8NChh+\n\n"
	}, { 0,
		"333 69937 81/14 87/60;\n",
    		"333+6RRRt+8NChh+8NChh+\n"
	}, { 0,
		"22273 ICE 52//2;\n",
      		"222Dv+ICE+cSbDz+\n"
	}, { 1,
		"03204 12580 12505 10117 20064 30103 40123 57006 60002 81100 91750\n",
  		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+9GGgg+\n"
	}, { 0,
		"333 21153 42021 70000 91105;\n",
		"333+2sTTT_min+4Esss+7RRRR+9SSss+\n"
	}, { 0,
		"333 21061 43094 70000 91102;\n",
		"333+2sTTT_min+4Esss+7RRRR+9SSss+\n"
	}, { 0,
		"10015 21014 30134 40152 52009 333 21006 91114;\n",
		"1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 333+2sTTT_min+9SSss+\n"
	}, { 1,
		"01102 46/// /1410 10058 20006 30069 40089 52006 333 20051 91116;\n",
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 333+2sTTT_min+9SSss+\n"
	}, { 0,
		"333 20085 91710;\n",
		"333+2sTTT_min+9SSss+\n"
	}, { 1,
		"SNCN19 CWAO 280023\n"
		"OOXX\n"
		"MRP43 27221 99181 70159 ///// 00101\n"
		"26/// /2504 10242 29081 30139 92200 333 60000=\n",
		"TTAAii+CCCC+YYGGgg+\n"
		"OOXX+IIIII+YYGGi+99LLL+QLLLL+MMMULaULo+h0h0h0h0im+\n"
		"iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+9GGgg+ 333+6RRRt+\n"
	}, { 0, // Cannot find WMO station:72358
		"ZCZC\n"
		"SM 190600\n"
		"AAXX 19064\n"
		"72358 14/// /1019 10244 29095 60071 7////\n"
		"333 10297 20236 3/025 55300 2//// 70082 91129 91219\n"
		"555 00245 1011/ 20255 91129 91219\n"
		"666 10245 20242 7////=\n"
		"NNNN\n",
		"ZCZC+\n"
		"SM 190600\n"
		"AAXX+YYGGi+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+6RRRt+7wwWW+\n"
		"333+1sTTT_max+2sTTT_min+3Ejjj+553SS+2FFFF+7RRRR+9SSss+9SSss+\n"
		"555+0sTTT_land+1RRRr+2sTTT_avg+911ff+912ff+\n"
		"666+1snTxTxTx+2snTxTxTx+7VVVV+\n"
		"NNNN+\n"
	}, { 1,
		"04202 NIL;\n"
		"04203 46/// /1510 11048 21067 30142 40160 52006;\n",
      		"IIiii+NIL+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+\n"
	}, { 1,
		"01482 46/// /0510 10059 20014 30021 40030 57005 333 20059 91115;\n",
      		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 333+2sTTT_min+9SSss+\n"
	}, { 1,
		"03091 15981 /1208 10090 20044 30053 40133 57007 60002 91750\n"
  		"333 10107 55310 21294 8//99;\n",
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+9GGgg+\n"
  		"333+1sTTT_max+553SS+2FFFF+8NChh+\n"
	}, { 1,
		"AAXX 15064\n\n"
		"06011 05584 50805 10064 20008 30136 40205 57012 6///2\n",
		"AAXX+YYGGi+\n\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+\n"
	}, { 1,
		"AAXX 27064\n\n"
		"04018 42584 62909 10061 20035 40224 52020 86500 555 3//11 8662.04048 4211 QWOQU QPPYU WPPYE\n",
		"AAXX+YYGGi+\n\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+8NCCC+ 555+3Ejjj+ 8662.04048 4211 QWOQU QPPYU WPPYE\n"
	}, { 1,
		"AAXX 26184\n\n"
		"04018 21245 82021 10080 20076 40173 58016 72052 886// 333 10107 20079 69918 555 3//22 88703;\n",
		"AAXX+YYGGi+\n\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+8NCCC+ 333+1sTTT_max+2sTTT_min+6RRRt+ 555+3Ejjj+8NChh+\n"
	}, { 1,
		"06060 01675 60809 10191 20126 30053 40115895000 69902 72162 82172\n\n"
		"  333 10228 20130 69907 82840 85358;\n",
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+ 40115895000 6RRRt+7wwWW+8NCCC+\n\n"
		"  333+1sTTT_max+2sTTT_min+6RRRt+8NChh+ 85358;\n"
	}, { 1,
		"06070 05970 50506 10163 20104 30096 40126 57003 6///2\n\n"
		"  333 10201 20122 6///7 85/67;\n",
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+\n\n"
		"  333+1sTTT_max+2sTTT_min+6RRRt+8NChh+\n"
	}, { 2,
		"BSH03 20001 99540 10081 46/// ///// 22200 00067 20501 70003;\n\n"
		"BSH05 20001 99549 10082 46/// ///// 22200 20601 70003;\n",
		"IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+ 222Dv+0sTTT+2PPHH+70HHH+\n\n"
		"IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+ 222Dv+2PPHH+70HHH+\n"
	}, { 1,
		"13600 20123 99328 70293 46/// ///// 40331 52003\n"
		"222// 00200;\n",
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+4PPPP+5appp+\n"
		"222Dv+0sTTT+\n"
	}, { 1,
		"SMMJ01 LWOH 190000\n"
		"AAXX 1900\n"
		"13579 32998 03606 11106 21153 39397 40298 52004=\n",
		"TTAAii+CCCC+YYGGgg+\n"
		"AAXX 1900\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+\n"
	}, { 1,
		"SMML01 LMMM 190000\n"
		"AAXX 19004\n"
		"16597 32670 10403 10069 21015 30177 40268 52009 81500 333 81640 =\n",
		"TTAAii+CCCC+YYGGgg+\n"
		"AAXX+YYGGi+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+ 333+8NChh+ =\n"
	}, { 5,
		"SMOS01 LOWM 190000\n"
		"AAXX 19001\n"
		"11036 32565 73208 10000 21038 30065 40306 57008 8353/ 333 83629\n"
		"86360 91013 91113 91209=\n"
		"11010 35561 /2504 11031 21043 39946 40345 57009=\n"
		"11120 36/17 /9901 11111 21112 39620 40386 57005=\n"
		"11150 36429 /1802 11050 21055 39790 40363 57005=\n"
		"11240 36966 /1703 11031 21060 39870 40310 57004=\n",
		"TTAAii+CCCC+YYGGgg+\n"
		"AAXX+YYGGi+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+ 333+8NChh+8NChh+9SSss+9SSss+9SSss+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+\n"
	}, { 1,
		"SNVD01 KWBC 190700\n"
		"BBXX\n"
		"S6IG 19071 99278 70923 41/9/ /2603 10193 20100 40160 50001 7////\n"
		"22234 04239=\n",
		"TTAAii+CCCC+YYGGgg+\n"
		"BBXX+IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+\n"
		"222Dv+0sTTT+\n"
	}, { 1,
		"AAXX 23004 47411 15/84 /3603 10144 20114 30043 40074 50000 60012 333 20126=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+ 333+2sTTT_min+\n"
	}, { 1,
		"AAXX 22184 47409 11/50 80501 10090 20086 30023 40076 57002 69951 78085 887//=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+8NCCC+\n"
	}, { 1,
		"SMDL40 EDZW 201800\n"
		"AAXX 20181\n"
		"10004 46/60 /0408 10124 20107 30077 40077 57015\n"
		"222// 00103\n"
		"333 10141 20106 55304;\n",
		"TTAAii+CCCC+YYGGgg+\n"
		"AAXX+YYGGi+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+\n"
		"222Dv+0sTTT+\n"
		"333+1sTTT_max+2sTTT_min+553SS+\n"
	}, { 1,
		"10147 12882 50605 10213 20111 30048 40065 58008 69902 81031\n"
		"333 10265 20152 30017 55304 20454 30310 41284 81358 84076;\n",
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+\n"
		"333+1sTTT_max+2sTTT_min+3Ejjj+553SS+2FFFF+3FFFF+4FFFF+8NChh+8NChh+\n"
	}, { 1,
		"10200 07961 20503 10204 20155 30065 40063 55009 69932 70060\n"
		"333 10215 20130 3/012 553// 2//// 3//// 69907 82/60;\n",
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+\n"
		"333+1sTTT_max+2sTTT_min+3Ejjj+553SS+2FFFF+3FFFF+6RRRt+8NChh+\n"
	}, { 1,
		"03302 15973 /0106 10133 20069 30105 40117 57002 60002 91750\n"
		"333 10143 55310 21364 8//99;\n",
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+9GGgg+\n"
		"333+1sTTT_max+553SS+2FFFF+8NChh+\n"
		// "333+1sTTT_max+55jjj+jjjjj+8NChh+\n"
	}, { 0,
		"333 10188 91107;\n",
		"333+1sTTT_max+9SSss+\n"
	}, { 1,
		"AAXX 23061 15108 02298 62702 10135 20135 38165 48563 53002 60022 86500 333 10151 20131 30/// 60007 70022 95080 444 86154=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+1sTTT_max+2sTTT_min+3Ejjj+6RRRt+7RRRR+9SSss+ 444+NCHHC+\n"
	}, { 1,
		"AAXX 23051 15346 22997 03601 10237 20171 39879 40154 52011 333 60005 91002 91102 95090=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 333+6RRRt+9SSss+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 23034 47090 32665 61407 10231 20206 30062 40083 57008 82501=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+\n"
	}, { 1,
		"AAXX 23004 47155 32962 60903 10241 20175 30051 40093 57004 80001 333 20194 30034=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+ 333+2sTTT_min+3Ejjj+\n"
	}, { 1,
		"AAXX 22124 47090 12668 60000 10200 20189 30080 40101 51003 69912 86500 333 10227 31020 92020=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+1sTTT_max+3Ejjj+9SSss+\n"
	}, { 1,
		"AAXX 22124 47104 11650 62902 10193 20174 30008 40099 53003 69952 71022 85500 333 10247 30020=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+8NCCC+ 333+1sTTT_max+3Ejjj+\n"
	}, { 1,
		"AAXX 23004 47127 329// /0001 10248 20175 39959 40090 57009 333 20189 3/028=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 333+2sTTT_min+3Ejjj+\n"
	}, { 1,
		"AAXX 22181 26029 27/70 /0704 10155 20073 30150 40178 57008 70000 333 10167 20133 60007 91109 555 20133 50142= \n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+7wwWW+ 333+1sTTT_max+2sTTT_min+6RRRt+9SSss+ 555+2sTTT_avg+5jjjj+ \n"
	}, { 1,
		"AAXX 22151 26045 27/81 00505 10187 20068 30193 40195 56008 70000 80/// 333 10190 20104 60007 80/// 555 1/036 20104 3/010 50134=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+7wwWW+8NCCC+ 333+1sTTT_max+2sTTT_min+6RRRt+8NChh+ 555+1VVff+2sTTT_avg+3Ejjj+5jjjj+\n"
	}, { 1,
		"AAXX 22181 26058 27/84 00505 10176 20076 30175 40182 58007 70000 80/// 333 10199 20158 60007 80/// 91109 555 1/018 20158 3/018 50162 52018=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+7wwWW+8NCCC+ 333+1sTTT_max+2sTTT_min+6RRRt+8NChh+9SSss+ 555+1VVff+2sTTT_avg+3Ejjj+5jjjj+5jjjj+\n"
	}, { 1,
		"AAXX 22124 47407 11/60 82904 10150 20130 39900 40065 53006 69902 72582 886// 333 10188=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+8NCCC+ 333+1sTTT_max+\n"
	}, { 1,
		"AAXX 23034 08055 NIL=\n",
		"AAXX+YYGGi+ IIiii+NIL+\n"
	}, { 1,
		"AAXX 23074 08045 46/// /2102 10139 20110 39933 40241 53003 555 60005=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 555+6GGmm+\n"
	}, { 1,
		"AAXX 22184 08042 02680 23609 10161 20084 39815 40246 53003 60002 81508 333 10183 60007=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+1sTTT_max+6RRRt+\n"
	}, { 1,
		"AAXX 22184 08140 12970 33006 10260 21017 39231 48564 55001 60002 80001 333 10269=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+1sTTT_max+\n"
	}, { 1,
		"AAXX 22124 08085 02580 23210 10221 20100 39696 40213 57005 60001 81101 333 60007=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+6RRRt+\n"
	}, { 1,
		"AAXX 22124 08130 12970 00406 10220 20070 39462 40213 58005 60001 333 50620=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+ 333+5jjjj+\n"
	}, { 1,
		"AAXX 23064 08141 12960 23504 10106 20077 39383 40240 52007 60002 80008 333 20103 30009 55141 70000=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+2sTTT_min+3Ejjj+55jjj+jjjjj+\n"
	}, { 1,
		"AAXX 23064 08160 12970 52812 10163 20102 39928 40232 53004 60002 80008 333 20157 55138 70000=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+2sTTT_min+55jjj+jjjjj+\n"
	}, { 1,
		"AAXX 23064 08184 02980 20000 10204 20125 30068 40219 5//// 60002 80001 333 20152 3/015 55087 60007 70000=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+2sTTT_min+3Ejjj+55jjj+jjjjj+7RRRR+\n"
	}, { 1,
		"AAXX 22124 08202 02970 11104 10243 20044 39301 42842 58006 60001 80001 333 50880 60007=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+5jjjj+6RRRt+\n"
	}, { 1,
		"AAXX 22154 07330 22680 32612 10198 20110 30165 40237 52003 83100 333 60007 83840 90710 91121 93100 555 60005=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+ 333+6RRRt+8NChh+9SSss+9SSss+9SSss+ 555+6GGmm+\n"
	}, { 1,
		"AAXX 23064 70174 32766 60000 10206 20044 39892 40131 56007 90553 333 10217 20067 555 92306=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+9GGgg+ 333+1sTTT_max+2sTTT_min+ 555+9SSss+\n"
	}, { 0, // Cannot find WMO station:91320
		"AAXX 22124 91320 32474 80808 10283 20261 30122 40127 83101 333 562/9 58007 83815 85073 555 92212=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+8NCCC+ 333+5jjjj+5jjjj+8NChh+8NChh+ 555+9SSss+\n"
	}, { 1,
		"AAXX 22214 47409 41/50 80102 10094 20091 30030 40083 51007 72588 887//=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+7wwWW+8NCCC+\n"
	}, { 1,
		"AAXX 22184 08015 12560 80204 10146 20100 39854 40255 54000 60002 8277/ 333 10178=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+1sTTT_max+\n"
	}, { 1,
		"AAXX 22181 26124 01/84 10504 10188 20047 30140 40169 57002 60002 70200 80008 333 10227 20178 60007 80/// 91110 555 1/023 20178 3/022 50160 52020=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+8NCCC+ 333+1sTTT_max+2sTTT_min+6RRRt+8NChh+9SSss+ 555+1VVff+2sTTT_avg+3Ejjj+5jjjj+5jjjj+\n"
	}, { 1,
		"AAXX 22091 26231 21/81 00605 10206 20078 30173 40187 57007 70200 333 10211 20060 60007 80/// 91008 91108 555 1/049 20060 3/005 50160= \n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+7wwWW+ 333+1sTTT_max+2sTTT_min+6RRRt+8NChh+9SSss+9SSss+ 555+1VVff+2sTTT_avg+3Ejjj+5jjjj+ \n"
	}, { 1,
		"AAXX 22154 64500 42460 42006 10275 20211 30108 40120 84500 333 58008 83611 84630=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+8NCCC+ 333+5jjjj+8NChh+8NChh+\n"
	}, { 1,
		"AAXX 22184 64550 32458 8//// 10248 20231 30//0 40//0 885// 333 10274 5//// 84610 88623=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+8NCCC+ 333+1sTTT_max+5jjjj+8NChh+8NChh+\n"
	}, { 1,
		"AAXX 22094 64550 42460 8//// 10250 20234 30//0 40//0 888// 333 5//// 84813 88626 94939 95839=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+8NCCC+ 333+5jjjj+8NChh+8NChh+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 22134 61901 41580 71116 10189 20127 39699 40205 72582 878// 91250 333 58003 81822 87635 91026 90710 91133=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+7wwWW+8NCCC+9GGgg+ 333+5jjjj+8NChh+8NChh+9SSss+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 27064 65222 11458 70000 10237 20234 3//// 4//// 60092 76066 86538 333 20233 5//// 86610=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+6RRRt+7wwWW+8NCCC+ 333+2sTTT_min+5jjjj+8NChh+\n"
	}, { 0, // Cannot find WMO station:65213
		"AAXX 27094 65213 42460 72004 10265 20236 3//// 4//// 875// 333 5//// 87612=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+8NCCC+ 333+5jjjj+8NChh+\n"
	}, { 1,
		"AAXX 27094 91582 24570 /0810 10227 20178 30135 40170 50009 700// 333 69907 90710 91120 555 60005=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+7wwWW+ 333+6RRRt+9SSss+9SSss+ 555+6GGmm+\n"
	}, { 1,
		"AAXX 27094 62318 32560 43110 10300 20231 40104 54000 84800=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+8NCCC+\n"
	}, { 1,
		"AAXX 22154 08015 41558 80306 10148 20094 39854 40255 52003 70522 8271/=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+7wwWW+8NCCC+\n"
	}, { 1,
		"AAXX 23064 08015 11550 52802 10122 20113 39848 40252 54000 60002 71022 82806 333 20118 30010 50144 55016 70000=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+8NCCC+ 333+2sTTT_min+3Ejjj+5jjjj+55jjj+jjjjj+\n"
	}, { 1,
		"AAXX 22154 08001 42475 13207 10183 20109 30168 40248 55000 81541=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+\n"
	}, { 1,
		"AAXX 22124 72654 15966 60000 10167 20150 39718 40175 53006 69931 91155 333 10278 20139 70003 555 92212=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+9GGgg+ 333+1sTTT_max+2sTTT_min+7RRRR+ 555+9SSss+\n"
	}, { 1,
		"AAXX 22181 15015 02598 53502 10239 20169 39573 42804 52009 60002 84301 333 10269 20226 30042 60007 91003 91105=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+1sTTT_max+2sTTT_min+3Ejjj+6RRRt+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 22184 08213 12770 23108 10273 21007 39056 48560 53001 60002 81041 333 10293=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+1sTTT_max+\n"
	}, { 1,
		"AAXX 22184 08215 12870 12210 10201 21023 38179 48556 53002 60002 80005 333 10230 95000=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+1sTTT_max+9SSss+\n"
	}, { 1,
		"AAXX 23064 08231 12967 12401 10161 20130 39150 48569 53005 60002 80001 333 20161 30016 50904 55109 70000=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+2sTTT_min+3Ejjj+5jjjj+55jjj+jjjjj+\n"
	}, { 1,
		"AAXX 23064 08284 02470 12802 10216 20159 30154 40227 53013 60002 81600 333 20195 30019 50484 55082 60007 70000=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+2sTTT_min+3Ejjj+5jjjj+55jjj+jjjjj+7RRRR+\n"
	}, { 1,
		"AAXX 23061 10004 46/29 /2313 10143 20114 30153 40153 53011 222// 00140 333 10153 20137 55069 55308 91117 91214=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 222Dv+0sTTT+ 333+1sTTT_max+2sTTT_min+55jjj+jjjjj+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 23021 10004 46/60 /2213 10147 20124 30139 40139 53002 222// 00140 333 55300 91116 91213=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 222Dv+0sTTT+ 333+553SS+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 22091 10004 46/58 /2011 10145 20113 30110 40110 51013 222// 00137 333 20133 55306 91117 91213=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 222Dv+0sTTT+ 333+2sTTT_min+553SS+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 23061 10007 46/59 /2110 10138 20116 30158 40158 51008 222// 00130 333 10151 20133 55068 55300 91114 91211=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 222Dv+0sTTT+ 333+1sTTT_max+2sTTT_min+55jjj+jjjjj+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 22181 10007 46/57 /1903 10142 20113 30145 40145 51002 222// 00136 333 10147 20125 55301 91116 91212=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 222Dv+0sTTT+ 333+1sTTT_max+2sTTT_min+553SS+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 23081 10015 42560 72111 10145 20120 30162 40172 53010 81275 333 55307 21589 30922 81828 84366 85071 91113 91211=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+ 333+553SS+2FFFF+3FFFF+8NChh+8NChh+8NChh+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 22151 10015 21580 52303 10156 20110 30138 40148 51008 72598 82972 333 55304 21373 30784 60017 82930 83075 91114 91211 96481=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+7wwWW+8NCCC+ 333+553SS+2FFFF+3FFFF+6RRRt+8NChh+8NChh+9SSss+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 23081 10022 47466 82109 10149 20125 30161 40170 53009 72365 333 55301 20826 30718 85/16 87/21 88/30 91113=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+7wwWW+ 333+553SS+2FFFF+3FFFF+8NChh+8NChh+8NChh+9SSss+\n"
	}, { 1,
		"AAXX 22171 10022 45977 02105 10169 20109 30142 40151 50002 333 55307 21236 30490=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 333+553SS+2FFFF+3FFFF+\n"
	}, { 1,
		"AAXX 22081 10022 45571 52207 10161 20123 30116 40125 53018 333 55303 21177 30689 83/23 84/29 85/35=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 333+553SS+2FFFF+3FFFF+8NChh+8NChh+8NChh+\n"
	}, { 1,
		"AAXX 23064 07005 02475 22307 10128 20105 30143 40233 53013 60002 82201 333 10163 20107 31010 55053 60007 70012 82816 90710 91113 555 60005=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+ 333+1sTTT_max+2sTTT_min+3Ejjj+55jjj+jjjjj+7RRRR+8NChh+9SSss+9SSss+ 555+6GGmm+\n"
	}, { 1,
		"AAXX 27094 96315 41459 72207 10273 20236 40089 72582 83968 333 82816 81917 85277=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+7wwWW+8NCCC+ 333+8NChh+8NChh+8NChh+\n"
	}, { 1,
		"SNVD17 CWTO 261300 \n"
		"BBXX \n"
		"45142 26131 99427 70793 46/// /0006 10171 39936 40142 52008 \n"
		"22200 00202 10301 70003 333 91207 =\n",
		"TTAAii+CCCC+YYGGgg+ \n"
		"BBXX+IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+3PPPP+4PPPP+5appp+ \n"
		"222Dv+0sTTT+1PPHH+70HHH+ 333+9SSss+ =\n"
	}, { 2,
		"SNVD22 KWNB 261300 RRR\n"
		"BBXX\n"
		"46232 26131 99325 71174 46/// ///// 1//// 91330 22200 00190 10703\n"
		"20703 320// 41201 70015=\n"
		"46247 26131 99378 71228 46/// ///// 1//// 91321 22200 00139 10802\n"
		"20802 322// 41301 70012=\n",
		"TTAAii+CCCC+YYGGgg+RRx+\n"
		"BBXX+IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+9GGgg+ 222Dv+0sTTT+1PPHH+2PPHH+3dddd+4PPHH+70HHH+\n"
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+9GGgg+ 222Dv+0sTTT+1PPHH+2PPHH+3dddd+4PPHH+70HHH+\n"
	}, { 1,
		"641 \n"
		"SMVE01 KWBC 141800 RRA\n"
		"BBXX\n"
		"A8OK5 14183 99132 51250 41598 50816 10280 20229 40152 51014 70222\n"
 		"83145 22214 04273 20302 316// 40504 5//// 80245=\n",
		"641 \n"
		"TTAAii+CCCC+YYGGgg+RRx+\n"
		"BBXX+IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+8NCCC+ 222Dv+0sTTT+2PPHH+3dddd+4PPHH+5PPHH+8aTTT+\n"
	}, { 1,
		"765 \n"
		"SMCA02 KWBC 141800\n"
		"AAXX 14184\n"
		"78255 42559 51603 10269 20148 39972 40136 52200 84502 333 10270\n"
		"20195 84620=\n"
		"78317 NIL=\n"
		"78318 NIL=\n",
		"765 \n"
		"TTAAii+CCCC+YYGGgg+\n"
		"AAXX+YYGGi+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+ 333+1sTTT_max+2sTTT_min+8NChh+\n"
		"IIiii+NIL+\n"
		"IIiii+NIL+\n"
	}, { 2,
		"965 \n"
		"SMVD20 KWNB 141800\n"
		"BBXX\n"
		"44097 14181 99410 70711 46/// ///// 1//// 91731 22200 00040 11308\n"
		"21005 317// 41306 70042=\n"
		"572 \n"
		"SMJD50 OJAM 141800\n"
		"AAXX 14184\n"
		"40255 32960 00000 10252 20115 39382 40065 54002 333 10310=\n",
		"965 \n"
		"TTAAii+CCCC+YYGGgg+\n"
		"BBXX\n"
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+9GGgg+ 222Dv+0sTTT+1PPHH+2PPHH+3dddd+4PPHH+70HHH+\n"
		"572 \n"
		"TTAAii+CCCC+YYGGgg+\n"
		"AAXX+YYGGi+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 333+1sTTT_max+\n"
	}, { 6,
		"SNVD01 KWBC 261300 RRK\n"
		"BBXX\n"
		"46036 26131 99484 71339 46/// /2103 10107 40103 57005 22200 00095\n"
 		"11403 70013 333 91203=\n"
		"41024 26131 99338 70785 46/// /0510 10225 40094 53030 91300=\n"
		"44022 26131 99409 70737 46/// /3107 10178 20113 40067 91330 22200\n"
 		"00199 333 91209 555 11072 22072=\n"
		"44140 26131 99429 70515 46/// ///// 1//// 4//// 5//// 22200 0////\n"
 		"1//// 70/// 333 912//=\n"
		"PCHM 26134 99525 71298 41298 81617 10110 20100 40132 53001 7//22\n"
 		"8/7// 22234 00120 20402 80105=\n"
		"PDAN 26134 99552 71314 41298 51909 10090 20045 40120 58020 7//11\n"
 		"8/7// 22273 00080 2//// 30000 80070=\n",
		"TTAAii+CCCC+YYGGgg+RRx+\n"
		"BBXX\n"
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+4PPPP+5appp+ 222Dv+0sTTT+1PPHH+70HHH+ 333+9SSss+\n"
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+4PPPP+5appp+9GGgg+\n"
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+9GGgg+ 222Dv+0sTTT+ 333+9SSss+ 555+110ff+220ff+\n"
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+4PPPP+5appp+ 222Dv+0sTTT+1PPHH+70HHH+ 333+9SSss+\n"
		"IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+8NCCC+ 222Dv+0sTTT+2PPHH+8aTTT+\n"
		"IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+8NCCC+ 222Dv+0sTTT+2PPHH+3dddd+8aTTT+\n"
	}, { 1,
		"AAXX 28091 11464 42470 53204 10168 20133 39196 42792 50005 83272\n"
		"333 83813 555 380//=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+\n"
		"333+8NChh+ 555+3Ejjj+\n"
	}, { 1,
		"AAXX 28091 06490 45981 01408 10225 20146 39561 40097 58015\n"
		"333 91112 91209=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+\n"
		"333+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 28091 06479 42970 41202 10257 20173 30028 40102 58016 83031\n"
		"333 83364 83072 91105 91203=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+\n"
		"333+8NChh+8NChh+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 28064 71048 16/// /2511 10119 20091 60001\n"
		"333 10209 20088 70347\n"
		"555 32948 40133=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+6RRRt+\n"
		"333+1sTTT_max+2sTTT_min+7RRRR+\n"
		"555+3Ejjj+4Esss+\n"
	}, { 1,
		"AAXX 28094 71050 36/// /3003 10062 20031 39083 40136 58005\n"
		"333 60001=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+\n"
		"333+6RRRt+\n"
	}, { 1,
		"AAXX 28064 71079 11574 61403 10153 20131 39624 49887 58011 69981 78082 86100\n"
		"333 10278 20150 70104 90932\n"
		"555 10000 20000 31628 40231=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+8NCCC+\n"
		"333+1sTTT_max+2sTTT_min+7RRRR+9SSss+\n"
		"555+1VVff+2sTTT_avg+3Ejjj+4Esss+\n"
	}, { 0, // Cannot find WMO station:27020
		"AAXX 28061 27020 32698 72202 10158 20127 39885 40026 52009 69950 87500\n"
		"333 20099 876//=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+8NCCC+\n"
		"333+2sTTT_min+8NChh+\n"
	}, { 1,
		"AAXX 28064 71133 17/// /2613 10151 20104 39343 40026 51025 69921 7//4/\n"
		"333 10170 20115 70012\n"
		"555 32833 40212=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+\n"
		"333+1sTTT_max+2sTTT_min+7RRRR+\n"
		"555+3Ejjj+4Esss+\n"
	}, { 0, // Cannot find WMO station:71547
		"AAXX 28064 71547 16/// /2513 10129 20046 60001\n"
		"333 10184 20098 70000\n"
		"555 32428 40022=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+6RRRt+\n"
		"333+1sTTT_max+2sTTT_min+7RRRR+\n"
		"555+3Ejjj+4Esss+\n"
	}, { 3,
		"599 \n"
		"SNCN19 CWAO 141806\n"
		"OOXX\n"
		"ARP01 14161 99345 50585 ///// 00681\n"
     		"26/// /0000 10114 29029 30044 91610 333 60000=\n"
		"ARP01 14161 99345 50585 ///// 00681\n"
     		"26/// /0000 10114 29028 30044 91620 333 60000=\n"
		"ARP01 14161 99345 50585 ///// 00681\n"
     		"26/// /0000 10115 29028 30043 91630 333 60000=\n",
		"599 \n"
		"TTAAii+CCCC+YYGGgg+\n"
		"OOXX+IIIII+YYGGi+99LLL+QLLLL+MMMULaULo+h0h0h0h0im+\n"
		"iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+9GGgg+ 333+6RRRt+\n"
		"IIIII+YYGGi+99LLL+QLLLL+MMMULaULo+h0h0h0h0im+\n"
		"iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+9GGgg+ 333+6RRRt+\n"
		"IIIII+YYGGi+99LLL+QLLLL+MMMULaULo+h0h0h0h0im+\n"
		"iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+9GGgg+ 333+6RRRt+\n"
	}, { 1,
		"SMVA13 LFPW 280000\n"
		"BBXX\n"
		"BAREU65 28004 99202 70180 46/// ///// 40137 52005\n"
		"22283=\n",
		"TTAAii+CCCC+YYGGgg+\n"
		"BBXX+IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+4PPPP+5appp+\n"
		"222Dv+\n"
	}, { 2,
		"SNCN19 CWAO 280016\n"
		"OOXX\n"
		"ARP01 27221 99345 50585 ///// 00311\n"
		"26/// /0000 10070 29040 30084 92220 333 60000=\n"
		"ARP01 27221 99345 50585 ///// 00311\n"
		"26/// /0000 10070 29041 30087 92230 333 60000=\n",
		"TTAAii+CCCC+YYGGgg+\n"
		"OOXX+IIIII+YYGGi+99LLL+QLLLL+MMMULaULo+h0h0h0h0im+\n"
		"iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+9GGgg+ 333+6RRRt+\n"
		"IIIII+YYGGi+99LLL+QLLLL+MMMULaULo+h0h0h0h0im+\n"
		"iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+9GGgg+ 333+6RRRt+\n"
	}, { 1,
		"AAXX 28091 11146 42/86 33201 10071 20018 37029 47148 53009 81202\n"
		"333 55310 818//=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+\n"
		"333+553SS+8NChh+\n"
	}, { 1,
		"AAXX 28061 11406 01465 39901 10182 20150 39593 40153 52003 60002 703// 83101\n"
		"333 20135 30009 50254 60005 70000 83813\n"
		"555 382// 50176 60175 70179 80170 90150=\n",
		"\n"
	}, { 1,
		"AAXX 23001 15090 02997 23001 10250 20177 30067 40154 52013 60001 81041 333 55300 10173 20000 3//// 55131 01487 22929 30369 60007 91005 91106=\n",
		"\n"
	}, { 1,
		"391\n"
		"SNVF01 KWBC 190700\n"
		"BBXX\n"
		"9VBL 19071 99514 10028 41/9/ /3009 10076 20023 40319 57012 7////\n"
		"22263 04074=\n",
		"391\n"
		"TTAAii+CCCC+YYGGgg+\n"
		"BBXX+IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+\n"
		"222Dv+0sTTT+\n"
	}, { 2,
		"439\n"
		"SNVD15 KWBC 190700\n"
		"BBXX\n"
		"41009 19071 99285 70802 46/// /2601 10205 40140 54000 90650 22200\n"
		"00251 10802 70011 333 91201 555 11007 22007=\n"
		"42002 19071 99259 70936 46/// /1602 10215 20136 40156 50000 90650\n"
		"22200 00244 10801 70007 333 91203 555 11021 22021 30602 41304\n"
		"60649 159018 136007 135016 139014 136023 132012=\n",
		"439\n"
		"TTAAii+CCCC+YYGGgg+\n"
		"BBXX\n"
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+4PPPP+5appp+9GGgg+ 222Dv+0sTTT+1PPHH+70HHH+ 333+9SSss+ 555+110ff+220ff+\n"
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+9GGgg+\n"
		"222Dv+0sTTT+1PPHH+70HHH+ 333+9SSss+ 555+110ff+220ff+3GGmm+4ddff+6GGmm+dddfff+dddfff+dddfff+dddfff+dddfff+dddfff+\n"
	}, { 1,
		"AAXX 22191 15420 22997 03401 10225 20204 30015 40121 52010 333 55300 0//// 20030 3//// 60005 91001 91101 99706=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 333+55jjj+jjjjj+2sTTT_min+3Ejjj+6RRRt+9SSss+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 23001 15420 02997 03201 10189 20176 30028 40135 52007 60001 333 55300 0//// 20000 3//// 55129 0//// 22868 3//// 60007 91002 91102=\n",
		"\n"
	}, { 0, // Cannot find WMO station:11244
		"AAXX 28091 11244 36/// /1402 10268 20170 39835 40140 57007\n"
		"333 55310=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+\n"
		"333+553SS+\n"
	}, { 1,
		"AAXX 23001 10015 01465 72110 10144 20119 30144 40154 54000 60021 72586 878// 333 31/// 55/// 21817 30776 55300 20000 30000 69927 83816 86631 91115 91213 96481=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+8NCCC+ 333+3Ejjj+55jjj+jjjjj+3FFFF+553SS+2FFFF+3FFFF+6RRRt+8NChh+8NChh+9SSss+9SSss+9SSss+\n"
	}, { 1,
		"AAXX 22181 10022 07780 82204 10158 20112 30148 40157 53008 60022 76160 333 10187 20133 3/012 55302 20414 30270 69907 85/50 87/58 88/62=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+ 333+1sTTT_max+2sTTT_min+3Ejjj+553SS+2FFFF+3FFFF+6RRRt+8NChh+8NChh+8NChh+\n"
	}, { 5,
		"SMSQ10 LZIB 190000\n"
		"AAXX 19001\n"
		"11816 32565 63603 10006 21030 30124 40294 57006 83830\n"
		"333 55002 82820 86360=\n"
		"11826 35/64 /3304 10002 21020 30086 40294 57006\n"
		"333 55000=\n"
		"11903 32970 71501 11019 21044 39874 40271 58006 87070\n"
		"333 55047 87360=\n"
		"11934 11658 82710 11023 21050 39412 42834 56004 69911 77077 8452/\n"
		"333 55031 84646 88458=\n"
		"11968 11335 80000 11063 21070 39978 40276 58003 69901 77172 8652/\n"
		"333 55000 83708 85638 88461=\n",
		"TTAAii+CCCC+YYGGgg+\n"
		"AAXX+YYGGi+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+\n"
		"333+55jjj+jjjjj+8NChh+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+\n"
		"333+55jjj+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+8NCCC+\n"
		"333+55jjj+jjjjj+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+8NCCC+\n"
		"333+55jjj+jjjjj+8NChh+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+8NCCC+\n"
		"333+55jjj+jjjjj+8NChh+8NChh+\n"
	}, { 7,
		"SMVF01 EGRR 261200 RRE\n"
		"BBXX\n"
		"62107 26124 99501 70061 46/// /2515 10139 20139 40207 52008 22200\n"
		"00137 10702 70008=\n"
		"AMOUK12 26124 99533 70062 46/// ///// 10183 20144 40164 57017=\n"
		"AMOUK06 26124 99534 10017 46/// ///// 10143 20113 40214 54003=\n"
		"AMOUK03 26124 99513 10032 46/// ///// 10198 20129 40227 52007=\n"
		"AMOUK34 26124 99513 10032 46/// ///// 10203 20108 4//// 52005=\n"
		"63106 26124 99610 10017 46/// /3309 10106 40167 51009 22200=\n"
		"63105 26124 99610 10017 47/98 /3310 10132 20048 40167 51008 700// 22200=\n",
		"TTAAii+CCCC+YYGGgg+RRx+\n"
		"BBXX\n"
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+ 222Dv+0sTTT+1PPHH+70HHH+\n"
		"IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+\n"
		"IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+\n"
		"IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+\n"
		"IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+\n"
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+4PPPP+5appp+ 222Dv+\n"
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+ 222Dv+\n"
	}, { 1,
		"AAXX 23044 61901 461// /1319 10151 20148 39710 40224 58010 90350 333 87/02 91028 90710 91128 555 7/097=\n",
		"\n"
	}, { 3,
		"628 \n"
		"SMVC01 KWBC 141800 RRA\n"
		"BBXX\n"
		"MZBN2 14183 99530 50772 41497 82328 10075 20040 40110 54000 70288\n"
 		"887// 22233 00090 20201 323// 40808 5//// 80060=\n"
		"NWS0020 14184 99008 50903 43/// /1103 10270 20250 40103 5////\n"
 		"7//// 8//// 222// 04276 2//// 3//// 4//// 5//// 6//// 8//// ICE ////=\n"
		"NWS0029 14184 99039 50376 43/// /0711 10254 20227 40116 5////\n"
 		"7//// 8//// 222// 04295 2//// 3//// 4//// 5//// 6//// 8//// ICE\n"
 		"////=\n",
		"628 \n"
		"TTAAii+CCCC+YYGGgg+RRx+\n"
		"BBXX+IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+8NCCC+ 222Dv+0sTTT+2PPHH+3dddd+4PPHH+5PPHH+8aTTT+\n"
		"IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+8NCCC+ 222Dv+0sTTT+2PPHH+3dddd+4PPHH+5PPHH+6IEER+8aTTT+ICE+cSbDz+\n"
		"IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+8NCCC+ 222Dv+0sTTT+2PPHH+3dddd+4PPHH+5PPHH+6IEER+8aTTT+ICE+cSbDz+\n"
	}, { 1,
		"04018 42588 60507 10006 21025 40109 51002 81258 555 3//07 81830 83357 85363\n",
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+8NCCC+ 555+3Ejjj+8NChh+8NChh+8NChh+\n"
	}, { 1,
		"ZCZC 412\n"
		"SIGL26 EKMI 252100\n"
		"AAXX 25214\n"
		"04418 46/// /1914 11150 21169 57011\n"
		"  333 553// 21328 ;\n"
		"04425 NIL ;\n"
		// "02 NIL ;\n"
		"04436 46/// /0107 11158 21202 ;\n"
		"04464 46/// /1610 11137 21171\n"
		"  333 553// 21530 ;\n"
		"04485 46/// /2503 11152 21175 37398 52006\n"
		"  333 553// 21624 ;\n"
		"04488 46/// /2810 11067 21094 37452 52013\n"
		"  333 553// 21310 ;\n"
		"NNNN\n",
		"ZCZC+ZCZC_id+\n"
		"TTAAii+CCCC+YYGGgg+\n"
		"AAXX+YYGGi+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+5appp+\n"
		"  333+553SS+2FFFF+ ;\n"
		"IIiii+NIL+ ;\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+ ;\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+\n"
		"  333+553SS+2FFFF+ ;\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+5appp+\n"
		"  333+553SS+2FFFF+ ;\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+5appp+\n"
		"  333+553SS+2FFFF+ ;\n"
		"NNNN+\n"
	}, { 1,
		"ZCZC 440\n"
		"SIGL26 EKMI 260300\n"
		"AAXX 26034\n"
		"04418 46/// /2615 11258 21285 57007\n"
		"  333 553// 20104 ;\n"
		"04425 NIL ;\n"
		"04432 NIL ;\n"
		"04436 NIL ;\n"
		"04464 46/// /1711 11236 21257 57128\n"
		"  333 553// 20011 ;\n"
		"04485 NIL ;\n"
		"NNNN\n",
		"ZCZC+ZCZC_id+\n"
		"TTAAii+CCCC+YYGGgg+\n"
		"AAXX+YYGGi+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+5appp+\n"
		"  333+553SS+2FFFF+ ;\n"
		"IIiii+NIL+ ;\n"
		"IIiii+NIL+ ;\n"
		"IIiii+NIL+ ;\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+5appp+\n"
		"  333+553SS+2FFFF+ ;\n"
		"IIiii+NIL+ ;\n"
		"NNNN+\n"
	}, { 1,
		"ZCZC 506\n"
		"SMEN43 EDZW 261200 CCB\n"
		"AAXX 26121\n"
		"01001 11275 32514 10065 21022 30096 40108 57005 69911 70161 81641\n"
		"      222// 00004 333 91123;\n"
		"NNNN\n",
		"ZCZC+ZCZC_id+\n"
		"TTAAii+CCCC+YYGGgg+CCx+\n"
		"AAXX+YYGGi+\n"
		"IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+6RRRt+7wwWW+8NCCC+\n"
		"      222Dv+0sTTT+ 333+9SSss+\n"
		"NNNN+\n"
	}, { 1,// Must check that one message only is created.
		"ZCZC 616\n"
		"SMVX41 EDZW 151800 RRC\n"
		"BBXX\n"
		"62119 05184\n",
		"ZCZC+ZCZC_id+\n"
		"TTAAii+CCCC+YYGGgg+RRx+\n"
		"BBXX\n"
		"IIiii+YYGGi+\n"
	}, { 1,// Must check that one message only is created.
		"ZCZC 619\n"
		"SMEN43 EDZW 15180  CCB\n"
		"AAXX 1518\n"
		"00271 17/82 0040 10870 20040 0116 4 16 57017 60002 700// 333\n"
      		" 10180 91108;\n"
		"NNNN\n",
		"\n"
	}, { 1,
		"AAXX 22231 15420 22997 03200 10194 20178 30027 40134 52009 333 55300 0//// 20000 3//// 60005 91001 91102=\n",
		"AAXX+YYGGi+ IIiii+iihVV+Nddff+1sTTT_air+2sTTT_dew+3PPPP+4PPPP+5appp+ 333+55jjj+jjjjj+2sTTT_min+3Ejjj+6RRRt+9SSss+9SSss+\n"
	}, { 1,
		"62144 20184 99534 10017 47297 /3612 10086 20080 40104#580\n",
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+ 40104#580\n"
	}, { 1,
		"63057 20004 99592 100-#110)6 10084 20028 49967 51002 700// 22200 10605 70024;\n\n"
		"63110 20004 99595 10015 47697 /1122 10080 20031 49979 50000 700// 22200 10504 70021;\n",
		"IIiii+YYGGi+99LLL+ 100-#110)6 1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+ 222Dv+1PPHH+70HHH+\n\n"
		"IIiii+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+ 222Dv+1PPHH+70HHH+\n"
	}, { 1,
		"DGEN 20004 99547 10078 41/96 22215 10085 20013 49950 54000 74100\n",
		"IIIII+YYGGi+99LLL+QLLLL+ iihVV+Nddff+1sTTT_air+2sTTT_dew+4PPPP+5appp+7wwWW+\n"
	}
};
static const size_t nb_tests = G_N_ELEMENTS(tests_arr);

// TODO: Test this command to detect unpublished messages:
// egrep "No publish2" flsynop.log | more


// ----------------------------------------------------------------------------

static void tstone( synop * ptr_synop, const synop_test * tst_arr, int tstnb )
{
	std::stringstream strm_tst ;
	dbg_strm = &strm_tst ;
	int nb_errors_parse = 0 ;
	int nb_errors_msg = 0 ;
	for(int i = 0; i < tstnb; ++i )
	{
		strm_tst.str(std::string());
		ptr_synop->cleanup();
		KmlServer::GetInstance()->Reset();
		for( const char * pc = tst_arr[i].m_input; *pc != '\0'; ++pc )
			ptr_synop->add( *pc );
		ptr_synop->flush(true);

		bool diff_parse = ( strm_tst.str() != tst_arr[i].m_output );
		if(diff_parse) ++nb_errors_parse;
		bool diff_msg = ( KmlServer::GetInstance()->NbBroadcasts() != tst_arr[i].m_expected_nb_msgs );
		if( diff_msg ) ++nb_errors_msg;

		if( diff_parse || diff_msg ) {
			std::cout << "Input  [" << tst_arr[i].m_input << "]\n" ;
		}
		if( diff_parse ) {
			std::cout << "Expect [" << tst_arr[i].m_output << "]\n" ;
			std::cout << "Actual [" << strm_tst.str() << "]\n" ;
		}
		if( diff_msg ) {
			std::cout << "Expected messages :" << tst_arr[i].m_expected_nb_msgs << "\n" ;
			std::cout << "Actual            :" << KmlServer::GetInstance()->NbBroadcasts() << "\n" ;
		}
		std::cout << "=============================================================\n";
	}

	std::cout << "Nb tests=" << tstnb
		<< " nb errors_parse=" << nb_errors_parse
		<< " nb errors_msg=" << nb_errors_msg
		<< '\n' ;
}

// ----------------------------------------------------------------------------

/* TODO: For testing detection on strings with errors, we will corrupt the existing strings,
 * and try to decode them with one or two insertions/deletions/changes. */

static void process_file( synop * ptr_synop, const char * namin )
{
	std::cout << "Processing:" << namin << "\n";
	ptr_synop->cleanup();
	std::ifstream filin( namin );
	std::string namout = namin + std::string(".out");
	std::ofstream filout( namout.c_str() );
	dbg_strm = &filout ;
	char c;
	int nb = 0 ;
	while( filin.get(c) )
	{
		ptr_synop->add( c );
		++nb ;
	}
	ptr_synop->flush(true);
	filin.close();
	filout.close();
	std::cout << "====== " << nb << " chars =======================================================\n";
}

// ----------------------------------------------------------------------------
//
// These stub definitions so we do not link with too much fldigi code.
//


static std::ofstream g_adif_file;

QsoHelper::QsoHelper(int the_mode)
	: qso_rec( NULL )
{
}

QsoHelper::~QsoHelper()
{
	g_adif_file << "========================================\n";
}

void QsoHelper::Push( ADIF_FIELD_POS pos, const std::string & value )
{
#define QSO_TITLE(n) case n : g_adif_file << #n ; break ;
	switch(pos) {
		QSO_TITLE(FREQ)
		QSO_TITLE(CALL)
		QSO_TITLE(MODE)
		QSO_TITLE(NAME)
		QSO_TITLE(QSO_DATE)
		QSO_TITLE(QSO_DATE_OFF)
		QSO_TITLE(TIME_OFF)
		QSO_TITLE(TIME_ON)
		QSO_TITLE(QTH)
		QSO_TITLE(RST_RCVD)
		QSO_TITLE(RST_SENT)
		QSO_TITLE(STATE)
		QSO_TITLE(VE_PROV)
		QSO_TITLE(NOTES)
		QSO_TITLE(QSLRDATE)
		QSO_TITLE(QSLSDATE)
		QSO_TITLE(GRIDSQUARE)
		QSO_TITLE(BAND)
		QSO_TITLE(CNTY)
		QSO_TITLE(COUNTRY)
		QSO_TITLE(CQZ)
		QSO_TITLE(DXCC)
		QSO_TITLE(IOTA)
		QSO_TITLE(ITUZ)
		QSO_TITLE(CONT)
		QSO_TITLE(MYXCHG)
		QSO_TITLE(XCHG1)
		QSO_TITLE(SRX)
		QSO_TITLE(STX)
		QSO_TITLE(TX_PWR)
		QSO_TITLE(EXPORT)
		default: g_adif_file << pos ;
	}
	g_adif_file << ":" << value << "\n";
#undef QSO_TITLE
}

// ----------------------------------------------------------------------------

// Test program: Text files containing synop samples are given on the command line.

// Do we run a couple of internal self-test functions ?
static	bool internal_test = false ;

static	bool kml_balloon_as_matrix = false ;

// If set, the output document, instead of having Synop messages, will get only
// the regular expressions names. This helps for debugging and the output
// is locale-independent.
static	bool regex_output_only = false ;

// If set, at the end of the execution, prints all the regular expression
// and the number of times each of them was used.
static	bool display_synop_usage = false ;

// Where the CSV files for Synop decoding are loaded from.
static	std::string data_dir = "data/";

// Where the KML files are periodically written to.
static	std::string kml_dir = "kml/";

// Where the KML files are loaded from at startup.
static	std::string load_dir ;

// Contains the output of all logged nformation.
static	std::string dbg_file = "FlSynop.log";

// This command must be executed each time KML files are saved to disk.
static	std::string exec_cmd = "echo Subprocess called; date";

static const char * g_adif_name = "adif.txt";

int main(int argC, char * argV[] )
try {
	int option_index = 0 ;
	opterr = 0;

	for(;;) {
		static const char shortopts[] = "b:k:l:d:utmrvwh";
		static const struct option longopts[] = {
			{ "data_dir",  required_argument, 0, 'b' },
			{ "kml_dir",   required_argument, 0, 'k' },
			{ "load_dir",  required_argument, 0, 'l' },
			{ "dbg",       required_argument, 0, 'd' },
			{ "usage",     no_argument,       0, 'u' },
			{ "test",      no_argument,       0, 't' },
			{ "matrix",    no_argument,       0, 'm' },
			{ "regex",     no_argument,       0, 'r' },
			{ "version",   no_argument,       0, 'v' },
			{ "help",      no_argument,       0, 'h' },
			{ NULL, 0, 0, 0 }
		};

		int c = getopt_long(argC, (char * const *)argV, shortopts, longopts, &option_index);

		switch (c) {
			case -1:
				break;
			case 0:
				// handle options with non-0 flag here
				if (longopts[option_index].flag != 0)
					continue;
				printf ("option %s", longopts[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				printf ("\n");
				continue;
			case 'b':
				data_dir = optarg;
				continue;
			case 'k':
				kml_dir = optarg;
				continue;
			case 'l':
				load_dir = optarg;
				continue;
			case 'd':
				dbg_file = optarg;
				continue;
			case 't':
				internal_test = true ;
				continue ;
			case 'm':
				kml_balloon_as_matrix = true;
				continue;
			case 'r':
				regex_output_only = true ;
				continue ;
			case 'u':
				display_synop_usage = true ;
				continue ;
			case 'v':
				std::cout << "version 1.0\n";
				exit(EXIT_SUCCESS);
			case ':':
			default:
				std::cerr << "Unrecognized option\n";
				exit(EXIT_FAILURE);
			case 'h':
			case '?':
				std::cout << "Valid options are:\n" ;
				for( size_t i = 0; longopts[i].name != NULL; ++i ) {
					std::cout << "    " << longopts[i].name << "\n" ;
				}
				exit(EXIT_SUCCESS);
		}
		break;
	}

	g_adif_file.open( g_adif_name, std::ios_base::out );

	/// Where the warning, informational and error messages are written.
//	debug::start(dbg_file.c_str());

	/// Just for testing the loading. We load from one dir and save in another.
	if( ! load_dir.empty() ) {
		std::cout << "Loading from " << load_dir << "\n";
		KmlServer::GetInstance()->InitParams( exec_cmd, load_dir );
	}
	KmlServer::GetInstance()->InitParams( exec_cmd, kml_dir, 10000, 0, 120, kml_balloon_as_matrix );
	if( ! load_dir.empty() ) {
		std::cout << "Loading tested: Destination=" << kml_dir << "\n";
		exit(0);
	}

	// Must be done before any use of WMO stations data.
	// http://weather.noaa.gov/data/nsd_bbsss.txt
	std::cout << "Opening:" << data_dir << "\n";
	if( ! SynopDB::Init(data_dir) ) {
		std::cerr << "Error opening:" << data_dir << ":" << strerror(errno) << "\n";
		exit(EXIT_FAILURE);
	}

	// Serializer::SetSrl( & fldigiSerial );

	if( internal_test ) {
		test_coordinates();
		test_wmo();
		test_buoy();
		test_ship();
		test_jcomm();
	}

	synop::setup<tst_callback>();
	synop * ptr_synop = synop::instance();

	if( internal_test ) {
		KmlServer::GetInstance()->Reset();

		synop::SetTestMode(false);
		tstone( ptr_synop, tests_arr_full, nb_tests_full );

		synop::SetTestMode(true);
		tstone( ptr_synop, tests_arr, nb_tests );
		KmlServer::GetInstance()->Reset();
	}
	
	synop::SetTestMode(regex_output_only);

	while (optind < argC)
		process_file( ptr_synop, argV[optind++] );

	if( display_synop_usage ) {
		synop::regex_usage();
	}

	KmlServer::Exit();

	g_adif_file.close();
	// std::cin.get();
	return 0 ;
}
catch( const std::exception & exc )
{
	std::cout << "Exception:" << exc.what() << '\n';
	exit(EXIT_FAILURE);
}

// ----------------------------------------------------------------------------
