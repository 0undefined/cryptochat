CC=gcc
SRC:=$(wildcard src/*.c)
OBJ:=$(addprefix obj/,$(notdir $(SRC:.c=.o)))
LDFLAGS=-lssl -lcrypto -lpthread -lncurses
CCFLAGS=-Wall -Werror -Wextra -pedantic -c
DFLAGS=-DDEBUG -g -rdynamic -finstrument-functions
RFLAGS=-O2
OUT=cchat

BUILD_DIRS=bin obj

.PHONY: build_dirs clean test

# default adds compile flags
default: CCFLAGS += $(DFLAGS)
default: build_dirs compile
	@echo Debug Build
	@bin/$(OUT) -v

# release build, without debug flags and with optimizations
release: CCFLAGS += $(RFLAGS)
release: build_dirs compile
	@echo Release Build

compile: $(OBJ)
	$(CC) $(LDFLAGS) -o bin/$(OUT) $^

obj/%.o: src/%.c
	@echo Building debug $<
	$(CC) $(CCFLAGS) -o $@ $<

debug:
	gdb bin/$(OUT)

clean:
	@echo Cleaning up
	rm -f obj/*.o

clean-all: clean
	rm -f bin/*

build_dirs:
	@mkdir -p ${BUILD_DIRS}
