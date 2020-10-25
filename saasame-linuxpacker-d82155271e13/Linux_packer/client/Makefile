BINARY_NAME = TestPackerClient
#INSTALLDIR = $(PREFIX)/bin
LIBDIR += -L../lib -L/usr/local/lib -L/usr/lib
INCLUDEDIR += -I../gen-cpp/ -I/usr/local/include/thrift -I/usr/local/include/boost
LIB= -lgen-cpp -lthrift -lssl -lcrypt -lrt -lboost_filesystem -lboost_system -lpthread 
SOURCES = $(wildcard *.cpp)
SOURCEO = $(patsubst %.cpp,%.o,$(SOURCES))
CC=g++
CCFLAGS=-DHAVE_CONFIG_H -g -O2 -std=c++11 -fpermissive
.PHONY: all clean

all: ${SOURCEO}
	@echo linux_host_packer
	$(CC) $(CCFLAGS) -std=c++11 -o $(BINARY_NAME) $(INCLUDEDIR) $(LIBDIR) $(SOURCEO) $(LIB)
clean:
	rm -f $(BINARY_NAME) *.o

${SOURCEO}: %.o: %.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIR) $< -o $@
