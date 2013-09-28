//
//	navtex_shapes.cxx
//
// Copyright (C) 2013
//		Remi Chateauneu, F4ECW
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fldigi; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "navtex_shapes.h"

ShapesSetT::ShapesSetT( const std::string & rawText )
{
	/** In a given message, all coordinates are encoded using the same format.
	 * Therefore, when parsing a message, only one regular expression should be used.
	 *
	 * We can start by the most commonly used, and keep the format which yields the
	 * biggest number of coordinates. Normally, only one should match anything.
	 *
	 * When preceded by "BOUND" or "AREA", this is a closed contour.
	 *
	 * When preceded by "LIMIT", "ROUTE" or "JOIN", this is a path.
	 *
	 * When preceeded by "LIGHTHOUSE", "PLATFORM", "BUOY", "ESTABLISHED",
	 * "POSITION" or "LOCATED", this is a point
	 * or a set of points.
	 */
}

#ifdef NAVTEX_SHAPES_COMMAND_LINE

/// Test data used only in command-line mode, for testing.
static const char * tstInputs[] = {

	"LIGHT\n"
	"55-13.7N 011-05.4E INOPERATIVE",

	"BOUNDED BY:\n"
	"55-37.0N 020-46.0E\n"
	"55-34.0N 021-00.0E\n"
	"55-25.0N 021-00.0E\n"
	"55-22.0N 020-46.0E\n"
	"55-25.0N 020-42.0E\n"
	"55-34.0N 020-42.0E.\n"
	"AREA\n",

	"AREA 54-38.3N 019-53.3E\n"
	"54-37.7N 019-54.8E\n"
	"54-35.6N 019-57.8E\n"
	"54-34.7N 019-54.8E\n"
	"AREA\n",

	"BOUNDED BY:\n"
	"55-39.0N 020-56.0E\n"
	"55-38.0N 020-56.0E\n"
	"55-38.0N 020-55.0E\n"
	"55-39.0N 020-55.0E\n"
	"IS\n",

	"LOCATED 56-01.68N 020-45.99E. LEAST\n",

	"BOUNDED BY 59-54.2N 027-04.6E\n"
	"59-57.4N 027-13.7E 59-53.8N 027-17.5E 59-51.0N 027-07.5E\n",

	"LIGHTHOUSE 60-02.1N 028-21.9E. TEMPY\n",

	"JOINING:\n"
	"59-13N 021-08E\n"
	"58-52N 020-29E\n"
	"500 METER\n",

	"ESTIMATED LIMIT OF ALL KNOWN ICE:\n"
	"4649N 5411W TO 4530N 5400W TO\n"
	"4400N 4900W TO 4545N 4530W TO\n"
	"4715N 4530W TO 5000N 4715W TO\n"
	"5530N 5115W TO 5700N 5545W.\n",

	"THE FOLLOWING ROUTE:\n"
	"41 53.5N - 016 14.7E,\n"
	"42 14,6N - 016 40,2E,\n"
	"42 04.4N - 016 40.8E,\n"
	"41 31.3N - 016 00.9E,\n"
	"41 25.4N - 016 11.8E,\n"
	"41 56.0N - 016 46.8E,\n"
	"41 46.1N - 016 51.8E,\n"
	"41 19.7N A PQY WWMYE,\n"
	"41 14.8N - 016 35.4E,\n"
	"41 37.6N_- 016 58.2E,=\n"
	"-1 28.1N - 017 05.4E,\n"
	"41 10.7N - 016 49.0E,\n"
	"41 06.9N - 017 01.1E,\n"
	"41 16.2N - 017 10.4E,\n"
	"41 12.0N - 017 19.8E,\n",

	"(FOUR) BOE POSITIONED IN THE FOLLOWING POINTS:\n"
	"40 27.800N - 018 39.262E DEPTH  645 MT,\n"
	"40 27.737N - 018 36.990E DEPTH  400 MT,\n"
	"40 21.506N - 018 26.501E DEPTH  100 MT,\n"
	"40 19.209N - 018 23.715E DEPTH  23 MT.\n"
	"TRANSIT SHIPS BEWARE\n",

	"MOORING BUOYS ESTABLISHED IN\n"
	"55-18.39N 019-39.85E\n"
	"55-18.50N 019-39.77E\n"
	"55-27.99N 019-26.57E\n"
	"NNNN\n",

	"AREA BOUNDED BY:\n"
	"55-39.0N 020-56.0E\n"
	"55-38.0N 020-56.0E\n"
	"55-38.0N 020-55.0E\n"
	"55-39.0N 020-55.0E\n"
	"IS TEMPO RESTRICTED\n",

	"GAS PIPELINE JOINING_\n"
	"59-24N 022-09E\n"
	"59-13N 021-08E\n"
	"500 METER BERTH REQUESTED\n",

	"BOE POSITIONED IN THE FOLLOWING POINTS:\n"
	"C6. LAT. 40 27.800N - LONG. 018 39.262E DEPTH  645 MT,\n"
	"C7. LAT. 40 27.737N - LONG. 018 36.990E DEPTH  400 MT,\n"
	"C8. LAT. 40 21.506N - LONG. 018 26.501E DEPTH  100 MT,\n"
	"C9. LAT. 40 19.209N - LONG. 018 23.715E DEPTH  23 MT.\n"
	"TRANSIT SHIPS BEWARE\n",
	
	"THE FOLLOWING ROUTE:\n"
	"01. LAT. 41 53.5N - LONG. 016 14.7E,\n"
	"02. LAT. 42 14,6N - LONG. 016 40,2E,\n"
	"03. LAT. 42 04.4N - LONG. 016 40.8E,\n"
	"04. LAT. 41 31.3N - LONG. 016 00.9E,\n"
	"05. LAT. 41 25.4N - LONG. 016 11.8E,\n"
	"06. LAT. 41 56.0N - LONG. 016 46.8E,\n"
	"07. LAT. 41 46.1N - LONG. 016 51.8E,\n"
	"08. LAT. 41 19.7N - LONG. 016 22.6E,\n"
	"09. LAT. 41 14.8N - LONG. 016 35.4E,\n"
	"10. LAT. 41 37.6N - LONG. 016 58.2E,\n"
	"11. LAT. 41 28.1N - LONG. 017 05.4E,\n"
	"12. LAT. 41 10.7N - LONG. 016 49.0E,\n"
	"13. LAT. 41 06.9N - LONG. 017 01.1E,\n"
	"14. LAT. 41 16.2N - LONG. 017 10.4E,\n"
	"15. LAT. 41 12.0N - LONG. 017 19.8E,\n"
	"16. LAT. 41 02.7N - LONG. 017 11.6E,\n"
	"17. LAT. 40 58.2N - LONG. 017 20.7E,\n"
	"18. LAT. 41 05.0N - LONG. 017 26.9E,\n"
	"19. LAT. 41 01.9N - LONG. 017 37.0E,\n"
	"20. LAT. 40 51.1N - LONG. 017 29.5E,\n"
	"21. LAT. 40 47.4N - LONG. 017 39.2E,\n"
	"22. LAT. 41 01.4N - LONG. 017 48.9E.\n"
	"TRANSIT SHIPS BEWARE\n",

	"POSITION:\n"
	"- LAT. 40 39N - LONG. 018 17E.\n"
	"TRANSIT SHIPS BEWARE\n",

	"BOUNDED BY THE FOLLOWING POINTS:\n"
	"A. - 40 25 31N - 18 15 30E, B. - 40 30 20N - 18 16 30E,\n"
	"C. - 40 29 25N - 18 19 03E, D. - 40 27 45N - 18 20 58E,\n"
	"E. - 40 25 55N - 18 22 28E, F. - 40 23 05N - 18 23 18E,\n"
	"G. - 40 23 54N - 18 17 30E.\n"
	"SAILING\n",

	"MOORING BUOYS ESTABLISHED IN\n"
	"55-18.39N 019-39.85E\n"
	"55-18.50N 019-39.77E\n"
	"55-27.99N 019-2_.57E\n"
	"NNN\n",

	"LIGHTHOUSE 52-19.63N 001-40.89E. RADAR\n",

	"MAST, 53-53.1N 001-59.5E, ALL NAVAIDS\n",

	"PLATFORM, 52-54.3N 002-35.9E, ALL\n",

	"PLATFORMS 49/17-CD,  53-25.4N 002-22.5E AND 49/17-HD 53-29.8N 002-19.4E\n"
	"ALL NAVAIDS INOPERATIVE.\n",

	"CONSTRUCTION 54-08.98N 002-49.41E.\n",

	"BOUNDED BY 55-39N, 56-45N, 001-11E AND 003-38E.\n",

	"ESTABLISHED \n"
	"1. NO 1 STARBOARD HAND 67-05-32N 032-31-39E\n"
	"2. NO 2 PORT HAND 67-05-30N 032-32-18E\n"
	"3. NO 4 PORT HAND 67-04-42N 032-32-02E\n"
	"4. NO 3 STARBOARD HAND 67-04-06N 032-31-38E=\n"
	"NNNN\n",

	"A. BELOMORSKIY NO 1 MIDCHANNEL 64-34-47N 035-13-58E\n"
	"B. NO 1 STARBOARD SIDE 64-33-27N 034-55-51E\n"
	"C. NO 2 PORT SIDE 64-33-24N 034-55-52E\n"
	"D. NO 3 STARBOARD SIDE 64-32-59N 034-54-10E\n"
	"E. NO 4 PORT SIDE 64-32-56N 034-54-14E\n"
	"F. NO 5 STARBOARD SIDE 64-32-37N 034-53-02E\n"
	"G. NO 6 PORT SIDE 64-32-35N 034-53-04E\n"
	"H. NO 8 PORT SIDE 64-32-25N 034-52-32E\n"
	"2. BUOYS ESTABLISHED\n"
	"A. NO 7 STARBOARD SIDE 64-31-38N 034-49-42E\n"
	"B. NO 10 PORT SIDE 64-31-36N 034-49-43E=\n"
	"NNNN\n",

	"ESTABLISHED\n"
	"1. 64-33.00N 035-08.60E\n"
	"2. 65-50.00N 036-12.00E TWO MOORING BUOYS\n"
	"3. 66-33.38N 034-48.40E TWO MOORING BUOYS\n"
	"4. 66-34.16N 034-48.92E TWO MOORING BUOYS\n"
	"5. 66-37.84N 034-21.00E FOUR MOORING BUOYS=\n"
	"NNNN\n",

	"SECTOR LIGHT W R G F AL 6M VISIBLE SECTORS 112.70-G \n"
	"F-113.17-W G AL-113.44-W F-113.76-W R AL-113.99-R \n"
	"F-114.45 BEARING W 113.60 DEGREE ESTABLICHED \n"
	"IN 67-07-03.13N 032-24-37.07E TO BE USED FOR NAVIGATION=\n"
	"NNNN\n",

	"ESTABLISHED IN\n"
	"1. STARBOARD HAND 67-05-39.4N 032-33-04.2E\n"
	"2. PORT HAND 67-05-28.3N 032-33-41.6E\n"
	"3. STARBOARD HAND 67-05-48.0N 032-32-09.3E\n"
	"4. PORT HAND 67-05-31.5N 032-33-20.4E\n"
	"5. STARBOARD HAND 67-05-52.7N 032-31-39.9E\n"
	"6. PORT HAND 67-05-35.7N 032-32-58.5E\n"
	"7. PORT HAND 67-05-54.8N 032-31-06.9E\n"
	"8. SOUTH CARDINAL 67-04 53.1N 032-37-24.3E=\n"
	"NNNN\n",

	"BOUNDED BY:\n"
	"A:53-22 N   003-35 E \n"
	"B:53-22 N   004-07 E\n"
	"C:52-50 N   004-28 E\n"
	"D:52-50 N   003-56 E\n"
	"VESSEL\n",

	"POSITIONS:\n"
	"A: 52-06.932N 004-13.521E\n"
	"B: 52-06.109N 004-14.557E\n"
	"C: 52-06.006N 004-14.849E\n"
	"AND\n",

	"AT 191200UTC, LOW 48 NORTH 45 WEST 1005 EXPECTED 50 NORTH 35 WEST 1000 BY 201200UTC.\n"
	"LOW 53 NORTH 27 WEST 1009 EXPECTED 59 NORTH 29 WEST 1006 BY SAME TIME. LOW 50 NORTH\n"
	"26 WEST 1010 EXPECTED 51 NORTH 21 WEST 1011 BY THAT TIME. AT 191200UTC, DEVELOPING\n"
	"LOW NEAR 68 NORTH 12 WEST EXPECTED 70 NORTH 11 EAST 1006 BY 201200UTC. HIGH 59 NORTH\n"
	"05 WEST 1030 EXPECTED 58 NORTH 01 WEST 1028 BY SAME TIME. LOWS 56 NORTH 51 WEST 996\n"
	"AND 65 NORTH 30 WEST 1008 BOTH LOSING THEIR IDENTITIES\n"
	"AREA FORECASTS _OR THE NEXT 24 HOURS\n",

	"IN POS 51-18.91N 0_3-06.21E.\n"
	"BLACK\n",

	"QIM LAT. 41 05.0N - LONG. 017 26.9E,\n"
	"9. LAT. 41 01.9N - LONG. 017 37.0E,\n"
	"20. LAT. 40 51.1N - LONG. 017 29.5E,\n"
	"21. LAT. 40 47.4N - LONG. 017 39.2E,\n"
	"22. LAT. 41 81.2N - LONG. 017 48.9E.\n",

	"LIGHT-BUOY,\n"
	"51-05.0N 001-46.8, MISSING.\n",

BUOY ELIA3
Y. FL 5S, 51-35.45N 002-49.93E,

THE LINE THROUGH THE FOLLOWING POSITIONS:
A: 52-06.932N 004-13.521E
B: 52-06.109N 004-14.557E
C: 52-06.006N 004-14.849E
AND TO THE SHORE.

AREA BOUNDED BY:
A:53-22 N   003-35 E 
B:53-22 N   004-07 E
C:52-50 N   004-28 E
D:52-50 N   003-56 E
VESSEL

VIKING GAS FIELD. 
1. PLATFORMS 49/17-CD,  53-25.4N 002-22.5E AND 49/7-HD 53-29.8N 002-19.4E
_LL NAVAIDS INOPERATIVE.
2. PLATFORMS 49/17-DD, 5_826.4N 002-23.6E AND 49/17-GD,  53-26.8N 002-15.3E
UNLIT.

PLATFORM 52-54.3N 002-35.9E,


		/** Now we could search for a coordinate in the message, and we will keep the station which is the closest
		 * to this coordinate. We wish to do that anyway in order to map things in KML.
		 *
		 * The stations file is not necessary.
		 * If four consecutive frequencies with less than two or three punctuations chars
		 * between them, it is a closed path. If not four, this is a simple path.
		 * We might check the angles to see if this is a path or a closed shape.
		 *
		 * For example, see http://navtex.lv 
		 * http://www.sjofartsverket.se/sv/Sjofart/Sjotrafikinformation/Navigationsvarningar/NAVTEX/
		 *
		 * The frequencies will be highlighted when detected.
		 * As long as the message is decoded, we built a list of frequencies plus their offsets and length
		 * in the message.
		 *
		 * Possible formats are -(This is experimental):
		52-08.5N 003-18.0E
		51-03.93N 001-09.17E
		50-40.2N 001-03.7W
 		67-04.0N 032-25.7E
 		47-29'30N 003-16'00W
		6930.1N 01729.9E
		48-21'45N 004-31'45W
		58-37N 003-32W
		314408N 341742E
		42-42N 005-10E
		54-02.3N 004-45.8E
		55-20.76N 014-45.27E
		55-31.1 N 012-44.7 E
		5330.4N 01051.5W
		43 45.0 N - 015 44.8 E
		34-33.7N 012-28.7E
		51 10.55 N - 001 51.02 E
		51.21.67N 002.13.29E
		73 NORTH 14 EAST
		58-01.20N 005-27.08W
		50.56N 007.00,5W
		5630,1N- 00501,6E
		LAT. 41.06N - LONG 012.57E
		42 40 01 N - 018 05 10 E
		40 25 31N - 18 15 30E
		40-32.2N 000-33.5E
		58-01.2 NORTH 005-27.1 WEST
		39-07,7N 026-39,2E
		55-27.99N .19-26.57E

		




*/
};

static const size_t nbInputs = sizeof(tstInputs) / sizeof(*tstInputs);

/** For testing purpose, this file can be compiled and run separately as a command-line program.
 * It is much simpler to test a standalone program, quick to build, with test data.
 * */
int main(int , const char ** )
{
	for( size_t i = 0; i < nbInputs; ++i )
	{
		/// This extracts shapes from Navtex messages.
		ShapesSetT myShapes( buf );
	}
	return 0 ;
}
#endif // NAVTEX_SHAPES_COMMAND_LINE



