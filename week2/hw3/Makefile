INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g3 $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lpthread -lrt

HFILES= 
CFILES= pthread.c posix_clock.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	pthread posix_clock

clean:
	-rm -f *.o *.d
	-rm -f pthread posix_clock

pthread: pthread.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o $(LIBS)

posix_clock:	posix_clock.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ posix_clock.o $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
