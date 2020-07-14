CC=gcc
waver: waver.c 
	$(CC) waver.c -o waver -l X11 -lm -ljpeg -lasound -lpthread
all: waver