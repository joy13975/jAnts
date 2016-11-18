EXE=jtruck
CC=gcc
CFLAGS=-O3 -fopenmp
DEFS=
COMPILE=$(CC) $(CFLAGS) $(DEFS)
RUN_REAL_ARGS=

all: $(EXE)

%.o: %.c %.h
	$(COMPILE) -c $^

$(EXE): $(EXE).c input_parser.o util.o ga.o
	$(COMPILE) $^ -o $@

fresh: clean all

test: $(EXE)
	./$(EXE) $(ARGS)

copyutil: util.h util.c
	cp util.h ~/src/jproc
	cp util.c ~/src/jproc
	cp util.h ~/src/jutil
	cp util.c ~/src/jutil

run_real: $(EXE)
	./$(EXE) $(RUN_REAL_ARGS)

PHONY: clean

clean:
	rm -rf $(EXE) *.o *.dSYM *.gch