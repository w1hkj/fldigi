#include <config.h>

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

#  include <stdlib.h>
#include "re.h"
long ver2int(const char* version)
{
	const char version_re[] = "([0-9])\\.([0-9]+)\\.?([0-9]+)?";
	re_t re(version_re, REG_EXTENDED);
	long v = 0;

	if (!re.match(version))
		return 0;

	if (re.nsub() == 4)
		v += strtol(re.submatch(3), NULL, 10);
	v += strtol(re.submatch(2), NULL, 10) * 1000L;
	v += strtol(re.submatch(1), NULL, 10) * 1000000L;

	return v;
}

#if !HAVE_STRCASESTR
#  include <ctype.h>
#  include <string.h>
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
		if (sigaction(i, &sigact[i-1], NULL) == -1)
			break;
	delete [] sigact;
	sigact = 0;
	nsig = 0;
	pthread_mutex_unlock(&sigmutex);
}
