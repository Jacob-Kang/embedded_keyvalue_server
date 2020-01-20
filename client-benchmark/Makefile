CC=gcc
CPP=g++
OPT=-O0
DEBUG=-g -ggdb #-DWITHOUT_LOG #-DONLY_FLASH #-DWITHOUT_LOG #-DONLY_FLASH
DEPENDENCY_TARGETS=
CFLAGS+= $(DEBUG) $(OPT) -std=c11 
CXXFLAGS+= $(DEBUG) $(OPT) -std=c++11
LDFLAGS=-Llib -Iinclude
LIBS=-pthread
OBJ_FOLDER = obj
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
# ifeq ($(uname_S),Darwin)
#   CFLAGS+=-fPIC
#   OBJ_FOLDER=obj_mac
#   BIN_FOLDER=bin_mac
# else
#   OBJ_FOLDER=obj_linux
#   BIN_FOLDER=bin_linux
# endif
TARGET=client
CLIENT_OBJ=net.o client.o util.o

all: $(TARGET)

dep:
	-(cd ../deps && $(MAKE) $(DEPENDENCY_TARGETS))

client: $(CLIENT_OBJ)
	$(CPP) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
	# mv *.o $(OBJ_FOLDER)/

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $<
%.o: %.cc
	$(CPP) $(CXXFLAGS) $(LDFLAGS) -c $<

clean:
	rm -rf $(TARGET) $(BIN_FOLDER)/*.o $(OBJ_FOLDER)/*.o *.o