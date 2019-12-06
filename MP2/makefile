CC = g++
CFLAGS = -Wall -O3
LDFLAGS =
OBJFILES = loser.o md5.o loser_peer.o
TARGET = loser_peer

all: $(TARGET)
$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)
clean:
	rm -f $(OBJFILES) $(TARGET) *~