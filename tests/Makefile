CC = gcc
CFLAGS = -Wall
LDFLAGS = -lzmq -std=c99 -D_POSIX_C_SOURCE=199309L
IDIR= ../concentrator_with_evrsupport/include
OBJFILES = zmq_simple_subscriber.o
TARGET = zmq_subscriber

DEPS = $(IDIR)/sockets.h

all: $(TARGET)

zmq_simple_subscriber.o : zmq_simple_subscriber.c
	$(CC) $(CFLAGS) -c $< -o $@ -I$(IDIR) $(LDFLAGS)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) -o $(TARGET) -I $(IDIR) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~