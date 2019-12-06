CC = g++
CFLAGS = -Wall -O3
LDFLAGS =
OBJFILES = loser.o md5.o
TARGET = loser

all: $(TARGET)
$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)
clean:
	rm -f $(OBJFILES) $(TARGET) *~