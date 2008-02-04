/* This file is included by config.h */

#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

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
#endif

#if defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3))
#    define hot__        __attribute__ ((__hot__))
#    define cold__       __attribute__ ((__cold__))
#else
#    define hot__
#    define cold__
#endif


const__ uint32_t ceil2(uint32_t n);
const__ uint32_t floor2(uint32_t n);


#ifdef __cplusplus
} // extern "C"
#endif

#if !defined(NDEBUG) && defined(deprecated__)
#include <stdio.h>
#include <string.h>
deprecated__ typeof(sprintf) sprintf;
/* there are far too many of these in the qrz code
deprecated__ typeof(strcpy) strcpy;
deprecated__ typeof(strcat) strcat;
*/
#endif

#endif /* UTIL_H */

/*
Local Variables:
mode: c++
c-file-style: "linux"
End:
*/
