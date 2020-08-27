/* 
 * Copyright (c) 2020 Fabian van Rissenbeck

* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

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

/* check if a given string is valid utf8 */
bool mregexp_valid_utf8(const char *s);

/* compile regular expression */
MRegexp *mregexp_compile(const char *re);

/* get error type if a function failed */
MRegexpError mregexp_error(void);

/* find the first matching substring in s */
bool mregexp_match(MRegexp *re, const char *s, MRegexpMatch *m);

/* get all non-overlapping matches in string s. returns NULL
 * if no matches are found. returned value must be freed */
MRegexpMatch *mregexp_all_matches(MRegexp *re, const char *s, size_t *sz);

/* get amount of capture groups inside of
 * a regular expression */
size_t mregexp_captures_len(MRegexp *re);

/* get captured slice from capture group number index */
const MRegexpMatch *mregexp_capture(MRegexp *re, size_t index);

/* free regular expression */
void mregexp_free(MRegexp *re);

#ifdef __cplusplus
}
#endif

#endif
