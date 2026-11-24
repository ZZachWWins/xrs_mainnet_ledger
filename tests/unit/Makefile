CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -fsanitize=address,undefined
LDFLAGS = -fsanitize=address,undefined

.PHONY: all run clean

all: test_runner

test_runner: test_parser.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

run: test_runner
	./test_runner

clean:
	rm -f test_runner *.o
