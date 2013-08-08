CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-lpng -lm
SOURCES=main.c gridfloat.c linear.c cubic.c gfpng.c gfstl.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=gridfloat

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm *.o $(EXECUTABLE)

.PHONY: clean
