// ----------------------------------------------------------------------------
// county lists
//
// Copyright (C) 2006-2010
//		Dave Freese, W1HKJ
// Copyright (C) 2008-2009
//		Stelios Bounanos, M0GLD
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

#include <string>

#include "counties.h"

//std::string strSQSO 
const char *szSQSO = "\
State/Province, ST/PR, County/City/District, CCD\n\
NIL,--,none\n\
Alaska,AK,AK1st (SE),SE\n\
,AK,AK2nd (NW),NW\n\
,AK,AK3rd (SC),SC\n\
,AK,AK4th (C),C\n\
Alberta,AB,MD of Acadia No. 34,ACAD\n\
,AB,Athabasca County,ATHA\n\
,AB,County of Barrhead No. 11,BARR\n\
,AB,Beaver County,BEAV\n\
,AB,Big Lakes County,BIGL\n\
,AB,MD of Bighorn No. 8,BIGH\n\
,AB,Birch Hills County,BIRC\n\
,AB,MD of Bonnyville No. 87,BONN\n\
,AB,Brazeau County,BRAZ\n\
,AB,Camrose County,CAMR\n\
,AB,Cardston County,CARD\n\
,AB,Clear Hills County,CLHI\n\
,AB,Clearwater County,CLWA\n\
,AB,Cypress County,CYPR\n\
,AB,MD of Fairview No. 136,FAIR\n\
,AB,Flagstaff County,FLAG\n\
,AB,Foothills County,FOOT\n\
,AB,County of Forty Mile No. 8,FORT\n\
,AB,County of Grande Prairie No. 1,GRAN\n\
,AB,MD of Greenview No. 16,GREE\n\
,AB,Kneehill County,KNEE\n\
,AB,Lac Ste. Anne County,LACS\n\
,AB,Lacombe County,LACO\n\
,AB,Lamont County,LAMO\n\
,AB,Leduc County,LEDU\n\
,AB,MD of Lesser Slave River No. 124,LESS\n\
,AB,Lethbridge County,LETH\n\
,AB,County of Minburn No. 27,MINB\n\
,AB,Mountain View County,MOUN\n\
,AB,County of Newell,NEWE\n\
,AB,County of Northern Lights,NOLI\n\
,AB,Northern Sunrise County,NOSU\n\
,AB,MD of Opportunity No. 17,OPPO\n\
,AB,County of Paintearth No. 18,PAIN\n\
,AB,Parkland County,PARK\n\
,AB,MD of Peace No. 135,PEAC\n\
,AB,MD of Pincher Creek No. 9,PINC\n\
,AB,Ponoka County,PONO\n\
,AB,MD of Provost No. 52,PROV\n\
,AB,MD of Ranchland No. 66,RANC\n\
,AB,Red Deer County,RDDR\n\
,AB,Rocky View County,ROCK\n\
,AB,Saddle Hills County,SADD\n\
,AB,Smoky Lake County,SMLA\n\
,AB,MD of Smoky River No. 130,SMRI\n\
,AB,MD of Spirit River No. 133,SPRI\n\
,AB,County of St. Paul No. 19,STPA\n\
,AB,Starland County,STAR\n\
,AB,County of Stettler No. 6,STET\n\
,AB,Sturgeon County,STUR\n\
,AB,MD of Taber,TABR\n\
,AB,Thorhild County,THOR\n\
,AB,County of Two Hills No. 21,TWHI\n\
,AB,County of Vermilion River,VERM\n\
,AB,Vulcan County,VULC\n\
,AB,MD of Wainwright No. 61,WAIN\n\
,AB,County of Warner No. 5,WARN\n\
,AB,Westlock County,WEST\n\
,AB,County of Wetaskiwin No. 10,WETA\n\
,AB,Wheatland County,WHEA\n\
,AB,MD of Willow Creek No. 26,WILL\n\
,AB,Woodlands County,WOOD\n\
,AB,Yellowhead County,YELL\n\
Alabama,AL,Autauga,AUTA\n\
,AL,Baldwin,BALD\n\
,AL,Barbour,BARB\n\
,AL,Bibb,BIBB\n\
,AL,Blount,BLOU\n\
,AL,Bullock,BULL\n\
,AL,Butler,BUTL\n\
,AL,Calhoun,CHOU\n\
,AL,Chambers,CHMB\n\
,AL,Cherokee,CKEE\n\
,AL,Chilton,CHIL\n\
,AL,Choctaw,CHOC\n\
,AL,Clarke,CLRK\n\
,AL,Clay,CLAY\n\
,AL,Cleburne,CLEB\n\
,AL,Coffee,COFF\n\
,AL,Colbert,COLB\n\
,AL,Conecuh,CONE\n\
,AL,Coosa,COOS\n\
,AL,Covington,COVI\n\
,AL,Crenshaw,CREN\n\
,AL,Cullman,CULM\n\
,AL,Dale,DALE\n\
,AL,Dallas,DLLS\n\
,AL,DeKalb,DKLB\n\
,AL,Elmore,ELMO\n\
,AL,Escambia,ESCA\n\
,AL,Etowah,ETOW\n\
,AL,Fayette,FAYE\n\
,AL,Franklin,FRNK\n\
,AL,Geneva,GENE\n\
,AL,Greene,GREE\n\
,AL,Hale,HALE\n\
,AL,Henry,HNRY\n\
,AL,Houston,HOUS\n\
,AL,Jackson,JKSN\n\
,AL,Jefferson,JEFF\n\
,AL,Lamar,LAMA\n\
,AL,Lauderdale,LAUD\n\
,AL,Lawrence,LAWR\n\
,AL,Lee,LEE\n\
,AL,Limestone,LIME\n\
,AL,Lowndes,LOWN\n\
,AL,Macon,MACO\n\
,AL,Madison,MDSN\n\
,AL,Marengo,MRGO\n\
,AL,Marion,MARI\n\
,AL,Marshall,MRSH\n\
,AL,Mobile,MOBI\n\
,AL,Monroe,MNRO\n\
,AL,Montgomery,MGMY\n\
,AL,Morgan,MORG\n\
,AL,Perry,PERR\n\
,AL,Pickens,PICK\n\
,AL,Pike,PIKE\n\
,AL,Randolph,RAND\n\
,AL,Russell,RSSL\n\
,AL,Shelby,SHEL\n\
,AL,St. Clair,SCLR\n\
,AL,Sumter,SUMT\n\
,AL,Talladega,TDEG\n\
,AL,Tallapoosa,TPOO\n\
,AL,Tuscaloosa,TUSC\n\
,AL,Walker,WLKR\n\
,AL,Washington,WASH\n\
,AL,Wilcox,WLCX\n\
,AL,Winston,WINS\n\
Arizona,AZ,Apache,APH\n\
,AZ,Cochise,CHS\n\
,AZ,Coconino,CNO\n\
,AZ,Gila,GLA\n\
,AZ,Graham,GHM\n\
,AZ,Greenlee,GLE\n\
,AZ,La Paz,LPZ\n\
,AZ,Maricopa,MCP\n\
,AZ,Mohave,MHV\n\
,AZ,Navajo,NVO\n\
,AZ,Pima,PMA\n\
,AZ,Pinal,PNL\n\
,AZ,Santa Cruz,SCZ\n\
,AZ,Yavapai,YVP\n\
,AZ,Yuma,YMA\n\
Arkansas,AR,Arkansas,ARKA\n\
,AR,Ashley,ASHL\n\
,AR,Baxter,BAXT\n\
,AR,Benton,BENT\n\
,AR,Boone,BOON\n\
,AR,Bradley,BRAD\n\
,AR,Calhoun,CALH\n\
,AR,Carroll,CARR\n\
,AR,Chicot,CHIC\n\
,AR,Clark,CLRK\n\
,AR,Clay,CLAY\n\
,AR,Cleburne,CLEB\n\
,AR,Cleveland,CLEV\n\
,AR,Columbia,COLU\n\
,AR,Conway,CONW\n\
,AR,Craighead,CRAG\n\
,AR,Crawford,CRAW\n\
,AR,Crittenden,CRIT\n\
,AR,Cross,CROS\n\
,AR,Dallas,DALL\n\
,AR,Desha,DESH\n\
,AR,Drew,DREW\n\
,AR,Faulkner,FAUL\n\
,AR,Franklin,FRNK\n\
,AR,Fulton,FULT\n\
,AR,Garland,GARL\n\
,AR,Grant,GRNT\n\
,AR,Greene,GREN\n\
,AR,Hempstead,HEMP\n\
,AR,Hot Spring,HSPR\n\
,AR,Howard,HOWA\n\
,AR,Independence,INDE\n\
,AR,Izard,IZRD\n\
,AR,Jackson,JACK\n\
,AR,Jefferson,JEFF\n\
,AR,Johnson,JOHN\n\
,AR,Lafayette,LAFA\n\
,AR,Lawrence,LAWR\n\
,AR,Lee,LEE\n\
,AR,Lincoln,LINC\n\
,AR,Little River,LRVR\n\
,AR,Logan,LOGN\n\
,AR,Lonoke,LONO\n\
,AR,Madison,MADI\n\
,AR,Marion,MARI\n\
,AR,Miller,MILL\n\
,AR,Mississippi,MISS\n\
,AR,Monroe,MONR\n\
,AR,Montgomery,MTGY\n\
,AR,Nevada,NEVA\n\
,AR,Newton,NEWT\n\
,AR,Ouachita,OUAC\n\
,AR,Perry,PERR\n\
,AR,Phillips,PHIL\n\
,AR,Pike,PIKE\n\
,AR,Poinsett,POIN\n\
,AR,Polk,POLK\n\
,AR,Pope,POPE\n\
,AR,Prairie,PRAR\n\
,AR,Pulaski,PULA\n\
,AR,Randolph,RAND\n\
,AR,Saline,SALI\n\
,AR,Scott,SCOT\n\
,AR,Searcy,SRCY\n\
,AR,Sebastian,SEBA\n\
,AR,Sevier,SEVR\n\
,AR,Sharp,SHRP\n\
,AR,St. Francis,STFR\n\
,AR,Stone,STON\n\
,AR,Union,UNIO\n\
,AR,Van Buren,VBRN\n\
,AR,Washington,WASH\n\
,AR,White,WHIE\n\
,AR,Woodruff,WOOD\n\
,AR,Yell,YELL\n\
British Columbia,BC,Abbotsford,ABF\n\
,BC,Burnaby North-Seymour,BNS\n\
,BC,Burnaby South,BUS\n\
,BC,Cariboo-Prince George,CPG\n\
,BC,Central Okanagan-Similkameen-Nicola,CSN\n\
,BC,Chilliwack-Hope,CHP\n\
,BC,Cloverdale-Langley City,CLC\n\
,BC,Coquitlam-Port Coquitlam,CPC\n\
,BC,Courtenay-Alberni,COA\n\
,BC,Cowichan-Malahat-Langford,CML\n\
,BC,Delta,DEL\n\
,BC,Esquimalt-Saanich-Sooke,ESQ\n\
,BC,Fleetwood-Port Kells,FPK\n\
,BC,Kamloops-Thompson-Cariboo,KTC\n\
,BC,Kelowna-Lake Country,KEL\n\
,BC,Kootenay-Columbia,KOC\n\
,BC,Langley-Aldergrove,LAA\n\
,BC,Mission-Matsqui-Fraser Canyon,MMF\n\
,BC,Nanaimo-Ladysmith,NAL\n\
,BC,New Westminster-Burnaby,NWB\n\
,BC,North Island-Powell River,NPR\n\
,BC,North Okanagan-Shuswap,NOS\n\
,BC,North Vancouver,NVA\n\
,BC,Pitt Meadows-Maple Ridge,PMM\n\
,BC,Port Moody-Coquitlam,PMC\n\
,BC,Prince George-Peace River-Northern Rockies,PPN\n\
,BC,Richmond Center,RIC\n\
,BC,Saanich-Gulf Islands,SGI\n\
,BC,Skeena-Bulkley Valley,SBV\n\
,BC,South Okanagan-West Kootenay,SWK\n\
,BC,South Surrey-White Rock,SWR\n\
,BC,Steveston-Richmond East,STR\n\
,BC,Surrey Centre,SUC\n\
,BC,Surrey-Newton,SUN\n\
,BC,Vancouver Centre,VAC\n\
,BC,Vancouver East,VAE\n\
,BC,Vancouver Kingsway,VAK\n\
,BC,Vancouver Quadra,VAQ\n\
,BC,Vancouver South,VAS\n\
,BC,Vancouver-Granville,VAG\n\
,BC,Victoria,VIC\n\
,BC,West Vanc,WVS\n\
California,CA,Alameda,ALAM\n\
,CA,Alpine,ALPI\n\
,CA,Amador,AMAD\n\
,CA,Butte,BUTT\n\
,CA,Calaveras,CALA\n\
,CA,Colusa,COLU\n\
,CA,Contra Costa,CCOS\n\
,CA,Del Norte,DELN\n\
,CA,El Dorado,ELDO\n\
,CA,Fresno,FRES\n\
,CA,Glenn,GLEN\n\
,CA,Humboldt,HUMB\n\
,CA,Imperial,IMPE\n\
,CA,Inyo,INYO\n\
,CA,Kern,KERN\n\
,CA,Kings,KING\n\
,CA,Lake,LAKE\n\
,CA,Lassen,LASS\n\
,CA,Los Angeles,LANG\n\
,CA,Madera,MADE\n\
,CA,Marin,MARN\n\
,CA,Mariposa,MARP\n\
,CA,Mendocino,MEND\n\
,CA,Merced,MERC\n\
,CA,Modoc,MODO\n\
,CA,Mono,MONO\n\
,CA,Monterey,MONT\n\
,CA,Napa,NAPA\n\
,CA,Nevada,NEVA\n\
,CA,Orange,ORAN\n\
,CA,Placer,PLAC\n\
,CA,Plumas,PLUM\n\
,CA,Riverside,RIVE\n\
,CA,Sacramento,SACR\n\
,CA,San Benito,SBEN\n\
,CA,San Bernardino,SBER\n\
,CA,San Diego,SDIE\n\
,CA,San Francisco,SFRA\n\
,CA,San Joaquin,SJOA\n\
,CA,San Luis Obispo,SLUI\n\
,CA,San Mateo,SMAT\n\
,CA,Santa Barbara,SBAR\n\
,CA,Santa Clara,SCLA\n\
,CA,Santa Cruz,SCRU\n\
,CA,Shasta,SHAS\n\
,CA,Sierra,SIER\n\
,CA,Siskiyou,SISK\n\
,CA,Solano,SOLA\n\
,CA,Sonoma,SONO\n\
,CA,Stanislaus,STAN\n\
,CA,Sutter,SUTT\n\
,CA,Tehama,TEHA\n\
,CA,Trinity,TRIN\n\
,CA,Tulare,TULA\n\
,CA,Tuolumne,TUOL\n\
,CA,Ventura,VENT\n\
,CA,Yolo,YOLO\n\
,CA,Yuba,YUBA\n\
Colorado,CO,Adams,ADA\n\
,CO,Alamosa,ALA\n\
,CO,Arapahoe,ARA\n\
,CO,Archuleta,ARC\n\
,CO,Baca,BAC\n\
,CO,Bent,BEN\n\
,CO,Boulder,BOU\n\
,CO,Broomfield,BRO\n\
,CO,Chaffee,CHA\n\
,CO,Cheyenne,CHE\n\
,CO,Clear Creek,CLC\n\
,CO,Conejos,CON\n\
,CO,Costilla,COS\n\
,CO,Crowley,CRO\n\
,CO,Custer,CUS\n\
,CO,Delta,DEL\n\
,CO,Denver,DEN\n\
,CO,Dolores,DOL\n\
,CO,Douglas,DOU\n\
,CO,Eagle,EAG\n\
,CO,El Paso,ELP\n\
,CO,Elbert,ELB\n\
,CO,Fremont,FRE\n\
,CO,Garfield,GAR\n\
,CO,Gilpin,GIL\n\
,CO,Grand,GRA\n\
,CO,Gunnison,GUN\n\
,CO,Hinsdale,HIN\n\
,CO,Huerfano,HUE\n\
,CO,Jackson,JAC\n\
,CO,Jefferson,JEF\n\
,CO,Kiowa,KIO\n\
,CO,Kit Carson,KIC\n\
,CO,La Plata,LAP\n\
,CO,Lake,LAK\n\
,CO,Larimer,LAR\n\
,CO,Las Animas,LAA\n\
,CO,Lincoln,LIN\n\
,CO,Logan,LOG\n\
,CO,Mesa,MES\n\
,CO,Mineral,MIN\n\
,CO,Moffat,MOF\n\
,CO,Montezuma,MON\n\
,CO,Montrose,MOT\n\
,CO,Morgan,MOR\n\
,CO,Otero,OTE\n\
,CO,Ouray,OUR\n\
,CO,Park,PAR\n\
,CO,Phillips,PHI\n\
,CO,Pitkin,PIT\n\
,CO,Prowers,PRO\n\
,CO,Pueblo,PUE\n\
,CO,Rio Blanco,RIB\n\
,CO,Rio Grande,RIG\n\
,CO,Routt,ROU\n\
,CO,Saguache,SAG\n\
,CO,San Juan,SAJ\n\
,CO,San Miguel,SAM\n\
,CO,Sedgwick,SED\n\
,CO,Summit,SUM\n\
,CO,Teller,TEL\n\
,CO,Washington,WAS\n\
,CO,Weld,WEL\n\
,CO,Yuma,YUM\n\
Connecticut,CT,Fairfield,FAI\n\
,CT,Hartford,HAR\n\
,CT,Litchfield,LIT\n\
,CT,Middlesex,MIC\n\
,CT,New Haven,NHV\n\
,CT,New London,NLN\n\
,CT,Tolland,TOL\n\
,CT,Windham,WIN\n\
Delaware,DE,Kent,KDE\n\
,DE,New Castle,NDE\n\
,DE,Sussex,SDE\n\
Dist. of Col.,DC,District of Columbia,\n\
Florida,FL,ALACHUA,ALC\n\
,FL,BAKER,BAK\n\
,FL,BAY,BAY\n\
,FL,BRADFORD,BRA\n\
,FL,BREVARD,BRE\n\
,FL,BROWARD,BRO\n\
,FL,CALHOUN,CAH\n\
,FL,CHARLOTTE,CHA\n\
,FL,CITRUS,CIT\n\
,FL,CLAY,CLA\n\
,FL,COLLIER,CLR\n\
,FL,COLUMBIA,CLM\n\
,FL,MIAMI-DADE,DAD\n\
,FL,DESOTO,DES\n\
,FL,DIXIE,DIX\n\
,FL,DUVAL,DUV\n\
,FL,ESCAMBIA,ESC\n\
,FL,FLAGLER,FLG\n\
,FL,FRANKLIN,FRA\n\
,FL,GADSDEN,GAD\n\
,FL,GILCHRIST,GIL\n\
,FL,GLADES,GLA\n\
,FL,GULF,GUL\n\
,FL,HAMILTON,HAM\n\
,FL,HARDEE,HAR\n\
,FL,HENDRY,HEN\n\
,FL,HERNANDO,HER\n\
,FL,HIGHLANDS,HIG\n\
,FL,HILLSBOROUGH,HIL\n\
,FL,HOLMES,HOL\n\
,FL,INDIAN RIVER,IDR\n\
,FL,JACKSON,JAC\n\
,FL,JEFFERSON,JEF\n\
,FL,LAFAYETTE,LAF\n\
,FL,LAKE,LAK\n\
,FL,LEE,LEE\n\
,FL,LEON,LEO\n\
,FL,LEVY,LEV\n\
,FL,LIBERTY,LIB\n\
,FL,MADISON,MAD\n\
,FL,MANATEE,MTE\n\
,FL,MARION,MAO\n\
,FL,MARTIN,MRT\n\
,FL,MONROE,MON\n\
,FL,NASSAU,NAS\n\
,FL,OKALOOSA,OKA\n\
,FL,OKEECHOBEE,OKE\n\
,FL,ORANGE,ORA\n\
,FL,OSCEOLA,OSC\n\
,FL,PALM BEACH,PAL\n\
,FL,PASCO,PAS\n\
,FL,PINELLAS,PIN\n\
,FL,POLK,POL\n\
,FL,PUTNAM,PUT\n\
,FL,SANTA ROSA,SAN\n\
,FL,SARASOTA,SAR\n\
,FL,SEMINOLE,SEM\n\
,FL,ST. JOHNS,STJ\n\
,FL,ST. LUCIE,STL\n\
,FL,SUMTER,SUM\n\
,FL,SUWANNEE,SUW\n\
,FL,TAYLOR,TAY\n\
,FL,UNION,UNI\n\
,FL,VOLUSIA,VOL\n\
,FL,WAKULLA,WAK\n\
,FL,WALTON,WAL\n\
,FL,WASHINGTON,WAG\n\
Georgia,GA,Appling,APPL\n\
,GA,Atkinson,ATKN\n\
,GA,Bacon,BACN\n\
,GA,Baker,BAKR\n\
,GA,Baldwin,BALD\n\
,GA,Banks,BANK\n\
,GA,Barrow,BARR\n\
,GA,Bartow,BART\n\
,GA,Ben Hill,BENH\n\
,GA,Berrien,BERR\n\
,GA,Bibb,BIBB\n\
,GA,Bleckley,BLEC\n\
,GA,Brantley,BRAN\n\
,GA,Brooks,BROK\n\
,GA,Bryan,BRYN\n\
,GA,Bulloch,BULL\n\
,GA,Burke,BURK\n\
,GA,Butts,BUTT\n\
,GA,Calhoun,CALH\n\
,GA,Camden,CMDN\n\
,GA,Candler,CAND\n\
,GA,Carroll,CARR\n\
,GA,Catoosa,CATO\n\
,GA,Charlton,CHAR\n\
,GA,Chatham,CHTM\n\
,GA,Chattahoochee,CHAT\n\
,GA,Chattooga,CHGA\n\
,GA,Cherokee,CHER\n\
,GA,Clarke,CLKE\n\
,GA,Clay,CLAY\n\
,GA,Clayton,CLTN\n\
,GA,Clinch,CLCH\n\
,GA,Cobb,COBB\n\
,GA,Coffee,COFF\n\
,GA,Colquitt,COLQ\n\
,GA,Columbia,COLU\n\
,GA,Cook,COOK\n\
,GA,Coweta,COWE\n\
,GA,Crawford,CRAW\n\
,GA,Crisp,CRIS\n\
,GA,Dade,DADE\n\
,GA,Dawson,DAWS\n\
,GA,Decatur,DECA\n\
,GA,DeKalb,DKLB\n\
,GA,Dodge,DODG\n\
,GA,Dooly,DOOL\n\
,GA,Dougherty,DHTY\n\
,GA,Douglas,DOUG\n\
,GA,Early,EARL\n\
,GA,Echols,ECHO\n\
,GA,Effingham,EFFI\n\
,GA,Elbert,ELBE\n\
,GA,Emanuel,EMAN\n\
,GA,Evans,EVAN\n\
,GA,Fannin,FANN\n\
,GA,Fayette,FAYE\n\
,GA,Floyd,FLOY\n\
,GA,Forsyth,FORS\n\
,GA,Franklin,FRAN\n\
,GA,Fulton,FULT\n\
,GA,Gilmer,GILM\n\
,GA,Glascock,GLAS\n\
,GA,Glynn,GLYN\n\
,GA,Gordon,GORD\n\
,GA,Grady,GRAD\n\
,GA,Greene,GREE\n\
,GA,Gwinnett,GWIN\n\
,GA,Habersham,HABE\n\
,GA,Hall,HALL\n\
,GA,Hancock,HANC\n\
,GA,Haralson,HARA\n\
,GA,Harris,HARR\n\
,GA,Hart,HART\n\
,GA,Heard,HEAR\n\
,GA,Henry,HNRY\n\
,GA,Houston,HOUS\n\
,GA,Irwin,IRWI\n\
,GA,Jackson,JACK\n\
,GA,Jasper,JASP\n\
,GA,Jeff Davis,JFDA\n\
,GA,Jefferson,JEFF\n\
,GA,Jenkins,JENK\n\
,GA,Johnson,JOHN\n\
,GA,Jones,JONE\n\
,GA,Lamar,LAMA\n\
,GA,Lanier,LANI\n\
,GA,Laurens,LAUR\n\
,GA,Lee,LEE\n\
,GA,Liberty,LIBE\n\
,GA,Lincoln,LINC\n\
,GA,Long,LONG\n\
,GA,Lowndes,LOWN\n\
,GA,Lumpkin,LUMP\n\
,GA,McDuffie,MCDU\n\
,GA,McIntosh,MCIN\n\
,GA,Macon,MACO\n\
,GA,Madison,MADI\n\
,GA,Marion,MARI\n\
,GA,Meriwether,MERI\n\
,GA,Miller,MILL\n\
,GA,Mitchell,MITC\n\
,GA,Monroe,MNRO\n\
,GA,Montgomery,MONT\n\
,GA,Morgan,MORG\n\
,GA,Murray,MURR\n\
,GA,Muscogee,MUSC\n\
,GA,Newton,NEWT\n\
,GA,Oconee,OCON\n\
,GA,Oglethorpe,OGLE\n\
,GA,Paulding,PAUL\n\
,GA,Peach,PEAC\n\
,GA,Pickens,PICK\n\
,GA,Pierce,PIER\n\
,GA,Pike,PIKE\n\
,GA,Polk,POLK\n\
,GA,Pulaski,PULA\n\
,GA,Putnam,PUTN\n\
,GA,Quitman,QCIT\n\
,GA,Rabun,RABU\n\
,GA,Randolph,RAND\n\
,GA,Richmond,RICH\n\
,GA,Rockdale,ROCK\n\
,GA,Schley,SCHL\n\
,GA,Screven,SCRE\n\
,GA,Seminole,SEMI\n\
,GA,Spalding,SPAL\n\
,GA,Stephens,STEP\n\
,GA,Stewart,STWT\n\
,GA,Sumter,SUMT\n\
,GA,Talbot,TLBT\n\
,GA,Taliaferro,TALI\n\
,GA,Tattnall,TATT\n\
,GA,Taylor,TAYL\n\
,GA,Telfair,TELF\n\
,GA,Terrell,TERR\n\
,GA,Thomas,THOM\n\
,GA,Tift,TIFT\n\
,GA,Toombs,TOOM\n\
,GA,Towns,TOWN\n\
,GA,Treutlen,TREU\n\
,GA,Troup,TROU\n\
,GA,Turner,TURN\n\
,GA,Twiggs,TWIG\n\
,GA,Union,UNIO\n\
,GA,Upson,UPSO\n\
,GA,Walker,WLKR\n\
,GA,Walton,WALT\n\
,GA,Ware,WARE\n\
,GA,Warren,WARR\n\
,GA,Washington,WASH\n\
,GA,Wayne,WAYN\n\
,GA,Webster,WEBS\n\
,GA,Wheeler,WHEE\n\
,GA,White,WHIT\n\
,GA,Whitfield,WFLD\n\
,GA,Wilcox,WCOX\n\
,GA,Wilkes,WILK\n\
,GA,Wilkinson,WKSN\n\
,GA,Worth,WORT\n\
Guam,GU,Guam,\n\
Hawaii,HI,Hawaii-HIL,HIL\n\
,HI,Hawaii-KOH,KOH\n\
,HI,Hawaii-KON,KON\n\
,HI,Hawaii-VOL,VOL\n\
,HI,Honolulu-HON,HON\n\
,HI,Honolulu-LHN,LHN\n\
,HI,Honolulu-PRL,PRL\n\
,HI,Honolulu-WHN,WHN\n\
,HI,Kalawao-KAL,KAL\n\
,HI,Kauai-KAU,KAU\n\
,HI,Kauai-NII,NII\n\
,HI,Maui-LAN,LAN\n\
,HI,Maui-MAU,MAU\n\
,HI,Maui-MOL,MOL\n\
Idaho,ID,Ada,ADA\n\
,ID,Adams,ADM\n\
,ID,Bannock,BAN\n\
,ID,Bear Lake,BEA\n\
,ID,Benewah,BEN\n\
,ID,Bingham,BIN\n\
,ID,Blaine,BLA\n\
,ID,Boise,BOI\n\
,ID,Bonner,BNR\n\
,ID,Bonneville,BNV\n\
,ID,Boundary,BOU\n\
,ID,Butte,BUT\n\
,ID,Camas,CAM\n\
,ID,Canyon,CAN\n\
,ID,Caribou,CAR\n\
,ID,Cassia,CAS\n\
,ID,Clark,CLA\n\
,ID,Clearwater,CLE\n\
,ID,Custer,CUS\n\
,ID,Elmore,ELM\n\
,ID,Franklin,FRA\n\
,ID,Fremont,FRE\n\
,ID,Gem,GEM\n\
,ID,Gooding,GOO\n\
,ID,Idaho,IDA\n\
,ID,Jefferson,JEF\n\
,ID,Jerome,JER\n\
,ID,Kootenai,KOO\n\
,ID,Latah,LAT\n\
,ID,Lemhi,LEM\n\
,ID,Lewis,LEW\n\
,ID,Lincoln,LIN\n\
,ID,Madison,MAD\n\
,ID,Minidoka,MIN\n\
,ID,Nez Perce,NEZ\n\
,ID,Oneida,ONE\n\
,ID,Owyhee,OWY\n\
,ID,Payette,PAY\n\
,ID,Power,POW\n\
,ID,Shoshone,SHO\n\
,ID,Teton,TET\n\
,ID,Twin Falls,TWI\n\
,ID,Valley,VAL\n\
,ID,Washington,WAS\n\
Illinois,IL,Adams,ADAM\n\
,IL,Alexander,ALEX\n\
,IL,Bond,BOND\n\
,IL,Boone,BOON\n\
,IL,Brown,BROW\n\
,IL,Bureau,BURO\n\
,IL,Calhoun,CALH\n\
,IL,Carroll,CARR\n\
,IL,Cass,CASS\n\
,IL,Champaign,CHAM\n\
,IL,Christian,CHRS\n\
,IL,Clark,CLRK\n\
,IL,Clay,CLAY\n\
,IL,Clinton,CLNT\n\
,IL,Coles,COLE\n\
,IL,Cook,COOK\n\
,IL,Crawford,CRAW\n\
,IL,Cumberland,CUMB\n\
,IL,DeWitt,DEWT\n\
,IL,DeKalb,DEKA\n\
,IL,Douglas,DOUG\n\
,IL,DuPage,DUPG\n\
,IL,Edgar,EDGR\n\
,IL,Edwards,EDWA\n\
,IL,Effingham,EFFG\n\
,IL,Fayette,FAYE\n\
,IL,Ford,FORD\n\
,IL,Franklin,FRNK\n\
,IL,Fulton,FULT\n\
,IL,Gallatin,GALL\n\
,IL,Greene,GREE\n\
,IL,Grundy,GRUN\n\
,IL,Hamilton,HAML\n\
,IL,Hancock,HANC\n\
,IL,Hardin,HARD\n\
,IL,Henderson,HNDR\n\
,IL,Henry,HENR\n\
,IL,Iroquois,IROQ\n\
,IL,Jackson,JACK\n\
,IL,Jasper,JASP\n\
,IL,Jefferson,JEFF\n\
,IL,Jersey,JERS\n\
,IL,JoDaviess,JODA\n\
,IL,Johnson,JOHN\n\
,IL,Kane,KANE\n\
,IL,Kankakee,KANK\n\
,IL,Kendall,KEND\n\
,IL,Knox,KNOX\n\
,IL,LaSalle,LASA\n\
,IL,Lake,LAKE\n\
,IL,Lawrence,LAWR\n\
,IL,Lee,LEE\n\
,IL,Livingston,LIVG\n\
,IL,Logan,LOGN\n\
,IL,Macon,MACN\n\
,IL,Macoupin,MCPN\n\
,IL,Madison,MADN\n\
,IL,Marion,MARI\n\
,IL,Marshall,MSHL\n\
,IL,Mason,MASN\n\
,IL,Massac,MSSC\n\
,IL,McDonough,MCDN\n\
,IL,McHenry,MCHE\n\
,IL,McLean,MCLN\n\
,IL,Menard,MNRD\n\
,IL,Mercer,MRCR\n\
,IL,Monroe,MNRO\n\
,IL,Montgomery,MNTG\n\
,IL,Morgan,MORG\n\
,IL,Moultrie,MOUL\n\
,IL,Ogle,OGLE\n\
,IL,Peoria,PEOR\n\
,IL,Perry,PERR\n\
,IL,Piatt,PIAT\n\
,IL,Pike,PIKE\n\
,IL,Pope,POPE\n\
,IL,Pulaski,PULA\n\
,IL,Putnam,PUTN\n\
,IL,Randolph,RAND\n\
,IL,Richland,RICH\n\
,IL,Rock Island,ROCK\n\
,IL,Saline,SALI\n\
,IL,Sangamon,SANG\n\
,IL,Schuyler,SCHY\n\
,IL,Scott,SCOT\n\
,IL,Shelby,SHEL\n\
,IL,St. Clair,SCLA\n\
,IL,Stark,STAR\n\
,IL,Stephenson,STEP\n\
,IL,Tazewell,TAZW\n\
,IL,Union,UNIO\n\
,IL,Vermilion,VERM\n\
,IL,Wabash,WABA\n\
,IL,Warren,WARR\n\
,IL,Washington,WASH\n\
,IL,Wayne,WAYN\n\
,IL,White,WHIT\n\
,IL,Whiteside,WTSD\n\
,IL,Will,WILL\n\
,IL,Williamson,WMSN\n\
,IL,Winnebago,WBGO\n\
,IL,Woodford,WOOD\n\
Indiana,IN,Adams,INADA\n\
,IN,Allen,INALL\n\
,IN,Bartholomew,INBAR\n\
,IN,Benton,INBEN\n\
,IN,Blackford,INBLA\n\
,IN,Boone,INBOO\n\
,IN,Brown,INBRO\n\
,IN,Carroll,INCAR\n\
,IN,Cass,INCAS\n\
,IN,Clark,INCLR\n\
,IN,Clay,INCLY\n\
,IN,Clinton,INCLI\n\
,IN,Crawford,INCRA\n\
,IN,Daviess,INDAV\n\
,IN,De Kalb,INDEK\n\
,IN,Dearborn,INDEA\n\
,IN,Decatur,INDEC\n\
,IN,Delaware,INDEL\n\
,IN,Dubois,INDUB\n\
,IN,Elkhart,INELK\n\
,IN,Fayette,INFAY\n\
,IN,Floyd,INFLO\n\
,IN,Fountain,INFOU\n\
,IN,Franklin,INFRA\n\
,IN,Fulton,INFUL\n\
,IN,Gibson,INGIB\n\
,IN,Grant,INGRA\n\
,IN,Greene,INGRE\n\
,IN,Hamilton,INHAM\n\
,IN,Hancock,INHAN\n\
,IN,Harrison,INHAR\n\
,IN,Hendricks,INHND\n\
,IN,Henry,INHNR\n\
,IN,Howard,INHOW\n\
,IN,Huntington,INHUN\n\
,IN,Jackson,INJAC\n\
,IN,Jasper,INJAS\n\
,IN,Jay,INJAY\n\
,IN,Jefferson,INJEF\n\
,IN,Jennings,INJEN\n\
,IN,Johnson,INJOH\n\
,IN,Knox,INKNO\n\
,IN,Kosciusko,INKOS\n\
,IN,La Porte,INLAP\n\
,IN,Lagrange,INLAG\n\
,IN,Lake,INLAK\n\
,IN,Lawrence,INLAW\n\
,IN,Madison,INMAD\n\
,IN,Marion,INMRN\n\
,IN,Marshall,INMRS\n\
,IN,Martin,INMRT\n\
,IN,Miami,INMIA\n\
,IN,Monroe,INMNR\n\
,IN,Montgomery,INMNT\n\
,IN,Morgan,INMOR\n\
,IN,Newton,INNEW\n\
,IN,Noble,INNOB\n\
,IN,Ohio,INOHI\n\
,IN,Orange,INORA\n\
,IN,Owen,INOWE\n\
,IN,Parke,INPAR\n\
,IN,Perry,INPER\n\
,IN,Pike,INPIK\n\
,IN,Porter,INPOR\n\
,IN,Posey,INPOS\n\
,IN,Pulaski,INPUL\n\
,IN,Putnam,INPUT\n\
,IN,Randolph,INRAN\n\
,IN,Ripley,INRIP\n\
,IN,Rush,INRUS\n\
,IN,Scott,INSCO\n\
,IN,Shelby,INSHE\n\
,IN,Spencer,INSPE\n\
,IN,St. Joseph,INSTJ\n\
,IN,Starke,INSTA\n\
,IN,Steuben,INSTE\n\
,IN,Sullivan,INSUL\n\
,IN,Switzerland,INSWI\n\
,IN,Tippecanoe,INTPP\n\
,IN,Tipton,INTPT\n\
,IN,Union,INUNI\n\
,IN,Vanderburgh,INVAN\n\
,IN,Vermillion,INVER\n\
,IN,Vigo,INVIG\n\
,IN,Wabash,INWAB\n\
,IN,Warren,INWRN\n\
,IN,Warrick,INWRK\n\
,IN,Washington,INWAS\n\
,IN,Wayne,INWAY\n\
,IN,Wells,INWEL\n\
,IN,White,INWHT\n\
,IN,Whitley,INWHL\n\
Iowa,IA,Adair,ADR\n\
,IA,Adams,ADM\n\
,IA,Allamakee,ALL\n\
,IA,Appanoose,APP\n\
,IA,Audubon,AUD\n\
,IA,Benton,BEN\n\
,IA,Black Hawk,BKH\n\
,IA,Boone,BOO\n\
,IA,Bremer,BRE\n\
,IA,Buchanan,BUC\n\
,IA,Buena Vista,BNV\n\
,IA,Butler,BTL\n\
,IA,Calhoun,CAL\n\
,IA,Carroll,CAR\n\
,IA,Cass,CAS\n\
,IA,Cedar,CED\n\
,IA,Cerro Gordo,CEG\n\
,IA,Cherokee,CHE\n\
,IA,Chickasaw,CHI\n\
,IA,Clarke,CLR\n\
,IA,Clay,CLA\n\
,IA,Clayton,CLT\n\
,IA,Clinton,CLN\n\
,IA,Crawford,CRF\n\
,IA,Dallas,DAL\n\
,IA,Davis,DAV\n\
,IA,Decatur,DEC\n\
,IA,Delaware,DEL\n\
,IA,Des Moines,DSM\n\
,IA,Dickinson,DIC\n\
,IA,Dubuque,DUB\n\
,IA,Emmet,EMM\n\
,IA,Fayette,FAY\n\
,IA,Floyd,FLO\n\
,IA,Franklin,FRA\n\
,IA,Fremont,FRE\n\
,IA,Greene,GRE\n\
,IA,Grundy,GRU\n\
,IA,Guthrie,GUT\n\
,IA,Hamilton,HAM\n\
,IA,Hancock,HAN\n\
,IA,Hardin,HDN\n\
,IA,Harrison,HRS\n\
,IA,Henry,HEN\n\
,IA,Howard,HOW\n\
,IA,Humboldt,HUM\n\
,IA,Ida,IDA\n\
,IA,Iowa,IOW\n\
,IA,Jackson,JAC\n\
,IA,Jasper,JAS\n\
,IA,Jefferson,JEF\n\
,IA,Johnson,JOH\n\
,IA,Jones,JON\n\
,IA,Keokuk,KEO\n\
,IA,Kossuth,KOS\n\
,IA,Lee,LEE\n\
,IA,Linn,LIN\n\
,IA,Louisa,LOU\n\
,IA,Lucas,LUC\n\
,IA,Lyon,LYN\n\
,IA,Madison,MAD\n\
,IA,Mahaska,MAH\n\
,IA,Marion,MRN\n\
,IA,Marshall,MSL\n\
,IA,Mills,MIL\n\
,IA,Mitchell,MIT\n\
,IA,Monona,MNA\n\
,IA,Monroe,MOE\n\
,IA,Montgomery,MTG\n\
,IA,Muscatine,MUS\n\
,IA,O'Brien,OBR\n\
,IA,Osceola,OSC\n\
,IA,Page,PAG\n\
,IA,Palo Alto,PLA\n\
,IA,Plymouth,PLY\n\
,IA,Pocahontas,POC\n\
,IA,Polk,POL\n\
,IA,Pottawattamie,POT\n\
,IA,Poweshiek,POW\n\
,IA,Ringgold,RIN\n\
,IA,Sac,SAC\n\
,IA,Scott,SCO\n\
,IA,Shelby,SHE\n\
,IA,Sioux,SIO\n\
,IA,Story,STR\n\
,IA,Tama,TAM\n\
,IA,Taylor,TAY\n\
,IA,Union,UNI\n\
,IA,Van Buren,VAN\n\
,IA,Wapello,WAP\n\
,IA,Warren,WAR\n\
,IA,Washington,WAS\n\
,IA,Wayne,WAY\n\
,IA,Webster,WEB\n\
,IA,Winnebago,WNB\n\
,IA,Winneshiek,WNS\n\
,IA,Woodbury,WOO\n\
,IA,Worth,WOR\n\
,IA,Wright,WRI\n\
Kansas,KS,Allen,ALL\n\
,KS,Anderson,AND\n\
,KS,Atchison,ATC\n\
,KS,Barber,BAR\n\
,KS,Barton,BRT\n\
,KS,Bourbon,BOU\n\
,KS,Brown,BRO\n\
,KS,Butler,BUT\n\
,KS,Chase,CHS\n\
,KS,Chautauqua,CHT\n\
,KS,Cherokee,CHE\n\
,KS,Cheyenne,CHY\n\
,KS,Clark,CLK\n\
,KS,Clay,CLY\n\
,KS,Cloud,CLO\n\
,KS,Coffey,COF\n\
,KS,Comanche,COM\n\
,KS,Cowley,COW\n\
,KS,Crawford,CRA\n\
,KS,Decatur,DEC\n\
,KS,Dickinson,DIC\n\
,KS,Doniphan,DON\n\
,KS,Douglas,DOU\n\
,KS,Edwards,EDW\n\
,KS,Elk,ELK\n\
,KS,Ellis,ELL\n\
,KS,Ellsworth,ELS\n\
,KS,Finney,FIN\n\
,KS,Ford,FOR\n\
,KS,Franklin,FRA\n\
,KS,Geary,GEA\n\
,KS,Gove,GOV\n\
,KS,Graham,GRM\n\
,KS,Grant,GRT\n\
,KS,Gray,GRY\n\
,KS,Greeley,GLY\n\
,KS,Greenwood,GRE\n\
,KS,Hamilton,HAM\n\
,KS,Harper,HPR\n\
,KS,Harvey,HVY\n\
,KS,Haskell,HAS\n\
,KS,Hodgeman,HOG\n\
,KS,Jackson,JAC\n\
,KS,Jefferson,JEF\n\
,KS,Jewell,JEW\n\
,KS,Johnson,JOH\n\
,KS,Kearny,KEA\n\
,KS,Kingman,KIN\n\
,KS,Kiowa,KIO\n\
,KS,Labette,LAB\n\
,KS,Lane,LAN\n\
,KS,Leavenworth,LEA\n\
,KS,Lincoln,LCN\n\
,KS,Linn,LIN\n\
,KS,Logan,LOG\n\
,KS,Lyon,LYO\n\
,KS,Marion,MRN\n\
,KS,Marshall,MSH\n\
,KS,McPherson,MCP\n\
,KS,Meade,MEA\n\
,KS,Miami,MIA\n\
,KS,Mitchell,MIT\n\
,KS,Montgomery,MGY\n\
,KS,Morris,MOR\n\
,KS,Morton,MTN\n\
,KS,Nemaha,NEM\n\
,KS,Neosho,NEO\n\
,KS,Ness,NES\n\
,KS,Norton,NOR\n\
,KS,Osage,OSA\n\
,KS,Osborne,OSB\n\
,KS,Ottawa,OTT\n\
,KS,Pawnee,PAW\n\
,KS,Phillips,PHI\n\
,KS,Pottawatomie,POT\n\
,KS,Pratt,PRA\n\
,KS,Rawlins,RAW\n\
,KS,Reno,REN\n\
,KS,Republic,REP\n\
,KS,Rice,RIC\n\
,KS,Riley,RIL\n\
,KS,Rooks,ROO\n\
,KS,Rush,RUS\n\
,KS,Russell,RSL\n\
,KS,Saline,SAL\n\
,KS,Scott,SCO\n\
,KS,Sedgwick,SED\n\
,KS,Seward,SEW\n\
,KS,Shawnee,SHA\n\
,KS,Sheridan,SHE\n\
,KS,Sherman,SMN\n\
,KS,Smith,SMI\n\
,KS,Stafford,STA\n\
,KS,Stanton,STN\n\
,KS,Stevens,STE\n\
,KS,Sumner,SUM\n\
,KS,Thomas,THO\n\
,KS,Trego,TRE\n\
,KS,Wabaunsee,WAB\n\
,KS,Wallace,WAL\n\
,KS,Washington,WAS\n\
,KS,Wichita,WIC\n\
,KS,Wilson,WIL\n\
,KS,Woodson,WOO\n\
,KS,Wyandotte,WYA\n\
Kentucky,KY,Adair,ADA\n\
,KY,Allen,ALL\n\
,KY,Anderson,AND\n\
,KY,Ballard,BAL\n\
,KY,Barren,BAR\n\
,KY,Bath,BAT\n\
,KY,Bell,BEL\n\
,KY,Boone,BOO\n\
,KY,Bourbon,BOU\n\
,KY,Boyd,BOY\n\
,KY,Boyle,BOL\n\
,KY,Bracken,BRA\n\
,KY,Breathitt,BRE\n\
,KY,Breckinridge,BRK\n\
,KY,Bullitt,BUL\n\
,KY,Butler,BUT\n\
,KY,Caldwell,CAL\n\
,KY,Calloway,CAW\n\
,KY,Campbell,CAM\n\
,KY,Carlisle,CAE\n\
,KY,Carroll,CRL\n\
,KY,Carter,CTR\n\
,KY,Casey,CAS\n\
,KY,Christian,CHR\n\
,KY,Clark,CLA\n\
,KY,Clay,CLY\n\
,KY,Clinton,CLI\n\
,KY,Crittenden,CRI\n\
,KY,Cumberland,CUM\n\
,KY,Daviess,DAV\n\
,KY,Edmonson,EDM\n\
,KY,Elliott,ELL\n\
,KY,Estill,EST\n\
,KY,Fayette,FAY\n\
,KY,Fleming,FLE\n\
,KY,Floyd,FLO\n\
,KY,Franklin,FRA\n\
,KY,Fulton,FUL\n\
,KY,Gallatin,GAL\n\
,KY,Garrard,GAR\n\
,KY,Grant,GRT\n\
,KY,Graves,GRV\n\
,KY,Grayson,GRY\n\
,KY,Green,GRE\n\
,KY,Greenup,GRP\n\
,KY,Hancock,HAN\n\
,KY,Hardin,HAR\n\
,KY,Harlan,HRL\n\
,KY,Harrison,HSN\n\
,KY,Hart,HRT\n\
,KY,Henderson,HEN\n\
,KY,Henry,HNY\n\
,KY,Hickman,HIC\n\
,KY,Hopkins,HOP\n\
,KY,Jackson,JAC\n\
,KY,Jefferson,JEF\n\
,KY,Jessamine,JES\n\
,KY,Johnson,JOH\n\
,KY,Kenton,KEN\n\
,KY,Knott,KNT\n\
,KY,Knox,KNX\n\
,KY,Larue,LAR\n\
,KY,Laurel,LAU\n\
,KY,Lawrence,LAW\n\
,KY,LEE,LEE\n\
,KY,Leslie,LES\n\
,KY,Letcher,LET\n\
,KY,Lewis,LEW\n\
,KY,Lincoln,LIN\n\
,KY,Livingston,LIV\n\
,KY,Logan,LOG\n\
,KY,Lyon,LYO\n\
,KY,McCracken,MCC\n\
,KY,McCreary,MCY\n\
,KY,McLean,MCL\n\
,KY,Madison,MAD\n\
,KY,Magoffin,MAG\n\
,KY,Marion,MAR\n\
,KY,Marshall,MSL\n\
,KY,Martin,MAT\n\
,KY,Mason,MAS\n\
,KY,Meade,MEA\n\
,KY,Menifee,MEN\n\
,KY,Mercer,MER\n\
,KY,Metcalfe,MET\n\
,KY,Monroe,MON\n\
,KY,Montgomery,MOT\n\
,KY,Morgan,MOR\n\
,KY,Muhlenberg,MUH\n\
,KY,Nelson,NEL\n\
,KY,Nicholas,NIC\n\
,KY,Ohio,OHI\n\
,KY,Oldham,OLD\n\
,KY,Owen,OWE\n\
,KY,Owsley,OWS\n\
,KY,Pendleton,PEN\n\
,KY,Perry,PER\n\
,KY,Pike,PIK\n\
,KY,Powell,POW\n\
,KY,Pulaski,PUL\n\
,KY,Robertson,ROB\n\
,KY,Rockcastle,ROC\n\
,KY,Rowan,ROW\n\
,KY,Russell,RUS\n\
,KY,Scott,SCO\n\
,KY,Shelby,SHE\n\
,KY,Simpson,SIM\n\
,KY,Spencer,SPE\n\
,KY,Taylor,TAY\n\
,KY,Todd,TOD\n\
,KY,Trigg,TRI\n\
,KY,Trimble,TRM\n\
,KY,Union,UNI\n\
,KY,Warren,WAR\n\
,KY,Washington,WAS\n\
,KY,Wayne,WAY\n\
,KY,Webster,WEB\n\
,KY,Whitley,WHI\n\
,KY,Wolfe,WOL\n\
,KY,Woodford,WOO\n\
Louisiana,LA,Acadia,ACAD\n\
,LA,Allen,ALLE\n\
,LA,Ascension,ASCE\n\
,LA,Assumption,ASSU\n\
,LA,Avoyelles,AVOY\n\
,LA,Beauregard,BEAU\n\
,LA,Bienville,BIEN\n\
,LA,Bossier,BOSS\n\
,LA,Caddo,CADD\n\
,LA,Calcasieu,CALC\n\
,LA,Caldwell,CALD\n\
,LA,Cameron,CAME\n\
,LA,Catahoula,CATA\n\
,LA,Claiborne,CLAI\n\
,LA,Concordia,CONC\n\
,LA,De Soto,DESO\n\
,LA,East Baton Rouge,EBR\n\
,LA,East Carroll,ECAR\n\
,LA,East Feliciana,EFEL\n\
,LA,Evangeline,EVAN\n\
,LA,Franklin,FRAN\n\
,LA,Grant,GRAN\n\
,LA,Iberia,IBER\n\
,LA,Iberville,IBVL\n\
,LA,Jackson,JACK\n\
,LA,Jefferson,JEFF\n\
,LA,Jefferson Davis,JFDV\n\
,LA,La Salle,LASA\n\
,LA,Lafayette,LAFA\n\
,LA,Lafourche,LAFO\n\
,LA,Lincoln,LINC\n\
,LA,Livingston,LIVI\n\
,LA,Madison,MADI\n\
,LA,Morehouse,MORE\n\
,LA,Natchitoches,NATC\n\
,LA,Orleans,ORLE\n\
,LA,Ouachita,OUAC\n\
,LA,Plaquemines,PLAQ\n\
,LA,Pointe Coupee,PCP\n\
,LA,Rapides,RAPI\n\
,LA,Red River,REDR\n\
,LA,Richland,RICH\n\
,LA,Sabine,SABI\n\
,LA,St. Bernard,SBND\n\
,LA,St. Charles,SCHL\n\
,LA,St. Helena,SHEL\n\
,LA,St. James,SJAM\n\
,LA,St. John the Baptist,SJB\n\
,LA,St. Landry,SLAN\n\
,LA,St. Martin,SMT\n\
,LA,St. Mary,SMAR\n\
,LA,St. Tammany,STAM\n\
,LA,Tangipahoa,TANG\n\
,LA,Tensas,TENS\n\
,LA,Terrebonne,TERR\n\
,LA,Union,UNIO\n\
,LA,Vermilion,VERM\n\
,LA,Vernon,VERN\n\
,LA,Washington,WASH\n\
,LA,Webster,WEBS\n\
,LA,West Baton Rouge,WBR\n\
,LA,West Carroll,WCAR\n\
,LA,West Feliciana,WFEL\n\
,LA,Winn,WINN\n\
Massachusetts,MA,Barnstable,BAR\n\
,MA,Berkshire,BER\n\
,MA,Bristol,BRM\n\
,MA,Dukes,DUK\n\
,MA,Essex,ESM\n\
,MA,Franklin,FRA\n\
,MA,Hampden,HMD\n\
,MA,Hampshire,MA\n\
,MA,Middlesex,MIM\n\
,MA,Nantucket,NAN\n\
,MA,Norfolk,NOR\n\
,MA,Plymouth,PLY\n\
,MA,Suffolk,SUF\n\
,MA,Worcester,WOR\n\
Maine,ME,Androscoggin,AND\n\
,ME,Aroostook,ARO\n\
,ME,Cumberland,CBL\n\
,ME,Franklin,FRA\n\
,ME,Hancock,HAN\n\
,ME,Kennebec,KEN\n\
,ME,Knox,KNO\n\
,ME,Lincoln,LIN\n\
,ME,Oxford,OXF\n\
,ME,Penobscot,PEN\n\
,ME,Piscataquis,PSQ\n\
,ME,Sagadahoc,SAG\n\
,ME,Somerset,SOM\n\
,ME,Waldo,WAL\n\
,ME,Washington,WAS\n\
,ME,York,YOR\n\
Manitoba,MB,,\n\
Maryland,MD,Allegany,ALY\n\
,MD,Anne Arundel,ANA\n\
,MD,Baltimore City,BAL\n\
,MD,Baltimore,BCT\n\
,MD,Calvert,CLV\n\
,MD,Caroline,CLN\n\
,MD,Carroll,CRL\n\
,MD,Cecil,CEC\n\
,MD,Charles,CHS\n\
,MD,Dorchester,DRC\n\
,MD,Frederick,FRD\n\
,MD,Garrett,GAR\n\
,MD,Harford,HFD\n\
,MD,Howard,HWD\n\
,MD,Kent,KEN\n\
,MD,Montgomery,MON\n\
,MD,Prince George,PGE\n\
,MD,Queen Anne,QAN\n\
,MD,St. Mary,STM\n\
,MD,Somerset,SMR\n\
,MD,Talbot,TAL\n\
,MD,Washington,WAS\n\
,MD,Washington DC,WRC\n\
,MD,Wicomico,WIC\n\
,MD,Worcester,WRC\n\
,MD,Worcester,\n\
Michigan,MI,Alcona,ALCO\n\
,MI,Alger,ALGE\n\
,MI,Allegan,ALLE\n\
,MI,Alpena,ALPE\n\
,MI,Antrim,ANTR\n\
,MI,Arenac,AREN\n\
,MI,Baraga,BARA\n\
,MI,Barry,BARR\n\
,MI,Bay,BAY\n\
,MI,Benzie,BENZ\n\
,MI,Berrien,BERR\n\
,MI,Branch,BRAN\n\
,MI,Calhoun,CALH\n\
,MI,Cass,CASS\n\
,MI,Charlevoix,CHAR\n\
,MI,Cheboygan,CHEB\n\
,MI,Chippewa,CHIP\n\
,MI,Clare,CLAR\n\
,MI,Clinton,CLIN\n\
,MI,Crawford,CRAW\n\
,MI,Delta,DELT\n\
,MI,Dickinson,DICK\n\
,MI,Eaton,EATO\n\
,MI,Emmet,EMME\n\
,MI,Genesee,GENE\n\
,MI,Gladwin,GLAD\n\
,MI,Gogebic,GOGE\n\
,MI,Gratiot,GRAT\n\
,MI,Grand Traverse,GRTR\n\
,MI,Hillsdale,HILL\n\
,MI,Houghton,HOUG\n\
,MI,Huron,HURO\n\
,MI,Ionia,IONI\n\
,MI,Iosco,IOSC\n\
,MI,Ingham,INGH\n\
,MI,Iron,IRON\n\
,MI,Isabella,ISAB\n\
,MI,Jackson,JACK\n\
,MI,Kalamazoo,KZOO\n\
,MI,Kalkaska,KALK\n\
,MI,Keweenaw,KEWE\n\
,MI,Kent,KENT\n\
,MI,Lake,LAKE\n\
,MI,Lapeer,LAPE\n\
,MI,Leelanau,LEEL\n\
,MI,Lenawee,LENA\n\
,MI,Livingston,LIVI\n\
,MI,Luce,LUCE\n\
,MI,Mackinac,MACK\n\
,MI,Macomb,MACO\n\
,MI,Manistee,MANI\n\
,MI,Marquette,MARQ\n\
,MI,Mason,MASO\n\
,MI,Mecosta,MECO\n\
,MI,Menominee,MENO\n\
,MI,Midland,MIDL\n\
,MI,Missaukee,MISS\n\
,MI,Monroe,MONR\n\
,MI,Montcalm,MCLM\n\
,MI,Montmorency,MTMO\n\
,MI,Muskegon,MUSK\n\
,MI,Newaygo,NEWA\n\
,MI,Oakland,OAKL\n\
,MI,Oceana,OCEA\n\
,MI,Ogemaw,OGEM\n\
,MI,Ontonagon,ONTO\n\
,MI,Osceola,OSCE\n\
,MI,Oscoda,OSCO\n\
,MI,Otsego,OTSE\n\
,MI,Ottawa,OTTA\n\
,MI,Presque Isle,PRES\n\
,MI,Roscommon,ROSC\n\
,MI,Saginaw,SAGI\n\
,MI,Sanilac,SANI\n\
,MI,Schoolcraft,SCHO\n\
,MI,Shiawassee,SHIA\n\
,MI,St. Clair,STCL\n\
,MI,St. Joseph,STJO\n\
,MI,Tuscola,TUSC\n\
,MI,Van Buren,VANB\n\
,MI,Washtenaw,WASH\n\
,MI,Wayne,WAYN\n\
,MI,Wexford,WEXF\n\
Minnesota,MN,Aitkin,AIT\n\
,MN,Anoka,ANO\n\
,MN,Becker,BEC\n\
,MN,Beltrami,BEL\n\
,MN,Benton,BEN\n\
,MN,Big Stone,BIG\n\
,MN,Blue Earth,BLU\n\
,MN,Brown,BRO\n\
,MN,Carlton,CRL\n\
,MN,Carver,CRV\n\
,MN,Cass,CAS\n\
,MN,Chippewa,CHP\n\
,MN,Chisago,CHS\n\
,MN,Clay,CLA\n\
,MN,Clearwater,CLE\n\
,MN,Cook,COO\n\
,MN,Cottonwood,COT\n\
,MN,Crow Wing,CRO\n\
,MN,Dakota,DAK\n\
,MN,Dodge,DOD\n\
,MN,Douglas,DOU\n\
,MN,Fairbault,FAI\n\
,MN,Fillmore,FIL\n\
,MN,Freeborn,FRE\n\
,MN,Goodhue,GOO\n\
,MN,Grant,GRA\n\
,MN,Hennepin,HEN\n\
,MN,Houston,HOU\n\
,MN,Hubbard,HUB\n\
,MN,Isanti,ISA\n\
,MN,Itasca,ITA\n\
,MN,Jackson,JAC\n\
,MN,Kanabec,KNB\n\
,MN,Kandiyohi,KND\n\
,MN,Kittson,KIT\n\
,MN,Koochiching,KOO\n\
,MN,Lac Qui Parle,LAC\n\
,MN,Lake,LAK\n\
,MN,Lake of the Woods,LKW\n\
,MN,Le Sueur,LES\n\
,MN,Lincoln,LIN\n\
,MN,Lyon,LYO\n\
,MN,McLeod,MCL\n\
,MN,Mahnomen,MAH\n\
,MN,Marshall,MRS\n\
,MN,Martin,MRT\n\
,MN,Meeker,MEE\n\
,MN,Mille Lacs,MIL\n\
,MN,Morrison,MOR\n\
,MN,Mower,MOW\n\
,MN,Murray,MUR\n\
,MN,Nicollet,NIC\n\
,MN,Nobles,NOB\n\
,MN,Norman,NOR\n\
,MN,Olmsted,OLM\n\
,MN,Ottertail,OTT\n\
,MN,Pennington,PEN\n\
,MN,Pine,PIN\n\
,MN,Pipestone,PIP\n\
,MN,Polk,POL\n\
,MN,Pope,POP\n\
,MN,Ramsey,RAM\n\
,MN,Red Lake,RDL\n\
,MN,Redwood,RDW\n\
,MN,Renville,REN\n\
,MN,Rice,RIC\n\
,MN,Rock,ROC\n\
,MN,Roseau,ROS\n\
,MN,St Louis,STL\n\
,MN,Scott,SCO\n\
,MN,Sherburne,SHE\n\
,MN,Sibley,SIB\n\
,MN,Stearns,STR\n\
,MN,Steele,STE\n\
,MN,Stevens,STV\n\
,MN,St. Louis,STL\n\
,MN,Swift,SWI\n\
,MN,Todd,TOD\n\
,MN,Traverse,TRA\n\
,MN,Wabasha,WAB\n\
,MN,Wadena,WAD\n\
,MN,Waseca,WSC\n\
,MN,Washington,WSH\n\
,MN,Watonwan,WAT\n\
,MN,Wilkin,WIL\n\
,MN,Winona,WIN\n\
,MN,Wright,WRI\n\
,MN,Yellow Medicine,YEL\n\
Missouri,MO,Adair,ADR\n\
,MO,Andrew,AND\n\
,MO,Atchison,ATC\n\
,MO,Audrain,AUD\n\
,MO,Barry,BAR\n\
,MO,Barton,BTN\n\
,MO,Bates,BAT\n\
,MO,Benton,BEN\n\
,MO,Bollinger,BOL\n\
,MO,Boone,BOO\n\
,MO,Buchanan,BUC\n\
,MO,Butler,BTR\n\
,MO,Caldwell,CWL\n\
,MO,Callaway,CAL\n\
,MO,Camden,CAM\n\
,MO,Cape Girardeau,CPG\n\
,MO,Carroll,CRL\n\
,MO,Carter,CAR\n\
,MO,Cass,CAS\n\
,MO,Cedar,CED\n\
,MO,Chariton,CHN\n\
,MO,Christian,CHR\n\
,MO,Clark,CLK\n\
,MO,Clay,CLA\n\
,MO,Clinton,CLN\n\
,MO,Cole,COL\n\
,MO,Cooper,COP\n\
,MO,Crawford,CRA\n\
,MO,Dade,DAD\n\
,MO,Dallas,DAL\n\
,MO,Daviess,DVS\n\
,MO,DeKalb,DEK\n\
,MO,Dent,DEN\n\
,MO,Douglas,DGL\n\
,MO,Dunklin,DUN\n\
,MO,Franklin,FRA\n\
,MO,Gasconade,GAS\n\
,MO,Gentry,GEN\n\
,MO,Greene,GRN\n\
,MO,Grundy,GRU\n\
,MO,Harrison,HAR\n\
,MO,Henry,HEN\n\
,MO,Hickory,HIC\n\
,MO,Holt,HLT\n\
,MO,Howard,HOW\n\
,MO,Howell,HWL\n\
,MO,Iron,IRN\n\
,MO,Jackson,JAC\n\
,MO,Jasper,JAS\n\
,MO,Jefferson,JEF\n\
,MO,Johnson,JON\n\
,MO,Knox,KNX\n\
,MO,Laclede,LAC\n\
,MO,Lafayette,LAF\n\
,MO,Lawrence,LAW\n\
,MO,Lewis,LEW\n\
,MO,Lincoln,LCN\n\
,MO,Linn,LIN\n\
,MO,Livingston,LIV\n\
,MO,Macon,MAC\n\
,MO,Madison,MAD\n\
,MO,Maries,MRE\n\
,MO,Marion,MAR\n\
,MO,McDonald,MCD\n\
,MO,Mercer,MER\n\
,MO,Miller,MIL\n\
,MO,Mississippi,MIS\n\
,MO,Moniteau,MNT\n\
,MO,Monroe,MON\n\
,MO,Montgomery,MGM\n\
,MO,Morgan,MOR\n\
,MO,New Madrid,NMD\n\
,MO,Newton,NWT\n\
,MO,Nodaway,NOD\n\
,MO,Oregon,ORE\n\
,MO,Osage,OSA\n\
,MO,Ozark,OZA\n\
,MO,Pemiscot,PEM\n\
,MO,Perry,PER\n\
,MO,Pettis,PET\n\
,MO,Phelps,PHE\n\
,MO,Pike,PIK\n\
,MO,Platte,PLA\n\
,MO,Polk,POL\n\
,MO,Pulaski,PUL\n\
,MO,Putnam,PUT\n\
,MO,Ralls,RAL\n\
,MO,Randolph,RAN\n\
,MO,Ray,RAY\n\
,MO,Reynolds,REY\n\
,MO,Ripley,RIP\n\
,MO,Saline,SAL\n\
,MO,Schuyler,SCH\n\
,MO,Scotland,SCT\n\
,MO,Scott,SCO\n\
,MO,Shannon,SHA\n\
,MO,Shelby,SHL\n\
,MO,St. Charles,STC\n\
,MO,St. Clair,SCL\n\
,MO,St. Francois,STF\n\
,MO,St. Genevieve,STG\n\
,MO,St. Louis City,STL\n\
,MO,St. Louis,SLC\n\
,MO,Stoddard,STD\n\
,MO,Stone,STN\n\
,MO,Sullivan,SUL\n\
,MO,Taney,TAN\n\
,MO,Texas,TEX\n\
,MO,Vernon,VRN\n\
,MO,Warren,WAR\n\
,MO,Washington,WAS\n\
,MO,Wayne,WAY\n\
,MO,Webster,WEB\n\
,MO,Worth,WOR\n\
,MO,Wright,WRT\n\
Mississippi,MS,Adams,ADA\n\
,MS,Alcorn,ALC\n\
,MS,Amite,AMI\n\
,MS,Attala,ATT\n\
,MS,Benton,BEN\n\
,MS,Bolivar,BOL\n\
,MS,Calhoun,CAL\n\
,MS,Carroll,CAR\n\
,MS,Chickasaw,CHI\n\
,MS,Choctaw,CHO\n\
,MS,Claiborne,CLB\n\
,MS,Clarke,CLK\n\
,MS,Clay,CLA\n\
,MS,Coahoma,COA\n\
,MS,Copiah,COP\n\
,MS,Covington,COV\n\
,MS,DeSoto,DES\n\
,MS,Forrest,FOR\n\
,MS,Franklin,FRA\n\
,MS,George,GEO\n\
,MS,Greene,GRN\n\
,MS,Grenada,GRE\n\
,MS,Hancock,HAN\n\
,MS,Harrison,HAR\n\
,MS,Hinds,HIN\n\
,MS,Holmes,HOL\n\
,MS,Humphreys,HUM\n\
,MS,Issaquena,ISS\n\
,MS,Itawamba,ITA\n\
,MS,Jackson,JAC\n\
,MS,Jasper,JAS\n\
,MS,Jefferson,JEF\n\
,MS,Jefferson Davis,JDV\n\
,MS,Jones,JON\n\
,MS,Kemper,KEM\n\
,MS,Lafayette,LAF\n\
,MS,Lamar,LAM\n\
,MS,Lauderdale,LAU\n\
,MS,Lawrence,LAW\n\
,MS,Leake,LEA\n\
,MS,Lee,LEE\n\
,MS,Leflore,LEF\n\
,MS,Lincoln,LIN\n\
,MS,Lowndes,LOW\n\
,MS,Madison,MAD\n\
,MS,Marion,MRN\n\
,MS,Marshall,MAR\n\
,MS,Monroe,MON\n\
,MS,Montgomery,MGY\n\
,MS,Neshoba,NES\n\
,MS,Newton,NEW\n\
,MS,Noxubee,NOX\n\
,MS,Oktibbeha,OKT\n\
,MS,Panola,PAN\n\
,MS,Pearl River,PEA\n\
,MS,Perry,PER\n\
,MS,Pike,PIK\n\
,MS,Pontotoc,PON\n\
,MS,Prentiss,PRE\n\
,MS,Quitman,QCI\n\
,MS,Rankin,RAN\n\
,MS,Scott,SCO\n\
,MS,Sharkey,SHA\n\
,MS,Simpson,SIM\n\
,MS,Smith,SMI\n\
,MS,Stone,STO\n\
,MS,Sunflower,SUN\n\
,MS,Tallahatchie,TAL\n\
,MS,Tate,TAT\n\
,MS,Tippah,TIP\n\
,MS,Tishomingo,TIS\n\
,MS,Tunica,TUN\n\
,MS,Union,UNI\n\
,MS,Walthall,WAL\n\
,MS,Warren,WAR\n\
,MS,Washington,WAS\n\
,MS,Wayne,WAY\n\
,MS,Webster,WEB\n\
,MS,Wilkinson,WIL\n\
,MS,Winston,WIN\n\
,MS,Yalobusha,YAL\n\
,MS,Yazoo,YAZ\n\
Montana,MT,Beaverhead,BEA\n\
,MT,Big Horn,BIG\n\
,MT,Blaine,BLA\n\
,MT,Broadwater,BRO\n\
,MT,Carbon,CRB\n\
,MT,Carter,CRT\n\
,MT,Cascade,CAS\n\
,MT,Chouteau,CHO\n\
,MT,Custer,CUS\n\
,MT,Daniels,DAN\n\
,MT,Dawson,DAW\n\
,MT,Deer Lodge,DEE\n\
,MT,Fallon,FAL\n\
,MT,Fergus,FER\n\
,MT,Flathead,FLA\n\
,MT,Gallatin,GAL\n\
,MT,Garfield,GAR\n\
,MT,Glacier,GLA\n\
,MT,Golden Valley,GOL\n\
,MT,Granite,GRA\n\
,MT,Hill,HIL\n\
,MT,Jefferson,JEF\n\
,MT,Judith Basin,JUD\n\
,MT,Lake,LAK\n\
,MT,Lewis & Clark,LEW\n\
,MT,Liberty,LIB\n\
,MT,Lincoln,LIN\n\
,MT,Madison,MAD\n\
,MT,McCone,MCC\n\
,MT,Meagher,MEA\n\
,MT,Mineral,MIN\n\
,MT,Missoula,MIS\n\
,MT,Musselshell,MUS\n\
,MT,Park,PAR\n\
,MT,Petroleum,PET\n\
,MT,Phillips,PHI\n\
,MT,Pondera,PON\n\
,MT,Powder River,PWD\n\
,MT,Powell,PWL\n\
,MT,Prairie,PRA\n\
,MT,Ravalli,RAV\n\
,MT,Richland,RIC\n\
,MT,Roosevelt,ROO\n\
,MT,Rosebud,ROS\n\
,MT,Sanders,SAN\n\
,MT,Sheridan,SHE\n\
,MT,Silver Bow,SIL\n\
,MT,Stillwater,STI\n\
,MT,Sweet Grass,SWE\n\
,MT,Teton,TET\n\
,MT,Toole,TOO\n\
,MT,Treasure,TRE\n\
,MT,Valley,VAL\n\
,MT,Wheatland,WHE\n\
,MT,Wibaux,WIB\n\
,MT,Yellowstone,YEL\n\
North Carolina,NC,Alamance,ALA\n\
,NC,Alexander,ALE\n\
,NC,Alleghany,ALL\n\
,NC,Anson,ANS\n\
,NC,Ashe,ASH\n\
,NC,Avery,AVE\n\
,NC,Beaufort,BEA\n\
,NC,Bertie,BER\n\
,NC,Bladen,BLA\n\
,NC,Brunswick,BRU\n\
,NC,Buncombe,BUN\n\
,NC,Burke,BUR\n\
,NC,Cabarrus,CAB\n\
,NC,Caldwell,CAL\n\
,NC,Camden,CAM\n\
,NC,Carteret,CAR\n\
,NC,Caswell,CAS\n\
,NC,Catawba,CAT\n\
,NC,Chatham,CHA\n\
,NC,Cherokee,CHE\n\
,NC,Chowan,CHO\n\
,NC,Clay,CLA\n\
,NC,Cleveland,CLE\n\
,NC,Columbus,COL\n\
,NC,Craven,CRA\n\
,NC,Cumberland,CUM\n\
,NC,Currituck,CUR\n\
,NC,Dare,DAR\n\
,NC,Davidson,DVD\n\
,NC,Davie,DAV\n\
,NC,Duplin,DUP\n\
,NC,Durham,DUR\n\
,NC,Edgecombe,EDG\n\
,NC,Forsyth,FOR\n\
,NC,Franklin,FRA\n\
,NC,Gaston,GAS\n\
,NC,Gates,GAT\n\
,NC,Graham,GRM\n\
,NC,Granville,GRA\n\
,NC,Greene,GRE\n\
,NC,Guilford,GUI\n\
,NC,Halifax,HAL\n\
,NC,Harnett,HAR\n\
,NC,Haywood,HAY\n\
,NC,Henderson,HEN\n\
,NC,Hertford,HER\n\
,NC,Hoke,HOK\n\
,NC,Hyde,HYD\n\
,NC,Iredell,IRE\n\
,NC,Jackson,JAC\n\
,NC,Johnston,JOH\n\
,NC,Jones,JON\n\
,NC,Lee,LEE\n\
,NC,Lenoir,LEN\n\
,NC,Lincoln,LIN\n\
,NC,Macon,MAC\n\
,NC,Madison,MAD\n\
,NC,Martin,MAR\n\
,NC,McDowell,MCD\n\
,NC,Mecklenburg,MEC\n\
,NC,Mitchell,MIT\n\
,NC,Montgomery,MON\n\
,NC,Moore,MOO\n\
,NC,Nash,NAS\n\
,NC,New Hanover,NEW\n\
,NC,Northampton,NOR\n\
,NC,Onslow,ONS\n\
,NC,Orange,ORA\n\
,NC,Pamlico,PAM\n\
,NC,Pasquotank,PAS\n\
,NC,Pender,PEN\n\
,NC,Perquimans,PEQ\n\
,NC,Person,PER\n\
,NC,Pitt,PIT\n\
,NC,Polk,POL\n\
,NC,Randolph,RAN\n\
,NC,Richmond,RIC\n\
,NC,Robeson,ROB\n\
,NC,Rockingham,ROC\n\
,NC,Rowan,ROW\n\
,NC,Rutherford,RUT\n\
,NC,Sampson,SAM\n\
,NC,Scotland,SCO\n\
,NC,Stanly,STA\n\
,NC,Stokes,STO\n\
,NC,Surry,SUR\n\
,NC,Swain,SWA\n\
,NC,Transylvania,TRA\n\
,NC,Tyrrell,TYR\n\
,NC,Union,UNI\n\
,NC,Vance,VAN\n\
,NC,Wake,WAK\n\
,NC,Warren,WAR\n\
,NC,Washington,WAS\n\
,NC,Watauga,WAT\n\
,NC,Wayne,WAY\n\
,NC,Wilkes,WLK\n\
,NC,Wilson,WIL\n\
,NC,Yadkin,YAD\n\
,NC,Yancey,YAN\n\
North Dakota,ND,Adams,ADM\n\
,ND,Barnes,BRN\n\
,ND,Benson,BSN\n\
,ND,Billings,BLL\n\
,ND,Bottineau,BOT\n\
,ND,Bowman,BOW\n\
,ND,Burke,BRK\n\
,ND,Burleigh,BUR\n\
,ND,Cass,CSS\n\
,ND,Cavalier,CAV\n\
,ND,Dickey,DIK\n\
,ND,Divide,DIV\n\
,ND,Dunn,DUN\n\
,ND,Eddy,EDY\n\
,ND,Emmons,EMN\n\
,ND,Foster,FOS\n\
,ND,Golden Valley,GNV\n\
,ND,Grand Forks,GFK\n\
,ND,Grant,GNT\n\
,ND,Griggs,GRG\n\
,ND,Hettinger,HET\n\
,ND,Kidder,KDR\n\
,ND,LaMoure,LMR\n\
,ND,Logan,LOG\n\
,ND,McHenry,MCH\n\
,ND,McIntosh,MCI\n\
,ND,McKenzie,MCK\n\
,ND,McLean,MCL\n\
,ND,Mercer,MCR\n\
,ND,Morton,MTN\n\
,ND,Mountrail,MRL\n\
,ND,Nelson,NEL\n\
,ND,Oliver,OLR\n\
,ND,Pembina,PBA\n\
,ND,Pierce,PRC\n\
,ND,Ramsey,RMY\n\
,ND,Ransom,RSM\n\
,ND,Renville,REN\n\
,ND,Richland,RLD\n\
,ND,Rolette,ROL\n\
,ND,Sargent,SGT\n\
,ND,Sheridan,SRN\n\
,ND,Sioux,SIX\n\
,ND,Slope,SLP\n\
,ND,Stark,STK\n\
,ND,Steele,STL\n\
,ND,Stutsman,STN\n\
,ND,Towner,TWR\n\
,ND,Traill,TRL\n\
,ND,Walsh,WLH\n\
,ND,Ward,WRD\n\
,ND,Wells,WLS\n\
,ND,Williams,WLM\n\
Nebraska,NE,Adams,ADMS\n\
,NE,Antelope,ANTE\n\
,NE,Arthur,ARTH\n\
,NE,Banner,BANN\n\
,NE,Blaine,BLAI\n\
,NE,Boone,BOON\n\
,NE,Box Butte,BOXB\n\
,NE,Boyd,BOYD\n\
,NE,Brown,BRWN\n\
,NE,Buffalo,BUFF\n\
,NE,Burt,BURT\n\
,NE,Butler,BUTL\n\
,NE,Cass,CASS\n\
,NE,Cedar,CEDA\n\
,NE,Chase,CHAS\n\
,NE,Cherry,CHER\n\
,NE,Cheyenne,CHEY\n\
,NE,Clay,CLAY\n\
,NE,Colfax,COLF\n\
,NE,Cuming,CUMI\n\
,NE,Custer,CUST\n\
,NE,Dakota,DAKO\n\
,NE,Dawes,DAWE\n\
,NE,Dawson,DAWS\n\
,NE,Deuel,DEUE\n\
,NE,Dixon,DIXO\n\
,NE,Dodge,DODG\n\
,NE,Douglas,DGLS\n\
,NE,Dundy,DUND\n\
,NE,Fillmore,FILL\n\
,NE,Franklin,FRNK\n\
,NE,Frontier,FRON\n\
,NE,Furnas,FURN\n\
,NE,Gage,GAGE\n\
,NE,Garden,GARD\n\
,NE,Garfield,GARF\n\
,NE,Gosper,GOSP\n\
,NE,Grant,GRAN\n\
,NE,Greeley,GREE\n\
,NE,Hall,HALL\n\
,NE,Hamilton,HAMI\n\
,NE,Harlan,HRLN\n\
,NE,Hayes,HAYE\n\
,NE,Hitchcock,HITC\n\
,NE,Holt,HOLT\n\
,NE,Hooker,HOOK\n\
,NE,Howard,HOWA\n\
,NE,Jefferson,JEFF\n\
,NE,Johnson,JOHN\n\
,NE,Kearney,KEAR\n\
,NE,Keith,KEIT\n\
,NE,Keya Paha,KEYA\n\
,NE,Kimball,KIMB\n\
,NE,Knox,KNOX\n\
,NE,Lancaster,LNCS\n\
,NE,Lincoln,LINC\n\
,NE,Logan,LOGA\n\
,NE,Loup,LOUP\n\
,NE,Madison,MDSN\n\
,NE,McPherson,MCPH\n\
,NE,Merrick,MERR\n\
,NE,Morrill,MORR\n\
,NE,Nance,NANC\n\
,NE,Nemaha,NEMA\n\
,NE,Nuckolls,NUCK\n\
,NE,Otoe,OTOE\n\
,NE,Pawnee,PAWN\n\
,NE,Perkins,PERK\n\
,NE,Phelps,PHEL\n\
,NE,Pierce,PIER\n\
,NE,Platte,PLAT\n\
,NE,Polk,POLK\n\
,NE,Red Willow,REDW\n\
,NE,Richardson,RICH\n\
,NE,Rock,ROCK\n\
,NE,Saline,SALI\n\
,NE,Sarpy,SARP\n\
,NE,Saunders,SAUN\n\
,NE,Scotts Bluff,SCOT\n\
,NE,Seward,SEWA\n\
,NE,Sheridan,SHRD\n\
,NE,Sherman,SHRM\n\
,NE,Sioux,SIOU\n\
,NE,Stanton,STAN\n\
,NE,Thayer,THAY\n\
,NE,Thomas,THOM\n\
,NE,Thurston,THUR\n\
,NE,Valley,VLLY\n\
,NE,Washington,WASH\n\
,NE,Wayne,WAYN\n\
,NE,Webster,WEBS\n\
,NE,Wheeler,WHEE\n\
,NE,York,YORK\n\
New Brunswick,NB,Albert,\
,NB,Carleton,\
,NB,Charlotte,\
,NB,Gloucester,\
,NB,Kent,\
,NB,Kings,\
,NB,Madawaska,\
,NB,Northumberland,\
,NB,Queens,\
,NB,Restigouche,\
,NB,Saint John,\
,NB,Sunbury,\
,NB,Victoria,\
,NB,Westmorland,\
,NB,York,\
Newfoundland and Labrador,NL,Avalon Peninsula-St. John's,\n\
,NL,Burin Peninsula-Marystown,\n\
,NL,South Coast-Channel-Port aux Basques,\n\
,NL,St. George's-Stephenville,\n\
,NL,Humber District-Corner Brook,\n\
,NL,Central Newfoundland-Grand Falls-Windsor,\n\
,NL,Bonavista/Trinity-Clarenville,\n\
,NL,Notre Dame Bay-Lewisporte,\n\
,NL,Northern Peninsula-St. Anthony,\n\
,NL,Labrador-Happy Valley-Goose Bay,\n\
,NL,Nunatsiavut-Nain,\n\
New Hampshire,NH,Belknap,BEL\n\
,NH,Carroll,CAR\n\
,NH,Cheshire,CHE\n\
,NH,Coos,COO\n\
,NH,Grafton,GRN\n\
,NH,Hillsborough,HIL\n\
,NH,Merrimack,MER\n\
,NH,Rockingham,ROC\n\
,NH,Strafford,STR\n\
,NH,Sullivan,SUL\n\
New Jersey,NJ,Atlantic,ATLA\n\
,NJ,Bergen,BERG\n\
,NJ,Burlington,BURL\n\
,NJ,Camden,CMDN\n\
,NJ,Cape May,CAPE\n\
,NJ,Cumberland,CUMB\n\
,NJ,Essex,ESSE\n\
,NJ,Gloucester,GLOU\n\
,NJ,Hudson,HUDS\n\
,NJ,Hunterdon,HUNT\n\
,NJ,Mercer,MERC\n\
,NJ,Middlesex,MIDD\n\
,NJ,Monmouth,MONM\n\
,NJ,Morris,MORR\n\
,NJ,Ocean,OCEA\n\
,NJ,Passaic,PASS\n\
,NJ,Salem,SALE\n\
,NJ,Somerset,SOME\n\
,NJ,Sussex,SUSS\n\
,NJ,Union,UNIO\n\
,NJ,Warren,WRRN\n\
New Mexico,NM,Bernalillo,BER\n\
,NM,Catron,CAT\n\
,NM,Chaves,CHA\n\
,NM,Cibola,CIB\n\
,NM,Colfax,COL\n\
,NM,Curry,CUR\n\
,NM,De Baca,DEB\n\
,NM,Dona Ana,DON\n\
,NM,Eddy,EDD\n\
,NM,Grant,GRA\n\
,NM,Guadalupe,GUA\n\
,NM,Harding,HAR\n\
,NM,Hidalgo,HID\n\
,NM,Lea,LEA\n\
,NM,Lincoln,LIN\n\
,NM,Los Alamos,LOS\n\
,NM,Luna,LUN\n\
,NM,McKinley,MCK\n\
,NM,Mora,MOR\n\
,NM,Otero,OTE\n\
,NM,Quay,QCA\n\
,NM,Rio Arriba,RIO\n\
,NM,Roosevelt,ROO\n\
,NM,San Juan,SJU\n\
,NM,San Miguel,SMI\n\
,NM,Sandoval,SAN\n\
,NM,Santa Fe,SFE\n\
,NM,Sierra,SIE\n\
,NM,Socorro,SOC\n\
,NM,Taos,TAO\n\
,NM,Torrance,TOR\n\
,NM,Union,UNI\n\
,NM,Valencia,VAL\n\
Nevada,NV,Carson City,CAR\n\
,NV,Churchill,CHU\n\
,NV,Clark,CLA\n\
,NV,Douglas,DOU\n\
,NV,Elko,ELK\n\
,NV,Esmeralda,ESM\n\
,NV,Eureka,EUR\n\
,NV,Humboldt,HUM\n\
,NV,Lander,LAN\n\
,NV,Lincoln,LIN\n\
,NV,Lyon,LYO\n\
,NV,Mineral,MIN\n\
,NV,Nye,NYE\n\
,NV,Pershing,PER\n\
,NV,Storey,STO\n\
,NV,Washoe,WAS\n\
,NV,White Pine,WHI\n\
New York,NY,Albany,ALB\n\
,NY,Allegany,ALL\n\
,NY,Bronx,BRX\n\
,NY,Broome,BRM\n\
,NY,Cattaraugus,CAT\n\
,NY,Cayuga,CAY\n\
,NY,Chautauqua,CHA\n\
,NY,Chemung,CHE\n\
,NY,Chenango,CGO\n\
,NY,Clinton,CLI\n\
,NY,Columbia,COL\n\
,NY,Cortland,COR\n\
,NY,Delaware,DEL\n\
,NY,Dutchess,DUT\n\
,NY,Erie,ERI\n\
,NY,Essex,ESS\n\
,NY,Franklin,FRA\n\
,NY,Fulton,FUL\n\
,NY,Genesee,GEN\n\
,NY,Greene,GRE\n\
,NY,Hamilton,HAM\n\
,NY,Herkimer,HER\n\
,NY,Jefferson,JEF\n\
,NY,Kings,KIN\n\
,NY,Lewis,LEW\n\
,NY,Livingston,LIV\n\
,NY,Madison,MAD\n\
,NY,Monroe,MON\n\
,NY,Montgomery,MTG\n\
,NY,Nassau,NAS\n\
,NY,New York,NEW\n\
,NY,Niagara,NIA\n\
,NY,Oneida,ONE\n\
,NY,Onondaga,ONO\n\
,NY,Ontario,ONT\n\
,NY,Orange,ORA\n\
,NY,Orleans,ORL\n\
,NY,Oswego,OSW\n\
,NY,Otsego,OTS\n\
,NY,Putnam,PUT\n\
,NY,Queens,QCE\n\
,NY,Rensselaer,REN\n\
,NY,Rockland,ROC\n\
,NY,Richmond,RIC\n\
,NY,Saratoga,SAR\n\
,NY,Schenectady,SCH\n\
,NY,Schoharie,SCO\n\
,NY,Schuyler,SCU\n\
,NY,Seneca,SEN\n\
,NY,St. Lawrence,STL\n\
,NY,Steuben,STE\n\
,NY,Suffolk,SUF\n\
,NY,Sullivan,SUL\n\
,NY,Tioga,TIO\n\
,NY,Tompkins,TOM\n\
,NY,Ulster,ULS\n\
,NY,Warren,WAR\n\
,NY,Washington,WAS\n\
,NY,Wayne,WAY\n\
,NY,Westchester,WES\n\
,NY,Wyoming,WYO\n\
,NY,Yates,YAT\n\
Northwest Territories,NT,Inuvik,\n\
,NT,Norman Wells,\n\
,NT,Behchokǫ̀,\n\
,NT,Fort Simpson,\n\
,NT,Fort Smith,\n\
,NT,Yellowknife,\n\
Nova Scotia,NS,Halifax,\n\
,NS,Sydney,\n\
,NS,Kentville,\n\
,NS,Truro,\n\
,NS,Liverpool,\n\
,NS,Shelburne,\n\
,NS,Yarmouth,\n\
Nunavut,NU,Kitikmeot,\n\
NU,Qikiqtaaluk,\n\
NU,Kivalliq,\n\
Ohio,OH,Adams,ADAM\n\
,OH,Allen,ALLE\n\
,OH,Ashland,ASHL\n\
,OH,Ashtabula,ASHT\n\
,OH,Athens,ATHE\n\
,OH,Auglaze,AUGL\n\
,OH,Belmont,BELM\n\
,OH,Brown,BROW\n\
,OH,Butler,BUTL\n\
,OH,Carroll,CARR\n\
,OH,Champaign,CHAM\n\
,OH,Clark,CLAR\n\
,OH,Clermont,CLER\n\
,OH,Clinton,CLIN\n\
,OH,Columbiana,COLU\n\
,OH,Coshocton,COSH\n\
,OH,Crawford,CRAW\n\
,OH,Cuyahoga,CUYA\n\
,OH,Darke,DARK\n\
,OH,Defiance,DEFI\n\
,OH,Delaware,DELA\n\
,OH,Erie,ERIE\n\
,OH,Fairfield,FAIR\n\
,OH,Fayette,FAYE\n\
,OH,Franklin,FRAN\n\
,OH,Fulton,FULT\n\
,OH,Gallia,GALL\n\
,OH,Geauga,GEAU\n\
,OH,Greene,GREE\n\
,OH,Guernsey,GUER\n\
,OH,Hamilton,HAMI\n\
,OH,Hancock,HANC\n\
,OH,Hardin,HARD\n\
,OH,Harrison,HARR\n\
,OH,Henry,HENR\n\
,OH,Highland,HIGH\n\
,OH,Hocking,HOCK\n\
,OH,Holmes,HOLM\n\
,OH,Huron,HURO\n\
,OH,Jackson,JACK\n\
,OH,Jefferson,JEFF\n\
,OH,Knox,KNOX\n\
,OH,Lake,LAKE\n\
,OH,Lawrence,LAWR\n\
,OH,Licking,LICK\n\
,OH,Logan,LOGA\n\
,OH,Lorain,LORA\n\
,OH,Lucas,LUCA\n\
,OH,Madison,MADI\n\
,OH,Mahoning,MAHO\n\
,OH,Marion,MARI\n\
,OH,Medina,MEDI\n\
,OH,Meigs,MEIG\n\
,OH,Mercer,MERC\n\
,OH,Miami,MIAM\n\
,OH,Monroe,MONR\n\
,OH,Montgomery,MONT\n\
,OH,Morgan,MORG\n\
,OH,Morrow,MORR\n\
,OH,Muskingum,MUSK\n\
,OH,Noble,NOBL\n\
,OH,Ottawa,OTTA\n\
,OH,Paulding,PAUL\n\
,OH,Perry,PERR\n\
,OH,Pickaway,PICK\n\
,OH,Pike,PIKE\n\
,OH,Portage,PORT\n\
,OH,Preble,PREB\n\
,OH,Putnam,PUTN\n\
,OH,Richland,RICH\n\
,OH,Ross,ROSS\n\
,OH,Sandusky,SAND\n\
,OH,Scioto,SCIO\n\
,OH,Seneca,SENE\n\
,OH,Shelby,SHEL\n\
,OH,Stark,STAR\n\
,OH,Summit,SUMM\n\
,OH,Trumbull,TRUM\n\
,OH,Tuscarawas,TUSC\n\
,OH,Union,UNIO\n\
,OH,VanWert,VANW\n\
,OH,Vinton,VINT\n\
,OH,Warren,WARR\n\
,OH,Washington,WASH\n\
,OH,Wayne,WAYN\n\
,OH,Williams,WILL\n\
,OH,Wood,WOOD\n\
,OH,Wyandot,WYAN\n\
Oklahoma,OK,Adair,ADA\n\
,OK,Alfalfa,ALF\n\
,OK,Atoka,ATO\n\
,OK,Beaver,BEA\n\
,OK,Beckham,BEC\n\
,OK,Blaine,BLA\n\
,OK,Bryan,BRY\n\
,OK,Caddo,CAD\n\
,OK,Canadian,CAN\n\
,OK,Carter,CAR\n\
,OK,Cherokee,CHE\n\
,OK,Choctaw,CHO\n\
,OK,Cimarron,CIM\n\
,OK,Cleveland,CLE\n\
,OK,Coal,COA\n\
,OK,Comanche,COM\n\
,OK,Cotton,COT\n\
,OK,Craig,CRA\n\
,OK,Creek,CRE\n\
,OK,Custer,CUS\n\
,OK,Delaware,DEL\n\
,OK,Dewey,DEW\n\
,OK,Ellis,ELL\n\
,OK,Garfield,GAR\n\
,OK,Garvin,GRV\n\
,OK,Grady,GRA\n\
,OK,Grant,GNT\n\
,OK,Greer,GRE\n\
,OK,Harmon,HAR\n\
,OK,Harper,HRP\n\
,OK,Haskell,HAS\n\
,OK,Hughes,HUG\n\
,OK,Jackson,JAC\n\
,OK,Jefferson,JEF\n\
,OK,Johnston,JOH\n\
,OK,Kay,KAY\n\
,OK,Kingfisher,KIN\n\
,OK,Kiowa,KIO\n\
,OK,Latimer,LAT\n\
,OK,Le Flore,LEF\n\
,OK,Lincoln,LIN\n\
,OK,Logan,LOG\n\
,OK,Love,LOV\n\
,OK,McClain,MCL\n\
,OK,McCurtain,MCU\n\
,OK,McIntosh,MCI\n\
,OK,Major,MAJ\n\
,OK,Marshall,MAR\n\
,OK,Mayes,MAY\n\
,OK,Murray,MUR\n\
,OK,Muskogee,MUS\n\
,OK,Noble,NOB\n\
,OK,Nowata,NOW\n\
,OK,Okfuskee,OKF\n\
,OK,Oklahoma,OKL\n\
,OK,Okmulgee,OKM\n\
,OK,Osage,OSA\n\
,OK,Ottawa,OTT\n\
,OK,Pawnee,PAW\n\
,OK,Payne,PAY\n\
,OK,Pittsburg,PIT\n\
,OK,Pontotoc,PON\n\
,OK,Pottawatomie,POT\n\
,OK,Pushmataha,PUS\n\
,OK,Roger Mills,RGM\n\
,OK,Rogers,ROG\n\
,OK,Seminole,SEM\n\
,OK,Sequoyah,SEQ\n\
,OK,Stephens,STE\n\
,OK,Texas,TEX\n\
,OK,Tillman,TIL\n\
,OK,Tulsa,TUL\n\
,OK,Wagoner,WAG\n\
,OK,Washington,WAS\n\
,OK,Washita,WAT\n\
,OK,Woods,WOO\n\
,OK,Woodward,WDW\n\
Ontario,ON,Algoma District,ALG\n\
,ON,City of Brant,BRA\n\
,ON,City of Brantford,BFD\n\
,ON,Bruce County,BRU\n\
,ON,City of Chatham-Kent,CHK\n\
,ON,Cochrane District,COC\n\
,ON,Dufferin County,DUF\n\
,ON,Durham Regional Municipality,DUR\n\
,ON,Elgin County,ELG\n\
,ON,Essex County,ESX\n\
,ON,Frontenac County,FRO\n\
,ON,Grey County,GRY\n\
,ON,Town of Haldimand,HAL\n\
,ON,Haliburton County,HLB\n\
,ON,Halton Regional Municipality,HTN\n\
,ON,City of Hamilton,HAM\n\
,ON,Hastings County,HAS\n\
,ON,Huron County,HUR\n\
,ON,City of Kawartha Lakes,KAW\n\
,ON,Kenora District,KEN\n\
,ON,Lambton County,LAM\n\
,ON,Lanark County,LAN\n\
,ON,Leeds Grenville United Counties,LGR\n\
,ON,Lennox-Addington County,LXA\n\
,ON,Manitoulin District,MAN\n\
,ON,Middlesex County,MSX\n\
,ON,Muskoka District,MUS\n\
,ON,Niagara Regional Municipality,NIA\n\
,ON,Nipissing District,NIP\n\
,ON,Town of Norfolk,NFK\n\
,ON,Northumberland County,NOR\n\
,ON,City of Ottawa,OTT\n\
,ON,Oxford County,OXF\n\
,ON,Parry Sound District,PSD\n\
,ON,Peel Regional Municipality,PEL\n\
,ON,Perth County,PER\n\
,ON,Peterborough County,PET\n\
,ON,United Counties of Prescott Russell,PRU\n\
,ON,City of Prince Edward,PED\n\
,ON,Rainy River District,RAI\n\
,ON,Renfrew County,REN\n\
,ON,Simcoe County,SIM\n\
,ON,United Counties of Stormont Dundas Glengarry,SDG\n\
,ON,Sudbury District,SUD\n\
,ON,Thunder Bay District,TBY\n\
,ON,Timiskaming District,TIM\n\
,ON,City of Toronto,TOR\n\
,ON,Waterloo Regional Municipality,WAT\n\
,ON,Wellington County,WEL\n\
,ON,York Regional Municipality,YRK\n\
Oregon,OR,Baker,BAK\n\
,OR,Benton,BEN\n\
,OR,Clackamas,CLK\n\
,OR,Clatsop,CLT\n\
,OR,Columbia,COL\n\
,OR,Coos,COO\n\
,OR,Crook,CRO\n\
,OR,Curry,CUR\n\
,OR,Deschutes,DES\n\
,OR,Douglas,DOU\n\
,OR,Gilliam,GIL\n\
,OR,Grant,GRA\n\
,OR,Harney,HAR\n\
,OR,Hood River,HOO\n\
,OR,Jackson,JAC\n\
,OR,Jefferson,JEF\n\
,OR,Josephine,JOS\n\
,OR,Klamath,KLA\n\
,OR,Lake,LAK\n\
,OR,Lane,LAN\n\
,OR,Lincoln,LCN\n\
,OR,Linn,LNN\n\
,OR,Malheur,MAL\n\
,OR,Marion,MAR\n\
,OR,Morrow,MOR\n\
,OR,Multnomah,MUL\n\
,OR,Polk,POL\n\
,OR,Sherman,SHE\n\
,OR,Tillamook,TIL\n\
,OR,Umatilla,UMA\n\
,OR,Union,UNI\n\
,OR,Wallowa,WAL\n\
,OR,Wasco,WCO\n\
,OR,Washington,WSH\n\
,OR,Wheeler,WHE\n\
,OR,Yamhill,YAM\n\
Pennsylvania,PA,Adams,ADA\n\
,PA,Allegheny,ALL\n\
,PA,Armstrong,ARM\n\
,PA,Beaver,BEA\n\
,PA,Bedford,BED\n\
,PA,Berks,BER\n\
,PA,Blair,BLA\n\
,PA,Bradford,BRA\n\
,PA,Bucks,BUX\n\
,PA,Butler,BUT\n\
,PA,Cambria,CMB\n\
,PA,Cameron,CRN\n\
,PA,Carbon,CAR\n\
,PA,Centre,CEN\n\
,PA,Chester,CHE\n\
,PA,Clarion,CLA\n\
,PA,Clearfield,CLE\n\
,PA,Clinton,CLI\n\
,PA,Columbia,COL\n\
,PA,Crawford,CRA\n\
,PA,Cumberland,CUM\n\
,PA,Dauphin,DAU\n\
,PA,Delaware,DCO\n\
,PA,Elk,ELK\n\
,PA,Erie,ERI\n\
,PA,Fayette,FAY\n\
,PA,Forest,FOR\n\
,PA,Franklin,FRA\n\
,PA,Fulton,FUL\n\
,PA,Greene,GRE\n\
,PA,Huntingdon,HUN\n\
,PA,Indiana,INN\n\
,PA,Jefferson,JEF\n\
,PA,Juniata,JUN\n\
,PA,Lackawanna,LAC\n\
,PA,Lancaster,LAN\n\
,PA,Lawrence,LAW\n\
,PA,Lebanon,LEB\n\
,PA,Lehigh,LEH\n\
,PA,Luzerne,LUZ\n\
,PA,Lycoming,LYC\n\
,PA,Mc Kean,MCK\n\
,PA,Mercer,MER\n\
,PA,Mifflin,MIF\n\
,PA,Monroe,MOE\n\
,PA,Montgomery,MGY\n\
,PA,Montour,MTR\n\
,PA,Northampton,NHA\n\
,PA,Northumberland,NUM\n\
,PA,Perry,PER\n\
,PA,Philadelphia,PHI\n\
,PA,Pike,PIK\n\
,PA,Potter,POT\n\
,PA,Schuylkill,SCH\n\
,PA,Snyder,SNY\n\
,PA,Somerset,SOM\n\
,PA,Sullivan,SUL\n\
,PA,Susquehanna,SUS\n\
,PA,Tioga,TIO\n\
,PA,Union,UNI\n\
,PA,Venango,VEN\n\
,PA,Warren,WAR\n\
,PA,Washington,WAS\n\
,PA,Wayne,WAY\n\
,PA,Westmoreland,WES\n\
,PA,Wyoming,WYO\n\
,PA,York,YOR\n\
Prince Edward Island,PE,Alberton,\n\
,PE,Charlottetown,\n\
,PE,Cornwall,\n\
,PE,Georgetown,\n\
,PE,Kensington,\n\
,PE,Montague,\n\
,PE,Souris,\n\
,PE,Stratford,\n\
,PE,Summerside,\n\
,PE,Tignish,\n\
Puerto Rico,PR,Adjuntas Municipio,\n\
,PR,Aguada Municipio,\n\
,PR,Aguadilla Municipio,\n\
,PR,Aguas Buenas Municipio,\n\
,PR,Aibonito Municipio,\n\
,PR,Anasco Municipio,\n\
,PR,Arecibo Municipio,\n\
,PR,Arroyo Municipio,\n\
,PR,Barceloneta Municipio,\n\
,PR,Barranquitas Municipio,\n\
,PR,Bayamon Municipio,\n\
,PR,Cabo Rojo Municipio,\n\
,PR,Caguas Municipio,\n\
,PR,Camuy Municipio,\n\
,PR,Canovanas Municipio,\n\
,PR,Carolina Municipio,\n\
,PR,Catano Municipio,\n\
,PR,Cayey Municipio,\n\
,PR,Ceiba Municipio,\n\
,PR,Ciales Municipio,\n\
,PR,Cidra Municipio,\n\
,PR,Coamo Municipio,\n\
,PR,Comerio Municipio,\n\
,PR,Corozal Municipio,\n\
,PR,Culebra Municipio,\n\
,PR,Dorado Municipio,\n\
,PR,Fajardo Municipio,\n\
,PR,Florida Municipio,\n\
,PR,Guanica Municipio,\n\
,PR,Guayama Municipio,\n\
,PR,Guayanilla Municipio,\n\
,PR,Guaynabo Municipio,\n\
,PR,Gurabo Municipio,\n\
,PR,Hatillo Municipio,\n\
,PR,Hormigueros Municipio,\n\
,PR,Humacao Municipio,\n\
,PR,Isabela Municipio,\n\
,PR,Jayuya Municipio,\n\
,PR,Juana Diaz Municipio,\n\
,PR,Juncos Municipio,\n\
,PR,Lajas Municipio,\n\
,PR,Lares Municipio,\n\
,PR,Las Marias Municipio,\n\
,PR,Las Piedras Municipio,\n\
,PR,Loiza Municipio,\n\
,PR,Luquillo Municipio,\n\
,PR,Manati Municipio,\n\
,PR,Maricao Municipio,\n\
,PR,Maunabo Municipio,\n\
,PR,Mayaguez Municipio,\n\
,PR,Moca Municipio,\n\
,PR,Morovis Municipio,\n\
,PR,Naguabo Municipio,\n\
,PR,Naranjito Municipio,\n\
,PR,Orocovis Municipio,\n\
,PR,Patillas Municipio,\n\
,PR,Penuelas Municipio,\n\
,PR,Ponce Municipio,\n\
,PR,Quebradillas Municipio,\n\
,PR,Rincon Municipio,\n\
,PR,Rio Grande Municipio,\n\
,PR,Sabana Grande Municipio,\n\
,PR,Salinas Municipio,\n\
,PR,San German Municipio,\n\
,PR,San Juan Municipio,\n\
,PR,San Lorenzo Municipio,\n\
,PR,San Sebastian Municipio,\n\
,PR,Santa Isabel Municipio,\n\
,PR,Toa Alta Municipio,\n\
,PR,Toa Baja Municipio,\n\
,PR,Trujillo Alto Municipio,\n\
,PR,Utuado Municipio,\n\
,PR,Vega Alta Municipio,\n\
,PR,Vega Baja Municipio,\n\
,PR,Vieques Municipio,\n\
,PR,Villalba Municipio,\n\
,PR,Yabucoa Municipio,\n\
,PR,Yauco Municipio,\n\
Quebec,QC,Bas-Saint-Laurent,\n\
,QC,Saguenay–Lac-Saint-Jean,\n\
,QC,Capitale-Nationale,\n\
,QC,Mauricie,\n\
,QC,Estrie,\n\
,QC,Montréal,\n\
,QC,Outaouais,\n\
,QC,Abitibi-Témiscamingue,\n\
,QC,Côte-Nord,\n\
,QC,Nord-du-Québec,\n\
,QC,CRÉ de la Baie-James,\n\
,QC,Cree Regional Authority,\n\
,QC,Kativik Regional Government,\n\
,QC,Gaspésie–Îles-de-la-Madeleine,\n\
,QC,Chaudière-Appalaches,\n\
,QC,Laval,\n\
,QC,Lanaudière,\n\
,QC,Laurentides,\n\
,QC,Montérégie,\n\
,QC,CRÉ de Longueuil,\n\
,QC,CRÉ Montérégie Est,\n\
,QC,CRÉ Vallée-du-Haut-Saint-Laurent,\n\
,QC,Centre-du-Québec,\n\
Rhode Island,RI,Bristol,BRI\n\
,RI,Kent,KNT\n\
,RI,Newport,NEW\n\
,RI,Providence,PRO\n\
,RI,Washington,WAR\n\
Saskatchewan,SA,,\n\
,SA,Assiniboia,,\n\
,SA,Battleford,,\n\
,SA,Estevan,,\n\
,SA,Kindersley,,\n\
,SA,La Ronge,,\n\
,SA,Lloydminster,,\n\
,SA,Maple Creek,,\n\
,SA,Melfort,,\n\
,SA,Melville,,\n\
,SA,Moose Jaw,,\n\
,SA,North Battleford,,\n\
,SA,Prince Albert,,\n\
,SA,Regina,,\n\
,SA,Saskatoon,,\n\
,SA,Swift Current,,\n\
,SA,Weyburn,,\n\
,SA,Wynyard,,\n\
,SA,Yorkton,,\n\
South Carolina,SC,Abbeville,ABBE\n\
,SC,Aiken,AIKE\n\
,SC,Allendale,ALLE\n\
,SC,Anderson,ANDE\n\
,SC,Bamberg,BAMB\n\
,SC,Barnwell,BARN\n\
,SC,Beaufort,BEAU\n\
,SC,Berkeley,BERK\n\
,SC,Calhoun,CHOU\n\
,SC,Charleston,CHAR\n\
,SC,Chester,CHES\n\
,SC,Chesterfield,CHFD\n\
,SC,Cherokee,CKEE\n\
,SC,Clarendon,CLRN\n\
,SC,Colleton,COLL\n\
,SC,Darlington,DARL\n\
,SC,Dillon,DILL\n\
,SC,Dorchester,DORC\n\
,SC,Edgefield,EDGE\n\
,SC,Fairfield,FAIR\n\
,SC,Florence,FLOR\n\
,SC,Georgetown,GEOR\n\
,SC,Greenwood,GRWD\n\
,SC,Greenville,GVIL\n\
,SC,Hampton,HAMP\n\
,SC,Horry,HORR\n\
,SC,Jasper,JASP\n\
,SC,Kershaw,KERS\n\
,SC,Laurens,LAUR\n\
,SC,Lee,LEE\n\
,SC,Lexington,LEXI\n\
,SC,Lancaster,LNCS\n\
,SC,Marion,MARI\n\
,SC,Marlboro,MARL\n\
,SC,McCormick,MCOR\n\
,SC,Newberry,NEWB\n\
,SC,Oconee,OCON\n\
,SC,Orangeburg,ORNG\n\
,SC,Pickens,PICK\n\
,SC,Richland,RICH\n\
,SC,Saluda,SALU\n\
,SC,Spartanburg,SPAR\n\
,SC,Sumter,SUMT\n\
,SC,Union,UNIO\n\
,SC,Williamsburg,WILL\n\
,SC,York,YORK\n\
South Dakota,SD,Aurora,AURO\n\
,SD,Beadle,BEAD\n\
,SD,Bennett,BENN\n\
,SD,Bon Homme,BONH\n\
,SD,Brookings,BROO\n\
,SD,Brule,BRUL\n\
,SD,Brown,BRWN\n\
,SD,Buffalo,BUFF\n\
,SD,Butte,BUTT\n\
,SD,Campbell,CAMP\n\
,SD,Charles Mix,CHAR\n\
,SD,Clay,CLAY\n\
,SD,Clark,CLRK\n\
,SD,Codington,CODI\n\
,SD,Corson,CORS\n\
,SD,Custer,CUST\n\
,SD,Davison,DAVI\n\
,SD,Day,DAY\n\
,SD,Deuel,DEUE\n\
,SD,Dewey,DEWY\n\
,SD,Douglas,DGLS\n\
,SD,Edmunds,EDMU\n\
,SD,Fall River,FALL\n\
,SD,Faulk,FAUL\n\
,SD,Grant,GRAN\n\
,SD,Gregory,GREG\n\
,SD,Haakon,HAAK\n\
,SD,Hamlin,HAML\n\
,SD,Hand,HAND\n\
,SD,Hanson,HNSN\n\
,SD,Harding,HRDG\n\
,SD,Hughes,HUGH\n\
,SD,Hutchinson,HUTC\n\
,SD,Hyde,HYDE\n\
,SD,Jerauld,JERA\n\
,SD,Jackson,JKSN\n\
,SD,Jones,JONE\n\
,SD,Kingsbury,KING\n\
,SD,Lake,LAKE\n\
,SD,Lawrence,LAWR\n\
,SD,Lincoln,LINC\n\
,SD,Lyman,LYMA\n\
,SD,McCook,MCOO\n\
,SD,McPherson,MCPH\n\
,SD,Meade,MEAD\n\
,SD,Mellette,MELL\n\
,SD,Miner,MINE\n\
,SD,Minnehaha,MINN\n\
,SD,Moody,MOOD\n\
,SD,Marshall,MRSH\n\
,SD,Oglala Lakota,OGLA\n\
,SD,Pennington,PENN\n\
,SD,Perkins,PERK\n\
,SD,Potter,POTT\n\
,SD,Roberts,ROBE\n\
,SD,Sanborn,SANB\n\
,SD,Spink,SPIN\n\
,SD,Stanley,STAN\n\
,SD,Sully,SULL\n\
,SD,Todd,TODD\n\
,SD,Tripp,TRIP\n\
,SD,Turner,TURN\n\
,SD,Union,UNIO\n\
,SD,Walworth,WALW\n\
,SD,Yankton,YANK\n\
,SD,Ziebach,ZIEB\n\
Tennessee,TN,Anderson,ANDE\n\
,TN,Bedford,BEDF\n\
,TN,Benton,BENT\n\
,TN,Bledsoe,BLED\n\
,TN,Blount,BLOU\n\
,TN,Bradley,BRAD\n\
,TN,Campbell,CAMP\n\
,TN,Cannon,CANN\n\
,TN,Carroll,CARR\n\
,TN,Carter,CART\n\
,TN,Cheatham,CHEA\n\
,TN,Chester,CHES\n\
,TN,Claiborne,CLAI\n\
,TN,Clay,CLAY\n\
,TN,Cocke,COCK\n\
,TN,Coffee,COFF\n\
,TN,Crockett,CROC\n\
,TN,Cumberland,CUMB\n\
,TN,Davidson,DAVI\n\
,TN,Decatur,DECA\n\
,TN,DeKalb,DEKA\n\
,TN,Dickson,DICK\n\
,TN,Dyer,DYER\n\
,TN,Fayette,FAYE\n\
,TN,Fentress,FENT\n\
,TN,Franklin,FRAN\n\
,TN,Gibson,GIBS\n\
,TN,Giles,GILE\n\
,TN,Grainger,GRAI\n\
,TN,Greene,GREE\n\
,TN,Grundy,GRUN\n\
,TN,Hamblen,HAMB\n\
,TN,Hamilton,HAMI\n\
,TN,Hancock,HANC\n\
,TN,Hardeman,HARD\n\
,TN,Hardin,HARN\n\
,TN,Hawkins,HAWK\n\
,TN,Haywood,HAYW\n\
,TN,Henderson,HEND\n\
,TN,Henry,HENR\n\
,TN,Hickman,HICK\n\
,TN,Houston,HOUS\n\
,TN,Humphreys,HUMP\n\
,TN,Jackson,JACK\n\
,TN,Jefferson,JEFF\n\
,TN,Johnson,JOHN\n\
,TN,Knox,KNOX\n\
,TN,Lake,LAKE\n\
,TN,Lauderdale,LAUD\n\
,TN,Lawrence,LAWR\n\
,TN,Lewis,LEWI\n\
,TN,Lincoln,LINC\n\
,TN,Loudon,LOUD\n\
,TN,Macon,MACO\n\
,TN,Madison,MADI\n\
,TN,Marion,MARI\n\
,TN,Marshall,MARS\n\
,TN,Maury,MAUR\n\
,TN,McMinn,MCMI\n\
,TN,McNairy,MCNA\n\
,TN,Meigs,MEIG\n\
,TN,Monroe,MONR\n\
,TN,Montgomery,MONT\n\
,TN,Moore,MOOR\n\
,TN,Morgan,MORG\n\
,TN,Obion,OBIO\n\
,TN,Overton,OVER\n\
,TN,Perry,PERR\n\
,TN,Pickett,PICK\n\
,TN,Polk,POLK\n\
,TN,Putnam,PUTN\n\
,TN,Rhea,RHEA\n\
,TN,Roane,ROAN\n\
,TN,Robertson,ROBE\n\
,TN,Rutherford,RUTH\n\
,TN,Scott,SCOT\n\
,TN,Sequatchie,SEQU\n\
,TN,Sevier,SEVI\n\
,TN,Shelby,SHEL\n\
,TN,Smith,SMIT\n\
,TN,Stewart,STEW\n\
,TN,Sullivan,SULL\n\
,TN,Sumner,SUMN\n\
,TN,Tipton,TIPT\n\
,TN,Trousdale,TROU\n\
,TN,Unicoi,UNIC\n\
,TN,Union,UNIO\n\
,TN,Van Buren,VANB\n\
,TN,Warren,WARR\n\
,TN,Washington,WASH\n\
,TN,Wayne,WAYN\n\
,TN,Weakley,WEAK\n\
,TN,White,WHIT\n\
,TN,Williamson,WILL\n\
,TN,Wilson,WILS\n\
Texas,TX,Anderson,ANDE\n\
,TX,Andrews,ANDR\n\
,TX,Angelina,ANGE\n\
,TX,Aransas,ARAN\n\
,TX,Archer,ARCH\n\
,TX,Armstrong,ARMS\n\
,TX,Atascosa,ATAS\n\
,TX,Austin,AUST\n\
,TX,Bailey,BAIL\n\
,TX,Bandera,BAND\n\
,TX,Bastrop,BAST\n\
,TX,Baylor,BAYL\n\
,TX,Bee,BEE\n\
,TX,Bell,BELL\n\
,TX,Bexar,BEXA\n\
,TX,Blanco,BLAN\n\
,TX,Borden,BORD\n\
,TX,Bosque,BOSQ\n\
,TX,Bowie,BOWI\n\
,TX,Brazoria,BZIA\n\
,TX,Brazos,BZOS\n\
,TX,Brewster,BREW\n\
,TX,Briscoe,BRIS\n\
,TX,Brooks,BROO\n\
,TX,Brown,BROW\n\
,TX,Burleson,BURL\n\
,TX,Burnet,BURN\n\
,TX,Caldwell,CALD\n\
,TX,Calhoun,CALH\n\
,TX,Callahan,CALL\n\
,TX,Cameron,CMRN\n\
,TX,Camp,CAMP\n\
,TX,Carson,CARS\n\
,TX,Cass,CASS\n\
,TX,Castro,CAST\n\
,TX,Chambers,CHAM\n\
,TX,Cherokee,CHER\n\
,TX,Childress,CHIL\n\
,TX,Clay,CLAY\n\
,TX,Cochran,COCH\n\
,TX,Coke,COKE\n\
,TX,Coleman,COLE\n\
,TX,Collin,COLN\n\
,TX,Collingsworth,COLW\n\
,TX,Colorado,COLO\n\
,TX,Comal,COML\n\
,TX,Comanche,COMA\n\
,TX,Concho,CONC\n\
,TX,Cooke,COOK\n\
,TX,Coryell,CORY\n\
,TX,Cottle,COTT\n\
,TX,Crane,CRAN\n\
,TX,Crockett,CROC\n\
,TX,Crosby,CROS\n\
,TX,Culberson,CULB\n\
,TX,Dallam,DALM\n\
,TX,Dallas,DALS\n\
,TX,Dawson,DAWS\n\
,TX,Deaf Smith,DSMI\n\
,TX,Delta,DELT\n\
,TX,Denton,DENT\n\
,TX,Dewitt,DEWI\n\
,TX,Dickens,DICK\n\
,TX,Dimmit,DIMM\n\
,TX,Donley,DONL\n\
,TX,Duval,DUVA\n\
,TX,Eastland,EAST\n\
,TX,Ector,ECTO\n\
,TX,Edwards,EDWA\n\
,TX,El Paso,EPAS\n\
,TX,Ellis,ELLI\n\
,TX,Erath,ERAT\n\
,TX,Falls,FALL\n\
,TX,Fannin,FANN\n\
,TX,Fayette,FAYE\n\
,TX,Fisher,FISH\n\
,TX,Floyd,FLOY\n\
,TX,Foard,FOAR\n\
,TX,Fort Bend,FBEN\n\
,TX,Franklin,FRAN\n\
,TX,Freestone,FREE\n\
,TX,Frio,FRIO\n\
,TX,Gaines,GAIN\n\
,TX,Galveston,GALV\n\
,TX,Garza,GARZ\n\
,TX,Gillespie,GILL\n\
,TX,Glasscock,GLAS\n\
,TX,Goliad,GOLI\n\
,TX,Gonzales,GONZ\n\
,TX,Gray,GRAY\n\
,TX,Grayson,GRSN\n\
,TX,Gregg,GREG\n\
,TX,Grimes,GRIM\n\
,TX,Guadalupe,GUAD\n\
,TX,Hale,HALE\n\
,TX,Hall,HALL\n\
,TX,Hamilton,HAMI\n\
,TX,Hansford,HANS\n\
,TX,Hardeman,HDMN\n\
,TX,Hardin,HRDN\n\
,TX,Harris,HARR\n\
,TX,Harrison,HRSN\n\
,TX,Hartley,HART\n\
,TX,Haskell,HASK\n\
,TX,Hays,HAYS\n\
,TX,Hemphill,HEMP\n\
,TX,Henderson,HEND\n\
,TX,Hidalgo,HIDA\n\
,TX,Hill,HILL\n\
,TX,Hockley,HOCK\n\
,TX,Hood,HOOD\n\
,TX,Hopkins,HOPK\n\
,TX,Houston,HOUS\n\
,TX,Howard,HOWA\n\
,TX,Hudspeth,HUDS\n\
,TX,Hunt,HUNT\n\
,TX,Hutchinson,HUTC\n\
,TX,Irion,IRIO\n\
,TX,Jack,JACK\n\
,TX,Jackson,JKSN\n\
,TX,Jasper,JASP\n\
,TX,Jeff Davis,JDAV\n\
,TX,Jefferson,JEFF\n\
,TX,Jim Hogg,JHOG\n\
,TX,Jim Wells,JWEL\n\
,TX,Johnson,JOHN\n\
,TX,Jones,JONE\n\
,TX,Karnes,KARN\n\
,TX,Kaufman,KAUF\n\
,TX,Kendall,KEND\n\
,TX,Kenedy,KENY\n\
,TX,Kent,KENT\n\
,TX,Kerr,KERR\n\
,TX,Kimble,KIMB\n\
,TX,King,KING\n\
,TX,Kinney,KINN\n\
,TX,Kleberg,KLEB\n\
,TX,Knox,KNOX\n\
,TX,Lamar,LAMA\n\
,TX,Lamb,LAMB\n\
,TX,Lampasas,LAMP\n\
,TX,La Salle,LSAL\n\
,TX,Lavaca,LAVA\n\
,TX,Lee,LEE\n\
,TX,Leon,LEON\n\
,TX,Liberty,LIBE\n\
,TX,Limestone,LIME\n\
,TX,Lipscomb,LIPS\n\
,TX,Live Oak,LIVO\n\
,TX,Llano,LLAN\n\
,TX,Loving,LOVI\n\
,TX,Lubbock,LUBB\n\
,TX,Lynn,LYNN\n\
,TX,Madison,MADI\n\
,TX,Marion,MARI\n\
,TX,Martin,MART\n\
,TX,Mason,MASO\n\
,TX,Matagorda,MATA\n\
,TX,Maverick,MAVE\n\
,TX,McCulloch,MCUL\n\
,TX,McLennan,MLEN\n\
,TX,McMullen,MMUL\n\
,TX,Medina,MEDI\n\
,TX,Menard,MENA\n\
,TX,Midland,MIDL\n\
,TX,Milam,MILA\n\
,TX,Mills,MILL\n\
,TX,Mitchell,MITC\n\
,TX,Montague,MONT\n\
,TX,Montgomery,MGMY\n\
,TX,Moore,MOOR\n\
,TX,Morris,MORR\n\
,TX,Motley,MOTL\n\
,TX,Nacogdoches,NACO\n\
,TX,Navarro,NAVA\n\
,TX,Newton,NEWT\n\
,TX,Nolan,NOLA\n\
,TX,Nueces,NUEC\n\
,TX,Ochiltree,OCHI\n\
,TX,Oldham,OLDH\n\
,TX,Orange,ORAN\n\
,TX,Palo Pinto,PPIN\n\
,TX,Panola,PANO\n\
,TX,Parker,PARK\n\
,TX,Parmer,PARM\n\
,TX,Pecos,PECO\n\
,TX,Polk,POLK\n\
,TX,Potter,POTT\n\
,TX,Presidio,PRES\n\
,TX,Rains,RAIN\n\
,TX,Randall,RAND\n\
,TX,Reagan,REAG\n\
,TX,Real,REAL\n\
,TX,Red River,RRIV\n\
,TX,Reeves,REEV\n\
,TX,Refugio,REFU\n\
,TX,Roberts,ROBE\n\
,TX,Robertson,RBSN\n\
,TX,Rockwall,ROCK\n\
,TX,Runnels,RUNN\n\
,TX,Rusk,RUSK\n\
,TX,Sabine,SABI\n\
,TX,San Augustine,SAUG\n\
,TX,San Jacinto,SJAC\n\
,TX,San Patricio,SPAT\n\
,TX,San Saba,SSAB\n\
,TX,Schleicher,SCHL\n\
,TX,Scurry,SCUR\n\
,TX,Shackelford,SHAC\n\
,TX,Shelby,SHEL\n\
,TX,Sherman,SHMN\n\
,TX,Smith,SMIT\n\
,TX,Somervell,SOME\n\
,TX,Starr,STAR\n\
,TX,Stephens,STEP\n\
,TX,Sterling,STER\n\
,TX,Stonewall,STON\n\
,TX,Sutton,SUTT\n\
,TX,Swisher,SWIS\n\
,TX,Tarrant,TARR\n\
,TX,Taylor,TAYL\n\
,TX,Terrell,TERL\n\
,TX,Terry,TERY\n\
,TX,Throckmorton,THRO\n\
,TX,Titus,TITU\n\
,TX,Tom Green,TGRE\n\
,TX,Travis,TRAV\n\
,TX,Trinity,TRIN\n\
,TX,Tyler,TYLE\n\
,TX,Upshur,UPSH\n\
,TX,Upton,UPTO\n\
,TX,Uvalde,UVAL\n\
,TX,Val Verde,VVER\n\
,TX,Van Zandt,VZAN\n\
,TX,Victoria,VICT\n\
,TX,Walker,WALK\n\
,TX,Waller,WALL\n\
,TX,Ward,WARD\n\
,TX,Washington,WASH\n\
,TX,Webb,WEBB\n\
,TX,Wharton,WHAR\n\
,TX,Wheeler,WHEE\n\
,TX,Wichita,WICH\n\
,TX,Wilbarger,WILB\n\
,TX,Willacy,WILY\n\
,TX,Williamson,WMSN\n\
,TX,Wilson,WLSN\n\
,TX,Winkler,WINK\n\
,TX,Wise,WISE\n\
,TX,Wood,WOOD\n\
,TX,Yoakum,YOAK\n\
,TX,Young,YOUN\n\
,TX,Zapata,ZAPA\n\
,TX,Zavala,ZAVA\n\
Midway,UM,Midway Islands,\n\
Utah,UT,Beaver,BEA\n\
,UT,Box Elder,BOX\n\
,UT,Cache,CAC\n\
,UT,Carbon,CAR\n\
,UT,Daggett,DAG\n\
,UT,Davis,DAV\n\
,UT,Duchesne,DUC\n\
,UT,Emery,EME\n\
,UT,Garfield,GAR\n\
,UT,Grand,GRA\n\
,UT,Iron,IRO\n\
,UT,Juab,JUA\n\
,UT,Kane,KAN\n\
,UT,Millard,MIL\n\
,UT,Morgan,MOR\n\
,UT,Piute,PIU\n\
,UT,Rich,RIC\n\
,UT,Salt Lake,SAL\n\
,UT,San Juan,SNJ\n\
,UT,Sanpete,SNP\n\
,UT,Sevier,SEV\n\
,UT,Summit,SUM\n\
,UT,Tooele,TOO\n\
,UT,Uintah,UIN\n\
,UT,Utah,UTA\n\
,UT,Wasatch,WST\n\
,UT,Washington,WSH\n\
,UT,Wayne,WAY\n\
,UT,Weber,WEB\n\
Vermont,VT,Addison,ADD\n\
,VT,Bennington,BEN\n\
,VT,Caledonia,CAL\n\
,VT,Chittenden,CHI\n\
,VT,Essex,ESS\n\
,VT,Franklin,FRA\n\
,VT,Grand Isle,GRA\n\
,VT,Lamoille,LAM\n\
,VT,Orange,ORA\n\
,VT,Orleans,ORL\n\
,VT,Rutland,RUT\n\
,VT,Washington,WAS\n\
,VT,Windham,WNH\n\
,VT,Windsor,WNS\n\
Virginia,VA,Accomack,ACC\n\
,VA,Albemarle,ALB\n\
,VA,Alexandria,ALX\n\
,VA,Alleghany,ALL\n\
,VA,Amelia,AME\n\
,VA,Amherst,AMH\n\
,VA,Appomattox,APP\n\
,VA,Arlington,ARL\n\
,VA,Augusta,AUG\n\
,VA,Bath,BAT\n\
,VA,Bedford,BED\n\
,VA,Bland,BLA\n\
,VA,Botetourt,BOT\n\
,VA,Bristol,BRX\n\
,VA,Brunswick,BRU\n\
,VA,Buchanan,BCH\n\
,VA,Buckingham,BHM\n\
,VA,Buena Vista,BVX\n\
,VA,Campbell,CAM\n\
,VA,Caroline,CLN\n\
,VA,Carroll,CRL\n\
,VA,Charles City,CCY\n\
,VA,Charlotte,CHA\n\
,VA,Charlottesville,CHX\n\
,VA,Chesapeake,CPX\n\
,VA,Chesterfield,CHE\n\
,VA,Clarke,CLA\n\
,VA,Colonial Heights,COX\n\
,VA,Covington,CVX\n\
,VA,Craig,CRA\n\
,VA,Culpeper,CUL\n\
,VA,Cumberland,CUM\n\
,VA,Danville,DAX\n\
,VA,Dickenson,DIC\n\
,VA,Dinwiddie,DIN\n\
,VA,Emporia,EMX\n\
,VA,Essex,ESS\n\
,VA,Fairfax,FFX\n\
,VA,Fairfax City,FXX\n\
,VA,Falls Church,FCX\n\
,VA,Fauquier,FAU\n\
,VA,Floyd,FLO\n\
,VA,Fluvanna,FLU\n\
,VA,Franklin,FRA\n\
,VA,Franklin City,FRX\n\
,VA,Frederick,FRE\n\
,VA,Fredericksburg,FBX\n\
,VA,Galax,GAX\n\
,VA,Giles,GIL\n\
,VA,Gloucester,GLO\n\
,VA,Goochland,GOO\n\
,VA,Grayson,GRA\n\
,VA,Greene,GRN\n\
,VA,Greensville,GVL\n\
,VA,Halifax,HAL\n\
,VA,Hampton,HAX\n\
,VA,Hanover,HAN\n\
,VA,Harrisonburg,HBX\n\
,VA,Henrico,HCO\n\
,VA,Henry,HRY\n\
,VA,Highland,HIG\n\
,VA,Hopewell,HOX\n\
,VA,Isle of Wight,IOW\n\
,VA,James City,JAM\n\
,VA,King and Queen,KQN\n\
,VA,King George,KGE\n\
,VA,King William,KWM\n\
,VA,Lancaster,LAN\n\
,VA,Lee,LEE\n\
,VA,Lexington,LEX\n\
,VA,Loudoun,LDN\n\
,VA,Louisa,LSA\n\
,VA,Lunenburg,LUN\n\
,VA,Lynchburg,LYX\n\
,VA,Madison,MAD\n\
,VA,Manassas,MAX\n\
,VA,Manassas Park,MPX\n\
,VA,Martinsville,MVX\n\
,VA,Mathews,MAT\n\
,VA,Mecklenburg,MEC\n\
,VA,Middlesex,MID\n\
,VA,Montgomery,MON\n\
,VA,Nelson,NEL\n\
,VA,New Kent,NEW\n\
,VA,Newport News,NNX\n\
,VA,Norfolk,NFX\n\
,VA,Northampton,NHA\n\
,VA,Northumberland,NUM\n\
,VA,Norton,NRX\n\
,VA,Nottoway,NOT\n\
,VA,Orange,ORG\n\
,VA,Page,PAG\n\
,VA,Patrick,PAT\n\
,VA,Petersburg,PBX\n\
,VA,Pittsylvania,PIT\n\
,VA,Poquoson,PQX\n\
,VA,Portsmouth,POX\n\
,VA,Powhatan,POW\n\
,VA,Prince Edward,PRE\n\
,VA,Prince George,PRG\n\
,VA,Prince William,PRW\n\
,VA,Pulaski,PUL\n\
,VA,Radford,RAX\n\
,VA,Rappahannock,RAP\n\
,VA,Richmond,RIC\n\
,VA,Richmond City,RIX\n\
,VA,Roanoke,ROA\n\
,VA,Roanoke City,ROX\n\
,VA,Rockbridge,RBR\n\
,VA,Rockingham,RHM\n\
,VA,Russell,RUS\n\
,VA,Salem,SAX\n\
,VA,Scott,SCO\n\
,VA,Shenandoah,SHE\n\
,VA,Smyth,SMY\n\
,VA,Southampton,SHA\n\
,VA,Spotsylvania,SPO\n\
,VA,Stafford,STA\n\
,VA,Staunton,STX\n\
,VA,Suffolk,SUX\n\
,VA,Surry,SUR\n\
,VA,Sussex,SUS\n\
,VA,Tazewell,TAZ\n\
,VA,Virginia Beach,VBX\n\
,VA,Warren,WAR\n\
,VA,Washington,WAS\n\
,VA,Waynesboro,WAX\n\
,VA,Westmoreland,WES\n\
,VA,Williamsburg,WMX\n\
,VA,Winchester,WIX\n\
,VA,Wise,WIS\n\
,VA,Wythe,WYT\n\
,VA,York,YOR\n\
U.S. Virgin Islands,VI,St. Croix Island,\n\
,VI,St. John Island,\n\
,VI,St. Thomas Island,\n\
Washington,WA,Adams,ADA\n\
,WA,Asotin,ASO\n\
,WA,Benton,BEN\n\
,WA,Chelan,CHE\n\
,WA,Clallam,CLAL\n\
,WA,Clark,CLAR\n\
,WA,Columbia,COL\n\
,WA,Cowlitz,COW\n\
,WA,Douglas,DOU\n\
,WA,Ferry,FER\n\
,WA,Franklin,FRA\n\
,WA,Garfield,GAR\n\
,WA,Grant,GRAN\n\
,WA,Grays Harbor,GRAY\n\
,WA,Island,ISL\n\
,WA,Jefferson,JEFF\n\
,WA,King,KING\n\
,WA,Kitsap,KITS\n\
,WA,Kittitas,KITT\n\
,WA,Klickitat,KLI\n\
,WA,Lewis,LEW\n\
,WA,Lincoln,LIN\n\
,WA,Mason,MAS\n\
,WA,Okanogan,OKA\n\
,WA,Pacific,PAC\n\
,WA,Pend Oreille,PEND\n\
,WA,Pierce,PIE\n\
,WA,San Juan,SAN\n\
,WA,Skagit,SKAG\n\
,WA,Skamania,SKAM\n\
,WA,Snohomish,SNO\n\
,WA,Spokane,SPO\n\
,WA,Stevens,STE\n\
,WA,Thurston,THU\n\
,WA,Wahkiakum,WAH\n\
,WA,Walla Walla,WAL\n\
,WA,Whatcom,WHA\n\
,WA,Whitman,WHI\n\
,WA,Yakima,YAK\n\
Wisconsin,WI,Adams,ADA\n\
,WI,Ashland,ASH\n\
,WI,Barron,BAR\n\
,WI,Bayfield,BAY\n\
,WI,Brown,BRO\n\
,WI,Buffalo,BUF\n\
,WI,Burnett,BUR\n\
,WI,Calumet,CAL\n\
,WI,Chippewa,CHI\n\
,WI,Clark,CLA\n\
,WI,Columbia,COL\n\
,WI,Crawford,CRA\n\
,WI,Dane,DAN\n\
,WI,Dodge,DOD\n\
,WI,Door,DOO\n\
,WI,Douglas,DOU\n\
,WI,Dunn,DUN\n\
,WI,Eau Claire,EAU\n\
,WI,Florence,FLO\n\
,WI,Fond du Lac,FON\n\
,WI,Forest,FOR\n\
,WI,Grant,GRA\n\
,WI,Green,GRE\n\
,WI,Green Lake,GRL\n\
,WI,Iowa,IOW\n\
,WI,Iron,IRO\n\
,WI,Jackson,JAC\n\
,WI,Jefferson,JEF\n\
,WI,Juneau,JUN\n\
,WI,Kenosha,KEN\n\
,WI,Kewaunee,KEW\n\
,WI,La Crosse,LAC\n\
,WI,Lafayette,LAF\n\
,WI,Langlade,LAN\n\
,WI,Lincoln,LIN\n\
,WI,Manitowoc,MAN\n\
,WI,Marathon,MAR\n\
,WI,Marinette,MRN\n\
,WI,Marquette,MRQ\n\
,WI,Menominee,MEN\n\
,WI,Milwaukee,MIL\n\
,WI,Monroe,MON\n\
,WI,Oconto,OCO\n\
,WI,Oneida,ONE\n\
,WI,Outagamie,OUT\n\
,WI,Ozaukee,OZA\n\
,WI,Pepin,PEP\n\
,WI,Pierce,PIE\n\
,WI,Polk,POL\n\
,WI,Portage,POR\n\
,WI,Price,PRI\n\
,WI,Racine,RAC\n\
,WI,Richland,RIC\n\
,WI,Rock,ROC\n\
,WI,Rusk,RUS\n\
,WI,Sauk,SAU\n\
,WI,Sawyer,SAW\n\
,WI,Shawano,SHA\n\
,WI,Sheboygan,SHE\n\
,WI,St. Croix,STC\n\
,WI,Taylor,TAY\n\
,WI,Trempealeau,TRE\n\
,WI,Vernon,VER\n\
,WI,Vilas,VIL\n\
,WI,Walworth,WAL\n\
,WI,Washburn,WSB\n\
,WI,Washington,WAS\n\
,WI,Waukesha,WAU\n\
,WI,Waupaca,WAP\n\
,WI,Waushara,WSR\n\
,WI,Winnebago,WIN\n\
,WI,Wood,WOO\n\
West Virginia,WV,Barbour,BAR\n\
,WV,Berkeley,BER\n\
,WV,Boone,BOO\n\
,WV,Braxton,BRA\n\
,WV,Brooke,BRO\n\
,WV,Cabell,CAB\n\
,WV,Calhoun,CAL\n\
,WV,Clay,CLA\n\
,WV,Doddridge,DOD\n\
,WV,Fayette,FAY\n\
,WV,Gilmer,GIL\n\
,WV,Grant,GRA\n\
,WV,Greenbrier,GRE\n\
,WV,Hampshire,HAM\n\
,WV,Hancock,HAN\n\
,WV,Hardy,HDY\n\
,WV,Harrison,HAR\n\
,WV,Jackson,JAC\n\
,WV,Jefferson,JEF\n\
,WV,Kanawha,KAN\n\
,WV,Lewis,LEW\n\
,WV,Lincoln,LIN\n\
,WV,Logan,LOG\n\
,WV,Marion,MRN\n\
,WV,Marshall,MAR\n\
,WV,Mason,MAS\n\
,WV,McDowell,MCD\n\
,WV,Mercer,MER\n\
,WV,Mineral,MIN\n\
,WV,Mingo,MGO\n\
,WV,Monongalia,MON\n\
,WV,Monroe,MRO\n\
,WV,Morgan,MOR\n\
,WV,Nicholas,NIC\n\
,WV,Ohio,OHI\n\
,WV,Pendleton,PEN\n\
,WV,Pleasants,PLE\n\
,WV,Pocahontas,POC\n\
,WV,Preston,PRE\n\
,WV,Putnam,PUT\n\
,WV,Raleigh,RAL\n\
,WV,Randolph,RAN\n\
,WV,Ritchie,RIT\n\
,WV,Roane,ROA\n\
,WV,Summers,SUM\n\
,WV,Taylor,TAY\n\
,WV,Tucker,TUC\n\
,WV,Tyler,TYL\n\
,WV,Upshur,UPS\n\
,WV,Wayne,WAY\n\
,WV,Webster,WEB\n\
,WV,Wetzel,WET\n\
,WV,Wirt,WIR\n\
,WV,Wood,WOO\n\
,WV,Wyoming,WYO\n\
Wyoming,WY,Albany,ALB\n\
,WY,Big Horn,BIG\n\
,WY,Campbell,CAM\n\
,WY,Carbon,CAR\n\
,WY,Converse,CON\n\
,WY,Crook,CRO\n\
,WY,Fremont,FRE\n\
,WY,Goshen,GOS\n\
,WY,Hot Springs,HOT\n\
,WY,Johnson,JOH\n\
,WY,Laramie,LAR\n\
,WY,Lincoln,LIN\n\
,WY,Natrona,NAT\n\
,WY,Niobrara,NIO\n\
,WY,Park,PAR\n\
,WY,Platte,PLA\n\
,WY,Sheridan,SHE\n\
,WY,Sublette,SUB\n\
,WY,Sweetwater,SWE\n\
,WY,Teton,TET\n\
,WY,Uinta,UIN\n\
,WY,Washakie,WAS\n\
,WY,Weston,WES\n\
Yukon,YT,Carmacks,\n\
,YT,Dawson,\n\
,YT,Faro,\n\
,YT,Haines Junction,\n\
,YT,Mayo,\n\
,YT,Teslin,\n\
,YT,Watson Lake,\n\
,YT,Whitehorse,\n";

//std::string strNEQP
const char *szNEQP = "\
State, ST, County/City, CC\n\
Connecticut,CT,Fairfield,FAICT\n\
,CT,Hartford,HARCT\n\
,CT,Litchfield,LITCT\n\
,CT,Middlesex,MIDCT\n\
,CT,New Haven,NHVCT\n\
,CT,New London,NLNCT\n\
,CT,Tolland,TOLCT\n\
,CT,Windham,WINCT\n\
Massechusetts,MA,Barnstable,BARMA\n\
,MA,Berkshire,BERMA\n\
,MA,Bristol,BRIMA\n\
,MA,Dukes,DUKMA\n\
,MA,Essex,ESSMA\n\
,MA,Franklin,FRAMA\n\
,MA,Hampden,HMDMA\n\
,MA,Hampshire,HMPMA\n\
,MA,Middlesex,MIDMA\n\
,MA,Nantucket,NANMA\n\
,MA,Norfolk,NORMA\n\
,MA,Plymouth,PLYMA\n\
,MA,Suffolk,SUFMA\n\
,MA,Worcester,WORMA\n\
Maine,ME,Androscoggin,ANDME\n\
,ME,Aroostook,AROME\n\
,ME,Cumberland,CUMME\n\
,ME,Franklin,FRAME\n\
,ME,Hancock,HANME\n\
,ME,Kennebec,KENME\n\
,ME,Knox,KNOME\n\
,ME,Lincoln,LINME\n\
,ME,Oxford,OXFME\n\
,ME,Penobscot,PENME\n\
,ME,Piscataquis,PISME\n\
,ME,Sagadahoc,SAGME\n\
,ME,Somerset,SOMME\n\
,ME,Waldo,WALME\n\
,ME,Washington,WASME\n\
,ME,York,YORME\n\
New Hampshire,NH,Belknap,BELNH\n\
,NH,Carroll,CARNH\n\
,NH,Cheshire,CHENH\n\
,NH,Coos,COONH\n\
,NH,Grafton,GRANH\n\
,NH,Hillsborough,HILNH\n\
,NH,Merrimack,MERNH\n\
,NH,Rockingham,ROCNH\n\
,NH,Strafford,STRNH\n\
,NH,Sullivan,SULNH\n\
Rhode Island,RI,Bristol,BRIRI\n\
,RI,Kent,KENRI\n\
,RI,Newport,NEWRI\n\
,RI,Providence,PRORI\n\
,RI,Washington,WASRI\n\
Vermont,VT,Addison,ADDVT\n\
,VT,Bennington,BENVT\n\
,VT,Caledonia,CALVT\n\
,VT,Chittenden,CHIVT\n\
,VT,Essex,ESSVT\n\
,VT,Franklin,FRAVT\n\
,VT,Grand Isle,GRAVT\n\
,VT,Lamoille,LAMVT\n\
,VT,Orange,ORAVT\n\
,VT,Orleans,ORLVT\n\
,VT,Rutland,RUTVT\n\
,VT,Washington,WASVT\n\
,VT,Windham,WNHVT\n\
,VT,Windsor,WNDVT\n";

//std::string str7QP 
const char *sz7QP = "\
State, ST, County/City, CC\n\
Arizona,AZ,Apache,AZAPH\n\
,AZ,Cochise,AZCHS\n\
,AZ,Coconino,AZCNO\n\
,AZ,Gila,AZGLA\n\
,AZ,Graham,AZGHM\n\
,AZ,Greenlee,AZGLE\n\
,AZ,La Paz,AZLPZ\n\
,AZ,Maricopa,AZMCP\n\
,AZ,Mohave,AZMHV\n\
,AZ,Navajo,AZNVO\n\
,AZ,Pima,AZPMA\n\
,AZ,Pinal,AZPNL\n\
,AZ,Santa Cruz,AZSCZ\n\
,AZ,Yavapai,AZYVP\n\
,AZ,Yuma,AZYMA\n\
Idaho,ID,Ada,IDADA\n\
,ID,Adams,IDADM\n\
,ID,Bannock,IDBAN\n\
,ID,Bear Lake,IDBEA\n\
,ID,Benewah,IDBEN\n\
,ID,Bingham,IDBIN\n\
,ID,Blaine,IDBLA\n\
,ID,Boise,IDBOI\n\
,ID,Bonner,IDBNR\n\
,ID,Bonneville,IDBNV\n\
,ID,Boundary,IDBOU\n\
,ID,Butte,IDBUT\n\
,ID,Camas,IDCAM\n\
,ID,Canyon,IDCAN\n\
,ID,Caribou,IDCAR\n\
,ID,Cassia,IDCAS\n\
,ID,Clark,IDCLA\n\
,ID,Clearwater,IDCLE\n\
,ID,Custer,IDCUS\n\
,ID,Elmore,IDELM\n\
,ID,Franklin,IDFRA\n\
,ID,Fremont,IDFRE\n\
,ID,Gem,IDGEM\n\
,ID,Gooding,IDGOO\n\
,ID,Idaho,IDIDA\n\
,ID,Jefferson,IDJEF\n\
,ID,Jerome,IDJER\n\
,ID,Kootenai,IDKOO\n\
,ID,Latah,IDLAT\n\
,ID,Lemhi,IDLEM\n\
,ID,Lewis,IDLEW\n\
,ID,Lincoln,IDLIN\n\
,ID,Madison,IDMAD\n\
,ID,Minidoka,IDMIN\n\
,ID,Nez Perce,IDNEZ\n\
,ID,Oneida,IDONE\n\
,ID,Owyhee,IDOWY\n\
,ID,Payette,IDPAY\n\
,ID,Power,IDPOW\n\
,ID,Shoshone,IDSHO\n\
,ID,Teton,IDTET\n\
,ID,Twin Falls,IDTWI\n\
,ID,Valley,IDVAL\n\
,ID,Washington,IDWAS\n\
Montana,MT,Beaverhead,MTBEA\n\
,MT,Big Horn,MTBIG\n\
,MT,Blaine,MTBLA\n\
,MT,Broadwater,MTBRO\n\
,MT,Carbon,MTCRB\n\
,MT,Carter,MTCRT\n\
,MT,Cascade,MTCAS\n\
,MT,Chouteau,MTCHO\n\
,MT,Custer,MTCUS\n\
,MT,Daniels,MTDAN\n\
,MT,Dawson,MTDAW\n\
,MT,Deer Lodge,MTDEE\n\
,MT,Fallon,MTFAL\n\
,MT,Fergus,MTFER\n\
,MT,Flathead,MTFLA\n\
,MT,Gallatin,MTGAL\n\
,MT,Garfield,MTGAR\n\
,MT,Glacier,MTGLA\n\
,MT,Golden Valley,MTGOL\n\
,MT,Granite,MTGRA\n\
,MT,Hill,MTHIL\n\
,MT,Jefferson,MTJEF\n\
,MT,Judith Basin,MTJUD\n\
,MT,Lake,MTLAK\n\
,MT,Lewis & Clark,MTLEW\n\
,MT,Liberty,MTLIB\n\
,MT,Lincoln,MTLIN\n\
,MT,Madison,MTMAD\n\
,MT,McCone,MTMCC\n\
,MT,Meagher,MTMEA\n\
,MT,Mineral,MTMIN\n\
,MT,Missoula,MTMIS\n\
,MT,Musselshell,MTMUS\n\
,MT,Park,MTPAR\n\
,MT,Petroleum,MTPET\n\
,MT,Phillips,MTPHI\n\
,MT,Pondera,MTPON\n\
,MT,Powder River,MTPWD\n\
,MT,Powell,MTPWL\n\
,MT,Prairie,MTPRA\n\
,MT,Ravalli,MTRAV\n\
,MT,Richland,MTRIC\n\
,MT,Roosevelt,MTROO\n\
,MT,Rosebud,MTROS\n\
,MT,Sanders,MTSAN\n\
,MT,Sheridan,MTSHE\n\
,MT,Silver Bow,MTSIL\n\
,MT,Stillwater,MTSTI\n\
,MT,Sweet Grass,MTSWE\n\
,MT,Teton,MTTET\n\
,MT,Toole,MTTOO\n\
,MT,Treasure,MTTRE\n\
,MT,Valley,MTVAL\n\
,MT,Wheatland,MTWHE\n\
,MT,Wibaux,MTWIB\n\
,MT,Yellowstone,MTYEL\n\
Nevada,NV,Carson City,NVCAR\n\
,NV,Churchill,NVCHU\n\
,NV,Clark,NVCLA\n\
,NV,Douglas,NVDOU\n\
,NV,Elko,NVELK\n\
,NV,Esmeralda,NVESM\n\
,NV,Eureka,NVEUR\n\
,NV,Humboldt,NVHUM\n\
,NV,Lander,NVLAN\n\
,NV,Lincoln,NVLIN\n\
,NV,Lyon,NVLYO\n\
,NV,Mineral,NVMIN\n\
,NV,Nye,NVNYE\n\
,NV,Pershing,NVPER\n\
,NV,Storey,NVSTO\n\
,NV,Washoe,NVWAS\n\
,NV,White Pine,NVWHI\n\
Oregon,OR,Baker,ORBAK\n\
,OR,Benton,ORBEN\n\
,OR,Clackamas,ORCLK\n\
,OR,Clatsop,ORCLT\n\
,OR,Columbia,ORCOL\n\
,OR,Coos,ORCOO\n\
,OR,Crook,ORCRO\n\
,OR,Curry,ORCUR\n\
,OR,Deschutes,ORDES\n\
,OR,Douglas,ORDOU\n\
,OR,Gilliam,ORGIL\n\
,OR,Grant,ORGRA\n\
,OR,Harney,ORHAR\n\
,OR,Hood River,ORHOO\n\
,OR,Jackson,ORJAC\n\
,OR,Jefferson,ORJEF\n\
,OR,Josephine,ORJOS\n\
,OR,Klamath,ORKLA\n\
,OR,Lake,ORLAK\n\
,OR,Lane,ORLAN\n\
,OR,Lincoln,ORLCN\n\
,OR,Linn,ORLNN\n\
,OR,Malheur,ORMAL\n\
,OR,Marion,ORMAR\n\
,OR,Morrow,ORMOR\n\
,OR,Multnomah,ORMUL\n\
,OR,Polk,ORPOL\n\
,OR,Sherman,ORSHE\n\
,OR,Tillamook,ORTIL\n\
,OR,Umatilla,ORUMA\n\
,OR,Union,ORUNI\n\
,OR,Wallowa,ORWAL\n\
,OR,Wasco,ORWCO\n\
,OR,Washington,ORWSH\n\
,OR,Wheeler,ORWHE\n\
,OR,Yamhill,ORYAM\n\
Utah,UT,Beaver,UTBEA\n\
,UT,Box Elder,UTBOX\n\
,UT,Cache,UTCAC\n\
,UT,Carbon,UTCAR\n\
,UT,Daggett,UTDAG\n\
,UT,Davis,UTDAV\n\
,UT,Duchesne,UTDUC\n\
,UT,Emery,UTEME\n\
,UT,Garfield,UTGAR\n\
,UT,Grand,UTGRA\n\
,UT,Iron,UTIRO\n\
,UT,Juab,UTJUA\n\
,UT,Kane,UTKAN\n\
,UT,Millard,UTMIL\n\
,UT,Morgan,UTMOR\n\
,UT,Piute,UTPIU\n\
,UT,Rich,UTRIC\n\
,UT,Salt Lake,UTSAL\n\
,UT,San Juan,UTSNJ\n\
,UT,Sanpete,UTSNP\n\
,UT,Sevier,UTSEV\n\
,UT,Summit,UTSUM\n\
,UT,Tooele,UTTOO\n\
,UT,Uintah,UTUIN\n\
,UT,Utah,UTUTA\n\
,UT,Wasatch,UTWST\n\
,UT,Washington,UTWSH\n\
,UT,Wayne,UTWAY\n\
,UT,Weber,UTWEB\n\
Washington,WA,Adams,WAADA\n\
,WA,Asotin,WAASO\n\
,WA,Benton,WABEN\n\
,WA,Chelan,WACHE\n\
,WA,Clallam,WACLL\n\
,WA,Clark,WACLR\n\
,WA,Columbia,WACOL\n\
,WA,Cowlitz,WACOW\n\
,WA,Douglas,WADOU\n\
,WA,Ferry,WAFER\n\
,WA,Franklin,WAFRA\n\
,WA,Garfield,WAGAR\n\
,WA,Grant,WAGRN\n\
,WA,Grays Harbor,WAGRY\n\
,WA,Island,WAISL\n\
,WA,Jefferson,WAJEF\n\
,WA,Klickitat,WAKLI\n\
,WA,King,WAKNG\n\
,WA,Kitsap,WAKTP\n\
,WA,Kittitas,WAKTT\n\
,WA,Lewis,WALEW\n\
,WA,Lincoln,WALIN\n\
,WA,Mason,WAMAS\n\
,WA,Okanogan,WAOKA\n\
,WA,Pacific,WAPAC\n\
,WA,Pend Oreille,WAPEN\n\
,WA,Pierce,WAPIE\n\
,WA,San Juan,WASAN\n\
,WA,Skagit,WASKG\n\
,WA,Skamania,WASKM\n\
,WA,Snohomish,WASNO\n\
,WA,Spokane,WASPO\n\
,WA,Stevens,WASTE\n\
,WA,Thurston,WATHU\n\
,WA,Wahkiakum,WAWAH\n\
,WA,Walla Walla,WAWAL\n\
,WA,Whatcom,WAWHA\n\
,WA,Whitman,WAWHI\n\
,WA,Yakima,WAYAK\n\
Wyoming,WY,Albany,WYALB\n\
,WY,Big Horn,WYBIG\n\
,WY,Campbell,WYCAM\n\
,WY,Carbon,WYCAR\n\
,WY,Converse,WYCON\n\
,WY,Crook,WYCRO\n\
,WY,Fremont,WYFRE\n\
,WY,Goshen,WYGOS\n\
,WY,Hot Springs,WYHOT\n\
,WY,Johnson,WYJOH\n\
,WY,Laramie,WYLAR\n\
,WY,Lincoln,WYLIN\n\
,WY,Natrona,WYNAT\n\
,WY,Niobrara,WYNIO\n\
,WY,Park,WYPAR\n\
,WY,Platte,WYPLA\n\
,WY,Sheridan,WYSHE\n\
,WY,Sublette,WYSUB\n\
,WY,Sweetwater,WYSWE\n\
,WY,Teton,WYTET\n\
,WY,Uinta,WYUIN\n\
,WY,Washakie,WYWAS\n\
,WY,Weston,WYWES\n";

