CC ?= gcc
CFLAGS ?= -O3 -fPIC -march=native -Wall

all: libcfastjson.so libcfastjson.a

yyjson.o: yyjson.c yyjson.h
	$(CC) $(CFLAGS) -c yyjson.c -o yyjson.o

c_fast_json.o: c_fast_json.c yyjson.h
	$(CC) $(CFLAGS) -c c_fast_json.c -o c_fast_json.o

c_http_date.o: c_http_date.c
	$(CC) $(CFLAGS) -c c_http_date.c -o c_http_date.o

picohttpparser.o: picohttpparser.c picohttpparser.h
	$(CC) $(CFLAGS) -c picohttpparser.c -o picohttpparser.o

c_http_parser.o: c_http_parser.c c_http_parser.h picohttpparser.h
	$(CC) $(CFLAGS) -c c_http_parser.c -o c_http_parser.o

c_pg_driver.o: c_pg_driver.c c_pg_driver.h
	$(CC) $(CFLAGS) -I/usr/include/postgresql -c c_pg_driver.c -o c_pg_driver.o

c_iouring.o: c_iouring.c c_iouring.h
	$(CC) $(CFLAGS) -c c_iouring.c -o c_iouring.o

libcfastjson.so: yyjson.o c_fast_json.o c_http_date.o picohttpparser.o c_http_parser.o c_pg_driver.o c_iouring.o
	$(CC) -shared yyjson.o c_fast_json.o c_http_date.o picohttpparser.o c_http_parser.o c_pg_driver.o c_iouring.o -lpq -o libcfastjson.so

libcfastjson.a: yyjson.o c_fast_json.o c_http_date.o picohttpparser.o c_http_parser.o c_pg_driver.o c_iouring.o
	ar rcs libcfastjson.a yyjson.o c_fast_json.o c_http_date.o picohttpparser.o c_http_parser.o c_pg_driver.o c_iouring.o

clean:
	rm -f *.o *.so *.a
