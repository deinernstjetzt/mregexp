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
	MREGEXP_INVALID_COMPLEX_QUANT,
	MREGEXP_UNEXPECTED_EOL,
	MREGEXP_INVALID_COMPLEX_CLASS,
	MREGEXP_UNCLOSED_SUBEXPRESSION,
} MRegexpError;

/* compile regular expression */
MRegexp *mregexp_compile(const char *re);

/* get error type if a function failed */
MRegexpError mregexp_error(void);

/* find a pattern in a string */
bool mregexp_match(MRegexp *re, const char *s, MRegexpMatch *m);

/* free regular expression */
void mregexp_free(MRegexp *re);

#ifdef __cplusplus
}
#endif

#endif
