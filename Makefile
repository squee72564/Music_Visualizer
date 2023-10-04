CC = gcc
CFLAGS = -O2 -Wall -Wextra 
LDFLAGS = -I ./include/ -L ./lib/
LDLIBS = -lraylib -lopengl32 -lgdi32 -lwinmm

main : src/visualizer.c
	$(CC) $(CFLAGS) -o visualizer.exe src/visualizer.c $(LDFLAGS) $(LDLIBS)
