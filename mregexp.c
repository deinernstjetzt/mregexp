#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <setjmp.h>

#include "mregexp.h"

static unsigned utf8_char_width(uint8_t c)
{
	size_t ret = 0;

	if ((c & 128) == 0)
		ret = 1;
	else if ((c & (128 + 64 + 32)) == 128 + 64)
		ret = 2;
	else if ((c & (128 + 64 + 32 + 16)) == 128 + 64 + 32)
		ret = 3;
	else if ((c & (128 + 64 + 32 + 16 + 8)) == 128 + 64 + 32 + 16)
		ret = 4;

	return ret;
}

static bool utf8_valid(const char *s)
{
	const size_t len = strlen(s);

	for (size_t i = 0; i < len;) {
		const unsigned width = utf8_char_width((uint8_t)s[i]);

		if (width == 0) {
			return false;
		}

		if (i + width > len) {
			return false;
		}

		for (unsigned j = 1; j < width; ++j)
			if ((s[i + j] & (128 + 64)) != 128) {
				return false;
			}

		i += width;
	}

	return true;
}

static uint32_t utf8_peek(const char *s)
{
	if (*s == 0)
		return 0;

	const unsigned width = utf8_char_width(s[0]);
	size_t ret = 0;

	switch (width) {
	case 1:
		ret = s[0] & 127;
		break;

	case 2:
		ret = s[0] & 31;
		break;

	case 3:
		ret = s[0] & 15;
		break;

	case 4:
		ret = s[0] & 7;
		break;

	default:
		return 0;
	}

	for (unsigned i = 1; i < width; ++i) {
		ret <<= 6;
		ret += s[i] & 63;
	}

	return ret;
}

static const char *utf8_next(const char *s)
{
	if (*s == 0)
		return NULL;

	const unsigned width = utf8_char_width((uint8_t)s[0]);
	return s + width;
}

union RegexNode;

/* function pointer type used to evaluate if a regex node
 * matched a given string */
typedef bool (*MatchFunc)(union RegexNode *node, const char *orig,
			  const char *cur, const char **next);

typedef struct GenericNode {
	union RegexNode *prev;
	union RegexNode *next;
	MatchFunc match;
} GenericNode;

typedef struct {
	GenericNode generic;
	uint32_t chr;
} CharNode;

typedef union RegexNode {
	GenericNode generic;
	CharNode chr;
} RegexNode;

static bool is_match(RegexNode *node, const char *orig, const char *cur,
		     const char **next)
{
	if (node == NULL) {
		*next = cur;
		return true;
	} else {
		if ((node->generic.match)(node, orig, cur, next)) {
			return is_match(node->generic.next, orig, *next, next);
		} else {
			return false;
		}
	}
}

static bool char_is_match(RegexNode *node, const char *orig, const char *cur,
			  const char **next)
{
	const uint32_t c1 = node->chr.chr;

	if (*cur == 0) {
		return false;
	}

	*next = utf8_next(cur);
	return c1 == utf8_peek(cur);
}

static bool start_is_match(RegexNode *node, const char *orig, const char *cur,
			   const char **next)
{
	*next = cur;
	return true;
}

static bool anchor_begin_is_match(RegexNode *node, const char *orig,
				  const char *cur, const char **next)
{
	*next = cur;
	return strlen(orig) == strlen(cur);
}

static bool anchor_end_is_match(RegexNode *node, const char *orig,
				const char *cur, const char **next)
{
	*next = cur;
	return strlen(cur) == 0;
}

static bool any_is_match(RegexNode *node, const char *orig, const char *cur,
			 const char **next)
{
	if (*cur) {
		*next = utf8_next(cur);
		return true;
	}

	return false;
}

/* Global error value with callback address */
struct {
	MRegexpError err;
	const char *s;
	jmp_buf buf;
} CompileException;

/* set global error value to the default value */
static void clear_compile_exception(void)
{
	CompileException.err = MREGEXP_OK;
	CompileException.s = NULL;
}

/* set global error value and jump back to the exception handler */
static void throw_compile_exception(MRegexpError err, const char *s)
{
	CompileException.err = err;
	CompileException.s = s;
	longjmp(CompileException.buf, 1);
}

/* get required amount of memory in amount of nodes
 * to compile regular expressions */
static const size_t calc_compiled_len(const char *s)
{
	if (*s == 0) {
		return 1;
	} else {
		const uint32_t chr = utf8_peek(s);
		size_t ret = 0;
		s = utf8_next(s);

		switch (chr) {
		default:
			ret = 1;
			break;
		}

		return ret + calc_compiled_len(s);
	}
}

/* compile next node. returns address of next available node.
 * returns NULL if re is empty */
static RegexNode *compile_next(const char *re, const char **leftover,
			       RegexNode *prev, RegexNode *cur)
{
	if (*re == 0)
		return NULL;

	const uint32_t chr = utf8_peek(re);
	re = utf8_next(re);
	RegexNode *next = cur + 1;

	switch (chr) {
	case '^':
		cur->generic.match = anchor_begin_is_match;
		break;

	case '$':
		cur->generic.match = anchor_end_is_match;
		break;

	case '.':
		cur->generic.match = any_is_match;
		break;

	default:
		cur->chr.chr = chr;
		cur->generic.match = char_is_match;
		break;
	}

	cur->generic.next = NULL;
	cur->generic.prev = prev;
	prev->generic.next = cur;
	*leftover = re;

	return next;
}

/* compile raw regular expression into a linked list of nodes */
static RegexNode *compile(const char *re, RegexNode *nodes)
{
	RegexNode *prev = nodes;
	RegexNode *cur = nodes + 1;

	prev->generic.next = NULL;
	prev->generic.prev = NULL;
	prev->generic.match = start_is_match;

	const char *end = re + strlen(re);
	while (cur != NULL && re != NULL && re < end) {
		const char *next = NULL;
		RegexNode *next_node = compile_next(re, &next, prev, cur);

		prev = cur;
		cur = next_node;
		re = next;
	}

	return nodes;
}

struct MRegexp {
	RegexNode *nodes;
};

MRegexp *mregexp_compile(const char *re)
{
	clear_compile_exception();
	if (re == NULL) {
		CompileException.err = MREGEXP_INVALID_PARAMS;
		return NULL;
	}

	if (!utf8_valid(re)) {
		CompileException.err = MREGEXP_INVALID_UTF8;
		CompileException.s = NULL;
		return NULL;
	}

	MRegexp *ret = (MRegexp *)calloc(1, sizeof(MRegexp));

	if (ret == NULL) {
		CompileException.err = MREGEXP_FAILED_ALLOC;
		CompileException.s = NULL;
		return NULL;
	}

	RegexNode *nodes = NULL;

	if (setjmp(CompileException.buf)) {
		// Error callback
		free(ret);
		free(nodes);

		return NULL;
	}

	const size_t compile_len = calc_compiled_len(re);
	nodes = (RegexNode *)calloc(compile_len, sizeof(RegexNode));
	ret->nodes = compile(re, nodes);

	return ret;
}

MRegexpError mregexp_error(void)
{
	return CompileException.err;
}

bool mregexp_match(MRegexp *re, const char *s, MRegexpMatch *m)
{
	clear_compile_exception();

	if (re == NULL || s == NULL || m == NULL) {
		CompileException.err = MREGEXP_INVALID_PARAMS;
		return false;
	}

	m->match_begin = __SIZE_MAX__;
	m->match_end = __SIZE_MAX__;

	if (!utf8_valid(s)) {
		CompileException.err = MREGEXP_INVALID_UTF8;
		return false;
	}

	for (const char *tmp_s = s; *tmp_s; tmp_s = utf8_next(tmp_s)) {
		const char *next = NULL;
		if (is_match(re->nodes, s, tmp_s, &next)) {
			m->match_begin = tmp_s - s;
			m->match_end = next - s;
			return true;
		}
	}

	return false;
}

void mregexp_free(MRegexp *re)
{
	if (re == NULL) {
		CompileException.err = MREGEXP_INVALID_PARAMS;
		return;
	}
	free(re->nodes);
	free(re);
}