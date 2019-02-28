CC:=gcc
CFLAGS:= -Wall -O2 -pthread

TARGET:=false

all: $(TARGET)

false: false.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	@rm -f *.o $(TARGET)
