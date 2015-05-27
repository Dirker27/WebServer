CC = gcc
#CFLAGS=-Wall -O3 -Werror
CFLAGS=-Wall -Werror

ASSETS=signal_handler.h global_header.h parse_utils.h html_utils.h uri.h http.h
#LDLIBS=-lpthread

# Use make's default rules
all: sysstatd

sysstatd: csapp.o server.o threadpool.o
	$(CC) $(CFLAGS) -pthread csapp.o server.o threadpool.o list.o -o ./sysstatd

server.o: server.c $(ASSETS)
	$(CC) $(CFLAGS) -c server.c

#http.o: http.c
#	$(CC) $(CFLAGS) -c http.c

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

threadpool.o: threadpool.c list.o
	$(CC) $(CFLAGS) -c threadpool.c

list.o: list.c
	$(CC) $(CFLAGS) -c list.c

clean:
	rm -f *.o sysstatd

