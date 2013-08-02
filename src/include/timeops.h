#ifndef TIMEOPS_H_
#define TIMEOPS_H_

#include <config.h>
#include <time.h>
#include <sys/time.h>
#ifdef __MINGW32__
#  include <pthread.h>
#endif

#if !HAVE_CLOCK_GETTIME
enum clockid_t { CLOCK_REALTIME, CLOCK_MONOTONIC };
int clock_gettime(clockid_t clock_id, struct timespec* tp);
#endif

struct timespec operator+(const struct timespec &t0, const double &t);
struct timespec operator-(const struct timespec &t0, const struct timespec &t1);
struct timespec& operator-=(struct timespec &t0, const struct timespec &t1);
bool operator>(const struct timespec &t0, const struct timespec &t1);
bool operator==(const struct timespec &t0, const struct timespec &t1);

struct timeval operator+(const struct timeval &t0, const double &t);
struct timeval operator-(const struct timeval &t0, const struct timeval &t1);
struct timeval& operator-=(struct timeval &t0, const struct timeval &t1);
bool operator>(const struct timeval &t0, const struct timeval &t1);
bool operator==(const struct timeval &t0, const struct timeval &t1);

#ifndef GMTIME_R
extern struct tm *gmtime_r(const time_t *timer, struct tm *tmbuf);
extern struct tm *localtime_r(const time_t *_Time,struct tm *_Tm);
#endif

#endif // TIMEOPS_H_
