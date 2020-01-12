CCPLUS=g++
CC=gcc
OPT=-O0
DEBUG=-g -ggdb
DEPENDENCY_TARGETS=
CFLAGS+= $(DEBUG) $(OPT) $(NVKVS_OPT) -std=c11
CFLAGS+= -lpthread
LDFLAGS=-Llib
LIBS=

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
# ifeq ($(uname_S),Darwin)
# 	CFLAGS+=-fPIC
# 	OBJ_FOLDER=obj_mac
# 	BIN_FOLDER=bin_mac
# else
# 	OBJ_FOLDER=obj_linux
# 	BIN_FOLDER=bin_linux
# endif
TARGET=server
SERVER_OBJ=net.o server.o util.o bworker.o command.o hash.o

all: $(TARGET)

dep:
	-(cd ../deps && $(MAKE) $(DEPENDENCY_TARGETS))

server: $(SERVER_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

client: $(CLI_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c .make-prerequisites
	$(CC) -c $<

clean:
	rm -rf server client $(BIN_FOLDER)/*.o $(OBJ_FOLDER)/*.o *.o