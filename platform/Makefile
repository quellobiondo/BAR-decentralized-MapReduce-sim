CC = gcc
CFLAGS = -Wall -g3

# INSTALL_PATH = $$HOME/simgrid
INSTALL_PATH = /usr
INCLUDES = -Iinclude -I$(INSTALL_PATH)/include
DEFS = -L$(INSTALL_PATH)/lib
LDADD = -lm -lsimgrid

BIN = libhadoopbft.a
OBJ = bin/common.o bin/simcore.o bin/dfs.o bin/master.o bin/worker.o bin/user.o bin/scheduling.o

all: $(BIN)

$(BIN): $(OBJ)
	ar rcs $(BIN) $(OBJ)
#	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $(LDADD) -o $@ $^

bin/%.o: src/%.c include/*.h
	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) -c -o $@ $<

verbose: clean
	$(eval CFLAGS += -DVERBOSE)

debug: clean
	$(eval CFLAGS += -O0)

final: clean
	$(eval CFLAGS += -O2)

check:
	@grep --color=auto -A4 -n -E "/[/*](FIXME|TODO)" include/*.h src/*.c

clean:
	rm -vf $(BIN) bin/*.o *.log *.trace

.SUFFIXES:
.PHONY: all check clean debug final verbose
