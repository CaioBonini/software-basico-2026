CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -g
SRC     = src
BUILD   = build

# Módulos compartilhados entre montador e ligador
COMMON  = opcode_table lexer symbol_table assembler errors object_writer
COMMON_OBJS = $(patsubst %,$(BUILD)/%.o,$(COMMON))

.PHONY: all clean test unit

all: montador ligador

montador: $(BUILD)/main_montador.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

ligador: $(BUILD)/main_ligador.o $(BUILD)/linker.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD):
	mkdir -p $@

# Testes unitários isolados (tests/unit/), sem lexer/montador
UNIT_TESTS = $(BUILD)/test_tables $(BUILD)/test_lexer $(BUILD)/test_assembler

$(BUILD)/test_tables: tests/unit/test_tables.c $(BUILD)/opcode_table.o $(BUILD)/symbol_table.o | $(BUILD)
	$(CC) $(CFLAGS) -I$(SRC) -o $@ $^

$(BUILD)/test_lexer: tests/unit/test_lexer.c $(BUILD)/lexer.o $(BUILD)/errors.o | $(BUILD)
	$(CC) $(CFLAGS) -I$(SRC) -o $@ $^

$(BUILD)/test_assembler: tests/unit/test_assembler.c $(COMMON_OBJS) | $(BUILD)
	$(CC) $(CFLAGS) -I$(SRC) -o $@ $^

unit: $(UNIT_TESTS)
	@for t in $(UNIT_TESTS); do ./$$t || exit 1; done

test: unit all
	./tests/run_tests.sh

clean:
	rm -rf $(BUILD) montador ligador

-include $(wildcard $(BUILD)/*.d)
