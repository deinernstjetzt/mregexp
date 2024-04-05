#ifndef MREGEXP_H
#define MREGEXP_H

#include <stdbool.h>
#include <stddef.h>

/** opaque pointer to a dfa generated from a regular expression */
typedef struct mregexp mregexp;

/** single match found within a string */
typedef struct mregexp_match {
  size_t pos;
  size_t len;
} mregexp_match;

/** error values passed to callers via the last function arguments */
typedef enum mregexp_error { MREGEXP_OK } mregexp_error;

/**
 * @brief compile a regular expression into an equivalent dfa
 * @param n length of the regular expression in bytes or 0 to use strlen()
 * @param regex regular expression string which is at least n-bytes large or null-terminated
 * @param out_error position to write an error value to if compilation fails
 * @return heap-allocated dfa that can be deallocated using free() or NULL on failure
 */
mregexp* mregexp_compile(size_t n, const char* regex, mregexp_error* out_error);

/**
 * @brief find the first matching substring within the haystack
 * @param regex dfa representing the search pattern
 * @param n length of the haystack in bytes or 0 to use strlen()
 * @param haystack string to find a substring within
 * @param out_match position to write the position and length of the first matching substring to
 * @return true if any match was found
 */
bool mregexp_first_match(const mregexp* regex, size_t n, const char* haystack,
                         mregexp_match* out_match);

#endif
