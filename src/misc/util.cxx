#include <config.h>

#include <stdlib.h>
#include <ctype.h>

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

#if !HAVE_STRCASESTR
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
