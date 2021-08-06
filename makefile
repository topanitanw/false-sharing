CC:=gcc
CFLAGS:= -Wall -O2 -pthread

TARGETS:=false true

all: $(TARGETS)

false: false.c
	$(CC) $(CFLAGS) -o $@ $<

true: true.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	@rm -f *.o $(TARGETS)
