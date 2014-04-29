
CC=gcc
CFLAGS=-fPIC -g -Wall -Wextra -O2 -Wno-unused-label -Wno-unused-parameter `mysql_config --cflags`
LIBS=`mysql_config --libs`
INCLUDES=-I/usr/local/include/rebol
LIBFLAGS=-shared

OBJS=mysqldb.o

all: lib

mysqldb_init.h: mysqldb.r
	r3 ./make-ext.r

lib: $(OBJS)
	@echo "Creating Shared library..."
	@$(CC) $(LIBFLAGS) -Wl,-soname,mysqldb.so -o mysqldb.so $(OBJS) $(LIBS)

mysqldb.o: mysqldb.c mysqldb_init.h
	@echo "Building $@"
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	@rm -f $(OBJS) mysqldb_init.h mysqldb.o mysqldb.so
