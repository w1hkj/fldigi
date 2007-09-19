#ifndef TIMEOPS_H_
#define TIMEOPS_H_

#include <time.h>

struct timespec operator+(const struct timespec &t0, const double &t);
struct timespec operator-(const struct timespec &t0, const struct timespec &t1);
bool operator>(const struct timespec &t0, const struct timespec &t1);

#endif // TIMEOPS_H_
