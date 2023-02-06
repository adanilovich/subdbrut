cflags=-Wall -g -O0 -std=gnu11 -Wno-missing-braces 
sanitize-cflags=-Wall -g -O0 -fsanitize=address -static-libasan -std=gnu11 -Wno-missing-braces 
objects=dns.o socket.o threads.o epoll_handler.o conns_store.o helpers.o
ldlibs=-lpthread -lglib-2.0 
include=-I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/sysprof-4 


conns_store.o: conns_store.c conns_store.h
	gcc -I. $(include) $(cflags) -c conns_store.c

socket.o: socket.c socket.h
	gcc -I. $(include) $(cflags) -c socket.c

helpers.o: helpers.c helpers.h
	gcc -I. $(include) $(cflags) -c helpers.c

threads.o: threads.c threads.h
	gcc -I. $(include) $(cflags) -c threads.c

epoll_handler.o: epoll_handler.c epoll_handler.h
	gcc -I. $(include) $(cflags) -c epoll_handler.c

dns.o: dns.c dns.h
	gcc -I. $(include) $(cflags) -c dns.c

client.o: client.c client.h
	gcc -I. $(include) $(cflags) -c client.c

all: client.o $(objects)
	@gcc $(cflags) client.o $(objects) $(ldlibs) -o subdbrut

build:all

tests.o:tests/tests.c
	gcc -c -I. $(cflags) -c tests/tests.c

test-bin: tests.o $(objects) 
	gcc $(cflags) tests.o $(objects) $(ldlibs) -o test

test:test-bin
	@./test

clean:
	@rm -rf ./*.o
	@rm -rf ./client
	@rm -rf ./test
	@rm -rf ./vgcore.*

v:all
	valgrind --tool=memcheck --leak-check=full ./client <1

debug:all
	G_MESSAGES_DEBUG=all D_GLIBCXX_SANITIZE_STD_ALLOCATOR=1 ./client < 1
