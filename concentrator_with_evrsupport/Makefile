IDIR = ./include
CC = gcc
CFLAGS = -Wall

MRFDIR = mrf-linux-api-master/api/

_DEPS = bpm.h sockets.h test_utils.h 
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = evr_capt_tests.o bpm.o sockets.o

TARGET = irq-capture

all : $(TARGET) $(OBJ)

#%.o: $(IDIR)/%.c $(DEPS)
#	$(CC) -c -o $@ $< $(CFLAGS)

bpm.o : $(IDIR)/bpm.c $(IDIR)/bpm.h
	$(CC) -c $^ -I$(IDIR)

sockets.o : $(IDIR)/sockets.c $(IDIR)/sockets.h
	$(CC) -c $^ -I$(IDIR)

evr_capt_tests.o: evr_capt_tests.c $(DEPS)
	$(CC) -c $^ -I$(IDIR) -I$(MRFDIR)

# Build irq-capture 
$(TARGET): $(OBJ) $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS) $(MRFDIR)erapi.o $(MRFDIR)fracdiv.o -lpthread 

.PHONY: clean

clean:
	rm -f $(OBJ) *~
