CC=gcc
CFLAGS=-Wall -Wextra -pedantic
TARGET=mysh

all: $(TARGET)

$(TARGET): mysh.c
	$(CC) $(CFLAGS) mysh.c -o $(TARGET)

clean:
	rm -f $(TARGET)
