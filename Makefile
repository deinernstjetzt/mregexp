CC=cc
CC_FLAGS=-std=c18 -Wall -Wpedantic -g

mregexp.o: mregexp.c
	$(CC) $(CC_FLAGS) -c -o $@ $^

test: test.c mregexp.o
	$(CC) $(CC_FLAGS) -lcheck -o $@ $^
	./test
	rm -f test

sandbox: sandbox.c mregexp.o
	$(CC) $(CC_FLAGS) -o $@ $^

clean:
	rm -f test
	rm -f mregexp.o
	rm -f sandbox