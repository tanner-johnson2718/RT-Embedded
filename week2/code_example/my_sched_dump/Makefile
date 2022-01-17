INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g3 $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lpthread -lrt

HFILES= 
CFILES= rt_thread.c clock.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	rt_thread

clean:
	-rm -f *.o *.d
	-rm -f rt_thread 

rt_thread: rt_thread.o clock.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o clock.o $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
