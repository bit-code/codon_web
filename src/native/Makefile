CC ?= gcc
CFLAGS ?= -O3 -fPIC -march=native -Wall

all: libcfastjson.so libcfastjson.a

yyjson.o: yyjson.c yyjson.h
	$(CC) $(CFLAGS) -c yyjson.c -o yyjson.o

c_fast_json.o: c_fast_json.c yyjson.h
	$(CC) $(CFLAGS) -c c_fast_json.c -o c_fast_json.o

libcfastjson.so: yyjson.o c_fast_json.o
	$(CC) -shared yyjson.o c_fast_json.o -o libcfastjson.so

libcfastjson.a: yyjson.o c_fast_json.o
	ar rcs libcfastjson.a yyjson.o c_fast_json.o

clean:
	rm -f *.o *.so *.a
