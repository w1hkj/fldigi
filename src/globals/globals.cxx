// ----------------------------------------------------------------------------
// globals.cxx  --  constants, variables, arrays & functions that need to be
//                  outside of any thread
//
// Copyright (C) 2006-2009
//		Dave Freese, W1HKJ
// Copyright (C) 2007-2009
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

#include <config.h>
#include <iosfwd>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cerrno>
#include <cstdio>

#include "gettext.h"
#include "globals.h"
#include "modem.h"
#include "data_io.h"
#include "strutil.h"

using namespace std;

// ************ Elements are in enum trx_mode order. ************
// N.B. it is not valid to use an _("NLS") string in this table!!
// ... doing so will break the Fl_menu_item table 'menu_'.  -Kamal

// Last value (true/false) determines if it's used with the KISS interface.
// It must have 8 bit support. Current selection based on the modems used in FLAMP.

//struc entries:
// mode, **modem, *sname, *name, *pskmail_name, *adif_name, *export_mode, *export_submode, *vid_name, iface_io
// *** DO NOT PUT A '/' in the name string

const struct mode_info_t mode_info[NUM_MODES] = {
{MODE_NULL,&null_modem,"NULL","NULL","","NULL","","","", DISABLED_IO },

{MODE_CW,&cw_modem,"CW","CW","CW","CW","CW","","CW", DISABLED_IO },

{MODE_CONTESTIA,&contestia_modem,"CONTESTIA","Contestia","","CONTESTI","CONTESTI","","CT",DISABLED_IO},

{MODE_CONTESTIA_4_125,&contestia_4_125_modem,"Cont-4/125","Cont4-125","","CONTESTI4/125","CONTESTI","","CT4/125",DISABLED_IO},
{MODE_CONTESTIA_4_250,&contestia_4_250_modem,"Cont-4/250","Cont4-250","","CONTESTI4/250","CONTESTI","","CT4/250",DISABLED_IO},
{MODE_CONTESTIA_4_500,&contestia_4_500_modem,"Cont-4/500","Cont4-500","","CONTESTI4/500","CONTESTI","","CT4/500",DISABLED_IO},
{MODE_CONTESTIA_4_1000,&contestia_4_1000_modem,"Cont-4/1K","Cont4-1K","","CONTESTI4/1000","CONTESTI","","CT4/1K",DISABLED_IO},
{MODE_CONTESTIA_4_2000,&contestia_4_2000_modem,"Cont-4/2K","Cont4-2K","","CONTESTI4/2000","CONTESTI","","CT4/2K",DISABLED_IO},

{MODE_CONTESTIA_8_125,&contestia_8_125_modem,"Cont-8/125","Cont8-125","","CONTESTI8/125","CONTESTI","","CT8/125",DISABLED_IO},
{MODE_CONTESTIA_8_250,&contestia_8_250_modem,"Cont-8/250","Cont8-250","","CONTESTI8/250","CONTESTI","","CT8/250",DISABLED_IO},
{MODE_CONTESTIA_8_500,&contestia_8_500_modem,"Cont-8/500","Cont8-500","","CONTESTI8/500","CONTESTI","","CT8/500",DISABLED_IO},
{MODE_CONTESTIA_8_1000,&contestia_8_1000_modem,"Cont-8/1K","Cont8-1K","","CONTESTI8/1000","CONTESTI","","CT8/1K",DISABLED_IO},
{MODE_CONTESTIA_8_2000,&contestia_8_2000_modem,"Cont-8/2K","Cont8-2K","","CONTESTI8/2000","CONTESTI","","CT8/2K",DISABLED_IO},

{MODE_CONTESTIA_16_250,&contestia_16_250_modem,"Cont-16/250","Cont16-250","","CONTESTI16/250","CONTESTI","","CT16/250",DISABLED_IO},
{MODE_CONTESTIA_16_500,&contestia_16_500_modem,"Cont-16/500","Cont16-500","","CONTESTI16/500","CONTESTI","","CT16/500",DISABLED_IO},
{MODE_CONTESTIA_16_1000,&contestia_16_1000_modem,"Cont-16/1K","Cont16-1K","","CONTESTI16/1000","CONTESTI","","CT16/1K",DISABLED_IO},
{MODE_CONTESTIA_16_2000,&contestia_16_2000_modem,"Cont-16/2K","Cont16-2K","","CONTESTI16/2000","CONTESTI","","CT16/2K",DISABLED_IO},

{MODE_CONTESTIA_32_1000,&contestia_32_1000_modem,"Cont-32/1K","Cont32-1K","","CONTESTI32/1000","CONTESTI","","CT32/1K",DISABLED_IO},
{MODE_CONTESTIA_32_2000,&contestia_32_2000_modem,"Cont-32/2K","Cont32-2K","","CONTESTI32/2000","CONTESTI","","CT32/2K",DISABLED_IO},

{MODE_CONTESTIA_64_500,&contestia_64_500_modem,"Cont-64/500","Cont64-500","","CONTESTI64/500","CONTESTI","","CT64/500",DISABLED_IO},
{MODE_CONTESTIA_64_1000,&contestia_64_1000_modem,"Cont-64/1K","Cont64-1K","","CONTESTI64/1000","CONTESTI","","CT64/1K",DISABLED_IO},
{MODE_CONTESTIA_64_2000,&contestia_64_2000_modem,"Cont-64/2K","Cont64-2K","","CONTESTI64/2000","CONTESTI","","CT64/2K",DISABLED_IO},

{MODE_DOMINOEXMICRO,&dominoexmicro_modem,"DOMEX Micro","DominoEX Micro","DOMINOEXMICRO","DOMINO","DOMINO","","DM M", DISABLED_IO },
{MODE_DOMINOEX4,&dominoex4_modem,"DOMEX4","DominoEX 4","DOMINOEX4","DOMINO","DOMINO","","DM 4", DISABLED_IO },
{MODE_DOMINOEX5,&dominoex5_modem,"DOMEX5","DominoEX 5","DOMINOEX5","DOMINO","DOMINO","","DM 5", DISABLED_IO },
{MODE_DOMINOEX8,&dominoex8_modem,"DOMEX8","DominoEX 8","DOMINOEX8","DOMINO","DOMINO","","DM 8", DISABLED_IO },
{MODE_DOMINOEX11,&dominoex11_modem,"DOMX11","DominoEX 11","DOMINOEX11","DOMINO","DOMINO","","DM11", DISABLED_IO },
{MODE_DOMINOEX16,&dominoex16_modem,"DOMX16","DominoEX 16","DOMINOEX16","DOMINO","DOMINO","","DM16", ARQ_IO | KISS_IO },
{MODE_DOMINOEX22,&dominoex22_modem,"DOMX22","DominoEX 22","DOMINOEX22","DOMINO","DOMINO","","DM22", ARQ_IO | KISS_IO },
{MODE_DOMINOEX44,&dominoex44_modem,"DOMX44","DominoEX 44","DOMINOEX44","DOMINO","DOMINO","","DM44", ARQ_IO | KISS_IO },
{MODE_DOMINOEX88,&dominoex88_modem,"DOMX88","DominoEX 88","DOMINOEX88","DOMINO","DOMINO","","DM88", ARQ_IO | KISS_IO },

{MODE_FELDHELL,&feld_modem,"FELDHELL","Feld Hell","","HELL","HELL","","HELL", DISABLED_IO },
{MODE_SLOWHELL,&feld_slowmodem,"SLOWHELL","Slow Hell","","HELL","HELL","","SHLL", DISABLED_IO },
{MODE_HELLX5,&feld_x5modem,"HELLX5","Feld Hell X5","","HELL","HELL","","HLX5", DISABLED_IO },
{MODE_HELLX9,&feld_x9modem,"HELLX9","Feld Hell X9","","HELL","HELL","","HLX9", DISABLED_IO },
{MODE_FSKH245,&feld_FMmodem,"FSKH245","FSK Hell-245","","FSKH245","HELL","FSKH245","FSKHL", DISABLED_IO  },
{MODE_FSKH105,&feld_FM105modem,"FSKH105","FSK Hell-105","","FSKH105","HELL","FSKH105","H105", DISABLED_IO },
{MODE_HELL80,&feld_80modem,"HELL80","Hell 80","","HELL80","HELL","HELL80","HL80", DISABLED_IO },

{MODE_MFSK8,&mfsk8_modem,"MFSK8","MFSK-8","MFSK8","MFSK8","MFSK","MFSK8","MK 8", DISABLED_IO  },
{MODE_MFSK16,&mfsk16_modem,"MFSK16","MFSK-16","MFSK16","MFSK16","MFSK","MFSK16","MK16", ARQ_IO | KISS_IO },
{MODE_MFSK32,&mfsk32_modem,"MFSK32","MFSK-32","MFSK32","MFSK32","MFSK","MFSK32","MK32", ARQ_IO | KISS_IO  },

{MODE_MFSK4,&mfsk4_modem,"MFSK4","MFSK-4","MFSK4","MFSK4","MFSK","MFSK4","MK 4", DISABLED_IO  },
{MODE_MFSK11,&mfsk11_modem,"MFSK11","MFSK-11","MFSK11","MFSK11","MFSK","MFSK11","MK11", DISABLED_IO  },
{MODE_MFSK22,&mfsk22_modem,"MFSK22","MFSK-22","MFSK22","MFSK22","MFSK","MFSK22","MK22", DISABLED_IO  },
{MODE_MFSK31,&mfsk31_modem,"MFSK31","MFSK-31","MFSK31","MFSK31","MFSK","MFSK31","MK31", ARQ_IO | KISS_IO },
{MODE_MFSK64,&mfsk64_modem,"MFSK64","MFSK-64","MFSK64","MFSK64","MFSK","MFSK64","MK64", ARQ_IO | KISS_IO   },
{MODE_MFSK128,&mfsk128_modem,"MFSK128","MFSK-128","MFSK128","MFSK128","MFSK","MFSK128","MK128", ARQ_IO | KISS_IO   },
{MODE_MFSK64L,&mfsk64l_modem,"MFSK64L","MFSK-64L","MFSK64L","MFSK64L","MFSK","MFSK64","MK64L", ARQ_IO | KISS_IO   },
{MODE_MFSK128L,&mfsk128l_modem,"MFSK128L","MFSK-128L","MFSK128L","MFSK128L","MFSK","MFSK128","MK128L", ARQ_IO | KISS_IO  },

{MODE_WEFAX_576,&wefax576_modem,"WEFAX576","WEFAX-IOC576","WEFAXIOC576","FAX","FAX","","FX576", DISABLED_IO },
{MODE_WEFAX_288,&wefax288_modem,"WEFAX288","WEFAX-IOC288","WEFAXIOC288","FAX","FAX","","FX288", DISABLED_IO },

{MODE_NAVTEX,&navtex_modem,"NAVTEX","NAVTEX","NAVTEX","TOR","NAVTEX","","NAVTEX", DISABLED_IO },
{MODE_SITORB,&sitorb_modem,"SITORB","SITORB","SITORB","TOR","SITORB","","SITORB", DISABLED_IO },

{MODE_MT63_500S,&mt63_500S_modem,"MT63-500S","MT63-500S","MT63-500S","MT63-500S","MT63","","MT63-500S", ARQ_IO | KISS_IO  },
{MODE_MT63_500L,&mt63_500L_modem,"MT63-500L","MT63-500L","MT63-500L","MT63-500L","MT63","","MT63-500L", ARQ_IO | KISS_IO  },
{MODE_MT63_1000S,&mt63_1000S_modem,"MT63-1KS","MT63-1000S","MT63-1XXS","MT63-1KS","MT63","","MT63 1KS", ARQ_IO | KISS_IO  },
{MODE_MT63_1000L,&mt63_1000L_modem,"MT63-1KL","MT63-1000L","MT63-1XXL","MT63-1KL","MT63","","MT63 1KL", ARQ_IO | KISS_IO  },
{MODE_MT63_2000S,&mt63_2000S_modem,"MT63-2KS","MT63-2000S","MT63-2XXS","MT63-2KS","MT63","","MT63 2KS", ARQ_IO | KISS_IO  },
{MODE_MT63_2000L,&mt63_2000L_modem,"MT63-2KL","MT63-2000L","MT63-2XXL","MT63-2KL","MT63","","MT63 2KL", ARQ_IO | KISS_IO  },

{MODE_PSK31,&psk31_modem,"BPSK31","BPSK-31","PSK31","PSK31","PSK","PSK31","P31", ARQ_IO | KISS_IO  },
{MODE_PSK63,&psk63_modem,"BPSK63","BPSK-63","PSK63","PSK63","PSK","PSK63","P63", ARQ_IO | KISS_IO  },
{MODE_PSK63F,&psk63f_modem,"BPSK63F","BPSK-63F","PSK63F","PSK63F","PSK","PSK63F","P63F", ARQ_IO  | KISS_IO },
{MODE_PSK125,&psk125_modem,"BPSK125","BPSK-125","PSK125","PSK125","PSK","PSK125","P125",  ARQ_IO | KISS_IO  },
{MODE_PSK250,&psk250_modem,"BPSK250","BPSK-250","PSK250","PSK250","PSK","PSK250","P250",  ARQ_IO | KISS_IO  },
{MODE_PSK500,&psk500_modem,"BPSK500","BPSK-500","PSK500","PSK500","PSK","PSK500","P500",  ARQ_IO | KISS_IO  },
{MODE_PSK1000,&psk1000_modem,"BPSK1000","BPSK-1000","PSK1000","PSK1000","PSK","PSK1000","P1000",  ARQ_IO | KISS_IO  },

{MODE_12X_PSK125,&psk125_c12_modem,"PSK125C12","12xPSK125","PSK125C12","PSK125C12","PSK","","P125C12", ARQ_IO | KISS_IO },
{MODE_6X_PSK250,&psk250_c6_modem,"PSK250C6","6xPSK250","PSK250C6","PSK250C6","PSK","","P2506", ARQ_IO | KISS_IO },
{MODE_2X_PSK500,&psk500_c2_modem,"PSK500C2","2xPSK500","PSK500C2","PSK500C2","PSK","","2xP500", ARQ_IO | KISS_IO },
{MODE_4X_PSK500,&psk500_c4_modem,"PSK500C4","4xPSK500","PSK500C4","PSK500C4","PSK","","4xP500", ARQ_IO | KISS_IO },
{MODE_2X_PSK800,&psk800_c2_modem,"PSK800C2","2xPSK800","PSK800C2","PSK800C2","PSK","","P800RC2", ARQ_IO | KISS_IO },
{MODE_2X_PSK1000,&psk1000_c2_modem,"PSK1000C2","2xPSK1000","PSK1000C2","PSK1000C2","PSK","","P1KC2", ARQ_IO | KISS_IO },

{MODE_QPSK31,&qpsk31_modem,"QPSK31","QPSK-31","QPSK31","QPSK31","PSK","QPSK31","Q31", ARQ_IO  | KISS_IO   },
{MODE_QPSK63,&qpsk63_modem,"QPSK63","QPSK-63","QPSK63","QPSK63","PSK","QPSK63","Q63", ARQ_IO  | KISS_IO   },
{MODE_QPSK125,&qpsk125_modem,"QPSK125","QPSK-125","QPSK125","QPSK125","PSK","QPSK125","Q125",  ARQ_IO | KISS_IO  },
{MODE_QPSK250,&qpsk250_modem,"QPSK250","QPSK-250","QPSK250","QPSK250","PSK","QPSK250","Q250",  ARQ_IO | KISS_IO  },
{MODE_QPSK500,&qpsk500_modem,"QPSK500","QPSK-500","QPSK500","QPSK500","PSK","QPSK500","Q500",  ARQ_IO | KISS_IO  },

{MODE_8PSK125,&_8psk125_modem,"8PSK125","8PSK-125","8PSK125","8PSK125","PSK","","8PSK125", ARQ_IO | KISS_IO },
{MODE_8PSK125FL,&_8psk125fl_modem,"8PSK125FL","8PSK-125FL","8PSK125FL","8PSK125FL","PSK","","8PSK125FL", ARQ_IO | KISS_IO },
{MODE_8PSK125F,&_8psk125f_modem,"8PSK125F","8PSK-125F","8PSK125F","8PSK125F","PSK","","8PSK125F", ARQ_IO | KISS_IO },
{MODE_8PSK250,&_8psk250_modem,"8PSK250","8PSK-250","8PSK250","8PSK250","PSK","","8PSK250", ARQ_IO | KISS_IO },
{MODE_8PSK250FL,&_8psk250fl_modem,"8PSK250FL","8PSK-250FL","8PSK250FL","8PSK250F","PSK","","8PSK250FL", ARQ_IO | KISS_IO },
{MODE_8PSK250F,&_8psk250f_modem,"8PSK250F","8PSK-250F","8PSK250F","8PSK250F","PSK","","8PSK250F", ARQ_IO | KISS_IO },
{MODE_8PSK500,&_8psk500_modem,"8PSK500","8PSK-500","8PSK500","8PSK500","PSK","","8PSK500", ARQ_IO | KISS_IO },
{MODE_8PSK500F,&_8psk500f_modem,"8PSK500F","8PSK-500F","8PSK500F","8PSK500F","PSK","","8PSK500F", ARQ_IO | KISS_IO },
{MODE_8PSK1000,&_8psk1000_modem,"8PSK1000","8PSK-1000","8PSK1000","8PSK1000","PSK","","8PSK1000", ARQ_IO | KISS_IO },
{MODE_8PSK1000F,&_8psk1000f_modem,"8PSK1000F","8PSK-1000F","8PSK1000F","8PSK1000F","PSK","","8PSK1000F", ARQ_IO | KISS_IO },
{MODE_8PSK1200F,&_8psk1200f_modem,"8PSK1200F","8PSK-1200F","8PSK1200F","8PSK1200F","PSK","","8PSK1200F", ARQ_IO | KISS_IO },

{MODE_OFDM_500F,&ofdm_500f_modem,"OFDM500F","OFDM-500F","OFDM500F","OFDM500F","OFDM","","OFDM500F", ARQ_IO | KISS_IO },
{MODE_OFDM_750F,&ofdm_750f_modem,"OFDM750F","OFDM-750F","OFDM750F","OFDM750F","OFDM","","OFDM750F", ARQ_IO | KISS_IO },
{MODE_OFDM_2000F,&ofdm_2000f_modem,"OFDM2000F","OFDM-2000F","OFDM2000F","OFDM2000F","OFDM","","OFDM2000F", ARQ_IO | KISS_IO },
{MODE_OFDM_2000,&ofdm_2000_modem,"OFDM2000","OFDM-2000","OFDM2000","OFDM2000","OFDM","","OFDM2000", ARQ_IO | KISS_IO },
{MODE_OFDM_3500,&ofdm_3500_modem,"OFDM3500","OFDM-3500","OFDM3500","OFDM3500","OFDM","","OFDM3500", ARQ_IO | KISS_IO },

{MODE_OLIVIA,&olivia_modem,"OLIVIA","OLIVIA","OLIVIA","OLIVIA","OLIVIA","","OL", DISABLED_IO },

{MODE_OLIVIA_4_125,&olivia_4_125_modem,"OLIVIA-4/125","OL 4-125","OLIV 4-125","OLIVIA 4/125","OLIVIA","OLIVIA 4/125","OL4/125", ARQ_IO  },
{MODE_OLIVIA_4_250,&olivia_4_250_modem,"OLIVIA-4/250","OL 4-250","OLIV 4-250","OLIVIA 4/250","OLIVIA","OLIVIA 4/250","OL4/250", ARQ_IO  },
{MODE_OLIVIA_4_500,&olivia_4_500_modem,"OLIVIA-4/500","OL 4-500","OLIV 4-500","OLIVIA 4/500","OLIVIA","OLIVIA 4/500","OL4/500", ARQ_IO | KISS_IO },
{MODE_OLIVIA_4_1000,&olivia_4_1000_modem,"OLIVIA-4/1K","OL 4-1K","OLIV 4-1K","OLIVIA 4/1K","OLIVIA","OLIVIA 4/1K","OL4/1K", ARQ_IO | KISS_IO },
{MODE_OLIVIA_4_2000,&olivia_4_2000_modem,"OLIVIA-4/2K","OL 4-2K","OLIV 4-2K","OLIVIA 4/2K","OLIVIA","OLIVIA 4/2K","OL4/2K", ARQ_IO | KISS_IO },

{MODE_OLIVIA_8_125,&olivia_8_125_modem,"OLIVIA-8/125","OL 8-125","OLIV 8-125","OLIVIA 8/125","OLIVIA","OLIVIA 8/125","OL8/125", ARQ_IO  },
{MODE_OLIVIA_8_250,&olivia_8_250_modem,"OLIVIA-8/250","OL 8-250","OLIV 8-250","OLIVIA 8/250","OLIVIA","OLIVIA 8/250","OL8/250", ARQ_IO  },
{MODE_OLIVIA_8_500,&olivia_8_500_modem,"OLIVIA-8/500","OL 8-500","OLIV 8-500","OLIVIA 8/500","OLIVIA","OLIVIA 8/500","OL8/500", ARQ_IO | KISS_IO },
{MODE_OLIVIA_8_1000,&olivia_8_1000_modem,"OLIVIA-8/1K","OL 8-1K","OLIV 8-1K","OLIVIA 8/1K","OLIVIA","OLIVIA 8/1K","OL8/1K", ARQ_IO | KISS_IO  },
{MODE_OLIVIA_8_2000,&olivia_8_2000_modem,"OLIVIA-8/2K","OL 8-2K","OLIV 8-2K","OLIVIA 8/2K","OLIVIA","OLIVIA 8/2K","OL8/1K", ARQ_IO | KISS_IO  },

{MODE_OLIVIA_16_500,&olivia_16_500_modem,"OLIVIA-16/500","OL 16-500","OLIV 16-500","OLIVIA 16/500","OLIVIA","OLIVIA 16/500","OL16/500", ARQ_IO | KISS_IO },
{MODE_OLIVIA_16_1000,&olivia_16_1000_modem,"OLIVIA-16/1K","OL 16-1K","OLIV 16-1K","OLIVIA 16/1K","OLIVIA","OLIVIA 16/1K","OL16/1K", ARQ_IO | KISS_IO  },
{MODE_OLIVIA_16_2000,&olivia_16_2000_modem,"OLIVIA-16/2K","OL 16-2K","OLIV 16-2K","OLIVIA 16/2K","OLIVIA","OLIVIA 16/2K","OL16/2K", ARQ_IO | KISS_IO  },

{MODE_OLIVIA_32_1000,&olivia_32_1000_modem,"OLIVIA-32/1K","OL 32-1K","OLIV 32-1K","OLIVIA 32/1K","OLIVIA","OLIVIA 32/1K","OL32/1K", ARQ_IO | KISS_IO  },
{MODE_OLIVIA_32_2000,&olivia_32_2000_modem,"OLIVIA-32/2K","OL 32-2K","OLIV 32-2K","OLIVIA 32/2K","OLIVIA","OLIVIA 32/2K","OL32/2K", ARQ_IO | KISS_IO  },

{MODE_OLIVIA_64_500,&olivia_64_500_modem,"OLIVIA-64/500","OL 64-500","OLIV 64-500","OLIVIA 64/500","OLIVIA","","OL64/500", ARQ_IO | KISS_IO  },
{MODE_OLIVIA_64_1000,&olivia_64_1000_modem,"OLIVIA-64/1K","OL 64-1K","OLIV 64-1K","OLIVIA 64/1K","OLIVIA","","OL64/1K", ARQ_IO | KISS_IO  },
{MODE_OLIVIA_64_2000,&olivia_64_2000_modem,"OLIVIA-64/2K","OL 64-2K","OLIV 64-2K","OLIVIA 64/2K","OLIVIA","","OL64/2K", ARQ_IO | KISS_IO  },

{MODE_RTTY,&rtty_modem,"RTTY","RTTY","RTTY","RTTY","RTTY","","RY", DISABLED_IO },

{MODE_THORMICRO,&thormicro_modem,"THOR Micro","THOR Micro","THORM","THOR-M","THOR","","THM", DISABLED_IO  },
{MODE_THOR4,&thor4_modem,"THOR4","THOR 4","THOR4","THOR4","THOR","","TH4", DISABLED_IO  },
{MODE_THOR5,&thor5_modem,"THOR5","THOR 5","THOR5","THOR5","THOR","","TH5", DISABLED_IO  },
{MODE_THOR8,&thor8_modem,"THOR8","THOR 8","THOR8","THOR8","THOR","","TH8", DISABLED_IO  },
{MODE_THOR11,&thor11_modem,"THOR11","THOR 11","THOR11","THOR11","THOR","","TH11", DISABLED_IO  },
{MODE_THOR16,&thor16_modem,"THOR16","THOR 16","THOR16","THOR16","THOR","","TH16", ARQ_IO | KISS_IO },
{MODE_THOR22,&thor22_modem,"THOR22","THOR 22","THOR22","THOR22","THOR","","TH22", ARQ_IO | KISS_IO },
{MODE_THOR25x4,&thor25x4_modem,"THOR25x4","THOR 25 x4","THOR25x4","THOR-25X4","THOR","","TH25", ARQ_IO | KISS_IO },
{MODE_THOR50x1,&thor50x1_modem,"THOR50x1","THOR 50 x1","THOR50x1","THOR-50X1","THOR","","TH51", ARQ_IO | KISS_IO },
{MODE_THOR50x2,&thor50x2_modem,"THOR50x2","THOR 50 x2","THOR50x2","THOR-50X2","THOR","","TH52", ARQ_IO | KISS_IO },
{MODE_THOR100,&thor100_modem,"THOR100","THOR 100","THOR100","THOR-100","THOR","","TH100", ARQ_IO | KISS_IO },

{MODE_THROB1,&throb1_modem,"THROB1","Throb 1","","THRB","THRB","","TB1", DISABLED_IO },
{MODE_THROB2,&throb2_modem,"THROB2","Throb 2","","THRB","THRB","","TB2", DISABLED_IO },
{MODE_THROB4,&throb4_modem,"THROB4","Throb 4","","THRB","THRB","","TB4", DISABLED_IO },
{MODE_THROBX1,&throbx1_modem,"THRBX1","ThrobX 1","","THRBX","THRB","THRBX","TX1", DISABLED_IO },
{MODE_THROBX2,&throbx2_modem,"THRBX2","ThrobX 2","","THRBX","THRB","THRBX","TX2", DISABLED_IO },
{MODE_THROBX4,&throbx4_modem,"THRBX4","ThrobX 4","","THRBX","THRB","THRBX","TX4", DISABLED_IO },

//{MODE_PACKET,&pkt_modem,"PACKET","Packet","","PKT","PKT","PKT",ARQ_IO | KISS_IO },

{MODE_PSK125R,&psk125r_modem,"PSK125R","PSK-125R","PSK125R","PSK125R","PSK","","P125R",  ARQ_IO | KISS_IO  },
{MODE_PSK250R,&psk250r_modem,"PSK250R","PSK-250R","PSK250R","PSK250R","PSK","","P250R",  ARQ_IO | KISS_IO  },
{MODE_PSK500R,&psk500r_modem,"PSK500R","PSK-500R","PSK500R","PSK500R","PSK","","P500R",  ARQ_IO | KISS_IO  },
{MODE_PSK1000R,&psk1000r_modem,"PSK1000R","PSK-1000R","PSK1000R","PSK1000R","PSK","","PSK1000R",  ARQ_IO | KISS_IO  },

{MODE_4X_PSK63R,&psk63r_c4_modem,"PSK63RC4","4xPSK63R","PSK63RC4","PSK63RC4","PSK","","P63R4", ARQ_IO | KISS_IO },
{MODE_5X_PSK63R,&psk63r_c5_modem,"PSK63RC5","5xPSK63R","PSK63RC5","PSK63RC5","PSK","","P63R5", ARQ_IO | KISS_IO },
{MODE_10X_PSK63R,&psk63r_c10_modem,"PSK63RC10","10xPSK63R","PSK63RC10","PSK63RC10","PSK","","P63R10", ARQ_IO | KISS_IO },
{MODE_20X_PSK63R,&psk63r_c20_modem,"PSK63RC20","20xPSK63R","PSK63RC20","PSK63RC20","PSK","","P63R20", ARQ_IO | KISS_IO },
{MODE_32X_PSK63R,&psk63r_c32_modem,"PSK63RC32","32xPSK63R","PSK63RC32","PSK63RC32","PSK","","P63R32", ARQ_IO | KISS_IO },

{MODE_4X_PSK125R,&psk125r_c4_modem,"PSK125RC4","4xPSK125R","PSK125RC4","PSK125RC4","PSK","","P125R4", ARQ_IO | KISS_IO },
{MODE_5X_PSK125R,&psk125r_c5_modem,"PSK125RC5","5xPSK125R","PSK125RC5","PSK125RC5","PSK","","P125R5", ARQ_IO | KISS_IO },
{MODE_10X_PSK125R,&psk125r_c10_modem,"PSK125RC10","10xPSK125R","PSK125RC10","PSK125RC10","PSK","","P125R10", ARQ_IO | KISS_IO },
{MODE_12X_PSK125R,&psk125r_c12_modem,"PSK125RC12","12xPSK125R","PSK125RC12","PSK125RC12","PSK","","P125R12", ARQ_IO | KISS_IO },
{MODE_16X_PSK125R,&psk125r_c16_modem,"PSK125RC16","16xPSK125R","PSK125RC16","PSK125RC16","PSK","","P125R16", ARQ_IO | KISS_IO },

{MODE_2X_PSK250R,&psk250r_c2_modem,"PSK250RC2","2xPSK250R","PSK250RC2","PSK250RC2","PSK","","P250R2", ARQ_IO | KISS_IO },
{MODE_3X_PSK250R,&psk250r_c3_modem,"PSK250RC3","3xPSK250R","PSK250RC3","PSK250RC3","PSK","","P250R3", ARQ_IO | KISS_IO },
{MODE_5X_PSK250R,&psk250r_c5_modem,"PSK250RC5","5xPSK250R","PSK250RC5","PSK250RC5","PSK","","P250R5", ARQ_IO | KISS_IO },
{MODE_6X_PSK250R,&psk250r_c6_modem,"PSK250RC6","6xPSK250R","PSK250RC6","PSK250RC6","PSK","","P250R6", ARQ_IO | KISS_IO },
{MODE_7X_PSK250R,&psk250r_c7_modem,"PSK250RC7","7xPSK250R","PSK250RC7","PSK250RC7","PSK","","P250R7", ARQ_IO | KISS_IO },

{MODE_2X_PSK500R,&psk500r_c2_modem,"PSK500RC2","2xPSK500R","PSK500RC2","PSK500RC2","PSK","","P500R2", ARQ_IO | KISS_IO },
{MODE_3X_PSK500R,&psk500r_c3_modem,"PSK500RC3","3xPSK500R","PSK500RC3","PSK500RC3","PSK","","P500R3", ARQ_IO | KISS_IO },
{MODE_4X_PSK500R,&psk500r_c4_modem,"PSK500RC4","4xPSK500R","PSK500RC4", "PSK500RC4","PSK","","P500R4", ARQ_IO | KISS_IO },

{MODE_2X_PSK800R,&psk800r_c2_modem,"PSK800RC2","2xPSK800R","PSK800RC2","PSK800RC2","PSK","","P800RC2", ARQ_IO | KISS_IO },

{MODE_2X_PSK1000R,&psk1000r_c2_modem,"PSK1000RC2","2xPSK1000R","PSK1000RC2","PSK1000RC2","PSK","","P1KRC2", ARQ_IO | KISS_IO },

{MODE_FSQ,&fsq_modem,"FSQ","FSQ","FSQ","FSQ","FSQ","","FSQ", DISABLED_IO },
{MODE_IFKP,&ifkp_modem,"IFKP","IFKP","IFKP","IFKP","IFKP","","IFKP", DISABLED_IO },

{MODE_SSB,&ssb_modem,"SSB","SSB","","SSB","SSB","","", DISABLED_IO },
{MODE_WWV,&wwv_modem,"WWV","WWV","","","","","", DISABLED_IO },
{MODE_ANALYSIS,&anal_modem,"ANALYSIS","Freq Analysis","","","","","", DISABLED_IO },
{MODE_FMT,&fmt_modem,"FMT","Frequency Measurement Test","","","","","", DISABLED_IO }
};

std::string adif2export(std::string adif)
{
	std::string test = ucasestr(adif);
	for (int n = 0; n < NUM_MODES; n++) {
		if (test == ucasestr(mode_info[n].sname) ||
			test == ucasestr(mode_info[n].adif_name) ||
			test == ucasestr(mode_info[n].export_mode) ||
			test == ucasestr(mode_info[n].export_submode))
			return ucasestr(mode_info[n].export_mode);
	}
	return test;
}

std::string adif2submode(std::string adif)
{
	std::string test = ucasestr(adif);
	for (int n = 0; n < NUM_MODES; n++) {
		if (test == ucasestr(mode_info[n].sname) ||
			test == ucasestr(mode_info[n].adif_name) ||
			test == ucasestr(mode_info[n].export_mode) ||
			test == ucasestr(mode_info[n].export_submode))
			return ucasestr(mode_info[n].export_submode);
	}
	return "";
}

std::ostream& operator<<(std::ostream& s, const qrg_mode_t& m)
{
	return s << m.rfcarrier << ' '
			 << m.rmode << ' '
			 << m.carrier << ' '
			 << mode_info[m.mode].sname << ' '
			 << m.usage;
}

std::istream& operator>>(std::istream& s, qrg_mode_t& m)
{
	string sMode;
	char temp[255];
	int mnbr;
	s >> m.rfcarrier >> m.rmode >> m.carrier >> sMode;

	s.getline(temp, 255);
	m.usage = temp;
	while (m.usage[0] == ' ') m.usage.erase(0,1);

// handle case for reading older type of specification string
	if (sscanf(sMode.c_str(), "%d",&mnbr)) {
		m.mode = mnbr;
		return s;
	}
	m.mode = MODE_PSK31;
	for (mnbr = MODE_CW; mnbr < NUM_MODES; mnbr++)
		if (sMode == mode_info[mnbr].sname) {
			m.mode = mnbr;
			break;
		}
	return s;
}

std::string qrg_mode_t::str(void)
{
	ostringstream s;
	s << setiosflags(ios::fixed)
	  << setprecision(3) << rfcarrier/1000.0 << '|'
	  << rmode << '|'
	  << (mode < NUM_MODES ? mode_info[mode].sname : "NONE") << '|'
//	  << carrier;
	  << carrier << '|'
	  << usage;
	return s.str();
}

band_t band(long long freq_hz)
{
	switch (freq_hz / 1000000LL) {
		case 0: case 1: return BAND_160M;
		case 3: return BAND_80M;
		case 4: return BAND_75M;
		case 5: return BAND_60M;
		case 7: return BAND_40M;
		case 10: return BAND_30M;
		case 14: return BAND_20M;
		case 18: return BAND_17M;
		case 21: return BAND_15M;
		case 24: return BAND_12M;
		case 28 ... 29: return BAND_10M;
		case 50 ... 54: return BAND_6M;
		case 70 ... 71: return BAND_4M;
		case 144 ... 148: return BAND_2M;
		case 222 ... 225: return BAND_125CM;
		case 420 ... 450: return BAND_70CM;
		case 902 ... 928: return BAND_33CM;
		case 1240 ... 1325: return BAND_23CM;
		case 2300 ... 2450: return BAND_13CM;
		case 3300 ... 3500: return BAND_9CM;
		case 5650 ... 5925: return BAND_6CM;
		case 10000 ... 10500: return BAND_3CM;
		case 24000 ... 24250: return BAND_125MM;
		case 47000 ... 47200: return BAND_6MM;
		case 75500 ... 81000: return BAND_4MM;
		case 119980 ... 120020: return BAND_2P5MM;
		case 142000 ... 149000: return BAND_2MM;
		case 241000 ... 250000: return BAND_1MM;
	}

	return BAND_OTHER;
}

band_t band(const char* freq_mhz)
{
	errno = 0;
	double d = strtod(freq_mhz, NULL);
	if (d != 0.0 && errno == 0)
		return band((long long)(d * 1e6));
	else
		return BAND_OTHER;
}

struct band_freq_t {
	const char* band;
	const char* freq;
};

static struct band_freq_t band_names[NUM_BANDS] = {
	{ "160m", "1.8" },
	{ "80m", "3.5" },
	{ "75m", "4.0" },
	{ "60m", "5.3" },
	{ "40m", "7.0" },
	{ "30m", "10.0" },
	{ "20m", "14.0" },
	{ "17m", "18.0" },
	{ "15m", "21.0" },
	{ "12m", "24.0" },
	{ "10m", "28.0" },
	{ "6m", "50.0" },
	{ "4m", "70.0" },
	{ "2m", "144.0" },
	{ "1.25m", "222.0" },
	{ "70cm", "420.0" },
	{ "33cm", "902.0" },
	{ "23cm", "1240.0" },
	{ "13cm", "2300.0" },
	{ "9cm", "3300.0" },
	{ "6cm", "5650.0" },
	{ "3cm", "10000.0" },
	{ "1.25cm", "24000.0" },
	{ "6mm", "47000.0" },
	{ "4mm", "75500.0" },
	{ "2.5mm", "119980.0" },
	{ "2mm", "142000.0" },
	{ "1mm", "241000.0" },
	{ "other", "" }
};

const char* band_name(band_t b)
{
	return band_names[CLAMP(b, 0, NUM_BANDS-1)].band;
}

const char* band_name(const char* freq_mhz)
{
	return band_name(band(freq_mhz));
}

const char* band_freq(band_t b)
{
	return band_names[CLAMP(b, 0, NUM_BANDS-1)].freq;
}

const char* band_freq(const char* band_name)
{
	for (size_t i = 0; i < BAND_OTHER; i++)
		if (!strcmp(band_names[i].band, band_name))
			return band_names[i].freq;

	return "";
}
