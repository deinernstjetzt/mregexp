#include "mregexp.h"

#include <assert.h>

int main(void) {
  const char* reg_a_s = "Hallo";
  const char* reg_b_s = "Welt";
  const char* reg_c_s = "Bye";

  mregexp_error err = MREGEXP_OK;

  mregexp* reg_a = mregexp_compile(0, reg_a_s, &err);
  assert(reg_a != NULL);

  mregexp* reg_b = mregexp_compile(0, reg_b_s, &err);
  assert(reg_b != NULL);

  mregexp* reg_c = mregexp_compile(0, reg_c_s, &err);
  assert(reg_c != NULL);

  const char* haystack = "Hallo Welt";

  mregexp_match match_a;
  mregexp_match match_b;
  mregexp_match match_c;

  assert(mregexp_first_match(reg_a, 0, haystack, &match_a));
  assert(mregexp_first_match(reg_b, 0, haystack, &match_b));
  assert(!mregexp_first_match(reg_c, 0, haystack, &match_c));

  assert(match_a.pos == 0 && match_a.len == 5);
  assert(match_b.pos == 6 && match_b.len == 4);

  return 0;
}
