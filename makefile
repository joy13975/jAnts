EXE=jtruck
CC=g++
CFLAGS=-MMD -std=c++11 -O3 -fopenmp -g3
DEFS=
COMPILE=$(CC) $(CFLAGS) $(DEFS)
RUN_REAL_ARGS=

SRC_DIR:=src
OBJ_DIR=.obj
$(shell mkdir -p $(OBJ_DIR))
C_SRC := jtruck.c util.c
CC_SRC:= spec.cc route.cc solution.cc input_parser.cc jrand.cc score.cc basic_random_search.cc output_writer.cc
OBJS := $(C_SRC:%.c=$(OBJ_DIR)/%.o) $(CC_SRC:%.cc=$(OBJ_DIR)/%.o)
DEPS := $(C_SRC:%.c=$(OBJ_DIR)/%.d) $(CC_SRC:%.cc=$(OBJ_DIR)/%.d)

all: $(EXE)

EXTS=c cc
define make_rule
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.$1
	$$(COMPILE) -o $$@ -c $$<
endef
$(foreach EXT,$(EXTS),$(eval $(call make_rule,$(EXT))))

$(EXE): $(OBJS)
	$(COMPILE) $(LDFLAGS) $^ $(LDLIBS) -o $@

-include $(DEPS)

fresh: clean all

test: $(EXE)
	./$(EXE) $(ARGS)

run_real: $(EXE)
	./$(EXE) $(RUN_REAL_ARGS)

PHONY: clean

clean:
	rm -rf $(EXE) $(OBJ_DIR)/* *.dSYM *.gch
