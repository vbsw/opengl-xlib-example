CC      = gcc
CFLAGS  = -O3 -Wall -lX11 -lGL -g
HEADERS = 
SOURCES = main.c
OUTPUT  = opengl-xlib-example

opengl-xlib-example: $(HEADERS) $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(OUTPUT)

clean:
	-rm $(OUTPUT)

