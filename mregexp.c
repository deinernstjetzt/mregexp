#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <setjmp.h>
#include <stdarg.h>

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

typedef struct {
	GenericNode generic;
	union RegexNode *subexp;
	size_t min, max;
} QuantNode;

typedef struct {
	GenericNode generic;
	uint32_t first, last;
} RangeNode;

typedef struct {
	GenericNode generic;
	RangeNode *ranges;
	bool negate;
} ClassNode;

typedef union RegexNode {
	GenericNode generic;
	CharNode chr;
	QuantNode quant;
	ClassNode cls;
	RangeNode range;
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

static bool quant_is_match(RegexNode *node, const char *orig, const char *cur,
			   const char **next)
{
	QuantNode *quant = (QuantNode *)node;
	size_t matches = 0;

	while (is_match(quant->subexp, orig, cur, next)) {
		matches++;
		cur = *next;

		if (matches >= quant->max)
			break;
	}

	*next = cur;
	return matches >= quant->min;
}

static bool class_is_match(RegexNode *node, const char *orig, const char *cur,
			   const char **next)
{
	ClassNode *cls = (ClassNode *)node;

	if (*cur == 0)
		return false;

	const uint32_t chr = utf8_peek(cur);
	*next = utf8_next(cur);

	bool found = false;
	for (RangeNode *range = cls->ranges; range != NULL;
	     range = (RangeNode *)range->generic.next) {
		if (chr >= range->first && chr <= range->last) {
			found = true;
			break;
		}
	}

	if (cls->negate)
		found = !found;

	return found;
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

static size_t calc_compiled_escaped_len(const char *s, const char **leftover)
{
	if (*s == 0)
		throw_compile_exception(MREGEXP_UNEXPECTED_EOL, s);

	const uint32_t chr = utf8_peek(s);
	*leftover = utf8_next(s);

	switch (chr) {
	case 's':
		return 5;

	case 'S':
		return 5;

	case 'd':
		return 2;

	case 'D':
		return 2;

	case 'w':
		return 5;

	case 'W':
		return 5;

	default:
		return 1;
	}
}

static const size_t calc_compiled_class_len(const char *s,
					    const char **leftover)
{
	if (*s == '^')
		s++;
	
	size_t ret = 1;

	while (*s && *s != ']') {
		uint32_t chr = utf8_peek(s);
		s = utf8_next(s);
		if (chr == '\\') {
			s = utf8_next(s);
		}

		if (*s == '-' && s[1] != ']') {
			s++;
			chr = utf8_peek(s);
			s = utf8_next(s);

			if (chr == '\\')
				s = utf8_next(s);
		}

		ret++;
	}

	if (*s == ']') {
		s++;
		*leftover = s;
	} else {
		throw_compile_exception(MREGEXP_INVALID_COMPLEX_CLASS, s);
	}

	return ret;
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
		case '{': {
			const char *end = strstr(s, "}");

			if (end == NULL)
				throw_compile_exception(
					MREGEXP_INVALID_COMPLEX_QUANT, s);

			s = end + 1;
			ret = 1;
			break;
		}

		case '\\':
			ret = calc_compiled_escaped_len(s, &s);
			break;

		case '[':
			ret = calc_compiled_class_len(s, &s);
			break;

		default:
			ret = 1;
			break;
		}

		return ret + calc_compiled_len(s);
	}
}

static void append_quant(RegexNode **prev, RegexNode *cur, size_t min,
			 size_t max, const char *re)
{
	cur->generic.match = quant_is_match;
	cur->generic.next = NULL;
	cur->generic.prev = NULL;

	cur->quant.max = max;
	cur->quant.min = min;
	cur->quant.subexp = *prev;

	*prev = (*prev)->generic.prev;
	if (*prev == NULL)
		throw_compile_exception(MREGEXP_EARLY_QUANTIFIER, re);

	cur->quant.subexp->generic.next = NULL;
	cur->quant.subexp->generic.prev = NULL;
}

static inline bool is_digit(uint32_t c)
{
	return c >= '0' && c <= '9';
}

static inline size_t parse_digit(const char *s, const char **leftover)
{
	size_t ret = 0;

	while (*s) {
		uint32_t chr = utf8_peek(s);

		if (is_digit(chr)) {
			ret *= 10;
			ret += chr - '0';
			s = utf8_next(s);
		} else {
			break;
		}
	}

	*leftover = s;
	return ret;
}

/* parse complex quantifier of format {m,n} 
 * valid formats: {,} {m,} {,n} {m} {m,n} */
static void parse_complex_quant(const char *re, const char **leftover,
				size_t *min_p, size_t *max_p)
{
	if (*re == 0)
		throw_compile_exception(MREGEXP_INVALID_COMPLEX_QUANT, re);

	uint32_t tmp = utf8_peek(re);
	size_t min = 0, max = __SIZE_MAX__;

	if (is_digit(tmp)) {
		min = parse_digit(re, &re);
	} else if (tmp != ',') {
		throw_compile_exception(MREGEXP_INVALID_COMPLEX_QUANT, re);
	}

	tmp = utf8_peek(re);

	if (tmp == ',') {
		re = utf8_next(re);
		if (is_digit(utf8_peek(re)))
			max = parse_digit(re, &re);
		else
			max = __SIZE_MAX__;
	} else {
		max = min;
	}

	tmp = utf8_peek(re);
	if (tmp == '}') {
		*leftover = re + 1;
		*min_p = min;
		*max_p = max;
	} else {
		throw_compile_exception(MREGEXP_INVALID_COMPLEX_QUANT, re);
	}
}

/* append character class to linked list of nodes with
 * ranges given as optional arguments. Returns pointer
 * to next */
static RegexNode *append_class(RegexNode *cur, bool negate, size_t n, ...)
{
	cur->cls.negate = negate;
	cur->cls.ranges = (RangeNode *)(n ? cur + 1 : NULL);
	cur->generic.match = class_is_match;
	cur->generic.next = NULL;
	cur->generic.prev = NULL;

	va_list ap;
	va_start(ap, n);
	RegexNode *prev = NULL;
	cur = cur + 1;

	for (size_t i = 0; i < n; ++i) {
		const uint32_t first = va_arg(ap, uint32_t);
		const uint32_t last = va_arg(ap, uint32_t);

		cur->generic.next = NULL;
		cur->generic.prev = prev;

		if (prev)
			prev->generic.next = cur;

		cur->range.first = first;
		cur->range.last = last;

		prev = cur;
		cur = cur + 1;
	}

	va_end(ap);

	return cur + 1 + n;
}

/** compile escaped characters. return pointer to the next free node. */
static RegexNode *compile_next_escaped(const char *re, const char **leftover,
				       RegexNode *cur)
{
	if (*re == 0)
		throw_compile_exception(MREGEXP_UNEXPECTED_EOL, re);

	const uint32_t chr = utf8_peek(re);
	*leftover = utf8_next(re);
	RegexNode *ret = cur + 1;

	switch (chr) {
	case 'n':
		cur->chr.chr = '\n';
		cur->generic.match = char_is_match;
		break;

	case 't':
		cur->chr.chr = '\t';
		cur->generic.match = char_is_match;
		break;

	case 'r':
		cur->chr.chr = '\r';
		cur->generic.match = char_is_match;
		break;

	case 's':
		ret = append_class(cur, false, 4, ' ', ' ', '\t', '\t', '\r',
				   '\r', '\n', '\n');
		break;

	case 'S':
		ret = append_class(cur, true, 4, ' ', ' ', '\t', '\t', '\r',
				   '\r', '\n', '\n');
		break;

	case 'w':
		ret = append_class(cur, false, 4, 'a', 'z', 'A', 'Z', '0', '9',
				   '_', '_');
		break;

	case 'W':
		ret = append_class(cur, true, 4, 'a', 'z', 'A', 'Z', '0', '9',
				   '_', '_');
		break;

	case 'd':
		ret = append_class(cur, false, 1, '0', '9');
		break;

	case 'D':
		ret = append_class(cur, true, 1, '0', '9');
		break;

	default:
		cur->chr.chr = chr;
		cur->generic.match = char_is_match;
		break;
	}

	return ret;
}

static RegexNode *compile_next_complex_class(const char *re,
	const char **leftover, RegexNode *cur)
{
	cur->generic.match = class_is_match;
	cur->generic.next = NULL;
	cur->generic.prev = NULL;
	
	if (*re == '^') {
		re++;
		cur->cls.negate = true;
	} else {
		cur->cls.negate = false;
	}

	cur->cls.ranges = NULL;

	cur = cur + 1;
	RegexNode *prev = NULL;

	while (*re && *re != ']') {
		uint32_t first = 0, last = 0;

		first = utf8_peek(re);
		re = utf8_next(re);
		if (first == '\\') {
			if (*re == 0)
				throw_compile_exception(MREGEXP_INVALID_COMPLEX_CLASS, re);
			
			first = utf8_peek(re);
			re = utf8_next(re);
		}

		if (*re == '-' && re[1] != ']' && re[1]) {
			re++;
			last = utf8_peek(re);
			re = utf8_next(re);

			if (last == '\\') {
				if (*re == 0)
					throw_compile_exception(MREGEXP_INVALID_COMPLEX_CLASS, re);
				
				last = utf8_peek(re);
				re = utf8_next(re);
			}
		} else {
			last = first;
		}

		cur->range.first = first;
		cur->range.last = last;
		cur->generic.prev = prev;
		cur->generic.next = NULL;

		if (prev == NULL) {
			(cur - 1)->cls.ranges = (RangeNode *) cur;
		} else {
			prev->generic.next = cur;
		}

		prev = cur;
		cur++;
	}

	if (*re == ']') {
		*leftover = re + 1;
		return cur;
	} else {
		throw_compile_exception(MREGEXP_INVALID_COMPLEX_CLASS, re);
		return NULL; // Unreachable
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

	case '*':
		append_quant(&prev, cur, 0, __SIZE_MAX__, re);
		break;

	case '+':
		append_quant(&prev, cur, 1, __SIZE_MAX__, re);
		break;

	case '{': {
		size_t min = 0, max = __SIZE_MAX__;
		const char *leftover = NULL;
		parse_complex_quant(re, &leftover, &min, &max);

		append_quant(&prev, cur, min, max, re);
		re = leftover;
		break;
	}

	case '[':
		next = compile_next_complex_class(re, &re, cur);
		break;

	case '\\':
		next = compile_next_escaped(re, &re, cur);
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