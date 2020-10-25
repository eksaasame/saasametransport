BINARY_NAME = Reboot_agent
#INSTALLDIR = $(PREFIX)/bin
LIBDIR += -L../lib -L/usr/local/lib -L/usr/lib -L/usr/lib64
INCLUDEDIR += -I../gen-cpp/ -I/usr/local/include/thrift -I/usr/local/include/boost
LIB= -static -lc -ltools -ljson_spirit -lboost_thread -lboost_date_time -lboost_program_options -lboost_filesystem -lboost_system -lpthread -ldl -lrt 
SOURCES = $(wildcard *.cpp)
SOURCEO = $(patsubst %.cpp,%.o,$(SOURCES))
CC=/opt/rh/devtoolset-2/root/usr/bin/g++
CCFLAGS=-DHAVE_CONFIG_H -g -O2 -static-libstdc++ -static-libgcc -std=c++0x -fpermissive
.PHONY: all clean

all: ${SOURCEO}
	@echo linux_host_packer
	$(CC) $(CCFLAGS) -o $(BINARY_NAME) $(INCLUDEDIR) $(LIBDIR) $(SOURCEO) $(LIB)
clean:
	rm -f $(BINARY_NAME) *.o

${SOURCEO}: %.o: %.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIR) $< -o $@
