CC = gcc
CFLAGS = -Wall -Wextra -std=c17
TARGET = scissos
OBJS = scissos_os.o scissos_processes.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
# 	rm -f $(OBJS)

scissos_os.o: scissos_os.c ScisSos.h
	$(CC) $(CFLAGS) -c scissos_os.c

scissos_processes.o: scissos_processes.c ScisSos.h
	$(CC) $(CFLAGS) -c scissos_processes.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: cleanmake