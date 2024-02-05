IDIR = ./include
CC = gcc
CFLAGS = -Wall -I.

ODIR = obj
MRFDIR = mrf-linux-api-master/api/	#

_DEPS = bpm.h sockets.h test_utils.h 
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = bpm.o sockets.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

TARGET = irq-capture #

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

# Build irq-capture 
$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(CFLAGS) $(MRFDIR)erapi.o $(MRFDIR)fracdiv.o -lpthread -I$(MRFDIR)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 