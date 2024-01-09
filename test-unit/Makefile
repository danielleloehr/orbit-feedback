IDIR = mrf-linux-api-master/api/
#ODIR=mrf-linux-api-master/api/
CC = gcc
CFLAGS = -lpthread -I$(IDIR)

DEPS = bpm.h sockets.h test_utils.h 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

evrtestmake: evr_capt_tests.o
	$(CC) -o irq-capture evr_capt_tests.o $(IDIR)erapi.o $(IDIR)fracdiv.o
        