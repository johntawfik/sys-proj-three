CC=gcc
CFLAGS=-Wall -Wextra -pedantic
TARGET=mysh
OBJS=mysh.o queue.o

# Compiles the final executable
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Compile mysh.c to mysh.o
mysh.o: mysh.c
	$(CC) $(CFLAGS) -c mysh.c -o mysh.o

# Compile queue.c to queue.o
queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c -o queue.o

clean:
	rm -f $(TARGET) $(OBJS)
