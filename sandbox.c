#include <stdio.h>
#include <stdlib.h>

#include "mregexp.h"

char *readline(const char *prompt)
{
	printf("%s", prompt);
	char *ret = NULL;
	size_t retsz = 0;

	do {
		if (retsz > 0 && ret[retsz - 1] == '\n') {
			ret[retsz - 1] = 0;
			break;
		}
		ret = realloc(ret, (++retsz));
	} while (fread(ret + retsz - 1, 1, 1, stdin));

	return ret;
}

int main(void)
{
	char *raw_re = readline("Enter regular expression > ");
	char *text = readline("Enter text > ");

	MRegexp *re = mregexp_compile(raw_re);

	if (mregexp_error() || re == NULL) {
		printf("Invalid regular expression: Compile failed with error %d\n",
		       mregexp_error());
        return EXIT_FAILURE;
	}

    MRegexpMatch m;
    if (mregexp_match(re, text, &m)) {
        fwrite(text, 1, m.match_begin, stdout);
        printf("%c[31;1m", 27);
        fwrite(text + m.match_begin, 1, m.match_end - m.match_begin, stdout);
        printf("%c[0m", 27);
        printf("%s\n", text + m.match_end);
    } else {
        puts("No match :c");
    }

	return 0;
}