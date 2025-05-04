# Makefile for dirtree library and executable

CC = gcc
CFLAGS = -Wall -Werror -std=c99 -fPIC -D_GNU_SOURCE
LDFLAGS = 

# Library version
VERSION = 1.0.0
SONAME = libdirtree.so.1

# Target names
LIB_TARGET = libdirtree.so.$(VERSION)
LIB_SONAME = $(SONAME)
LIB_LINKNAME = libdirtree.so
EXEC_TARGET = dirtree

# Source files
SOURCES = dirtree.c
HEADERS = dirtree.h
OBJ = dirtree.o

# Default target
all: $(EXEC_TARGET)

# Compile object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Build the executable
$(EXEC_TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# Build the library with library-only flag
$(LIB_TARGET): CFLAGS += -DDIRTREE_LIBRARY_ONLY
$(LIB_TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -shared -Wl,-soname,$(LIB_SONAME) -o $@ $(SOURCES) $(LDFLAGS)
	ln -sf $(LIB_TARGET) $(LIB_SONAME)
	ln -sf $(LIB_SONAME) $(LIB_LINKNAME)

# Build dynamic shared library
shared: $(LIB_TARGET)

# Install library and headers to system paths
install: shared
	install -d $(DESTDIR)/usr/lib
	install -d $(DESTDIR)/usr/include
	install -m 644 $(HEADERS) $(DESTDIR)/usr/include/
	install -m 755 $(LIB_TARGET) $(DESTDIR)/usr/lib/
	ln -sf $(LIB_TARGET) $(DESTDIR)/usr/lib/$(LIB_SONAME)
	ln -sf $(LIB_SONAME) $(DESTDIR)/usr/lib/$(LIB_LINKNAME)
	ldconfig

# Clean up
clean:
	rm -f $(EXEC_TARGET) $(OBJ) $(LIB_TARGET) $(LIB_SONAME) $(LIB_LINKNAME)

# Phony targets
.PHONY: all shared install clean
