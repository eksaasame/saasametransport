LIB_NAME = libtools.a
INSTALLDIR = ../lib
INCLUDEDIR = -Igen-cpp/ -I/usr/local/include/thrift -I/usr/local/include/boost -I/usr/local/include
#INCLUDEDIR = -I/root/glibc-build/bin/include -Igen-cpp/ -I/usr/local/include/thrift -I/usr/local/include/boost -I/usr/local/include
SOURCES = $(wildcard *.cpp)
SOURCEO = $(patsubst %.cpp,%.o,$(SOURCES))
CCFLAGS = -fpermissive -std=c++0x -O3 -rdynamic
#-Wall -Wextra
.PHONY: static install clean
#CC=/opt/rh/devtoolset-2/root/usr/bin/g++
#CC=clang++
static: ${SOURCEO}
	$(AR) rcs ${LIB_NAME} ${SOURCEO}
	mkdir -p ${INSTALLDIR}
	cp -f ${LIB_NAME} ${INSTALLDIR}/${LIB_NAME}
clean:
	rm -f $(LIB_NAME) $(INSTALLDIR)/$(LIB_NAME) *.o

${SOURCEO}: %.o: %.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIR) $< -o $@
