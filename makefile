EXE=jsolve
CC=gcc
CFLAGS=-O3 -fopenmp
DEFS=
COMPILE=$(CC) $(CFLAGS) $(DEFS)

all: $(EXE)

%.o: %.c %.h
	$(COMPILE) -c $^

$(EXE): $(EXE).c input_parser.o util.o
	$(COMPILE) $^ -o $@

fresh: clean all

test: $(EXE)
	./$(EXE) $(ARGS)

copyutil: util.h util.c
	cp util.h ~/src/jproc
	cp util.c ~/src/jproc
	cp util.h ~/src/jutil
	cp util.c ~/src/jutil

PHONY: clean

clean:
	rm -rf $(EXE) *.o *.dSYM *.gch