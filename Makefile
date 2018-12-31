CC      = gcc
CFLAGS  = -O3 -Wall -g
HEADERS = 
SOURCES = main.c
OUTPUT  = opengl-xlib-example

opengl-xlib-example: $(HEADERS) $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(OUTPUT) -lX11 -lGL

clean:
	-rm $(OUTPUT)

