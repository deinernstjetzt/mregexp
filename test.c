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

START_TEST(compile_match_quantifiers)
{
	MRegexp *re = mregexp_compile("ä+");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re, NULL);

	MRegexpMatch m;
	ck_assert(mregexp_match(re, "ääb", &m));
	ck_assert_uint_eq(m.match_begin, 0);
	ck_assert_uint_eq(m.match_end, 4);

	ck_assert(mregexp_match(re, "bäbb", &m));
	ck_assert_uint_eq(m.match_begin, 1);
	ck_assert_uint_eq(m.match_end, 3);

	ck_assert(!mregexp_match(re, "bbb", &m));

	mregexp_free(re);

	re = mregexp_compile("bä*");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re, NULL);

	ck_assert(mregexp_match(re, "bääb", &m));
	ck_assert_uint_eq(m.match_begin, 0);
	ck_assert_uint_eq(m.match_end, 5);

	ck_assert(mregexp_match(re, "bäbb", &m));
	ck_assert_uint_eq(m.match_begin, 0);
	ck_assert_uint_eq(m.match_end, 3);

	ck_assert(mregexp_match(re, "bbb", &m));
	ck_assert_uint_eq(m.match_begin, 0);
	ck_assert_uint_eq(m.match_end, 1);

	mregexp_free(re);
}
END_TEST

START_TEST(compile_match_complex_quants)
{
	MRegexp *re1 = mregexp_compile("ä{1,3}");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re1, NULL);

	MRegexp *re2 = mregexp_compile("ä{1}");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re2, NULL);

	MRegexp *re3 = mregexp_compile("ä{,}");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re3, NULL);

	MRegexp *re4 = mregexp_compile("ä{,3}");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re4, NULL);

	MRegexpMatch m;
	ck_assert(mregexp_match(re1, "ääb", &m));
	ck_assert_uint_eq(m.match_begin, 0);
	ck_assert_uint_eq(m.match_end, 4);
	ck_assert(mregexp_match(re1, "äääb", &m));
	ck_assert(mregexp_match(re1, "äb", &m));
	ck_assert(!mregexp_match(re1, "b", &m));

	ck_assert(mregexp_match(re2, "ää", &m));
	ck_assert_uint_eq(m.match_begin, 0);
	ck_assert_uint_eq(m.match_end, 2);
	ck_assert(mregexp_match(re2, "bbäb", &m));
	ck_assert(!mregexp_match(re2, "bbbb", &m));

	ck_assert(mregexp_match(re3, "ääääääääääb", &m));
	ck_assert_uint_eq(m.match_begin, 0);
	ck_assert_uint_eq(m.match_end, 20);
	ck_assert(mregexp_match(re3, "b", &m));

	ck_assert(mregexp_match(re4, "bä", &m));
	ck_assert(mregexp_match(re4, "bää", &m));
	ck_assert(mregexp_match(re4, "bäää", &m));

	mregexp_free(re1);
	mregexp_free(re2);
	mregexp_free(re3);
	mregexp_free(re4);
}
END_TEST

START_TEST(compile_match_escaped_chars)
{
	MRegexp *re = mregexp_compile("\\n\\r\\t\\{");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re, NULL);

	MRegexpMatch m;
	ck_assert(mregexp_match(re, "\n\r\t{", &m));
	ck_assert(!mregexp_match(re, "\n\r\t", &m));

	mregexp_free(re);
}
END_TEST

START_TEST(compile_match_class_simple)
{
	MRegexp *re1 = mregexp_compile("\\s");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re1, NULL);
	MRegexp *re2 = mregexp_compile("\\w");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re2, NULL);
	MRegexp *re3 = mregexp_compile("\\D");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re3, NULL);

	MRegexpMatch m;
	ck_assert(mregexp_match(re1, " ", &m));
	ck_assert(mregexp_match(re1, "\r", &m));
	ck_assert(mregexp_match(re1, "\n", &m));

	ck_assert(mregexp_match(re2, "a", &m));
	ck_assert(mregexp_match(re2, "0", &m));
	ck_assert(mregexp_match(re2, "_", &m));
	
	ck_assert(mregexp_match(re3, "k", &m));
	ck_assert(!mregexp_match(re3, "0", &m));

	mregexp_free(re1);
	mregexp_free(re2);
	mregexp_free(re3);
}
END_TEST

START_TEST(compile_match_class_complex_0)
{
	MRegexp *re = mregexp_compile("[asdf]");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re, NULL);

	MRegexpMatch m;
	ck_assert(mregexp_match(re, "a", &m));
	ck_assert(mregexp_match(re, "s", &m));
	ck_assert(mregexp_match(re, "d", &m));
	ck_assert(mregexp_match(re, "f", &m));

	mregexp_free(re);
}
END_TEST

START_TEST(compile_match_class_complex_1)
{
	MRegexp *re = mregexp_compile("[a-zä0-9öA-Z]");
	ck_assert_int_eq(mregexp_error(), MREGEXP_OK);
	ck_assert_ptr_ne(re, NULL);

	MRegexpMatch m;
	ck_assert(mregexp_match(re, "a", &m));
	ck_assert(mregexp_match(re, "5", &m));
	ck_assert(mregexp_match(re, "A", &m));
	ck_assert(mregexp_match(re, "ä", &m));
	ck_assert(mregexp_match(re, "ö", &m));

	mregexp_free(re);
}
END_TEST

START_TEST(invalid_quantifier)
{
	ck_assert_ptr_eq(mregexp_compile("+"), NULL);
	ck_assert_int_eq(mregexp_error(), MREGEXP_EARLY_QUANTIFIER);
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
	tcase_add_test(tcase, compile_match_quantifiers);
	tcase_add_test(tcase, invalid_quantifier);
	tcase_add_test(tcase, compile_match_complex_quants);
	tcase_add_test(tcase, compile_match_escaped_chars);
	tcase_add_test(tcase, compile_match_class_simple);
	tcase_add_test(tcase, compile_match_class_complex_0);
	tcase_add_test(tcase, compile_match_class_complex_1);

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