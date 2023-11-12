CC = gcc-13
CFLAGS = -Wall -Wextra -o2 -std=c99
OSLIBS = -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
LIBS = -L ./lib/ ./lib/librarylib.a
INCLUDES = -I ./include/

main : src/visualizer.c
	$(CC) $(CFLAGS) -o visualizer  src/visualizer.c $(OSLIBS) $(LIBS) $(INCLUDES)
