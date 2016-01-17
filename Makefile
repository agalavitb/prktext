CFLAGS ?= -Wall -Wextra -pedantic -std=c89 -O2 -g3 -Wno-parentheses

OBJ := lib/bit.o lib/file.o lib/read.o lib/write.o

.c.o: prk.h
	cc $(CFLAGS) -Dbitput=prk_out -Dbitget=prk_in -I. -c $< -o $@

all: prktext

prktext: libprk.a prktext.c prk.h
	cc $(CFLAGS) -I. prktext.c -L. -lprk -lm -o prktext

libprk.a: $(OBJ)
	ar rcs libprk.a $(OBJ)

test: all
	@cc $(CFLAGS) -I. lib/test.c -L. -lprk -lm -o lib/prktest
	@lib/prktest ../parks/*.PRK || true
	@rm -f lib/prktest

clean:
	rm -rf prktext libprk.a $(OBJ)
