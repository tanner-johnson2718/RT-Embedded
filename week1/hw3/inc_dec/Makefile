INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= inc_dec.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all: inc_dec

clean:
	-rm -f *.o *.d
	-rm -f inc_dec

inc_dec: inc_dec.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
