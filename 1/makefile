CC = rcc
CFLAGS = -g

sok: sok.o melt.o
	$(CC) $(CFLAGS) -o sok sok.o melt.o

sokv: sokv.o melt.o
	$(CC) $(CFLAGS) -o sokv sokv.o melt.o

trv: trv.c
	$(CC) $(CFLAGS) -o trv trv.c

melt.c: melt.h
