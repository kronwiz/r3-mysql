
all: lib

lib:
	@make -C src
	@cp src/mysqldb.so .

clean:
	@make -C src clean
	@rm -f mysqldb.so

