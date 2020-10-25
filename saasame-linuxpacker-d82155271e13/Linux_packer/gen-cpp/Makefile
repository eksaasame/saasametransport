LIB_NAME = libgen-cpp.a
INSTALLDIR = ../lib
INCLUDEDIR = -Igen-cpp/ -I/usr/local/include/thrift -I/usr/local/include/boost -I/usr/local/include
SOURCES = $(wildcard *.cpp)
SOURCEO = $(patsubst %.cpp,%.o,$(SOURCES))
CCFLAGS = -fpermissive -O3
.PHONY: static install clean
CC=g++
static: ${SOURCEO}
	$(AR) rcs ${LIB_NAME} ${SOURCEO}
	mkdir -p ${INSTALLDIR}
	cp -f ${LIB_NAME} ${INSTALLDIR}/${LIB_NAME}
clean:
	rm -f $(LIB_NAME) $(INSTALLDIR)/$(LIB_NAME) *.o

${SOURCEO}: %.o: %.cpp
	$(CC) -c $(CCFLAGS) $(INCLUDEDIR) $< -o $@
