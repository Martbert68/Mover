CC=gcc
waver: mover.c 
	$(CC) mover.c -o mover -l X11 -lm -ljpeg -lasound -lpthread
all: mover 
