CC = gcc
CFLAGS = -Wall
LDFLAGS = -lzmq

OBJFILES = zmq_simple_server.o
TARGET = zmq_server

all: $(TARGET)

%.o : .%c
	$(CC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~