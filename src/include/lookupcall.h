#ifndef LOOKUPCALL_H
#define LOOKUPCALL_H

#include <cstring>

extern std::string lookup_latd;
extern std::string lookup_lond;
extern std::string lookup_addr1;
extern std::string lookup_addr2;
extern std::string lookup_qth;
extern std::string lookup_state;
extern std::string lookup_province;
extern std::string lookup_zip;
extern std::string lookup_country;

extern void clear_Lookup();

extern void CALLSIGNquery();

enum qrz_xmlquery_t { 
QRZXML_EXIT = -1, 
QRZXMLNONE, 
QRZNET, QRZCD, 
HAMCALLNET,
CALLOOK, 
HAMQTH };

enum qrz_webquery_t { 
QRZWEB_EXIT = -1, 
QRZWEBNONE, 
QRZHTML, HAMCALLHTML, HAMQTHHTML };

extern void sendEQSL(const char *url);
extern void makeEQSL(const char *msg);

#endif
