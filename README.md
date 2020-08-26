# mregexp - small regex library written for C/C++
mregexp is a small utf8 compatible regex library consisting of only two files written for C99/C++11.
## Usage
### Compiling a regular expression
```c
MRegexp *re = mregexp_compile("[0-9]+");
```
If an error occurs ```mregexp_compile``` returns NULL. To get the specific error code use ```mregexp_error```. Error values and their meaning can be found in ```mregexp.h.```
### Getting the first match
```c
// Lets find the first sequence of digits in a string
const char *s = "hello 12345 world";
MRegexp *re = mregexp_compile("\\d");

MRegexpMatch m;
if (mregexp_match(re, s, &m)) {
    printf("Found digits at position %lu\n", m.match_begin);
} else {
    printf("Could not find any digits\n");
}

// Compiled regular expressions are stored on the heap
// and must be freed
mregexp_free(re);
```
The ```MRegexpMatch``` type looks somewhat like this:
```c
typedef struct {
	size_t match_begin;
	size_t match_end;
} MRegexpMatch;
```
The ```match_begin``` field represents a byte offset in the matched string to the first occurence of a pattern, so that ```s + m.match_begin``` points to the beginning of the match. ```match_end``` is a byte offset in the matched string to the first byte which did not match the pattern.

## Using mregexp in a project
First of all, mregexp is still in a very early stage of development.

To use mregexp you will need two files: ```mregexp.c``` and ```mregexp.h```. Include ```mregexp.h``` wherever you wish to use it. ```mregexp.c``` can be compiled independently into an object file and then be linked with your project.
### Running the tests
mregexp comes with a few tests to ensure that changes won't break anything. To run the tests you'll need [libcheck](https://libcheck.github.io/check/). Then just run
```bash
make test
```
## Regex Cheatsheet
| Metacharacter | Description |
|:--:|:--:|
| c | Most characters (like c) match themselve literally |
| \c | Some characters are used as metacharacters. To use them literally escape them |
| \n \t \r | newline, tab, carriage return |
| \d \s \w | digit, whitespace, alphanumeric character (a-z, A-Z, 0-9 and _) |
| \D \S \W | do not match the groups described above |
| . | Matches any character (including newline) |
| * | Matches the preceding token as often as possible |
| + | Matches the preceding token at least once and as often as possible |
| {m,n} | Matches the preceding token at least m times and at most n times. m and n may be ommited to ignore the min or max value. |
| (c) | Matches the expression inside the parentheses. |
| [c] | Matches all characters inside the brackets. Ranges like a-z may also be used |
| [^c] | Does not match the characters inside the bracket. |