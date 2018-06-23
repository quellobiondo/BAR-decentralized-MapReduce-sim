CC = gcc

INSTALL_PATH = /usr
INCLUDES = -I../platform/include -I$(INSTALL_PATH)/include
DEFS = -L$(INSTALL_PATH)/lib
LDADD = -lm -lsimgrid
SOURCE_FOLDER = src
BIN_FOLDER = bin
TRACE_FOLDER = traces

SRC_FILES=$(wildcard $(SOURCE_FOLDER)/*.c)
BIN=$(SRC_FILES:$(SOURCE_FOLDER)/%.c=$(BIN_FOLDER)/%.bin)

all: $(BIN)

$(BIN_FOLDER)/%.bin: $(SOURCE_FOLDER)/%.c ../platform/libmrsg.a
	$(CC) $(INCLUDES) $(DEFS) -o $@ $^ $(LDADD)

../platform/libmrsg.a:
	make -C ../platform/

clean:
	rm -vf $(BIN_FOLDER)/*.bin $(TRACE_FOLDER)/*

.PHONY: clean
