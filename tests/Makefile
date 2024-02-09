CC = gcc
CFLAGS = -Wall
LDFLAGS = -lzmq
IDIR= ../concentrator_with_evrsupport/include
OBJFILES = zmq_simple_server.o
TARGET = zmq_server

DEPS = $(IDIR)/sockets.h

all: $(TARGET)

zmq_simple_server.o : zmq_simple_server.c
	$(CC) $(CFLAGS) -c $< -o $@ -I$(IDIR) $(LDFLAGS)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) -o $(TARGET) -I $(IDIR) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~