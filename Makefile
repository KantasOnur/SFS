CC = gcc
CFLAGS = -Wall
LDFLAGS = -lrt

all: queue.o a3func.o diskinfo disklist diskget diskput

queue.o: queue.c queue.h
	$(CC) $(CLFAGS) -c queue.c
a3func.o: a3func.c a3func.h
	$(CC) $(CFLAGS) -c a3func.c
diskinfo: diskinfo.c
	$(CC) $(CFLAGS) a3func.o -o diskinfo diskinfo.c $(LDFLAGS)
diskget: diskget.c
	$(CC) $(CFLAGS) a3func.o -o diskget diskget.c $(LDFLAGS)
diskput: diskput.c
	$(CC) $(CFLAGS) a3func.o -o diskput diskput.c $(LDFLAGS)
disklist: disklist.c queue.o
	$(CC) $(CLFAGS) disklist.c $(LDFLAGS) a3func.o queue.o -o disklist

clean: 
	rm -f diskinfo disklist diskget diskput a3func.o queue.o
