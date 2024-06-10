IDIR = ./include
CC = gcc
CFLAGS = -Wall -std=c11 -Wno-unused-variable

_DEPS = bpm.h sockets.h test_utils.h 
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = evr_capt_tests.o bpm.o sockets.o utils.o

TARGET = irq-capture

all : $(TARGET) $(OBJ)

#%.o: $(IDIR)/%.c $(DEPS)
#	$(CC) -c -o $@ $< $(CFLAGS)

utils.o : $(IDIR)/utils.c $(IDIR)/test_utils.h
	$(CC) -c $^

bpm.o : $(IDIR)/bpm.c $(IDIR)/bpm.h
	$(CC) -c $^

sockets.o : $(IDIR)/sockets.c $(IDIR)/sockets.h
	$(CC) -c $^

evr_capt_tests.o: evr_capt_tests.c $(DEPS)
	$(CC) $(CFLAGS) -c $^ -I$(IDIR)

# Build irq-capture and do housekeeping
$(TARGET): $(OBJ) $(DEPS)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread
	rm -f *~ $(IDIR)/*.gch

.PHONY: clean

clean:
	rm -f $(OBJ) *~ $(IDIR)/*.gch
