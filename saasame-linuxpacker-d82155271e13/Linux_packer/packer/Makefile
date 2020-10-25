BINARY_NAME = linux_packer
#INSTALLDIR = $(PREFIX)/bin
LIBDIR = -L../lib -L/usr/local/lib -L/usr/lib -L/usr/lib64
#LIBDIR = -L../lib -L/root/glibc-build/bin/lib -L/usr/local/lib -L/usr/lib -L/usr/lib64
INCLUDEDIR = -I../gen-cpp/ -I/usr/local/include/thrift -I/usr/local/include/boost -I/usr/lib
#INCLUDEDIR = -I/root/glibc-build/bin/include -I../gen-cpp/ -I/usr/local/include/thrift -I/usr/local/include/boost -I/usr/lib
#LIB= -lc -ldl -lgen-cpp -ltools -lthrift -lboost_filesystem -lboost_system -ljson_spirit -lboost_thread -lboost_date_time -lboost_program_options -lssl -lcrypt -lcrypto -lrt -luuid -lpthread
#LIB= -Wl,-Bstatic -lc -ldl -lgen-cpp -ltools -lthrift -lboost_filesystem -lboost_system -ljson_spirit -lboost_thread -lboost_date_time -lboost_program_options -lboost_regex -Wl,-Bdynamic -lssl -lcrypt -lcrypto -lrt -luuid -lpthread
#LIB= -Wl,-Bstatic -lc -ldl -lgen-cpp -ltools -llz4 -lthrift -lboost_filesystem -lboost_system -ljson_spirit -lboost_thread -lboost_date_time -lboost_program_options -lboost_regex -lssl -Wl,-Bdynamic -lcrypt -lcrypto -lrt -luuid -lpthread
#LIB= -static -lc -ldl -ltools -lgen-cpp -lthrift -lboost_filesystem -lboost_system -ljson_spirit -lboost_thread -lboost_date_time -lboost_program_options -lssl -lcrypt -lcrypto -lrt -luuid -lpthread
LIB= -static -lc -ltools -lgen-cpp -lthrift -llz4 -lboost_filesystem -lboost_system -ljson_spirit -lcurl -lz -lboost_thread -lboost_date_time -lboost_program_options -lboost_regex -lssl -lcrypt -lcrypto -lrt -luuid -lpthread -ldl
SOURCES = $(wildcard *.cpp)
SOURCEO = $(patsubst %.cpp,%.o,$(SOURCES))
CC=g++
#CC=g++
CCFLAGS=-DHAVE_CONFIG_H -O3 -rdynamic -static-libstdc++ -static-libgcc -std=c++0x -fpermissive
# -Wall -Wextra
.PHONY: all clean

all: ${SOURCEO}
	@echo linux_host_packer
	$(CC) $(CCFLAGS) -o $(BINARY_NAME) $(INCLUDEDIR) $(LIBDIR) ${SOURCEO} $(LIB)
clean:
	rm -f $(BINARY_NAME) *.o

${SOURCEO}: %.o: %.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIR) $< -o $@
