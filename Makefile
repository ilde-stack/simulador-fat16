CC = gcc
CFLAGS = -Wall -Wextra -std=c11

PROGRAMS = fat16-info fat16-ls fat16-fileclusters fat16-sh
COMMON_OBJS = fat16.o
SHELL_OBJS = fat16-sh.o fat16-sh-info.o fat16-sh-ls.o fat16-sh-fileclusters.o fat16-sh-cat.o fat16-sh-rm.o

all: $(PROGRAMS)

fat16-info: fat16-info.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ fat16-info.o $(COMMON_OBJS)

fat16-ls: fat16-ls.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ fat16-ls.o $(COMMON_OBJS)

fat16-fileclusters: fat16-fileclusters.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ fat16-fileclusters.o $(COMMON_OBJS)

fat16-sh: $(SHELL_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SHELL_OBJS) $(COMMON_OBJS)

fat16.o: fat16.c fat16.h
	$(CC) $(CFLAGS) -c fat16.c

fat16-info.o: fat16-info.c fat16.h
	$(CC) $(CFLAGS) -c fat16-info.c

fat16-ls.o: fat16-ls.c fat16.h
	$(CC) $(CFLAGS) -c fat16-ls.c

fat16-fileclusters.o: fat16-fileclusters.c fat16.h
	$(CC) $(CFLAGS) -c fat16-fileclusters.c

fat16-sh.o: fat16-sh.c fat16-sh.h fat16.h
	$(CC) $(CFLAGS) -c fat16-sh.c

fat16-sh-info.o: fat16-sh-info.c fat16-sh.h fat16.h
	$(CC) $(CFLAGS) -c fat16-sh-info.c

fat16-sh-ls.o: fat16-sh-ls.c fat16-sh.h fat16.h
	$(CC) $(CFLAGS) -c fat16-sh-ls.c

fat16-sh-fileclusters.o: fat16-sh-fileclusters.c fat16-sh.h fat16.h
	$(CC) $(CFLAGS) -c fat16-sh-fileclusters.c

fat16-sh-cat.o: fat16-sh-cat.c fat16-sh.h fat16.h
	$(CC) $(CFLAGS) -c fat16-sh-cat.c

fat16-sh-rm.o: fat16-sh-rm.c fat16-sh.h fat16.h
	$(CC) $(CFLAGS) -c fat16-sh-rm.c

clean:
	rm -f $(PROGRAMS) *.o