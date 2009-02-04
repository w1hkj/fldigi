#include <config.h>

#include <string.h>
#include "util.h"

/* Return the smallest power of 2 not less than n */
uint32_t ceil2(uint32_t n)
{
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;

        return n + 1;
}

/* Return the largest power of 2 not greater than n */
uint32_t floor2(uint32_t n)
{
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;

        return n - (n >> 1);
}

#include <stdlib.h>
unsigned long ver2int(const char* version)
{
	unsigned long v;
	char* p;

	v = (unsigned long)(strtod(version, &p) * 1e7 + 0.5);
	while (*p)
		v += *p++;

	return v;
}

#if !HAVE_STRCASESTR
#  include <ctype.h>
// a simple inefficient implementation of strcasestr
char* strcasestr(const char* haystack, const char* needle)
{
	char *h = NULL, *n = NULL, *p = NULL;
	if ((h = strdup(haystack)) == NULL || (n = strdup(needle)) == NULL)
		goto ret;
	for (p = h; *p; p++)
		*p = tolower(*p);
	for (p = n; *p; p++)
		*p = tolower(*p);
	p = strstr(h, n);
ret:
	free(h);
	free(n);
	return p;
}
#endif // !HAVE_STRCASESTR

#include <unistd.h>
#include <fcntl.h>
int set_cloexec(int fd, unsigned char v)
{
	int f;

	if ((f = fcntl(fd, F_GETFD)) != -1)
		f = fcntl(fd, F_SETFD, f | (v ? FD_CLOEXEC : 0));
	return f;
}

#include <pthread.h>
#include <signal.h>
#ifndef NSIG
#  define NSIG 64
#endif
static size_t nsig = 0;
static struct sigaction* sigact = 0;
static pthread_mutex_t sigmutex = PTHREAD_MUTEX_INITIALIZER;

void save_signals(void)
{
	pthread_mutex_lock(&sigmutex);
	if (!sigact)
		sigact = new struct sigaction[NSIG];
	for (nsig = 1; nsig <= NSIG; nsig++)
		if (sigaction(nsig, NULL, &sigact[nsig-1]) == -1)
			break;
	pthread_mutex_unlock(&sigmutex);
}

void restore_signals(void)
{
	pthread_mutex_lock(&sigmutex);
	for (size_t i = 1; i <= nsig; i++)
		sigaction(i, &sigact[i-1], NULL);
	delete [] sigact;
	sigact = 0;
	nsig = 0;
	pthread_mutex_unlock(&sigmutex);
}

uint32_t simple_hash_data(const unsigned char* buf, size_t len, uint32_t code)
{
	for (size_t i = 0; i < len; i++)
		code = ((code << 4) | (code >> (32 - 4))) ^ (uint32_t)buf[i];

	return code;
}
uint32_t simple_hash_str(const unsigned char* str, uint32_t code)
{
	while (*str)
		code = ((code << 4) | (code >> (32 - 4))) ^ (uint32_t)*str++;
	return code;
}

static const char hexsym[] = "0123456789ABCDEF";
char* str2hex(const unsigned char* in, size_t ilen, char* out, size_t olen)
{
	if (unlikely(ilen == 0 || olen == 0))
		return out;
	if (unlikely(olen < ilen * 3))
		ilen = olen / 3;

	char* r = out;
	size_t i;
	for (i = 0; i < ilen; i++) {
		*out++ = hexsym[in[i] >> 4];
		*out++ = hexsym[in[i] & 0xF];
		*out++ = ' ';
	}
	*(out - 1) = '\0';

	return r;
}

static char* hexbuf = 0;
static size_t hexlen = 0;
const char* printhex(const unsigned char* str, size_t len)
{
	if (len == 0)
		return "";
	len *= 3;
	if (hexlen < len && (hexbuf = (char*)realloc(hexbuf, (hexlen = len))) == NULL) {
		hexlen = 0;
		return "";
	}
	return str2hex(str, len, hexbuf, hexlen);
}

const char* printhex(const char* str, size_t len)
{
	return printhex((const unsigned char*)str, len ? len : strlen(str));
}
