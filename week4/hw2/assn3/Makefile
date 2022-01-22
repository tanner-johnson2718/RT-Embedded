INCLUDE_DIRS = 
LIB_DIRS = 
CC=gcc

CDEFS=
CFLAGS= -O3 $(INCLUDE_DIRS) $(CDEFS)
LIBS= 

HFILES= 
CFILES= sum.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	sum

clean:
	-rm -f *.o *.d
	-rm -f perfmon sum

distclean:
	-rm -f *.o *.d
	-rm -f sum

sum: sum.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o -lpthread

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<
