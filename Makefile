CC     ?= clang
CFLAGS += -std=c11 -pedantic -Wall -Wextra -O3

CFILES  = $(shell find -type f -name '*.c')
OFILES  = $(subst .c,.o,$(CFILES))

ponzi: $(OFILES)
	$(CC) $(OFILES) -o $@

$(OFILES): src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -Isrc/inc

clean:
	rm src/*.o
