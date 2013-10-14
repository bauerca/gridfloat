CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-lpng -lm
SOURCES=src/main.c src/gridfloat.c src/linear.c src/quadratic.c src/gfpng.c src/gfstl.c
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
