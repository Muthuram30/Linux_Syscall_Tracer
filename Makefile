CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -g -O0
CFLAGS  += -Iinclude
PREFIX  ?= $(HOME)/.local
SRC      = src/main.c src/syscall_tracer.c src/process_monitor.c src/logger.c src/syscall_table.c
OBJ      = $(SRC:src/%.c=build/%.o)
BIN      = build/syscall-tracer

.PHONY: all clean test run install help dashboard demo

all: $(BIN) build/test_program   ## build tracer + test program

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: src/%.c $(wildcard include/*.h) include/syscall_list.inc | build
	$(CC) $(CFLAGS) -c $< -o $@

build/test_program: tests/test_program.c | build
	$(CC) $(CFLAGS) -o $@ $<

build:
	mkdir -p build logs

test: all                        ## run automated tests
	bash tests/run_tests.sh

run: all                         ## trace the test program
	./$(BIN) --run ./build/test_program

dashboard:                       ## start the web dashboard on http://127.0.0.1:5000
	python3 dashboard/app.py

install: all                     ## install to $(PREFIX)/bin (no root needed)
	install -d $(PREFIX)/bin
	install -m 755 $(BIN) $(PREFIX)/bin/

clean:                           ## remove build output (logs are kept)
	rm -rf build

help:
	@echo "make            build everything"
	@echo "make test       run the test-suite (compares with strace if installed)"
	@echo "make run        trace ./build/test_program"
	@echo "make dashboard  start the dashboard (python3 + flask)"
	@echo "make install    install to $(PREFIX)/bin"
	@echo "make clean      delete build/"
