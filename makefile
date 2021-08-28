SHELL=bash

CC:=gcc
CFLAGS:= -Wall -O2 -pthread
OBJDUMP:=objdump

TARGETS:=false non-sharing original_false

PROJ_ROOT=$(shell cd ..; git rev-parse --show-toplevel)
SCRIPT_DIR:=${PROJ_ROOT}/scripts
NTHREAD:=2
NSKIP:=1

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
	$(OBJDUMP) -d $@ > $@.obj

original_false: original_false.c
	$(CC) $(CFLAGS) -o $@ $<
.PHONY: original_false

non-sharing: non-sharing.c
	$(CC) $(CFLAGS) -o $@ $<
	$(OBJDUMP) -d $@ > $@.obj

# -l 64 -s ${NSKIP}"
eval:
	for bin in $(TARGETS); do \
		bash ${SCRIPT_DIR}/toplev_debug.sh \
			"./$${bin} -n ${NTHREAD} -a $$[10**5]" \
			&> $${bin}.csv; \
		python ${SCRIPT_DIR}/read_perf.py -f $${bin}.csv; \
	done
.PHONY: eval

clean:
	@rm -f *.{o,obj} $(TARGETS) *.csv
