#include <stdio.h>
#include <stdlib.h>
#include <check.h>

#include "mregexp.h"

START_TEST(compile_match_char)
{
	MRegexp *re = mregexp_compile("äsdf");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re, NULL);

	MRegexpMatch m;
	ck_assert(mregexp_match(re, "äsdf", &m));
	ck_assert_uint_eq(m.match_begin, 0);
	ck_assert_uint_eq(m.match_end, 5); // ä is two bytes wide

	ck_assert(mregexp_match(re, "zäsdf", &m));
	ck_assert_uint_eq(m.match_begin, 1);
	ck_assert_uint_eq(m.match_end, 6);

	ck_assert(mregexp_match(re, "äsdf", &m));
	ck_assert_uint_eq(m.match_begin, 0);
	ck_assert_uint_eq(m.match_end, 5);

	mregexp_free(re);
}
END_TEST

START_TEST(compile_match_anchors)
{
	MRegexp *re = mregexp_compile("^äs.f$");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re, NULL);

	MRegexpMatch m;
	ck_assert(mregexp_match(re, "äsdf", &m));
	ck_assert_uint_eq(m.match_begin, 0);
	ck_assert_uint_eq(m.match_end, 5);

	ck_assert(mregexp_match(re, "äs♥f", &m));
	ck_assert(mregexp_match(re, "äsöf", &m));

	mregexp_free(re);
}
END_TEST

/* Test that invalid parameters do not cause a
 * segmentation fault */
START_TEST(invalid_params)
{
	mregexp_compile(NULL);
	ck_assert_int_ne(mregexp_error(), MREGEXP_OK);
	mregexp_match(NULL, NULL, NULL);
	ck_assert_int_ne(mregexp_error(), MREGEXP_OK);
	mregexp_free(NULL);
	ck_assert_int_ne(mregexp_error(), MREGEXP_OK);
}
END_TEST

/* Test that invalid utf8 sequences cause a
 * MREGEXP_INVALID_UTF8 error */
START_TEST(invalid_utf8)
{
	char s1[3] = "ä";
	s1[1] = 65; // invalid continuation byte

	ck_assert_ptr_eq(mregexp_compile(s1), NULL);
	ck_assert_int_eq(mregexp_error(), MREGEXP_INVALID_UTF8);

	MRegexp *re = mregexp_compile("asdf"); // valid regular expression
	MRegexpMatch m;
	ck_assert(!mregexp_match(re, s1, &m));
	ck_assert_int_eq(mregexp_error(), MREGEXP_INVALID_UTF8);

	mregexp_free(re);
}
END_TEST

Suite *mregexp_test_suite(void)
{
	Suite *ret = suite_create("mregexp");
	TCase *tcase = tcase_create("mregexp");

	tcase_add_test(tcase, compile_match_char);
	tcase_add_test(tcase, invalid_params);
	tcase_add_test(tcase, invalid_utf8);
	tcase_add_test(tcase, compile_match_anchors);

	suite_add_tcase(ret, tcase);
	return ret;
}

int main(void)
{
	SRunner *sr = srunner_create(mregexp_test_suite());
	srunner_run_all(sr, CK_NORMAL);
	int fails = srunner_ntests_failed(sr);
	srunner_free(sr);
	return fails ? EXIT_FAILURE : EXIT_SUCCESS;
}