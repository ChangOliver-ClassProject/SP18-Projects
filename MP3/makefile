CC = g++
CFLAGS = -Wall -O3
LDFLAGS =
OBJFILES1 = csie_box_server.cpp
OBJFILES2 = csie_box_client.cpp
TARGET1 = csie_box_server
TARGET2 = csie_box_client

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJFILES1)
	$(CC) $(CFLAGS) -o $(TARGET1) $(OBJFILES1) $(LDFLAGS)
$(TARGET2): $(OBJFILES2)
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJFILES2) $(LDFLAGS)
clean:
	rm -f $(TARGET1) $(TARGET2) *~