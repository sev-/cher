CC = rcc
CFLAGS = -Ox

ch: ch.o melt.o
	$(CC) $(CFLAGS) -o ch ch.o melt.o

cher: cher.o melt.o
	$(CC) $(CFLAGS) -o cher cher.o melt.o

convch: convch.c
	$(CC) $(CFLAGS) -o convch convch.c

melt.c: melt.h
