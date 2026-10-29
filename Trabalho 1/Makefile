CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -g
SRC     = src
BUILD   = build

# Módulos compartilhados entre montador e ligador
COMMON  = opcode_table lexer symbol_table assembler errors object_writer
COMMON_OBJS = $(patsubst %,$(BUILD)/%.o,$(COMMON))

.PHONY: all clean test

all: montador ligador

montador: $(BUILD)/main_montador.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

ligador: $(BUILD)/main_ligador.o $(BUILD)/linker.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD):
	mkdir -p $@

test: all
	./tests/run_tests.sh

clean:
	rm -rf $(BUILD) montador ligador

-include $(wildcard $(BUILD)/*.d)
