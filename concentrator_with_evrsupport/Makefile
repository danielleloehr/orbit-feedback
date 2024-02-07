IDIR = ./include
CC = gcc
CFLAGS = -Wall -std=c11
LDFLAGS = -lpthread -lzmq

MRFDIR = mrf-linux-api-master/api/
_MRFSRC = egapi.o erapi.o fracdiv.o 
MRFSRC = $(patsubst %,$(MRFDIR)/%,$(_MRFSRC))

_DEPS = bpm.h net_helpers.h misc_utils.h 
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = evr_capt_tests.o bpm.o net_helpers.o misc_utils.o

TARGET = irq-capture

all : $(TARGET) $(OBJ)

#%.o: $(IDIR)/%.c $(DEPS)
#	$(CC) -c -o $@ $< $(CFLAGS)

misc_utils.o : $(IDIR)/misc_utils.c $(IDIR)/misc_utils.h
	$(CC) $(CFLAGS) -c $^

bpm.o : $(IDIR)/bpm.c $(IDIR)/bpm.h
	$(CC) $(CFLAGS) -c $^

net_helpers.o : $(IDIR)/net_helpers.c $(IDIR)/net_helpers.h
	$(CC) $(CFLAGS) -c $^

evr_capt_tests.o: evr_capt_tests.c
	$(CC) $(CFLAGS) -c $^ -I$(IDIR) -I$(MRFDIR)

# Build irq-capture 
$(TARGET): $(OBJ) $(DEPS) $(MRFSRC)
	$(CC) $(CFLAGS) -o $@ $^  -lpthread -lzmq

.PHONY: clean

clean:
	rm -f $(OBJ) *~
