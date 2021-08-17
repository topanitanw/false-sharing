CC:=gcc
CFLAGS:= -Wall -O2 -pthread

TARGETS:=false true

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

clean:
	@rm -f *.o $(TARGETS)
