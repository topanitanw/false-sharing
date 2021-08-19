SHELL=bash

CC:=gcc
CFLAGS:= -Wall -O3 -pthread
OBJDUMP:=objdump

TARGETS:=false # true original_false

PROJ_ROOT=$(shell cd ..; git rev-parse --show-toplevel)
SCRIPT_DIR:=${PROJ_ROOT}/scripts
NTHREAD:=4
NSKIP:=1

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
	$(OBJDUMP) -d $@ > $@.obj

original_false: original_false.c
	$(CC) $(CFLAGS) -o $@ $<
	$(OBJDUMP) -d $@ > $@.obj

true: true.c
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
