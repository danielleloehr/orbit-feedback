CC = gcc
CFLAGS = -Wall
LDFLAGS = -lzmq
IDIR= ../concentrator_with_evrsupport/include
OBJFILES = zmq_simple_server.o
TARGET = zmq_server
DEPS = $(IDIR)/sockets.h
all: $(TARGET)

%.o : .%c
	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)

$(TARGET): $(OBJFILES) $(DEPS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~