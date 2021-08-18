SHELL=bash

CC:=gcc
CFLAGS:= -Wall -O2 -pthread

TARGETS:=false true

PROJ_ROOT=$(shell cd ..; git rev-parse --show-toplevel)
SCRIPT_DIR:=${PROJ_ROOT}/scripts

all: $(TARGETS)
.PHONY: all

tests:
	./false -h
	@echo "-------------------------------------------------------------\n"
	./false -n 2 -a 10000 -l 64 -s 10000
	@echo "-------------------------------------------------------------\n"
	./true -h
	@echo "-------------------------------------------------------------\n"
	./true -n 2 -a 10000 -l 64 -s 10000
	@echo "-------------------------------------------------------------\n"
.PHONY: tests

false: false.c
	$(CC) $(CFLAGS) -o $@ $<
.PHONY: false

true: true.c
	$(CC) $(CFLAGS) -o $@ $<
.PHONY: true

eval:
	for bin in $(TARGETS); do \
		bash ${SCRIPT_DIR}/toplev_debug.sh "./$${bin} -n 2 -a 1000000 -l 64 -s 2" &> $${bin}.csv; \
		python ${SCRIPT_DIR}/read_perf.py -f $${bin}.csv; \
	done
.PHONY: eval

clean:
	@rm -f *.o $(TARGETS) *.csv
