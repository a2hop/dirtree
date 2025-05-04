CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -shared -fPIC
TARGET = dirtree
LIB_TARGET = libdirtree.so
WIN_LIB_TARGET = libdirtree.dll

all: $(TARGET)

$(TARGET): dirtree.c
	$(CC) $(CFLAGS) -o $(TARGET) dirtree.c

shared: dirtree.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(LIB_TARGET) dirtree.c

clean:
	rm -f $(TARGET) $(LIB_TARGET) $(WIN_LIB_TARGET)
