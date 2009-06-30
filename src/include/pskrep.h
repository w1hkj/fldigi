#ifndef PSKREP_H_
#define PSKREP_H_

bool pskrep_start(void);
void pskrep_stop();
const char* pskrep_error(void);
unsigned pskrep_count(void);

// The regular expression that matches the spotter's buffer when it calls us.
// It must define at least two capturing groups, the second of which is the
// spotted callsign.
#define CALLSIGN_RE "[[:alnum:]]?[[:alpha:]/]+[[:digit:]]+[[:alnum:]/]+"
#define PSKREP_RE "(de|cq|qrz)[^[:alnum:]/\n]+"  "(" CALLSIGN_RE ")"  " +(.* +)?\\2[^[:alnum:]]+$"
#define PSKREP_RE_INDEX 2

#endif // PSKREP_H_
