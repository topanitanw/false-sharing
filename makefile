SHELL=bash

CC:=gcc
CFLAGS:= -Wall -O2 -pthread

TARGETS:=false true

PROJ_ROOT=$(shell cd ..; git rev-parse --show-toplevel)
SCRIPT_DIR:=${PROJ_ROOT}/scripts
NTHREAD:=2
NSKIP:=0

all: $(TARGETS)
.PHONY: all

tests:
	for bin in $(TARGETS); do \
		./$${bin} -h; \
		echo "----------------------------------------------------------\n"; \
		./$${bin} -n 2 -a 10000 ; \
		echo "----------------------------------------------------------\n"; \
	done
.PHONY: tests

false: false.c
	$(CC) $(CFLAGS) -o $@ $<
.PHONY: false

original_false: original_false.c
	$(CC) $(CFLAGS) -o $@ $<
.PHONY: original_false

true: true.c
	$(CC) $(CFLAGS) -o $@ $<
.PHONY: true

eval:
	for bin in $(TARGETS); do \
		bash ${SCRIPT_DIR}/toplev_debug.sh "./$${bin} -n ${NTHREAD} -a 1000000 -l 64 -s ${NSKIP}" &> $${bin}.csv; \
		python ${SCRIPT_DIR}/read_perf.py -f $${bin}.csv; \
	done
.PHONY: eval

clean:
	@rm -f *.o $(TARGETS) *.csv
