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
enum qrz_query_t { QRZ_EXIT = -1, QRZNONE, QRZNET, QRZCD, HAMCALLNET, QRZHTML, HAMCALLHTML, CALLOOK, HAMQTH };

extern void sendEQSL(const char *url);
extern void makeEQSL(const char *msg);

#endif
