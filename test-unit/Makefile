IDIR = mrf-linux-api-master/api/
#ODIR=mrf-linux-api-master/api/
CC = gcc
CFLAGS = -lpthread -I$(IDIR)

DEPS = bpm.h sockets.h test_utils.h 
OBJ = evr_capt_tests.o bpm.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

irq-capture: $(OBJ)
	$(CC) -o $@ $^ $(IDIR)erapi.o $(IDIR)fracdiv.o -lpthread

clean:
	rm -f *.o 