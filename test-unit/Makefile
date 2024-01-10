MRFDIR = mrf-linux-api-master/api/
CC = gcc
CFLAGS = -lpthread -I$(MRFDIR)

DEPS = bpm.h sockets.h test_utils.h 

OBJ = evr_capt_tests.o bpm.o sockets.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

irq-capture: $(OBJ)
	$(CC) -o $@ $^ $(MRFDIR)erapi.o $(MRFDIR)fracdiv.o -lpthread

clean:
	rm -f *.o 