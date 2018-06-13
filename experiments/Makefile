CC = gcc

INSTALL_PATH = /usr/
INCLUDES = -I../platform/include -I$(INSTALL_PATH)/include
DEFS = -L$(INSTALL_PATH)/lib
LDADD = -lm -lsimgrid

SRC=$(wildcard *.c)
BIN=$(SRC:.c=.bin)

all: $(BIN)

%.bin: %.c ../platform/libmrsg.a
	$(CC) $(INCLUDES) $(DEFS) -o $@ $^ $(LDADD)

../platform/libmrsg.a:
	make -C ../platform/

clean:
	rm -vf *.bin *.csv *.trace *.plist

.PHONY: clean
