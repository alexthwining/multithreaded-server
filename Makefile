CC = gcc
CFLAGS = -g -pthread -Wall -Wextra -Wpedantic -Wshadow
TARGET = httpproxy
OBJ = httpproxy.o queue.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^
	
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

clean:
	$(RM) $(TARGET)
