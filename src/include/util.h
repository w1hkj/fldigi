/* -----------------------------------------------------------------------------
 *    util.h -- included by config.h
 *
 *    Copyright (C) 2007-2009
 *   		Stelios Bounanos, M0GLD
 *
 *    This file is part of fldigi.
 *
 *    Fldigi is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Fldigi is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with fldigi.  If not, see <http://www.gnu.org/licenses/>.
 * -----------------------------------------------------------------------------
 */

#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __STDC_LIMIT_MACROS
#  define __STDC_LIMIT_MACROS
#endif
#ifndef __STDC_FORMAT_MACROS
#    define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>

#ifndef powerof2
#    define powerof2(n) ((((n) - 1) & (n)) == 0)
#endif
#ifndef MAX
#    define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#    define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef CLAMP
#    define CLAMP(x, low, high) (((x)>(high))?(high):(((x)<(low))?(low):(x)))
#endif
#define WCLAMP(x, low, high) (((x)>(high))?(low):(((x)<(low))?(high):(x)))

#ifdef __GNUC__
#    if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
#        define full_memory_barrier() __sync_synchronize()
#        define read_memory_barrier() full_memory_barrier()
#        define write_memory_barrier() full_memory_barrier()
#    elif defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__x86_64__)
#        define full_memory_barrier() asm volatile ("lock; addl $0,0(%%esp)":::"memory")
#        define read_memory_barrier() full_memory_barrier()
#        define write_memory_barrier() full_memory_barrier()
/*
 These would be faster on SSE2-capable processors:
#        define full_memory_barrier() asm volatile ("mfence":::"memory")
#        define read_memory_barrier() asm volatile ("lfence":::"memory")
#        define write_memory_barrier() asm volatile ("sfence":::"memory")
*/
#    elif defined(__ppc__) || defined(__powerpc__) || defined(__PPC__)
#        define full_memory_barrier() asm volatile("sync":::"memory")
#        define read_memory_barrier() full_memory_barrier()
#        define write_memory_barrier() full_memory_barrier()
#    else
#        warning Memory barriers not defined on this system
#        define full_memory_barrier() ((void)0)
#        define read_memory_barrier() full_memory_barrier()
#        define write_memory_barrier() full_memory_barrier()
#    endif
#else
#    warning Memory barriers not defined on this system
#    define full_memory_barrier() ((void)0)
#    define read_memory_barrier() full_memory_barrier()
#    define write_memory_barrier() full_memory_barrier()
#endif

/* http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html */
#if defined(__GNUC__) && (__GNUC__ >= 3)
#    define likely(x)    __builtin_expect (!!(x), 1)
#    define unlikely(x)  __builtin_expect (!!(x), 0)
#    define used__       __attribute__ ((__used__))
#    define unused__     __attribute__ ((__unused__))
#    define must_check__ __attribute__ ((__warn_unused_result__))
#    define deprecated__ __attribute__ ((__deprecated__))
#    define noreturn__   __attribute__ ((__noreturn__))
#    define pure__       __attribute__ ((__pure__))
#    define const__      __attribute__ ((__const__))
#    define malloc__     __attribute__ ((__malloc__))
#    define packed__     __attribute__ ((__packed__))
#    define inline__     inline __attribute__ ((__always_inline__))
#    define noinline__   __attribute__ ((__noinline__))
#    define nonnull__(x) __attribute__ ((__nonnull__(x)))
#    define format__(type_, index_, first_) __attribute__ ((format(type_, index_, first_)))
#else
#    define likely(x)    (x)
#    define unlikely(x)  (x)
#    define used__
#    define unused__
#    define must_check__
#    define deprecated__
#    define noreturn__
#    define pure__
#    define const__
#    define malloc__
#    define packed__
#    define inline__
#    define noinline__
#    define nonnull__(x)
#    define format__(type_, index_, first_)
#endif

#if defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
#    define hot__        __attribute__ ((__hot__))
#    define cold__       __attribute__ ((__cold__))
#else
#    define hot__
#    define cold__
#endif

#include <stddef.h>

const__ uint32_t ceil2(uint32_t n);
const__ uint32_t floor2(uint32_t n);

#if !HAVE_STRCASESTR
char* strcasestr(const char* haystack, const char* needle);
#endif

#if !HAVE_STRLCPY
size_t strlcpy(char* dest, const char* src, size_t size);
#endif

#if !HAVE_SETENV
int setenv(const char *name, const char *value, int replace);
#endif

#if !HAVE_UNSETENV
int unsetenv(const char *name);
#endif

int set_cloexec(int fd, unsigned char v);
int set_nonblock(int fd, unsigned char v);
int set_nodelay(int fd, unsigned char v);
int get_bufsize(int fd, int dir, int* len);
int set_bufsize(int fd, int dir, int len);

unsigned long ver2int(const char* version);

void save_signals(void);
void restore_signals(void);

void MilliSleep(long msecs);

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus
uint32_t simple_hash_data(const unsigned char* buf, size_t len, uint32_t code = 0);
uint32_t simple_hash_str(const unsigned char* str, uint32_t code = 0);
#endif

#ifdef __cplusplus
const char* str2hex(const unsigned char* str, size_t len);
const char* str2hex(const char* str, size_t len = 0);
#else
const char* str2hex(const unsigned* str, size_t len);
#endif

const char* uint2bin(unsigned u, size_t len);

#if 0 && !defined(NDEBUG) && defined(deprecated__) && defined(__GNUC__) && !defined(__MINGW32__)
#include <stdio.h>
#include <string.h>
deprecated__ typeof(sprintf) sprintf;
/* there are far too many of these in the qrz code
deprecated__ typeof(strcpy) strcpy;
deprecated__ typeof(strcat) strcat;
*/
#endif

#ifdef __WOE32__
#  define NOMINMAX 1
#endif

#ifndef __MINGW32__
#  define PRIuSZ "zu"
#  define PRIdSZ "zd"
#else
#  define PRIuSZ "Iu"
#  define PRIdSZ "Id"
#endif

#  define PATH_SEP "/"

/// Unnamed sempahores are not supported on OS X (and named semaphores are broken on cygwin).
#ifdef __APPLE__
#  define USE_NAMED_SEMAPHORES 1
#else
#  define USE_NAMED_SEMAPHORES 0
#endif

/// Returns 0 if a process is running, 0 if not there and -1 if the test cannot be made.
int test_process(int pid);

/// Starts a process and returns its pid, and -1 if error. Returns 0 if this cannot be made.
int fork_process( const char * cmd );

/// Returns NULL if no error.
const char * create_directory( const char * dir );

int directory_is_created( const char * dir );

#endif /* UTIL_H */

/*
Local Variables:
mode: c++
c-file-style: "linux"
End:
*/
