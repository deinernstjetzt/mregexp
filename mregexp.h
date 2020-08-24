#ifndef _MREGEXP_H
#define _MREGEXP_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

typedef struct MRegexp MRegexp;

typedef struct {
	size_t match_begin;
	size_t match_end;
} MRegexpMatch;

typedef enum {
	MREGEXP_OK = 0,
	MREGEXP_FAILED_ALLOC,
	MREGEXP_INVALID_UTF8,
	MREGEXP_INVALID_PARAMS,
	MREGEXP_EARLY_QUANTIFIER,
} MRegexpError;

/* compile regular expression */
MRegexp *mregexp_compile(const char *re);

/* get error if the compile failed */
MRegexpError mregexp_error(void);

/* find a pattern in a string */
bool mregexp_match(MRegexp *re, const char *s, MRegexpMatch *m);

/* free regular expression */
void mregexp_free(MRegexp *re);

#ifdef __cplusplus
}
#endif

#endif
