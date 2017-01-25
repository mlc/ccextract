CC = gcc
CFLAGS = -Wall -g -I/usr/include/ncursesw `pkg-config libquicktime libdv --cflags` -finput-charset=utf-8
LIBS = `pkg-config libquicktime libdv --libs` -lncursesw

OBJS = tst.o eia608.o smpte.o

tst : $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

clean :
	rm -f tst *.o *~
