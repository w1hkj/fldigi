#include "filters.h"

// #define X1    0    /* 350 dialtone */
// #define X2    1    /* 440 ring, dialtone */
// #define X3    2    /* 480 ring, busy */
// #define X4    3    /* 620 busy */

// #define R1    4    /* 697, dtmf row 1 */
// #define R2    5    /* 770, dtmf row 2 */
// #define R3    6    /* 852, dtmf row 3 */
// #define R4    8    /* 941, dtmf row 4 */
// #define C1   10    /* 1209, dtmf col 1 */
// #define C2   12    /* 1336, dtmf col 2 */
// #define C3   13    /* 1477, dtmf col 3 */
// #define C4   14    /* 1633, dtmf col 4 */

// #define B1    4    /* 700, blue box 1 */
// #define B2    7    /* 900, bb 2 */
// #define B3    9    /* 1100, bb 3 */
// #define B4   11    /* 1300, bb4 */
// #define B5   13    /* 1500, bb5 */
// #define B6   15    /* 1700, bb6 */
// #define B7   16    /* 2400, bb7 */
// #define B8   17    /* 2600, bb8 */

// #define NUMTONES 18 

/* values returned by detect 
 *  0-9     DTMF 0 through 9 or MF 0-9
 *  10-11   DTMF *, #
 *  12-15   DTMF A,B,C,D
 *  16-20   MF last column: C11, C12, KP1, KP2, ST
 *  21      2400
 *  22      2600
 *  23      2400 + 2600
 *  24      DIALTONE
 *  25      RING
 *  26      BUSY
 *  27      silence
 *  -1      invalid
 */
/*
#define D0    0
#define D1    1
#define D2    2
#define D3    3
#define D4    4
#define D5    5
#define D6    6
#define D7    7
#define D8    8
#define D9    9
#define DSTAR 10
#define DPND  11
#define DA    12
#define DB    13
#define DC    14
#define DD    15
#define DC11  16
#define DC12  17
#define DKP1  18
#define DKP2  19
#define DST   20
#define D24   21 
#define D26   22
#define D2426 23
#define DDT   24
#define DRING 25
#define DBUSY 26
#define DSIL  27
*/
/* translation of above codes into text */
/*
char *dtran[] = {
  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
  "*", "#", "A", "B", "C", "D", 
  "+C11 ", "+C12 ", " KP1+", " KP2+", "+ST ",
  " 2400 ", " 2600 ", " 2400+2600 ",
  " DIALTONE ", " RING ", " BUSY ","" };
*/
//#define RANGE  0.1           /* any thing higher than RANGE*peak is "on" */
//#define THRESH 100.0         /* minimum level for the loudest tone */
//#define FLUSH_TIME 100       /* 100 frames = 3 seconds */

class cDTMF {
struct PAIRS {int lo; int hi; char ch;};
private:
	static int row[];
	static int col[];
	static int tones[];
	static PAIRS pairs[];
	double bins[8];
	double outbuf[16384];
	double shape[128];
	int RT;
	int duration;

public:
	cDTMF();
	~cDTMF() {}
	void receive(const float* buf, size_t len);
	void makeshape();
	void silence(int);
	void two_tones(int);
	void send();

};
